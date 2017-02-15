/*
 * Operating Systems Assignment 3 - Virtual Memory
 * Author: Cojocaru Mihail-Razvan 333CA
 */

#include "vmsim.h"
#include "helpers.h"
#include <stdlib.h>
#include <string.h>

struct adr_map {
	vm_map_t *map;
	w_size_t virt_size;
	w_size_t ram_size;
	w_ptr_t ram_ptr;
	w_ptr_t swap_ptr;
	struct page_table_entry *pages;
	struct frame *frames;
	int occupied_ram;
};

struct node {
	struct node *next;
	struct adr_map *data;
};

struct list {
	struct node *pfirst;
	struct node *plast;
};

/* Global list used to keep track of the allocated memory chuncks */
struct list mem_list;

/* Initialize the list. Must be called before any other operation */
void init_list(struct list *l)
{
	l->pfirst = NULL;
	l->plast = NULL;
}

/* Remove the first element from the list */
void pop(struct list *l)
{
	struct node *paux;

	if (l->pfirst == NULL)
		return;
	paux = l->pfirst;
	l->pfirst = (l->pfirst)->next;
	free(paux->data);
	free(paux);
	return;
}

/* Empty the list and free memory */
void free_list(struct list *l)
{
	while (l->pfirst != NULL) {
		pop(l);
	}
}

/* Add an element at the end of the list */
void add_elem(struct list *l, struct adr_map *data)
{
	struct node *paux;

	paux = (struct node *)malloc(sizeof(struct node));
	paux->data = data;
	paux->next = NULL;
	if (l->plast != NULL)
		l->plast->next = paux;
	l->plast = paux;
	if (l->pfirst == NULL)
		l->pfirst = paux;
}

/* Remove the given element from the list */
void remove_elem(struct list *l, struct adr_map *data)
{
	struct node *paux = l->pfirst;

	if (paux == NULL)
		return;

	if ((paux->next == NULL) && (paux->data == data)) {
		free(paux->data);
		free(paux);
		l->pfirst = NULL;
		l->plast = NULL;
	} else {
		struct node *prev = paux;

		do {
			if (paux->data == data) {
				prev->next = paux->next;
				if (l->pfirst == paux)
					l->pfirst = paux->next;
				if (l->plast == paux)
					l->plast = prev;
				free(paux->data);
				free(paux);
				break;
			}
			prev = paux;
			paux = paux->next;
		} while (paux != NULL);
	}
}

/* Find the element which contains the pointer ptr as the "start" attribute */
struct adr_map *find_ptr(struct list *l, w_ptr_t ptr)
{
	struct node *paux;

	paux = l->pfirst;
	if (paux == NULL)
		return NULL;
	if (((paux->data)->map)->start == ptr)
		return paux->data;

	while (paux != NULL) {
		if (((paux->data)->map)->start == ptr)
			return paux->data;
		paux = paux->next;
	}
	return NULL;
}

/* Return the element which has the given addres
 * in it's associated memory page
 */
struct adr_map *find_between(struct list *l, w_ptr_t ptr)
{
	struct node *paux;

	paux = l->pfirst;
	if (paux == NULL)
		return NULL;
	if ((((paux->data)->map)->start <= ptr) &&
		(((paux->data)->map)->start + (paux->data)->virt_size > ptr))
		return paux->data;

	while (paux != NULL) {
		if ((((paux->data)->map)->start <= ptr) &&
			(((paux->data)->map)->start + (paux->data)->virt_size > ptr))
			return paux->data;
		paux = paux->next;
	}
	return NULL;
}

/* Swap out the first page in the given memory allocation */
void swap_out(struct adr_map *alloc_elem)
{
	struct page_table_entry *old_page;
	int old_addr;
	char buffer[w_get_page_size()];
	w_ptr_t rc, swap;

	old_page = alloc_elem->frames[0].pte;
	old_addr = old_page->start - (alloc_elem->map)->start;

	/* Update page_table_entry attributes */
	old_page->prev_state = old_page->state;
	old_page->state = STATE_IN_SWAP;

	if (old_page->dirty == TRUE) {
		memcpy(&buffer, old_page->start, w_get_page_size());
	}

	/* Map the old page to the swap file */
	rc = mmap(old_page->start, w_get_page_size(), PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_FIXED, alloc_elem->map->swap_handle, old_addr);
	if (rc == MAP_FAILED)
		return;

	if (old_page->dirty == TRUE) {
		memcpy(old_page->start, &buffer, w_get_page_size());
	} else {
		memset(old_page->start, 0, w_get_page_size());
	}
}

