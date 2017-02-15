#ifndef VMSIM_H_
#define VMSIM_H_

#include "common.h"

/* virtual memory mapping encapsulation; initialized by vm_alloc */
typedef struct vm_map {
	w_ptr_t start;
	w_handle_t ram_handle;
	w_handle_t swap_handle;
} vm_map_t;

/* initialize and cleanup library -- consider exception handler */
w_boolean_t vmsim_init(void);
w_boolean_t vmsim_cleanup(void);

/*
  * allocate physical pages in RAM mapped by virt_pages in swap
  * map is to be filled with start address and handle to RAM and swap
  * files
  */

w_boolean_t vm_alloc(w_size_t num_pages, w_size_t num_frames, vm_map_t *map);

/*
  * free space previously allocated with vm_alloc
  * start is the start address of the previously allocated area
  *
  * implementation has to also close handles corresponding to RAM and
  * swap files
  */

w_boolean_t vm_free(w_ptr_t start);

#endif
