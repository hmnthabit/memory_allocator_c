# Memory Allocator in C


Memory allocation/deallocation APIs implementation in C



## Impelemtenation

- The implemented allocator uses the `sbrk()` system call only for allocation/deallocation. 
- The search algorithm used by `malloc` is *best-fit algorithm*.
- Global double linked-list is used to store the allocated heap chunks headers addresses
- `free` merge the freed adjacent heap chunks to the target heap chunk to reduce the heap fragmentation
  - It releases the memory back the OS only if the target heap chunk is located at the top of the heap (program break).


- Implemented  APIs:
  - `malloc`: Allocate memory with the specified size and return a pointer to the start of the allocated space.
  - `free`: Free the allocated space by marking the allocated memory as free or releasing it to the OS.
  - `realloc`: Reallocate the allocated space to a specific size.
  - `calloc`: Allocate memory and initialize the allocated memory to zero.



## Testing

The testing program uses the implemented memory APIs and prints the status of the heap chunks after each operation 


```bash
# Compile
make 

# Testing malloc
./testing malloc

# Testing the merge funcionaily after the free
./testing merge

# Testing `realloc()` and `calloc()`
./testing data
```



## Future Improvements 

- Optimizing the storage by utilizing the bits operation (e.g. LSB) to reduce the heap chunk header struct size.
- Using `mmap()` instead of `sbrk()` for large allocations to make it easier to free the allocated space.
- Implement a segregated-list search to reduce the search time for the free heap chunks.