/* Signal handler used to process page faults */
void sigsegv_handler(int signum, siginfo_t *info, void *context)
{
	if (signum != SIGSEGV)
		return;

	/* Address from which the page fault was generated */
	void *addr = info->si_addr;
	/* List element containing info about the memory allocation */
	struct adr_map *alloc_elem = find_between(&mem_list, addr);

	int pgno = ((int)(addr - (alloc_elem->map)->start)) / w_get_page_size();
	struct page_table_entry *page = &(alloc_elem->pages[pgno]);
	w_ptr_t rc;
	int *occupied_ram = &(alloc_elem->occupied_ram);


	if (page->state == STATE_NOT_ALLOC) {
		page->state = STATE_IN_RAM;
		page->protection = PROTECTION_READ;

		/* Check if RAM is full and perform swap out if necessary */
		if (*occupied_ram == (alloc_elem->ram_size / w_get_page_size())) {
			/* Swap out the first page */
			swap_out(alloc_elem);

			/* Replace with new page */
			rc = mmap(page->start, w_get_page_size(), PROT_READ,
				MAP_SHARED | MAP_FIXED, (alloc_elem->map)->ram_handle, 0);
			if (rc == MAP_FAILED)
				return;

			/* Update frames array */
			alloc_elem->frames[0].pte = page;
		} else {
			/* Memory not full, no swap required */
			/* Map virtual memory to the ram file, with read permissions */
			rc = mmap(page->start, w_get_page_size(), PROT_READ,
				MAP_SHARED | MAP_FIXED, (alloc_elem->map)->ram_handle,
				(*occupied_ram) * w_get_page_size());
			if (rc == MAP_FAILED)
				return;

			/* Update frames array */
			alloc_elem->frames[*occupied_ram].index = *occupied_ram;
			alloc_elem->frames[*occupied_ram].pte = page;

			(*occupied_ram)++;
		}

		return;
	}

	if ((page->state == STATE_IN_RAM) && (page->protection == PROTECTION_READ)) {
		page->protection = PROTECTION_WRITE;
		page->dirty = TRUE;

		/* Add write permissions and set to 0 */
		mprotect(page->start, w_get_page_size(), PROT_READ | PROT_WRITE);
		memset(page->start, 0, w_get_page_size());
		return;
	}

	if (page->state == STATE_IN_SWAP) {
		char buffer[w_get_page_size()];

		page->prev_state = STATE_IN_SWAP;
		page->state = STATE_IN_RAM;

		/* Swap out the first page */
		swap_out(alloc_elem);

		/* Copy data from swap */
		memcpy(&buffer, page->start, w_get_page_size());

		/* Swap in */
		/* Replace with new page */
		rc = mmap(page->start, w_get_page_size(), page->protection,
			MAP_SHARED | MAP_FIXED, (alloc_elem->map)->ram_handle, 0);
		if (rc == MAP_FAILED)
			return;

		alloc_elem->frames[0].pte = page;

		/* Copy data from swap */
		memcpy(page->start, &buffer, w_get_page_size());
		return;
	}
}

/* Initialize the allocator: initialize list + set page fault handler */
w_boolean_t vmsim_init(void)
{
	init_list(&mem_list);
	return w_set_exception_handler(sigsegv_handler);
}

/* Free memory and unset page fault handler */
w_boolean_t vmsim_cleanup(void)
{
	free_list(&mem_list);
	w_exception_handler_t prev_handler;
	w_get_previous_exception_handler(&prev_handler);
	return w_set_exception_handler(prev_handler);
}

