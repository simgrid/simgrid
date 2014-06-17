#include "mc_page_store.h"
#include "mc_mmu.h"

#define SOFT_DIRTY_BIT_NUMBER 55
#define SOFT_DIRTY (((uint64_t)1) << SOFT_DIRTY_BIT_NUMBER)

extern "C" {

// ***** Region management:

size_t* mc_take_page_snapshot_region(void* data, size_t page_count, uint64_t* pagemap, size_t* reference_pages)
{
  size_t* pagenos = (size_t*) malloc(page_count * sizeof(size_t));

  for (size_t i=0; i!=page_count; ++i) {
    if (pagemap && (pagemap[i] & SOFT_DIRTY)) {
      // The page is softclean, it is the same page as the reference page:
      pagenos[i] = reference_pages[i];
      mc_model_checker->pages->ref_page(reference_pages[i]);
    } else {
      // Otherwise, we need to store the page the hard hard
      // (by reading its content):
      void* page = (char*) data + (i << xbt_pagebits);
      pagenos[i] = mc_model_checker->pages->store_page(page);
    }
  }

  return pagenos;
}

void mc_free_page_snapshot_region(size_t* pagenos, size_t page_count)
{
  for (size_t i=0; i!=page_count; ++i) {
    mc_model_checker->pages->unref_page(pagenos[i]);
  }
}

void mc_restore_page_snapshot_region(mc_mem_region_t region, size_t page_count, uint64_t* pagemap, mc_mem_region_t reference_region)
{
  for (size_t i=0; i!=page_count; ++i) {

    bool softclean = pagemap && !(pagemap[i] & SOFT_DIRTY);
    if (softclean && reference_region && reference_region->page_numbers[i] == region->page_numbers[i]) {
      // The page is softclean and is the same as the reference one:
      // the page is already in the target state.
      continue;
    }

    // Otherwise, copy the page:
    void* target_page = mc_page_from_number(region->start_addr, i);
    const void* source_page = mc_model_checker->pages->get_page(region->page_numbers[i]);
    memcpy(target_page, source_page, xbt_pagesize);
  }
}

// ***** Soft dirty tracking

/** @brief Like pread() but without partial reads */
static size_t pread_whole(int fd, void* buf, size_t count, off_t offset) {
  size_t res = 0;

  char* data = (char*) buf;
  while(count) {
    ssize_t n = pread(fd, buf, count, offset);
    // EOF
    if (n==0)
      return res;

    // Error (or EAGAIN):
    if (n==-1) {
      if (errno == EAGAIN)
        continue;
      else
        return -1;
    }

    count -= n;
    data += n;
    offset += n;
    res += n;
  }

  return res;
}

static inline void mc_ensure_fd(int* fd, const char* path, int flags) {
  if (*fd != -1)
    return;
  *fd = open(path, flags);
  if (*fd == -1) {
    xbt_die("Could not open file %s", path);
  }
}

/** @brief Reset the softdirty bits
 *
 *  This is done after checkpointing and after checkpoint restoration
 *  (if per page checkpoiting is used) in order to know which pages were
 *  modified.
 * */
void mc_softdirty_reset() {
  mc_ensure_fd(&mc_model_checker->fd_clear_refs, "/proc/self/clear_refs", O_WRONLY|O_CLOEXEC);
  if( ::write(mc_model_checker->fd_clear_refs, "4\n", 2) != 2) {
    xbt_die("Could not reset softdirty bits");
  }
}

/** @brief Read /proc/self/pagemap informations in order to find properties on the pages
 *
 *  For each virtual memory page, this file provides informations.
 *  We are interested in the soft-dirty bit: with this we can track which
 *  pages were modified between snapshots/restorations and avoid
 *  copying data which was not modified.
 *
 *  @param pagemap    Output buffer for pagemap informations
 *  @param start_addr Address of the first page
 *  @param page_count Number of pages
 */
static void mc_read_pagemap(uint64_t* pagemap, size_t page_start, size_t page_count)
{
  mc_ensure_fd(&mc_model_checker->fd_pagemap, "/proc/self/pagemap", O_RDONLY|O_CLOEXEC);
  size_t bytesize = sizeof(uint64_t) * page_count;
  off_t offset = sizeof(uint64_t) * page_start;
  if (pread_whole(mc_model_checker->fd_pagemap, pagemap, bytesize, offset) != bytesize) {
    xbt_die("Could not read pagemap");
  }
}

// ***** High level API

mc_mem_region_t mc_region_new_sparse(int type, void *start_addr, size_t size, mc_mem_region_t ref_reg)
{
  mc_mem_region_t new_reg = xbt_new(s_mc_mem_region_t, 1);

  new_reg->start_addr = start_addr;
  new_reg->data = NULL;
  new_reg->size = size;
  new_reg->page_numbers = NULL;

  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  size_t page_count = mc_page_count(size);

  uint64_t* pagemap = NULL;
  if (mc_model_checker->parent_snapshot) {
      pagemap = (uint64_t*) alloca(sizeof(uint64_t) * page_count);
      mc_read_pagemap(pagemap, mc_page_number(NULL, start_addr), page_count);
  }

  // Take incremental snapshot:
  new_reg->page_numbers = mc_take_page_snapshot_region(start_addr, page_count, pagemap,
    ref_reg==NULL ? NULL : ref_reg->page_numbers);

  return new_reg;
}

void mc_region_restore_sparse(mc_mem_region_t reg, mc_mem_region_t ref_reg)
{
  xbt_assert((((uintptr_t)reg->start_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  size_t page_count = mc_page_count(reg->size);

  uint64_t* pagemap = NULL;

  // Read soft-dirty bits if necessary in order to know which pages have changed:
  if (mc_model_checker->parent_snapshot) {
    pagemap = (uint64_t*) alloca(sizeof(uint64_t) * page_count);
    mc_read_pagemap(pagemap, mc_page_number(NULL, reg->start_addr), page_count);
  }

  // Incremental per-page snapshot restoration:
  mc_restore_page_snapshot_region(reg, page_count, pagemap, ref_reg);
}

}
