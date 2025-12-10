/* Copyright (c) 2002-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid's pthread interposer. Actual implementation of the symbols (see the comment in sthread.h) */

#include "simgrid/s4u/Barrier.hpp"
#include "simgrid/s4u/ConditionVariable.hpp"
#include "smpi/smpi.h"
#include "xbt/asserts.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/string.hpp"
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
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string_view>

XBT_LOG_NEW_DEFAULT_CATEGORY(sthread, "pthread intercepter");
namespace sg4 = simgrid::s4u;

static sg4::Host* lilibeth = nullptr;

static bool sthread_quiet = false;

int sthread_main(int argc, char** argv, char** envp, int (*raw_main)(int, char**, char**))
{
  /* Check whether we should keep silent about non-error situations */
  for (int i = 0; envp[i] != nullptr; i++)
    if (std::string_view(envp[i]).rfind("STHREAD_QUIET", 0) == 0)
      sthread_quiet = true;

  /* Do not initialize the simulation world when running from SMPI (but still intercepts pthread calls) */
  for (int i = 0; envp[i] != nullptr; i++)
    if (std::string_view(envp[i]).rfind("SMPI_GLOBAL_SIZE", 0) == 0) {
      if (not sthread_quiet)
        printf("sthread intercepts pthreads in your SMPI application.\n");
      return raw_main(argc, argv, envp);
    }

  /* Do not intercept system binaries such as valgrind step 1 */
  std::vector<std::string> binaries = {"/usr/bin/env", "/usr/bin/lua", "/usr/bin/valgrind.bin", "/usr/bin/python3",
                                       "/bin/sh", "/bin/bash", "addr2line", "cat", "dirname", "gdb", "grep", "ls",
                                       "ltrace", "make", "md5sum", "mktemp", "rm", "sed", "sh", "strace",
                                       // "simgrid-mc",
                                       "wc"};
  for (int i = 0; envp[i] != nullptr; i++) {
    auto view = std::string_view(envp[i]);
    /* If you want to ignore more than one binary, export STHREAD_IGNORE_BINARY1=toto STHREAD_IGNORE_BINARY2=tutu */
    /* Note that this cannot be configured with --cfg because we are before the main() */
    if (view.rfind("STHREAD_IGNORE_BINARY", 0) == 0) {
      view.remove_prefix(std::min(view.rfind("=") + 1, view.size()));
      binaries.push_back(std::string(view));
    }
  }
  auto binary_view = std::string_view(argv[0]);
  for (auto binary : binaries) {
    // FIXME in C++20, we could use `binary_view.ends_with(binary)`
    if (binary_view.size() >= binary.size() &&
        binary_view.compare(binary_view.size() - binary.size(), binary.size(), binary) == 0) {
      return raw_main(argc, argv, envp);
    }
  }

  if (not sthread_quiet) {
    fprintf(stderr,
            "sthread is intercepting the execution of %s. If it's not what you want, export STHREAD_IGNORE_BINARY=%s\n",
            argv[0], argv[0]);
  }

  sg4::Engine e(&argc, argv);
  auto* zone = e.get_netzone_root();
  lilibeth   = zone->add_host("Lilibeth", 1e15);
  zone->seal();

  /* If not in SMPI, the old main becomes an actor in a newly created simulation. Do not activate sthread yet: creating
   * contextes won't like it */
  sg4::ActorPtr main_actor = lilibeth->add_actor("main thread", raw_main, argc, argv, envp);
  sg4::Engine::get_instance()->run();

  if (not sthread_quiet)
    XBT_INFO("All threads exited. Terminating the simulation.");

  return 0;
}

struct sthread_mutex {
  s4u_Mutex* mutex;
};

