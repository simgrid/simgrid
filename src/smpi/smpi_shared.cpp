/* Copyright (c) 2007, 2009-2017. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Shared allocations are handled through shared memory segments.
 * Associated data and metadata are used as follows:
 *
 *                                                                    mmap #1
 *    `allocs' dict                                                     ---- -.
 *    ----------      shared_data_t               shared_metadata_t   / |  |  |
 * .->| <name> | ---> -------------------- <--.   -----------------   | |  |  |
 * |  ----------      | fd of <name>     |    |   | size of mmap  | --| |  |  |
 * |                  | count (2)        |    |-- | data          |   \ |  |  |
 * `----------------- | <name>           |    |   -----------------     ----  |
 *                    --------------------    |   ^                           |
 *                                            |   |                           |
 *                                            |   |   `allocs_metadata' dict  |
 *                                            |   |   ----------------------  |
 *                                            |   `-- | <addr of mmap #1>  |<-'
 *                                            |   .-- | <addr of mmap #2>  |<-.
 *                                            |   |   ----------------------  |
 *                                            |   |                           |
 *                                            |   |                           |
 *                                            |   |                           |
 *                                            |   |                   mmap #2 |
 *                                            |   v                     ---- -'
 *                                            |   shared_metadata_t   / |  |
 *                                            |   -----------------   | |  |
 *                                            |   | size of mmap  | --| |  |
 *                                            `-- | data          |   | |  |
 *                                                -----------------   | |  |
 *                                                                    \ |  |
 *                                                                      ----
 */
#include <map>
#include <cstring>

#include "private.h"
#include "private.hpp"
#include "smpi/smpi_shared_malloc.hpp"
#include "xbt/dict.h"
#include "xbt/ex.hpp"
#include <errno.h>

#include <sys/types.h>
#ifndef WIN32
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_shared, smpi, "Logging specific to SMPI (shared memory macros)");

#define PTR_STRLEN (2 + 2 * sizeof(void*) + 1)

namespace{
/** Some location in the source code
 *
 *  This information is used by SMPI_SHARED_MALLOC to allocate  some shared memory for all simulated processes.
 */

class smpi_source_location {
public:
  smpi_source_location(const char* filename, int line)
      : filename(xbt_strdup(filename)), filename_length(strlen(filename)), line(line)
  {
  }

  /** Pointer to a static string containing the file name */
  char* filename      = nullptr;
  int filename_length = 0;
  int line            = 0;

  bool operator==(smpi_source_location const& that) const
  {
    return filename_length == that.filename_length && line == that.line &&
           std::memcmp(filename, that.filename, filename_length) == 0;
  }
  bool operator!=(smpi_source_location const& that) const { return !(*this == that); }
};
}

namespace std {

template <> class hash<smpi_source_location> {
public:
  typedef smpi_source_location argument_type;
  typedef std::size_t result_type;
  result_type operator()(smpi_source_location const& loc) const
  {
    return xbt_str_hash_ext(loc.filename, loc.filename_length) ^
           xbt_str_hash_ext((const char*)&loc.line, sizeof(loc.line));
  }
};
}

namespace{

typedef struct {
  int fd    = -1;
  int count = 0;
} shared_data_t;

std::unordered_map<smpi_source_location, shared_data_t> allocs;
typedef std::unordered_map<smpi_source_location, shared_data_t>::value_type shared_data_key_type;

typedef struct {
  size_t size;
  size_t allocated_size;
  void *allocated_ptr;
  std::vector<std::pair<size_t, size_t>> private_blocks;
  shared_data_key_type* data;
} shared_metadata_t;

std::map<void*, shared_metadata_t> allocs_metadata;
xbt_dict_t calls = nullptr;           /* Allocated on first use */
#ifndef WIN32
static int smpi_shared_malloc_bogusfile           = -1;
static int smpi_shared_malloc_bogusfile_huge_page  = -1;
static unsigned long smpi_shared_malloc_blocksize = 1UL << 20;
#endif
}


void smpi_shared_destroy()
{
  allocs.clear();
  allocs_metadata.clear();
  xbt_dict_free(&calls);
}

static size_t shm_size(int fd) {
  struct stat st;

  if(fstat(fd, &st) < 0) {
    xbt_die("Could not stat fd %d: %s", fd, strerror(errno));
  }
  return static_cast<size_t>(st.st_size);
}

