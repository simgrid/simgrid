/* Copyright (c) 2002-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid's pthread interposer. Actual implementation of the symbols (see the comment in sthread.h) */

#include "xbt/asserts.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include <cerrno>
#include <cstring>
#include <err.h>
#include <simgrid/actor.h>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Mutex.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/s4u/Semaphore.hpp>
#include <sys/types.h>
#include <unistd.h>
#include <xbt/base.h>
#include <xbt/sysdep.h>

#include "src/internal_config.h"
#include "src/sthread/sthread.h"

#include <cmath>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sthread_disk, sthread, "pthread intercepter");
namespace sg4 = simgrid::s4u;

struct VirtualFile {
  int flags;
  std::vector<char>* content; // File content in memory (cache for written data)
  std::size_t offset = 0;     // Current offset for non-positioned operations (read/write/lseek)
};

static std::map<std::string, std::vector<char>> pathname_to_vfile; // pathname -> cached content
static std::map<int, VirtualFile>
    fd_to_vfile; // file descriptor -> VirtualFile. The content of each VF lives in pathname_to_vfile
static int next_virtual_fd =
    1000; // We arbitrarily skip some values. We could check in /proc which fd are used and which ones are free.

int sthread_open(const char* pathname, int flags, mode_t mode)
{
  // Create the file descriptor in our data structures
  int vfd            = next_virtual_fd++;
  VirtualFile* vfile = &fd_to_vfile[vfd];
  vfile->flags       = flags;
  vfile->offset      = 0;
  vfile->content     = &pathname_to_vfile[pathname];

  if (flags & O_TRUNC && (flags & O_RDWR || flags & O_WRONLY))
    // Trunc the file content as requested
    vfile->content->clear();
  else if (flags & O_RDWR || flags & O_RDONLY) {
    // Initialize the cache with the actual content, using the real function implementations
    sthread_disable();
    struct stat sb;
    int res = lstat(pathname, &sb);
    if (res != 0 && errno == ENOENT) {
      // File not found, no need to read its content
      sthread_enable();
      return vfd;
    }
    if (res != 0)
      xbt_backtrace_display_current();
    xbt_assert(res == 0, "Error in lstat(%s): %s", pathname, strerror(errno));
    vfile->content->resize(sb.st_size);
    int realfd = open(pathname, flags, mode);
    if (realfd == -1) {
      sthread_enable();
      return -1;
    }
    res = read(realfd, vfile->content->data(), sb.st_size);
    xbt_assert(res == sb.st_size, "Reading in %s yielded only %d bytes while stat() promised %d bytes (error: %s)",
               pathname, res, (int)sb.st_size, strerror(errno));
    close(realfd);
    sthread_enable();
  }

  return vfd;
}

int sthread_close(int fd)
{
  auto it = fd_to_vfile.find(fd);
  xbt_assert(it != fd_to_vfile.end(), "sthread is trying to close a file it didn't open. It's either a bug in your "
                                      "application, or in the sthread interception code");
  fd_to_vfile.erase(it);

  return 0;
}
int sthread_unlink(const char* pathname)
{
  auto it = pathname_to_vfile.find(pathname);
  if (it == pathname_to_vfile.end()) {
    errno = ENOENT;
    return -1;
  }
  pathname_to_vfile.erase(it);
  return 0;
}
ssize_t sthread_write(int fd, const void* buf, size_t count)
{
  auto it = fd_to_vfile.find(fd);
  if (it == fd_to_vfile.end()) {
    errno = EBADF;
    return -1;
  }
  VirtualFile* vfile = &it->second;

  int res = sthread_pwrite(fd, buf, count, vfile->offset);
  if (res > 0)
    vfile->offset += res;

  return res;
}
/* Writes data at a specific offset without modifying the file's current offset. */
ssize_t sthread_pwrite(int fd, const void* buf, size_t count, off_t offset)
{
  auto it = fd_to_vfile.find(fd);
  if (it == fd_to_vfile.end()) {
    errno = EBADF;
    return -1;
  }
  VirtualFile* vfile = &it->second;

  if (not(vfile->flags & O_RDWR || vfile->flags & O_WRONLY)) {
    errno = EBADF;
    return -1;
  }

  // Resize the vector if the write extends the file
  if (std::size_t new_size = offset + count; new_size > vfile->content->size())
    vfile->content->resize(new_size);

  memcpy(vfile->content->data() + offset, buf, count);

  errno = 0;
  return count;
}