int sthread_create(unsigned long int* thread, const void* /*pthread_attr_t* attr*/, void* (*start_routine)(void*),
                   void* arg)
{
  static int TID = 0;
  TID++;
  XBT_VERB("Create thread %d", TID);
  std::string name = std::string("thread ") + std::to_string(TID);
#if HAVE_SMPI
  if (SMPI_is_inited()) {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    name = simgrid::xbt::string_printf("%d:%d", rank, TID);
  }
  if (lilibeth == nullptr) {
    auto* zone = simgrid::s4u::Engine::get_instance()->get_netzone_root();
    for (auto* h : zone->get_all_hosts())
      if (h->get_name() == "Lilibeth")
        lilibeth = h;
    if (lilibeth == nullptr) { // Still not found. Let's create it
      zone->unseal();
      lilibeth = zone->add_host("Lilibeth", 1e15);
      zone->seal();
    }
    xbt_assert(lilibeth, "The host Lilibeth was not created. Something's wrong in sthread initialization.");
  }
#endif
  sg4::ActorPtr actor = lilibeth->add_actor(
      name,
      [](auto* user_function, auto* param) {
#if HAVE_SMPI
        if (SMPI_is_inited())
          SMPI_thread_create();
#endif
        sthread_enable();
        auto* ret = user_function(param);
        sthread_disable();
        sthread_exit(ret);
      },
      start_routine, arg);

  intrusive_ptr_add_ref(actor.get());
  *thread = reinterpret_cast<unsigned long>(actor.get());
  return 0;
}
int sthread_detach(sthread_t thread)
{
  return 0;
}
int sthread_join(sthread_t thread, void** retval)
{
  sg4::ActorPtr actor(reinterpret_cast<sg4::Actor*>(thread));
  actor->join();
  if (retval)
    *retval = actor->get_data<void>();
  intrusive_ptr_release(actor.get());

  return 0;
}
void sthread_exit(void* retval)
{
  if (retval)
    simgrid::s4u::Actor::self()->set_data(retval);
  simgrid::s4u::this_actor::exit();
}

int sthread_mutexattr_init(sthread_mutexattr_t* attr)
{
  memset(attr, 0, sizeof(*attr));
  return 0;
}
int sthread_mutexattr_settype(sthread_mutexattr_t* attr, int type)
{
  switch (type) {
    default: // Let's assume that unknown mutex policies behave as normal ones. That's somehow true for ADAPTIVE ones
    case PTHREAD_MUTEX_NORMAL:
      xbt_assert(not attr->recursive, "S4U does not allow to remove the recursivness of a mutex.");
      attr->recursive = 0;
      break;
    case PTHREAD_MUTEX_RECURSIVE:
      attr->recursive  = 1;
      attr->errorcheck = 0; // reset
      break;
    case PTHREAD_MUTEX_ERRORCHECK:
      attr->errorcheck = 1;
      break;
  }
  return 0;
}
int sthread_mutexattr_gettype(const sthread_mutexattr_t* attr, int* type)
{
  if (attr->recursive)
    *type = PTHREAD_MUTEX_RECURSIVE;
  else if (attr->errorcheck)
    *type = PTHREAD_MUTEX_ERRORCHECK;
  else
    *type = PTHREAD_MUTEX_NORMAL;
  return 0;
}
int sthread_mutexattr_getrobust(const sthread_mutexattr_t* attr, int* robustness)
{
  *robustness = attr->robust;
  return 0;
}
int sthread_mutexattr_setrobust(sthread_mutexattr_t* attr, int robustness)
{
  attr->robust = robustness;
  if (robustness)
    THROW_UNIMPLEMENTED;
  return 0;
}

int sthread_mutex_init(sthread_mutex_t* mutex, const sthread_mutexattr_t* attr)
{
  auto m = sg4::Mutex::create(attr != nullptr && attr->recursive);
  intrusive_ptr_add_ref(m.get());

  mutex->mutex      = m.get();
  mutex->errorcheck = attr ? attr->errorcheck : false;

  return 0;
}

int sthread_mutex_lock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, (xbt_log_no_loc ? (void*)0xDEADBEEF : mutex));
  if (mutex->errorcheck && static_cast<sg4::Mutex*>(mutex->mutex)->get_owner() != nullptr &&
      static_cast<sg4::Mutex*>(mutex->mutex)->get_owner()->get_pid() == simgrid::s4u::this_actor::get_pid())
    return EDEADLK;
  static_cast<sg4::Mutex*>(mutex->mutex)->lock();
  return 0;
}

int sthread_mutex_trylock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, mutex);
  if (static_cast<sg4::Mutex*>(mutex->mutex)->try_lock())
    return 0;
  return EBUSY;
}

