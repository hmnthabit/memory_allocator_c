#include "memory_apis.h"
#include <ctype.h>         // tolower()


void test_malloc_and_free(){

// ======================================================================================================
	// === 1. Testing malloc() and free() ===
	print_heap_chunks_info();
	void* a = malloc_test(20);
	void* b = malloc_test(200);
	void* c = malloc_test(100);

	print_heap_chunks_info();
	free_test(b);
    print_heap_chunks_info();

    // Malloc should split the chunk of `b` and return a chunk with the requested size
	void* d = malloc_test(60);
	print_heap_chunks_info();

}

void test_merge(){

	// ======================================================================================================
	// === 2. Testing the merge functinolity of `free()` ===

	// === 2.1 Testing `previous` heap chunks merge ===
	printf("[+] Testing `prev` chunks merge\n");

	void* a = malloc_test(20);
	void* b = malloc_test(30);
	void* c = malloc_test(100);
	
	print_heap_chunks_info();

	// No merge will happen	
	free_test(b);

	print_heap_chunks_info();
	
	// `a` and `b` should be merged togther
	free_test(a);

	print_heap_chunks_info();

	// === 2.2 Testing `next` heap chunks merge ===
	printf("[+] Testing `next` chunks merge\n");

	// `c` and the new merged heap chunk (a & b heap chunks) should be merged
	free_test(c);

	print_heap_chunks_info();

}

void test_heap_data(){

    // ======================================================================================================
	// === 3. Storing data in the heap chunks ===
	
	// Create a list of integers using calloc_test
	printf("[+] Allocate a list of intgers with size of 5 using `calloc`\n");
	int list_len = 5;
	int* list_int = (int*)calloc_test(list_len, sizeof(int));
	
	print_heap_chunks_info();

	for (int i = 0; i < list_len; ++i) {
            list_int[i] = i * 10;
        }
	
	for (int i =0; i < list_len; ++i){
		printf("list_int[%d]: %d \n", i, list_int[i]);
	}

	printf("[+] Extend the list size to 10 using `realloc` \n");
	list_int = (int*)realloc_test(list_int,10 * sizeof(int));
	print_heap_chunks_info();

	for (int i =list_len; i < list_len+5; ++i){
		list_int[i] = i * 10;
	}

	for (int i = 0 ; i < 10; ++i){
		printf("list_int[%d]: %d \n", i, list_int[i]);
	}
	
	// Free the allocated list
	free_test(list_int);
	printf("=======================\n");
	printf("[+] heap chunks after free \n");
	print_heap_chunks_info();

}

int main(int argc, char *argv[]){
	//  Testing
	
	if (argc != 2){

		printf("format: ./testing [malloc|merge|data ]\n");
		return -1;
	}

	// convert to the arguement lower-case
	for(int i = 0; argv[1][i]; i++){
 		 argv[1][i] = tolower(argv[1][i]);
	}

	if (strcmp(argv[1],"malloc") && strcmp(argv[1],"merge") && strcmp(argv[1],"data")) {
		printf("format: ./testing [malloc|merge|data ]\n");
		return -1;
	}

	if (!strcmp(argv[1],"malloc")){

		test_malloc_and_free();
		return 0;
	}

	else if (!strcmp(argv[1],"merge")){

		test_merge();
		return 0;
	}

	else {
		test_heap_data();
		return 0;

	}

	return 0;
}