#ifndef WIN32
static void* shm_map(int fd, size_t size, shared_data_key_type* data) {
  char loc[PTR_STRLEN];
  shared_metadata_t meta;

  if(size > shm_size(fd) && (ftruncate(fd, static_cast<off_t>(size)) < 0)) {
    xbt_die("Could not truncate fd %d to %zu: %s", fd, size, strerror(errno));
  }

  void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(mem == MAP_FAILED) {
    xbt_die(
        "Failed to map fd %d with size %zu: %s\n"
        "If you are running a lot of ranks, you may be exceeding the amount of mappings allowed per process.\n"
        "On Linux systems, change this value with sudo sysctl -w vm.max_map_count=newvalue (default value: 65536)\n"
        "Please see http://simgrid.gforge.inria.fr/simgrid/latest/doc/html/options.html#options_virt for more info.",
        fd, size, strerror(errno));
  }
  snprintf(loc, PTR_STRLEN, "%p", mem);
  meta.size = size;
  meta.data = data;
  allocs_metadata[mem] = meta;
  XBT_DEBUG("MMAP %zu to %p", size, mem);
  return mem;
}

static void *smpi_shared_malloc_local(size_t size, const char *file, int line)
{
  void* mem;
  smpi_source_location loc(file, line);
  auto res = allocs.insert(std::make_pair(loc, shared_data_t()));
  auto data = res.first;
  if (res.second) {
    // The insertion did not take place.
    // Generate a shared memory name from the address of the shared_data:
    char shmname[32]; // cannot be longer than PSHMNAMLEN = 31 on Mac OS X (shm_open raises ENAMETOOLONG otherwise)
    snprintf(shmname, 31, "/shmalloc%p", &*data);
    int fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
      if (errno == EEXIST)
        xbt_die("Please cleanup /dev/shm/%s", shmname);
      else
        xbt_die("An unhandled error occurred while opening %s. shm_open: %s", shmname, strerror(errno));
    }
    data->second.fd = fd;
    data->second.count = 1;
    mem = shm_map(fd, size, &*data);
    if (shm_unlink(shmname) < 0) {
      XBT_WARN("Could not early unlink %s. shm_unlink: %s", shmname, strerror(errno));
    }
    XBT_DEBUG("Mapping %s at %p through %d", shmname, mem, fd);
  } else {
    mem = shm_map(data->second.fd, size, &*data);
    data->second.count++;
  }
  XBT_DEBUG("Shared malloc %zu in %p (metadata at %p)", size, mem, &*data);
  return mem;
}

// Align functions, from http://stackoverflow.com/questions/4840410/how-to-align-a-pointer-in-c
#define PAGE_SIZE 0x1000
#define ALIGN_UP(n, align) (((n) + (align)-1) & -(align))
#define ALIGN_DOWN(n, align) ((n) & -(align))

#define HUGE_PAGE_SIZE 1<<21

/*
 * Similar to smpi_shared_malloc, but only sharing the blocks described by shared_block_offsets.
 * This array contains the offsets (in bytes) of the block to share.
 * Even indices are the start offsets (included), odd indices are the stop offsets (excluded).
 * For instance, if shared_block_offsets == {27, 42}, then the elements mem[27], mem[28], ..., mem[41] are shared. The others are not.
 */
