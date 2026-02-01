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

static bool lambda_append_to_text(char* out, const char* str, size_t size, size_t used)
{
  const char* sep = (used > 0) ? " | " : "";
  size_t sep_len  = strlen(sep);
  size_t str_len  = strlen(str);
  size_t needed   = strlen(sep) + strlen(str) + 1; /* + '\0' */

  if (used + needed <= size) {
    if (sep_len > 0) {
      memcpy(out + used, sep, sep_len);
      used += sep_len;
    }
    memcpy(out + used, str, str_len);
    used += str_len;
    out[used] = '\0';
    return true;
  }

  /* too long. Truncate the resulting string */
  if (size >= 4) {
    memcpy(out + size - 4, "...", 3);
    out[size - 1] = '\0';
  } else if (size >= 1) {
    out[size - 1] = '\0';
  }
  return false;
}
static char* open_flags_to_text(int flags, char* out, size_t out_size)
{
  if (out == nullptr || out_size == 0)
    return out;

  size_t used = 0;
  out[0]      = '\0';

  /* access mode */
  switch (flags & O_ACCMODE) {
    case O_RDONLY:
      lambda_append_to_text(out, "O_RDONLY", out_size, used);
      break;
    case O_WRONLY:
      lambda_append_to_text(out, "O_WRONLY", out_size, used);
      break;
    case O_RDWR:
      lambda_append_to_text(out, "O_RDWR", out_size, used);
      break;
    default:
      lambda_append_to_text(out, "O_ACCMODE?", out_size, used);
      break;
  }

  static const std::vector<std::pair<int, const char*>> flag_table = {
      {O_CREAT, "O_CREAT"},         {O_EXCL, "O_EXCL"},       {O_TRUNC, "O_TRUNC"},
      {O_APPEND, "O_APPEND"},       {O_CLOEXEC, "O_CLOEXEC"}, {O_SYNC, "O_SYNC"},
#ifdef O_DSYNC
      {O_DSYNC, "O_DSYNC"},
#endif
#ifdef O_RSYNC
      {O_RSYNC, "O_RSYNC"},
#endif
#ifdef O_NONBLOCK
      {O_NONBLOCK, "O_NONBLOCK"},
#endif
#ifdef O_NOFOLLOW
      {O_NOFOLLOW, "O_NOFOLLOW"},
#endif
#ifdef O_DIRECTORY
      {O_DIRECTORY, "O_DIRECTORY"},
#endif
#ifdef O_TMPFILE
      {O_TMPFILE, "O_TMPFILE"},
#endif
  };

  for (const auto& p : flag_table) {
    if ((flags & p.first) && not lambda_append_to_text(out, p.second, out_size, used))
      return out;
  }

  return out;
}
static char* open_mod_to_text(mode_t mode, char* out, size_t out_size)
{
  if (out == nullptr || out_size == 0)
    return out;

  size_t used = 0;
  out[0]      = '\0';

  static const std::pair<mode_t, const char*> mode_table[] = {
      {S_IRUSR, "S_IRUSR"}, {S_IWUSR, "S_IWUSR"}, {S_IXUSR, "S_IXUSR"}, // owner read/write/execute
      {S_IRGRP, "S_IRGRP"}, {S_IWGRP, "S_IWGRP"}, {S_IXGRP, "S_IXGRP"}, // group read/write/execute
      {S_IROTH, "S_IROTH"}, {S_IWOTH, "S_IWOTH"}, {S_IXOTH, "S_IXOTH"}, // others read/write/execute
#ifdef S_ISUID
      {S_ISUID, "S_ISUID"}, // set-user-ID
#endif
#ifdef S_ISGID
      {S_ISGID, "S_ISGID"}, // set-group-ID
#endif
#ifdef S_ISVTX
      {S_ISVTX, "S_ISVTX"}, // sticky bit
#endif
  };

  for (const auto& p : mode_table) {
    if ((mode & p.first) && not lambda_append_to_text(out, p.second, out_size, used))
      return out;
  }

  if (out[0] == '\0')
    strcat(out, "0");

  return out;
}