int sthread_mutex_unlock(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  XBT_DEBUG("%s(%p)", __func__, (xbt_log_no_loc ? (void*)0xDEADBEEF : mutex));
  if (mutex->errorcheck &&
      (static_cast<sg4::Mutex*>(mutex->mutex)->get_owner() == nullptr ||
       static_cast<sg4::Mutex*>(mutex->mutex)->get_owner()->get_pid() != simgrid::s4u::this_actor::get_pid()))
    return EPERM;
  static_cast<sg4::Mutex*>(mutex->mutex)->unlock();
  return 0;
}
int sthread_mutex_destroy(sthread_mutex_t* mutex)
{
  /* At least in glibc, PTHREAD_STATIC_INITIALIZER sets every fields to 0 */
  if (mutex->mutex == nullptr)
    sthread_mutex_init(mutex, nullptr);

  xbt_assert(static_cast<sg4::Mutex*>(mutex->mutex)->get_owner() == nullptr,
             "Destroying a mutex that is still owned is UB. See https://cwe.mitre.org/data/definitions/667.html");

  XBT_DEBUG("%s(%p)", __func__, (xbt_log_no_loc ? (void*)0xDEADBEEF : mutex));
  intrusive_ptr_release(static_cast<sg4::Mutex*>(mutex->mutex));
  return 0;
}

int sthread_barrier_init(sthread_barrier_t* barrier, const sthread_barrierattr_t* attr, unsigned count)
{
  auto b = sg4::Barrier::create(count);
  intrusive_ptr_add_ref(b.get());

  barrier->barrier = b.get();
  return 0;
}
int sthread_barrier_wait(sthread_barrier_t* barrier)
{
  XBT_DEBUG("%s(%p)", __func__, barrier);
  static_cast<sg4::Barrier*>(barrier->barrier)->wait();
  return 0;
}
int sthread_barrier_destroy(sthread_barrier_t* barrier)
{
  XBT_DEBUG("%s(%p)", __func__, barrier);
  intrusive_ptr_release(static_cast<sg4::Barrier*>(barrier->barrier));
  return 0;
}

int sthread_cond_init(sthread_cond_t* cond, sthread_condattr_t* attr)
{
  auto cv = sg4::ConditionVariable::create();
  intrusive_ptr_add_ref(cv.get());

  cond->cond  = cv.get();
  cond->mutex = nullptr;
  return 0;
}
int sthread_cond_signal(sthread_cond_t* cond)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  /* At least in glibc, PTHREAD_COND_INITIALIZER sets every fields to 0 */
  if (cond->cond == nullptr)
    sthread_cond_init(cond, nullptr);

  if (cond->mutex != nullptr) {
    auto* owner = static_cast<sg4::Mutex*>(cond->mutex)->get_owner();
    if (owner == nullptr)
      XBT_WARN("The mutex associated to condition %p is not currently owned by anyone when calling "
               "pthread_cond_signal(). The signal could get lost.",
               cond);
    else if (owner != simgrid::s4u::Actor::self())
      XBT_WARN("The mutex associated to condition %p is currently owned by %s, not by the thread currently calling "
               "calling pthread_cond_signal(). The signal could get lost.",
               cond, owner->get_cname());
  }

  static_cast<sg4::ConditionVariable*>(cond->cond)->notify_one();
  return 0;
}
int sthread_cond_broadcast(sthread_cond_t* cond)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  /* At least in glibc, PTHREAD_COND_INITIALIZER sets every fields to 0 */
  if (cond->cond == nullptr)
    sthread_cond_init(cond, nullptr);

  if (cond->mutex != nullptr) {
    auto* owner = static_cast<sg4::Mutex*>(cond->mutex)->get_owner();
    if (owner == nullptr)
      XBT_WARN("The mutex associated to condition %p is not currently owned by anyone when calling "
               "pthread_cond_broadcast(). The signal could get lost.",
               cond);
    else if (owner != simgrid::s4u::Actor::self())
      XBT_WARN("The mutex associated to condition %p is currently owned by %s, not by the thread currently calling "
               "calling pthread_cond_broadcast(). The signal could get lost.",
               cond, owner->get_cname());
  }

  static_cast<sg4::ConditionVariable*>(cond->cond)->notify_all();
  return 0;
}
int sthread_cond_wait(sthread_cond_t* cond, sthread_mutex_t* mutex)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  /* At least in glibc, PTHREAD_COND_INITIALIZER sets every fields to 0 */
  if (cond->cond == nullptr)
    sthread_cond_init(cond, nullptr);

  if (cond->mutex == nullptr)
    cond->mutex = mutex->mutex;
  else if (cond->mutex != mutex->mutex)
    XBT_WARN("The condition %p is now waited with mutex %p while it was previoulsy waited with mutex %p. sthread may "
             "not work with such a dangerous code.",
             cond, cond->mutex, mutex->mutex);

  static_cast<sg4::ConditionVariable*>(cond->cond)->wait(static_cast<sg4::Mutex*>(mutex->mutex));
  return 0;
}
int sthread_cond_timedwait(sthread_cond_t* cond, sthread_mutex_t* mutex, const struct timespec* abs_timeout)
{
  XBT_DEBUG("%s(%p)", __func__, cond);

  if (cond->mutex == nullptr)
    cond->mutex = mutex->mutex;
  else if (cond->mutex != mutex->mutex)
    XBT_WARN("The condition %p is now waited with mutex %p while it was previoulsy waited with mutex %p. sthread may "
             "not work with such a dangerous code.",
             cond, cond->mutex, mutex->mutex);

  std::cv_status res = static_cast<sg4::ConditionVariable*>(cond->cond)
                           ->wait_until(static_cast<sg4::Mutex*>(mutex->mutex),
                                        abs_timeout->tv_sec + ((double)abs_timeout->tv_nsec) / 1000000);
  if (res == std::cv_status::timeout)
    return ETIMEDOUT;
  return 0;
}
int sthread_cond_destroy(sthread_cond_t* cond)
{
  XBT_DEBUG("%s(%p)", __func__, cond);
  if (cond->cond == nullptr)
    sthread_cond_init(cond, nullptr);
  intrusive_ptr_release(static_cast<sg4::ConditionVariable*>(cond->cond));
  cond->cond = nullptr;
  return 0;
}