void* smpi_shared_malloc_partial(size_t size, size_t* shared_block_offsets, int nb_shared_blocks)
{
  char *huge_page_mount_point = xbt_cfg_get_string("smpi/shared-malloc-hugepage");
  const bool use_huge_page = huge_page_mount_point[0] != '\0';
  smpi_shared_malloc_blocksize = static_cast<unsigned long>(xbt_cfg_get_double("smpi/shared-malloc-blocksize"));
  void *mem, *allocated_ptr;
  size_t allocated_size;
  if(use_huge_page) {
    xbt_assert(smpi_shared_malloc_blocksize == HUGE_PAGE_SIZE, "the block size of shared malloc should be equal to the size of a huge page.");
    allocated_size = size + 2*smpi_shared_malloc_blocksize;
  }
  else {
    xbt_assert(smpi_shared_malloc_blocksize % PAGE_SIZE == 0, "the block size of shared malloc should be a multiple of the page size.");
    allocated_size = size;
  }


  /* First reserve memory area */
  allocated_ptr = mmap(NULL, allocated_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);


  xbt_assert(allocated_ptr != MAP_FAILED, "Failed to allocate %zuMiB of memory. Run \"sysctl vm.overcommit_memory=1\" as root "
                                "to allow big allocations.\n",
             size >> 20);

  if(use_huge_page)
    mem = (void*)ALIGN_UP((uint64_t)allocated_ptr, HUGE_PAGE_SIZE);
  else
    mem = allocated_ptr;

  XBT_DEBUG("global shared allocation. Blocksize %lu", smpi_shared_malloc_blocksize);
  /* Create a fd to a new file on disk, make it smpi_shared_malloc_blocksize big, and unlink it.
   * It still exists in memory but not in the file system (thus it cannot be leaked). */
  /* Create bogus file if not done already */
  if(use_huge_page && smpi_shared_malloc_bogusfile_huge_page == -1) {
    const char *const array[] = {huge_page_mount_point, "simgrid-shmalloc-XXXXXX", nullptr};
    char *huge_page_filename = xbt_str_join_array(array, "/");
    smpi_shared_malloc_bogusfile_huge_page = mkstemp(huge_page_filename);
    XBT_DEBUG("bogusfile_huge_page: %s\n", huge_page_filename);
    unlink(huge_page_filename);
    xbt_free(huge_page_filename);
  }
  if(smpi_shared_malloc_bogusfile == -1) {
    char *name                   = xbt_strdup("/tmp/simgrid-shmalloc-XXXXXX");
    smpi_shared_malloc_bogusfile = mkstemp(name);
    XBT_DEBUG("bogusfile         : %s\n", name);
    unlink(name);
    xbt_free(name);
    char* dumb = (char*)calloc(1, smpi_shared_malloc_blocksize);
    ssize_t err = write(smpi_shared_malloc_bogusfile, dumb, smpi_shared_malloc_blocksize);
    if(err<0)
      xbt_die("Could not write bogus file for shared malloc");
    xbt_free(dumb);
  }

  int mmap_base_flag = MAP_FIXED | MAP_SHARED | MAP_POPULATE;
  int mmap_flag = mmap_base_flag;
  int huge_fd = use_huge_page ? smpi_shared_malloc_bogusfile_huge_page : smpi_shared_malloc_bogusfile;
  if(use_huge_page)
    mmap_flag |= MAP_HUGETLB;

  XBT_DEBUG("global shared allocation, begin mmap");

  /* Map the bogus file in place of the anonymous memory */
  for(int i_block = 0; i_block < nb_shared_blocks; i_block ++) {
    XBT_DEBUG("\tglobal shared allocation, mmap block %d/%d", i_block+1, nb_shared_blocks);
    size_t start_offset = shared_block_offsets[2*i_block];
    size_t stop_offset = shared_block_offsets[2*i_block+1];
    xbt_assert(start_offset < stop_offset, "start_offset (%zu) should be lower than stop offset (%zu)", start_offset, stop_offset);
    xbt_assert(stop_offset <= size,         "stop_offset (%zu) should be lower than size (%zu)", stop_offset, size);
    if(i_block < nb_shared_blocks-1)
      xbt_assert(stop_offset < shared_block_offsets[2*i_block+2],
              "stop_offset (%zu) should be lower than its successor start offset (%zu)", stop_offset, shared_block_offsets[2*i_block+2]);
    size_t start_block_offset = ALIGN_UP(start_offset, smpi_shared_malloc_blocksize);
    size_t stop_block_offset = ALIGN_DOWN(stop_offset, smpi_shared_malloc_blocksize);
    for (unsigned block_id=0, i = start_block_offset / smpi_shared_malloc_blocksize; i < stop_block_offset / smpi_shared_malloc_blocksize; block_id++, i++) {
      XBT_DEBUG("\t\tglobal shared allocation, mmap block offset %d", block_id);
      void* pos = (void*)((unsigned long)mem + i * smpi_shared_malloc_blocksize);
      void* res = mmap(pos, smpi_shared_malloc_blocksize, PROT_READ | PROT_WRITE, mmap_flag,
                       huge_fd, 0);
      xbt_assert(res == pos, "Could not map folded virtual memory (%s). Do you perhaps need to increase the "
                             "size of the mapped file using --cfg=smpi/shared-malloc-blocksize=newvalue (default 1048576) ? "
                             "You can also try using  the sysctl vm.max_map_count. "
                             "If you are using huge pages, check that you have at least one huge page (/proc/sys/vm/nr_hugepages) "
                             "and that the directory you are passing is mounted correctly (mount /path/to/huge -t hugetlbfs -o rw,mode=0777).",
                 strerror(errno));
    }
    size_t low_page_start_offset = ALIGN_UP(start_offset, PAGE_SIZE);
    size_t low_page_stop_offset = start_block_offset < ALIGN_DOWN(stop_offset, PAGE_SIZE) ? start_block_offset : ALIGN_DOWN(stop_offset, PAGE_SIZE);
    if(low_page_start_offset < low_page_stop_offset) {
      XBT_DEBUG("\t\tglobal shared allocation, mmap block start");
      void* pos = (void*)((unsigned long)mem + low_page_start_offset);
      void* res = mmap(pos, low_page_stop_offset-low_page_start_offset, PROT_READ | PROT_WRITE, mmap_base_flag, // not a full huge page
                       smpi_shared_malloc_bogusfile, 0);
      xbt_assert(res == pos, "Could not map folded virtual memory (%s). Do you perhaps need to increase the "
                             "size of the mapped file using --cfg=smpi/shared-malloc-blocksize=newvalue (default 1048576) ?"
                             "You can also try using  the sysctl vm.max_map_count",
                 strerror(errno));
    }
    if(low_page_stop_offset <= stop_block_offset) {
      XBT_DEBUG("\t\tglobal shared allocation, mmap block stop");
      size_t high_page_stop_offset = stop_offset == size ? size : ALIGN_DOWN(stop_offset, PAGE_SIZE);
      if(high_page_stop_offset > stop_block_offset) {
        void* pos = (void*)((unsigned long)mem + stop_block_offset);
        void* res = mmap(pos, high_page_stop_offset-stop_block_offset, PROT_READ | PROT_WRITE, mmap_base_flag, // not a full huge page
                         smpi_shared_malloc_bogusfile, 0);
        xbt_assert(res == pos, "Could not map folded virtual memory (%s). Do you perhaps need to increase the "
                               "size of the mapped file using --cfg=smpi/shared-malloc-blocksize=newvalue (default 1048576) ?"
                               "You can also try using  the sysctl vm.max_map_count",
                   strerror(errno));
      }
    }
  }

  shared_metadata_t newmeta;
  //register metadata for memcpy avoidance
  shared_data_key_type* data = (shared_data_key_type*)xbt_malloc(sizeof(shared_data_key_type));
  data->second.fd = -1;
  data->second.count = 1;
  newmeta.size = size;
  newmeta.data = data;
  newmeta.allocated_ptr = allocated_ptr;
  newmeta.allocated_size = allocated_size;
  if(shared_block_offsets[0] > 0) {
    newmeta.private_blocks.push_back(std::make_pair(0, shared_block_offsets[0]));
  }
  int i_block;
  for(i_block = 0; i_block < nb_shared_blocks-1; i_block ++) {
    newmeta.private_blocks.push_back(std::make_pair(shared_block_offsets[2*i_block+1], shared_block_offsets[2*i_block+2]));
  }
  if(shared_block_offsets[2*i_block+1] < size) {
    newmeta.private_blocks.push_back(std::make_pair(shared_block_offsets[2*i_block+1], size));
  }
  allocs_metadata[mem] = newmeta;

  XBT_DEBUG("global shared allocation, allocated_ptr %p - %p", allocated_ptr, (void*)(((uint64_t)allocated_ptr)+allocated_size));
  XBT_DEBUG("global shared allocation, returned_ptr  %p - %p", mem, (void*)(((uint64_t)mem)+size));

  return mem;
}

