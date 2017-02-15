# Virtual-Mem-Sim
Virtual memory simulator that uses demand paging, implemented with mmap/unmap/mprotect.

A single-linked list is used to manage the memory chunks allocated by the library. In other words, every time the user has a
"vm_alloc", a new node is added to the list. The list is populated with the adr_map structure, containing information about the vm_map structure, file pointers, occupied frames of RAM, and 2 vectors containing structures for organising the pages and frames of the memory chunk.

When allocating, 2 temporary files are created, that are removed once the process is finished. Alloc also maps the 3 pointers, for virtual memory, ram file and swap file, using mmap calls. All the attributes of the linked list elements are initialized here.

Vm_free is responsible of closing the file descriptors and unmaping memory. It also removes the corresponding adr_map element from the list.

The sigsegv_handler is responsible for intercepting the page faults. It is set in the vm_init, and is called for each unallocated memory access. Using the linked list element stored previously, it gradually allocates memory using mmap permissions.

The swap_in / swap_out algorithm is the simplest possible - it keeps replacing the first frame in the RAM once all the frames are allocated.
