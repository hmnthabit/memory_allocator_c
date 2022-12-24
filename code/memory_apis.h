#include <stdio.h>          // printf()
#include <unistd.h>   		// sbrk
#include <pthread.h>       	// pthread_mutex_lock
#include <string.h>        // memcpy()

// =======================================================================================================
// Macros and global varabiles
typedef struct chunk_header{

		size_t size;
		int free;
		struct chunk_header *next;
		struct chunk_header *prev;

} chunk_header;


// Macro for alignment 
#define ALIGNMENT_SIZE 8          // 8 bytes for x64 systems 
#define ALIGN_SIZE(size) (((size) + (ALIGNMENT_SIZE-1)) & ~(ALIGNMENT_SIZE-1))

// Heap Chunk Structure: | heap_chunk_header | heap_chunk_data |
#define get_heap_chunk_header_address( ptr ) ( chunk_header* )( (chunk_header*) ptr-1 )
#define get_heap_chunk_memory_address( ptr ) ( chunk_header* )( (chunk_header*) ptr+1 )

#define heap_header_size sizeof(chunk_header)

// Points to the latest allocated heap
chunk_header *head = NULL;
pthread_mutex_t global_malloc_lock;

// ======================================================================================================
// Functions declarations

void *malloc_test(unsigned int size);
void free_test(void *heap_chunk);
chunk_header *get_free_chunk(size_t size);

void *calloc_test(size_t elements_num, size_t element_size);

void *realloc_test(void *heap_chunk, size_t size);

void print_heap_chunks_info();

// ======================================================================================================

chunk_header *get_free_chunk(size_t size)
{
	chunk_header *current_chunk = head;
	
	// Search for a free chunk that's larger than the requested size in the heap linked list
	while(current_chunk) {
		if (current_chunk->free && current_chunk->size >= size)
		{
			return current_chunk;
		}
		current_chunk = current_chunk->next;
	}
	return NULL;
}

void merge_free_heap_chunks(chunk_header* current_heap_chunk){
	
	// 1. merge the adjacent `next` free chunks
	chunk_header* next_temp = current_heap_chunk->next;
	
	if (next_temp && next_temp->free){
		// Add the next heap chunk to the current heap chunk
		current_heap_chunk->size += next_temp->size + heap_header_size;
		// Unlink the next heap chunk from the double linked list
		current_heap_chunk->next = next_temp->next;
		if (next_temp->next) {
		
			if (next_temp->next->prev){
				next_temp->next->prev = current_heap_chunk;
		}
		}

		
	}

	// 2. merge the adjacent `previous` free chunks 
	chunk_header* prev_temp = current_heap_chunk->prev;
	
	if (prev_temp && prev_temp->free){
		prev_temp->size += current_heap_chunk->size + heap_header_size;

		// Unlink the curren heap chunk from the double linked list
		prev_temp->next = current_heap_chunk->next;
		if (current_heap_chunk->next){
			current_heap_chunk->next->prev = prev_temp;
		}
	}

}

void free_test(void *heap_chunk)
{
	chunk_header *header, *tmp;

	if (!heap_chunk)
		return;

	// get the start address of the heap chunk  --> `heap_chunk_header`  --> | heap_chunk_header | heap_chunk_data |
	header = get_heap_chunk_header_address(heap_chunk);
	// header = (chunk_header*)heap_chunk - 1;

	// return if the heap chunk is already freed (Avoid double-free)
	if (header->free == 1){
				return;
	}

	pthread_mutex_lock(&global_malloc_lock);

	// Merge the adjacent heap chunks 
	merge_free_heap_chunks(header);


	// If the heap chunk is located at top of heap address (aka Programebreak), then we can release the heap memory to the OS via `sbrk()` system call
	// However, if the heap chunk is located betweeen other heap chunks, then it's not possible to release to the OS. This is to avoid removing the heap chunks above the target heap chunk
	// get the top of heap address (aka Programebreak)
	void * heap_top_address = sbrk(0);
	
	if ((char*)heap_chunk + header->size == heap_top_address) {

		// printf("head address: %p\n", head);

		if (head == header){

			// Update the head if there's more than one heap chunk
			if (head->next){
				head = head->next;
			}
			else {
				head = NULL;
			}
		}
		// Release the heap chunk memory 
		sbrk(0 - header->size - sizeof(chunk_header));
		pthread_mutex_unlock(&global_malloc_lock);
		return;
	}

	else {
	header->free = 1;
	pthread_mutex_unlock(&global_malloc_lock);
	
	}

}