int sthread_sem_init(sthread_sem_t* sem, int /*pshared*/, unsigned int value)
{
  auto s = sg4::Semaphore::create(value);
  intrusive_ptr_add_ref(s.get());

  sem->sem = s.get();
  return 0;
}
int sthread_sem_destroy(sthread_sem_t* sem)
{
  intrusive_ptr_release(static_cast<sg4::Semaphore*>(sem->sem));
  return 0;
}
int sthread_sem_post(sthread_sem_t* sem)
{
  static_cast<sg4::Semaphore*>(sem->sem)->release();
  return 0;
}
int sthread_sem_wait(sthread_sem_t* sem)
{
  static_cast<sg4::Semaphore*>(sem->sem)->acquire();
  return 0;
}
int sthread_sem_trywait(sthread_sem_t* sem)
{
  auto* s = static_cast<sg4::Semaphore*>(sem->sem);
  if (s->would_block()) {
    errno = EAGAIN;
    return -1;
  }
  s->acquire();
  return 0;
}
int sthread_sem_timedwait(sthread_sem_t* sem, const struct timespec* abs_timeout)
{
  if (static_cast<sg4::Semaphore*>(sem->sem)->acquire_timeout(static_cast<double>(abs_timeout->tv_sec) +
                                                              static_cast<double>(abs_timeout->tv_nsec) / 1E9)) {
    errno = ETIMEDOUT;
    return -1;
  }
  return 0;
}

int sthread_gettimeofday(struct timeval* tv)
{
  if (tv) {
    double now   = simgrid::s4u::Engine::get_clock();
    double secs  = trunc(now);
    double usecs = (now - secs) * 1e6;
    tv->tv_sec   = static_cast<time_t>(secs);
    tv->tv_usec  = static_cast<decltype(tv->tv_usec)>(usecs); // suseconds_t
  }
  return 0;
}

time_t sthread_time(time_t*)
{
  return trunc(simgrid::s4u::Engine::get_clock());
}

unsigned int sthread_sleep(double seconds)
{
  XBT_DEBUG("sleep(%lf)", seconds);
  simgrid::s4u::this_actor::sleep_for(seconds);
  return 0;
}
int sthread_usleep(double seconds)
{
  XBT_DEBUG("sleep(%lf)", seconds);
  simgrid::s4u::this_actor::sleep_for(seconds);
  return 0;
}

// --- START OF DISK INTERCEPTION IMPLEMENTATIONS ---
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