ssize_t sthread_read(int fd, void* buf, size_t count)
{
  auto it = fd_to_vfile.find(fd);
  if (it == fd_to_vfile.end()) {
    errno = EBADF;
    return -1;
  }
  VirtualFile* vfile = &it->second;

  ssize_t result = sthread_pread(fd, buf, count, vfile->offset);
  if (result >= 0)
    vfile->offset += result;

  return result;
}
/* Reads data from a specific offset without modifying the file's current offset. */
ssize_t sthread_pread(int fd, void* buf, size_t count, off_t offset)
{
  auto it = fd_to_vfile.find(fd);
  if (it == fd_to_vfile.end()) {
    errno = EBADF;
    return -1;
  }
  VirtualFile* vfile = &it->second;

  if (not(vfile->flags & O_RDWR || vfile->flags & O_RDONLY)) {
    errno = EBADF;
    return -1;
  }

  // This may be a short read
  std::size_t bytes_to_read = std::min(count, vfile->content->size() - offset);

  if (bytes_to_read > 0)
    memcpy(buf, vfile->content->data() + offset, bytes_to_read);

  errno = 0;
  return bytes_to_read;
}
ssize_t sthread_readv(int fd, const struct iovec* iov, int iovcnt)
{
  ssize_t res = 0;
  for (int i = 0; i < iovcnt; i++) {
    ssize_t got = sthread_read(fd, iov[i].iov_base, iov[i].iov_len);
    if (got < 0)
      return got;
    res += got;
  }
  return res;
}
ssize_t sthread_writev(int fd, const struct iovec* iov, int iovcnt)
{
  ssize_t res = 0;
  for (int i = 0; i < iovcnt; i++) {
    ssize_t got = sthread_write(fd, iov[i].iov_base, iov[i].iov_len);
    if (got < 0)
      return got;
    res += got;
  }
  return res;
}
ssize_t sthread_preadv(int fd, const struct iovec* iov, int iovcnt, off_t offset)
{
  ssize_t res = 0;
  for (int i = 0; i < iovcnt; i++) {
    ssize_t got = sthread_pread(fd, iov[i].iov_base, iov[i].iov_len, res + offset);
    if (got < 0)
      return got;
    res += got;
  }
  return res;
}
ssize_t sthread_pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset)
{
  ssize_t res = 0;
  for (int i = 0; i < iovcnt; i++) {
    ssize_t got = sthread_pwrite(fd, iov[i].iov_base, iov[i].iov_len, res + offset);
    if (got < 0)
      return got;
    res += got;
  }
  return res;
}
ssize_t sthread_preadv2(int fd, const struct iovec* iov, int iovcnt, off_t offset, int flags)
{
  THROW_UNIMPLEMENTED;
}
ssize_t sthread_pwritev2(int fd, const struct iovec* iov, int iovcnt, off_t offset, int flags)
{
  THROW_UNIMPLEMENTED;
}

off_t sthread_lseek(int fd, off_t offset, int whence)
{
  auto it = fd_to_vfile.find(fd);
  if (it == fd_to_vfile.end()) {
    errno = EBADF;
    return -1;
  }
  VirtualFile* vfile = &it->second;

  off_t new_offset;
  off_t size = vfile->content->size();

  switch (whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = vfile->offset + offset;
      break;
    case SEEK_END:
      new_offset = size + offset;
      break;
    default:
      errno = EINVAL;
      return (off_t)-1;
  }

  if (new_offset < 0) {
    errno = EINVAL;
    return (off_t)-1;
  }

  vfile->offset = new_offset;
  return new_offset;
}

/* Returns file information, ensuring the correct size is reported for virtual files. */
int sthread_fstat(int fd, struct stat* buf)
{
  auto it = fd_to_vfile.find(fd);
  xbt_assert(it != fd_to_vfile.end(), "sthread is trying to fstat() in a file it didn't open. It's either a bug in "
                                      "your application, or in the sthread interception code");
  VirtualFile* vfile = &it->second;

  buf->st_dev = 1;                     // Let's say everything is on disk number one
  buf->st_ino = (ino_t)vfile->content; // Use the pointer value as an inode number

  buf->st_mode  = (buf->st_mode & ~S_IFMT) | S_IFREG; // Mark as a regular file
  buf->st_nlink = 1;
  buf->st_uid   = getuid();
  buf->st_gid   = getgid();
  buf->st_rdev  = 1;

  buf->st_size    = vfile->content->size();
  buf->st_blksize = 4096;
  buf->st_blocks  = (buf->st_size + 4095) / 4096;

  long now      = sg4::Engine::get_instance()->get_clock();
  buf->st_atime = now;
  buf->st_mtime = now;
  buf->st_ctime = now;

  return 0;
}