int sthread_open(const char* pathname, int flags, mode_t mode)
{
  // Create the file descriptor in our data structures
  int vfd            = next_virtual_fd++;
  VirtualFile* vfile = &fd_to_vfile[vfd];
  vfile->flags       = flags;
  vfile->offset      = 0;
  vfile->content     = &pathname_to_vfile[pathname];

  // For debug messages
  char flags_buff[512];
  char mod_buff[512];

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

      XBT_VERB("open(%s, flags:%s, mode:%s) -> %d", pathname, open_flags_to_text(flags, flags_buff, 511),
               open_mod_to_text(mode, mod_buff, 511), vfd);
      return vfd;
    }
    if (res != 0)
      xbt_backtrace_display_current();
    xbt_assert(res == 0, "Error in lstat(%s): %s", pathname, strerror(errno));
    vfile->content->resize(sb.st_size);
    int realfd = open(pathname, flags, mode);
    if (realfd == -1) {
      sthread_enable();
      XBT_VERB("open(%s, flags:%s, mode:%s) -> -1", pathname, open_flags_to_text(flags, flags_buff, 511),
               open_mod_to_text(mode, mod_buff, 511));
      return -1;
    }
    res = read(realfd, vfile->content->data(), sb.st_size);
    xbt_assert(res == sb.st_size, "Reading in %s yielded only %d bytes while stat() promised %d bytes (error: %s)",
               pathname, res, (int)sb.st_size, strerror(errno));
    close(realfd);
    sthread_enable();
  }

  XBT_VERB("open(%s, flags:%s, mode:%s) -> %d", pathname, open_flags_to_text(flags, flags_buff, 511),
           open_mod_to_text(mode, mod_buff, 511), vfd);

  return vfd;
}

int sthread_close(int fd)
{
  auto it = fd_to_vfile.find(fd);
  if (it == fd_to_vfile.end()) {
    // That's maybe a socket instead of a file. So ask the OS to close it for itself
    sthread_disable();
    int res = close(fd);
    sthread_enable();
    return res;
  }

  fd_to_vfile.erase(it);
  XBT_VERB("close(fd:%d) -> 0", fd);

  return 0;
}
int sthread_unlink(const char* pathname)
{
  auto it = pathname_to_vfile.find(pathname);
  if (it == pathname_to_vfile.end()) {
    XBT_ERROR("Cannot unlink file %s which does not seem to exist in the virtualisation layer.", pathname);
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
    XBT_ERROR("File descriptor %d does not seem to be a valid sthread one. write() is failing.", fd);
    errno = EBADF;
    return -1;
  }
  VirtualFile* vfile = &it->second;

  ssize_t res = sthread_pwrite(fd, buf, count, vfile->offset);
  if (res > 0)
    vfile->offset += res;

  XBT_VERB("write(fd:%d, count:%zu) -> %zd", fd, count, res);

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
    char buffer[512];
    XBT_ERROR("The flags of the file descriptor %d are '%s'. write() not allowed.", fd,
              open_flags_to_text(vfile->flags, buffer, 511));
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
    XBT_ERROR("File descriptor %d does not seem to be a valid sthread one. read() is failing.", fd);
    errno = EBADF;
    return -1;
  }
  VirtualFile* vfile = &it->second;

  ssize_t result = sthread_pread(fd, buf, count, vfile->offset);
  if (result >= 0)
    vfile->offset += result;

  XBT_VERB("read(fd:%d, count:%zu) -> %zd", fd, count, result);
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
    char buffer[512];
    XBT_ERROR("The flags of the file descriptor %d are '%s'. read() not allowed.", fd,
              open_flags_to_text(vfile->flags, buffer, 511));
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