void split_heap_chunk(chunk_header* current_heap_chunk, unsigned int size){

	chunk_header *new_chunk; 

	new_chunk = current_heap_chunk + (size + heap_header_size);
	new_chunk->size = current_heap_chunk->size - (size + heap_header_size);
	new_chunk->free = 1;

	// Before: | current_heap_chunk | next_current heap_chunk
	// After: | current_heap_chunk | new_chunk | next_current heap_chunk
	new_chunk->next = current_heap_chunk->next;

	if (current_heap_chunk->next) {
		current_heap_chunk->next->prev = new_chunk;

	}

	new_chunk->prev = current_heap_chunk;
	
	current_heap_chunk->size  = size;
	current_heap_chunk->next = new_chunk;

	return;

}

void *malloc_test(unsigned int size)
{
	if (!size)
		return NULL;

	
	// lock before allocating
	pthread_mutex_lock(&global_malloc_lock);
	
	// === 1. Search if there's a free block with the requested size to use ===
	chunk_header *header = get_free_chunk(size);
	if (header) {
		
		pthread_mutex_unlock(&global_malloc_lock);

		// === 2. If the founded block is much larger than th requested size, then split it to 2 consecutive  heap chunks
		int target_size = ALIGN_SIZE(size);
		if (header->size > target_size){

			split_heap_chunk(header, target_size);
			header->free = 0;
			return header;
		}

		else {
			header->free = 0;
		// head+1 --> start of memory block (after the header)
			return get_heap_chunk_memory_address(header);

		// return (void*)(header + 1);
		}

	}
	
	// === 3. If there's no avalible free block, then extend the heap memory and initialize  the requested heap chunk ===

	// printf("sizeof(chunk_header): %d", sizeof(chunk_header));
	unsigned int total_size = sizeof(chunk_header) + size;
	total_size = ALIGN_SIZE(total_size);    // Align the size before calling `sbrk()`

	// Allocate memory with the target size in the heap and return the start of the allocated memory
	void *block = sbrk(total_size);

	if (!block) {
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}

	// make the header points to the allocated block
	header = block;
	header->size = total_size - sizeof(chunk_header);
	header->free = 0;
	header->prev = NULL;

	// set the head to the allocated chunk if not initlize before
	if (!head){
		// Set the chunk properties
		header->next = NULL;
		head = header;
	}
	
	else {
		// Insert before `head`
		header->size = total_size - sizeof(chunk_header);
		header->free = 0;
		
		header->next = head;
		head->prev = header;

		// Update the head to the new heap chunk
		head = header;
		
	}

	pthread_mutex_unlock(&global_malloc_lock);

	// return the address of the memory block after the `header`
	// header_address + sizeof(chunk_header)
	// return (void*)(header + 1);
	return (void*) get_heap_chunk_memory_address(header);
}

void *calloc_test(size_t elements_num, size_t element_size)
{
	if (!elements_num || !element_size)
		return NULL;
	
	size_t total_size = elements_num * element_size;

	// Avoid multiplication overflow
	if (total_size != 0 && element_size != total_size / elements_num)
		return NULL;
	
	void *heap_chunk = malloc_test(total_size);
	
	if (!heap_chunk)
		return NULL;
	
	memset(heap_chunk, 0, total_size);
	return heap_chunk;
}


// Extend the memory block
void *realloc_test(void *heap_chunk, size_t size)
{
	// chunk_header *header;
	// void *ret;
	
	// e.g. realloc_test(NULL, 50);
	if (!heap_chunk)
		return malloc_test(size);
	
	// Get the chunk header
	// chunk_header * header = (chunk_header*)heap_chunk - 1;
	chunk_header * header =  get_heap_chunk_header_address (heap_chunk);
	
	// If the requested size is 0, then simply free the block
	if (size == 0) {
		free_test(heap_chunk);
		return NULL;
	}


	// Allocate a new chunk with the requested size
	void *new_heap_chunk = malloc_test(size);
	if (!new_heap_chunk) {
 	   return NULL; 
  	}
	
	// Copy the old chunk data to the new one
	memcpy(new_heap_chunk, heap_chunk, header->size);
	// Free the old chunk
	free_test(heap_chunk);
	return new_heap_chunk;
}

void print_heap_chunks_info()
{
	chunk_header *curr = head;
	int counter = 0;

	while(curr) {
		counter+=1;
		printf("[%d] heap_chunk_address = %p, size = %zu, free=%u, next=%p prev=%p\n",
			counter,(void*)curr, curr->size, curr->free, (void*)curr->next, (void*)curr->prev);

		curr = curr->next;
	}

	printf("===================================================================================\n");
}

