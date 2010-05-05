#include <stdlib.h>
#include <stdio.h>
#include "mc/mc.h"
#include "xbt/sysdep.h"
#include "private.h"
#include "xbt/log.h"
#include <unistd.h>
#define _GNU_SOURCE

extern char *basename (__const char *__filename);

#define HEAP_OFFSET   20480000    /* Safety gap from the heap's break adress */
#define STD_HEAP_SIZE   20480000  /* Maximum size of the system's heap */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_memory, mc,
				"Logging specific to MC (memory)");

/* Pointers to each of the heap regions to use */
void *std_heap;
void *raw_heap;
void *actual_heap;

/* Pointers to the begining and end of the .data and .bss segment of libsimgrid */
/* They are initialized once at memory_init */
void *libsimgrid_data_addr_start = NULL;
size_t libsimgrid_data_size = 0;

/* Initialize the model-checker memory subsystem */
/* It creates the heap regions and set the default one */
void MC_memory_init()
{
/* Create the first region HEAP_OFFSET bytes after the heap break address */
  std_heap = mmalloc_attach(-1, (char *)sbrk(0) + HEAP_OFFSET);
  xbt_assert(std_heap != NULL);

/* Create the sencond region a page after the first one ends */  
/* FIXME: do not hardcode the page size to 4096 */
  raw_heap = mmalloc_attach(-1, (char *)(std_heap) + STD_HEAP_SIZE + 4096);
  xbt_assert(raw_heap != NULL);

  MC_SET_RAW_MEM;

/* Get the start address and size of libsimgrid's data segment */
/* CAVEAT: Here we are assuming several things, first that get_memory_map() */
/* returns an array with the maps sorted from lower addresses to higher */
/* ones. Second, that libsimgrid's data takes ONLY ONE page. */
  int i;
  char *libname, *tmp;

  /* Get memory map */
  memory_map_t maps;
  maps = get_memory_map();
  
  for(i=0; i < maps->mapsize; i++){
 
    if(maps->regions[i].inode != 0){
 
      tmp = xbt_strdup(maps->regions[i].pathname);
      libname = basename(tmp);
 
      if( strncmp("libsimgrid.so.2.0.0", libname, 18) == 0 && maps->regions[i].perms & MAP_WRITE){
        libsimgrid_data_addr_start = maps->regions[i].start_addr;
        libsimgrid_data_size = (size_t)((char *)maps->regions[i+1].end_addr - (char *)maps->regions[i].start_addr);
        xbt_free(tmp);
        break;
      }        
      xbt_free(tmp);
    }
  }
  
  xbt_assert0(libsimgrid_data_addr_start != NULL, 
             "Cannot determine libsimgrid's .data segment address");
             
  MC_UNSET_RAW_MEM;
}

/* Finish the memory subsystem */
void MC_memory_exit()
{
  mmalloc_detach(std_heap);
  mmalloc_detach(raw_heap);
  actual_heap = NULL;
}

void *malloc(size_t n)
{
  void *ret = mmalloc(actual_heap, n);
   
  DEBUG2("%zu bytes were allocated at %p",n, ret);
  return ret;
}

void *calloc(size_t nmemb, size_t size)
{
  size_t total_size = nmemb * size;
  void *ret = mmalloc(actual_heap, total_size);
   
/* Fill the allocated memory with zeroes to mimic calloc behaviour */
  memset(ret,'\0', total_size);

  DEBUG2("%zu bytes were mallocated and zeroed at %p",total_size, ret);
  return ret;
}
  
void *realloc(void *p, size_t s)
{
  void *ret = NULL;
	
  if (s) {
    if (p)
      ret = mrealloc(actual_heap, p,s);
    else
      ret = malloc(s);
  } else {
    if (p) {
      free(p);
    }
  }

  DEBUG2("%zu bytes were reallocated at %p",s,ret);
  return ret;
}

void free(void *p)
{
  DEBUG1("%p was freed",p);
  xbt_assert(actual_heap != NULL);
  return mfree(actual_heap, p);
}

/* FIXME: Horrible hack! because the mmalloc library doesn't provide yet of */
/* an API to query about the status of a heap, we simply call mmstats and */
/* because I now how does structure looks like, then I redefine it here */

struct mstats
  {
    size_t bytes_total;		/* Total size of the heap. */
    size_t chunks_used;		/* Chunks allocated by the user. */
    size_t bytes_used;		/* Byte total of user-allocated chunks. */
    size_t chunks_free;		/* Chunks in the free list. */
    size_t bytes_free;		/* Byte total of chunks in the free list. */
  };

extern struct mstats mmstats(void *);

/* Copy std_heap to "to_heap" allocating the required space for it */
size_t MC_save_heap(void **to_heap)
{  
  size_t heap_size = mmstats(std_heap).bytes_total;
  
  *to_heap = calloc(heap_size, 1);

  xbt_assert(*to_heap != NULL);
  
  memcpy(*to_heap, std_heap, heap_size);
  
  return heap_size;
}

/* Copy the data segment of libsimgrid to "data" allocating the space for it */
size_t MC_save_dataseg(void **data)
{
  *data = calloc(libsimgrid_data_size, 1);
  memcpy(*data, libsimgrid_data_addr_start, libsimgrid_data_size);
  return libsimgrid_data_size;
}

/* Restore std_heap from "src_heap" */
void MC_restore_heap(void *src_heap, size_t size)
{  
  memcpy(std_heap, src_heap, size);
}

/* Restore the data segment of libsimgrid from "src_data" */
void MC_restore_dataseg(void *src_data, size_t size)
{
  memcpy(libsimgrid_data_addr_start, src_data, size);
}