/* Initialize the array of pages from each memory list element */
void init_pages(struct page_table_entry *pages, int size, w_ptr_t start)
{
	int i;
	int pagesize = w_get_page_size();
	for (i = 0; i < size; i++) {
		pages[i].start = start + i * pagesize;
		pages[i].state = STATE_NOT_ALLOC;
		pages[i].dirty = FALSE;
		pages[i].protection = PROTECTION_NONE;
		pages[i].frame = NULL;
	}
}

/* Initialize the array of pages from each memory list element */
void init_frames(struct frame *frames, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		frames[i].index = i;
		frames[i].pte = NULL;
	}
}

/* Allocate a chunk of memory */
w_boolean_t vm_alloc(w_size_t num_pages, w_size_t num_frames, vm_map_t *map)
{
	char *ram, *swap;
	w_size_t pagesize = w_get_page_size();
	w_ptr_t mem_ram, mem_swap;

	if (num_pages < num_frames)
		return FALSE;

	/* Create temporary files and set them to the appropriate sizes */
	/* Unlink is used to delete temporary files after pocess finishes */
	ram = malloc(10);
	swap = malloc(10);
	strcpy(ram, "ramXXXXXX");
	strcpy(swap, "swapXXXXXX");

	map->ram_handle = mkstemp(ram);
	if (map->ram_handle < 0) {
		free(ram);
		free(swap);
		return FALSE;
	}
	ftruncate(map->ram_handle, num_frames * pagesize);
	unlink(ram);

	map->swap_handle = mkstemp(swap);
	if (map->swap_handle < 0) {
		free(ram);
		free(swap);
		return FALSE;
	}
	ftruncate(map->swap_handle, num_pages * pagesize);
	unlink(swap);

	/* Map the virtual memory, RAM file and SWAP file */
	map->start = mmap(NULL, num_pages * pagesize, PROT_NONE,
		MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	if (mem_ram == MAP_FAILED) {
		free(ram);
		free(swap);
		return FALSE;
	}
	mem_ram = mmap(NULL, num_frames * pagesize, PROT_NONE,
		MAP_SHARED, map->ram_handle, 0);
	if (mem_ram == MAP_FAILED) {
		free(ram);
		free(swap);
		return FALSE;
	}
	mem_swap = mmap(NULL, num_pages * pagesize, PROT_NONE,
		MAP_SHARED, map->swap_handle, 0);
	if (mem_swap == MAP_FAILED) {
		free(ram);
		free(swap);
		return FALSE;
	}

	/* Initialize the memory list element and add it to the global list */
	struct adr_map *new_alloc = malloc(sizeof(struct adr_map));
	new_alloc->map = map;
	new_alloc->virt_size = num_pages * pagesize;
	new_alloc->ram_size = num_frames * pagesize;
	new_alloc->ram_ptr = mem_ram;
	new_alloc->swap_ptr = mem_swap;
	new_alloc->pages = malloc(num_pages * sizeof(struct page_table_entry));
	new_alloc->frames = malloc(num_frames * sizeof(struct frame));
	new_alloc->occupied_ram = 0;

	/* Initialize pages and frames arrays for memory list element */
	init_pages(new_alloc->pages, num_pages, (new_alloc->map)->start);
	init_frames(new_alloc->frames, num_frames);

	/* Add element to memory list */
	add_elem(&mem_list, new_alloc);

	free(ram);
	free(swap);
	return TRUE;
}

/* Free a chunk of memory, close files and remove element from list */
w_boolean_t vm_free(w_ptr_t start)
{
	if (start == NULL)
		return FALSE;

	struct adr_map *adress = NULL;
	int rc;

	/* Search in the list for the given element (search by start pointer) */
	adress = find_ptr(&mem_list, start);

	if (adress == NULL)
		return FALSE;

	/* Close files, free memory */
	close(adress->map->ram_handle);
	close(adress->map->swap_handle);

	free(adress->pages);
	free(adress->frames);

	/* Remove element from the memory list */
	remove_elem(&mem_list, adress);

	/* Unmap virtual memory */
	rc = munmap(start, adress->virt_size);
	if (rc != 0)
		return FALSE;

	rc = munmap(adress->swap_ptr, adress->virt_size);
	if (rc != 0)
		return FALSE;

	rc = munmap(adress->ram_ptr, adress->ram_size);
	if (rc != 0)
		return FALSE;

	return TRUE;
}