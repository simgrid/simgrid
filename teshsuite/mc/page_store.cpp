#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "mc/mc_page_store.h"

static int value = 0;

static void new_content(void* data, size_t size)
{
  memset(data, ++value, size);
}

static void* getpage()
{
  return mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

int main(int argc, char** argv)
{
  // Init
  size_t pagesize = (size_t) getpagesize();
  mc_pages_store_t store = new s_mc_pages_store(500);
  void* data = getpage();

  // Init:
  xbt_assert(store->size()==0, "Bad size");

  // Store the page once:
  new_content(data, pagesize);
  size_t pageno1 = store->store_page(data);
  xbt_assert(store->get_ref(pageno1)==1, "Bad refcount");
  const void* copy = store->get_page(pageno1);
  xbt_assert(memcmp(data, copy, pagesize)==0, "Page data should be the same");
  xbt_assert(store->size()==1, "Bad size");

  // Store the same page again:
  size_t pageno2 = store->store_page(data);
  xbt_assert(pageno1==pageno2, "Page should be the same");
  xbt_assert(store->get_ref(pageno1)==2, "Bad refcount");
  xbt_assert(store->size()==1, "Bad size");

  // Store a new page:
  new_content(data, pagesize);
  size_t pageno3 = store->store_page(data);
  xbt_assert(pageno1 != pageno3, "New page should be different");
  xbt_assert(store->size()==2, "Bad size");

  // Unref pages:
  store->unref_page(pageno1);
  xbt_assert(store->get_ref(pageno1)==1, "Bad refcount");
  xbt_assert(store->size()==2, "Bad size");
  store->unref_page(pageno2);
  xbt_assert(store->size()==1, "Bad size");

  // Reallocate page:
  new_content(data, pagesize);
  size_t pageno4 = store->store_page(data);
  xbt_assert(pageno1 == pageno4, "Page was not reused");
  xbt_assert(store->get_ref(pageno4)==1, "Bad refcount");
  xbt_assert(store->size()==2, "Bad size");

  return 0;
}