void *smpi_shared_malloc(size_t size, const char *file, int line) {
  if (size > 0 && smpi_cfg_shared_malloc == shmalloc_local) {
    return smpi_shared_malloc_local(size, file, line);
  } else if (smpi_cfg_shared_malloc == shmalloc_global) {
    int nb_shared_blocks = 1;
    size_t shared_block_offsets[2] = {0, size};
    return smpi_shared_malloc_partial(size, shared_block_offsets, nb_shared_blocks);
  }
  XBT_DEBUG("Classic malloc %zu", size);
  return xbt_malloc(size);
}

int smpi_is_shared(void* ptr, std::vector<std::pair<size_t, size_t>> &private_blocks, size_t *offset){
  private_blocks.clear(); // being paranoid
  if (allocs_metadata.empty())
    return 0;
  if ( smpi_cfg_shared_malloc == shmalloc_local || smpi_cfg_shared_malloc == shmalloc_global) {
    auto low = allocs_metadata.lower_bound(ptr);
    if (low->first==ptr) {
      private_blocks = low->second.private_blocks;
      *offset = 0;
      return 1;
    }
    if (low == allocs_metadata.begin())
      return 0;
    low --;
    if (ptr < (char*)low->first + low->second.size) {
      xbt_assert(ptr > (char*)low->first, "Oops, there seems to be a bug in the shared memory metadata.");
      *offset = ((uint8_t*)ptr) - ((uint8_t*) low->first);
      private_blocks = low->second.private_blocks;
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

std::vector<std::pair<size_t, size_t>> shift_and_frame_private_blocks(const std::vector<std::pair<size_t, size_t>> vec, size_t offset, size_t buff_size) {
    std::vector<std::pair<size_t, size_t>> result;
    for(auto block: vec) {
        auto new_block = std::make_pair(std::min(std::max((size_t)0, block.first-offset), buff_size),
                                        std::min(std::max((size_t)0, block.second-offset), buff_size));
        if(new_block.second > 0 && new_block.first < buff_size)
            result.push_back(new_block);
    }
    return result;
}

std::vector<std::pair<size_t, size_t>> merge_private_blocks(std::vector<std::pair<size_t, size_t>> src, std::vector<std::pair<size_t, size_t>> dst) {
  std::vector<std::pair<size_t, size_t>> result;
  unsigned i_src = 0;
  unsigned i_dst = 0;
  while(i_src < src.size() && i_dst < dst.size()) {
    std::pair<size_t, size_t> block;
    if(src[i_src].second <= dst[i_dst].first) {
        i_src++;
    }
    else if(dst[i_dst].second <= src[i_src].first) {
        i_dst++;
    }
    else { // src.second > dst.first && dst.second > src.first → the blocks are overlapping
      block = std::make_pair(std::max(src[i_src].first, dst[i_dst].first),
                             std::min(src[i_src].second, dst[i_dst].second));
      result.push_back(block);
      if(src[i_src].second < dst[i_dst].second)
          i_src ++;
      else
          i_dst ++;
    }
  }
  return result;
}

void smpi_shared_free(void *ptr)
{
  if (smpi_cfg_shared_malloc == shmalloc_local) {
    char loc[PTR_STRLEN];
    snprintf(loc, PTR_STRLEN, "%p", ptr);
    auto meta = allocs_metadata.find(ptr);
    if (meta == allocs_metadata.end()) {
      XBT_WARN("Cannot free: %p was not shared-allocated by SMPI - maybe its size was 0?", ptr);
      return;
    }
    shared_data_t* data = &meta->second.data->second;
    if (munmap(meta->second.allocated_ptr, meta->second.allocated_size) < 0) {
      XBT_WARN("Unmapping of fd %d failed: %s", data->fd, strerror(errno));
    }
    data->count--;
    if (data->count <= 0) {
      close(data->fd);
      allocs.erase(allocs.find(meta->second.data->first));
      allocs_metadata.erase(ptr);
      XBT_DEBUG("Shared free - with removal - of %p", ptr);
    } else {
      XBT_DEBUG("Shared free - no removal - of %p, count = %d", ptr, data->count);
    }

  } else if (smpi_cfg_shared_malloc == shmalloc_global) {
    auto meta = allocs_metadata.find(ptr);
    if (meta != allocs_metadata.end()){
      meta->second.data->second.count--;
      if(meta->second.data->second.count==0)
        xbt_free(meta->second.data);
    }

    munmap(ptr, meta->second.size);
  } else {
    XBT_DEBUG("Classic free of %p", ptr);
    xbt_free(ptr);
  }
}
#endif

int smpi_shared_known_call(const char* func, const char* input)
{
  char* loc = bprintf("%s:%s", func, input);
  int known = 0;

  if (calls==nullptr) {
    calls = xbt_dict_new_homogeneous(nullptr);
  }
  try {
    xbt_dict_get(calls, loc); /* Succeed or throw */
    known = 1;
    xbt_free(loc);
  }
  catch (xbt_ex& ex) {
    xbt_free(loc);
    if (ex.category != not_found_error)
      throw;
  }
  catch(...) {
    xbt_free(loc);
    throw;
  }
  return known;
}

void* smpi_shared_get_call(const char* func, const char* input) {
  char* loc = bprintf("%s:%s", func, input);

  if (calls == nullptr)
    calls    = xbt_dict_new_homogeneous(nullptr);
  void* data = xbt_dict_get(calls, loc);
  xbt_free(loc);
  return data;
}

void* smpi_shared_set_call(const char* func, const char* input, void* data) {
  char* loc = bprintf("%s:%s", func, input);

  if (calls == nullptr)
    calls = xbt_dict_new_homogeneous(nullptr);
  xbt_dict_set(calls, loc, data, nullptr);
  xbt_free(loc);
  return data;
}

