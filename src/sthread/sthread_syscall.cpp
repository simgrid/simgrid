/* Copyright (c) 2002-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid's pthread interposer. Actual implementation of the symbols (see the comment in sthread.h) */

#include "src/sthread/sthread.h"
#include "xbt/log.h"

#include <sys/syscall.h>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sthread_syscall, sthread, "pthread intercepter");

static std::vector<std::pair<int, const char*>> table = {
#ifdef SYS_FAST_atomic_update
    {SYS_FAST_atomic_update, "SYS_FAST_atomic_update"},
#endif
#ifdef SYS_FAST_cmpxchg
    {SYS_FAST_cmpxchg, "SYS_FAST_cmpxchg"},
#endif
#ifdef SYS_FAST_cmpxchg64
    {SYS_FAST_cmpxchg64, "SYS_FAST_cmpxchg64"},
#endif
#ifdef SYS__llseek
    {SYS__llseek, "SYS__llseek"},
#endif
#ifdef SYS__newselect
    {SYS__newselect, "SYS__newselect"},
#endif
#ifdef SYS__sysctl
    {SYS__sysctl, "SYS__sysctl"},
#endif
#ifdef SYS_accept
    {SYS_accept, "SYS_accept"},
#endif
#ifdef SYS_accept4
    {SYS_accept4, "SYS_accept4"},
#endif
#ifdef SYS_access
    {SYS_access, "SYS_access"},
#endif
#ifdef SYS_acct
    {SYS_acct, "SYS_acct"},
#endif
#ifdef SYS_acl_get
    {SYS_acl_get, "SYS_acl_get"},
#endif
#ifdef SYS_acl_set
    {SYS_acl_set, "SYS_acl_set"},
#endif
#ifdef SYS_add_key
    {SYS_add_key, "SYS_add_key"},
#endif
#ifdef SYS_adjtimex
    {SYS_adjtimex, "SYS_adjtimex"},
#endif
#ifdef SYS_afs_syscall
    {SYS_afs_syscall, "SYS_afs_syscall"},
#endif
#ifdef SYS_alarm
    {SYS_alarm, "SYS_alarm"},
#endif
#ifdef SYS_alloc_hugepages
    {SYS_alloc_hugepages, "SYS_alloc_hugepages"},
#endif
#ifdef SYS_arc_gettls
    {SYS_arc_gettls, "SYS_arc_gettls"},
#endif
#ifdef SYS_arc_settls
    {SYS_arc_settls, "SYS_arc_settls"},
#endif
#ifdef SYS_arc_usr_cmpxchg
    {SYS_arc_usr_cmpxchg, "SYS_arc_usr_cmpxchg"},
#endif
#ifdef SYS_arch_prctl
    {SYS_arch_prctl, "SYS_arch_prctl"},
#endif
#ifdef SYS_arm_fadvise64_64
    {SYS_arm_fadvise64_64, "SYS_arm_fadvise64_64"},
#endif
#ifdef SYS_arm_sync_file_range
    {SYS_arm_sync_file_range, "SYS_arm_sync_file_range"},
#endif
#ifdef SYS_atomic_barrier
    {SYS_atomic_barrier, "SYS_atomic_barrier"},
#endif
#ifdef SYS_atomic_cmpxchg_32
    {SYS_atomic_cmpxchg_32, "SYS_atomic_cmpxchg_32"},
#endif
#ifdef SYS_attrctl
    {SYS_attrctl, "SYS_attrctl"},
#endif
#ifdef SYS_bdflush
    {SYS_bdflush, "SYS_bdflush"},
#endif
#ifdef SYS_bind
    {SYS_bind, "SYS_bind"},
#endif
#ifdef SYS_bpf
    {SYS_bpf, "SYS_bpf"},
#endif
#ifdef SYS_break
    {SYS_break, "SYS_break"},
#endif
#ifdef SYS_breakpoint
    {SYS_breakpoint, "SYS_breakpoint"},
#endif
#ifdef SYS_brk
    {SYS_brk, "SYS_brk"},
#endif
#ifdef SYS_cachectl
    {SYS_cachectl, "SYS_cachectl"},
#endif
#ifdef SYS_cacheflush
    {SYS_cacheflush, "SYS_cacheflush"},
#endif
#ifdef SYS_cachestat
    {SYS_cachestat, "SYS_cachestat"},
#endif
#ifdef SYS_capget
    {SYS_capget, "SYS_capget"},
#endif
#ifdef SYS_capset
    {SYS_capset, "SYS_capset"},
#endif
#ifdef SYS_chdir
    {SYS_chdir, "SYS_chdir"},
#endif
#ifdef SYS_chmod
    {SYS_chmod, "SYS_chmod"},
#endif
#ifdef SYS_chown
    {SYS_chown, "SYS_chown"},
#endif
#ifdef SYS_chown32
    {SYS_chown32, "SYS_chown32"},
#endif
#ifdef SYS_chroot
    {SYS_chroot, "SYS_chroot"},
#endif
#ifdef SYS_clock_adjtime
    {SYS_clock_adjtime, "SYS_clock_adjtime"},
#endif
#ifdef SYS_clock_adjtime64
    {SYS_clock_adjtime64, "SYS_clock_adjtime64"},
#endif
#ifdef SYS_clock_getres
    {SYS_clock_getres, "SYS_clock_getres"},
#endif
#ifdef SYS_clock_getres_time64
    {SYS_clock_getres_time64, "SYS_clock_getres_time64"},
#endif
#ifdef SYS_clock_gettime
    {SYS_clock_gettime, "SYS_clock_gettime"},
#endif
#ifdef SYS_clock_gettime64
    {SYS_clock_gettime64, "SYS_clock_gettime64"},
#endif
#ifdef SYS_clock_nanosleep
    {SYS_clock_nanosleep, "SYS_clock_nanosleep"},
#endif
#ifdef SYS_clock_nanosleep_time64
    {SYS_clock_nanosleep_time64, "SYS_clock_nanosleep_time64"},
#endif
#ifdef SYS_clock_settime
    {SYS_clock_settime, "SYS_clock_settime"},
#endif
#ifdef SYS_clock_settime64
    {SYS_clock_settime64, "SYS_clock_settime64"},
#endif
#ifdef SYS_clone
    {SYS_clone, "SYS_clone"},
#endif
#ifdef SYS_clone2
    {SYS_clone2, "SYS_clone2"},
#endif
#ifdef SYS_clone3
    {SYS_clone3, "SYS_clone3"},
#endif
#ifdef SYS_close
    {SYS_close, "SYS_close"},
#endif
#ifdef SYS_close_range
    {SYS_close_range, "SYS_close_range"},
#endif
#ifdef SYS_cmpxchg_badaddr
    {SYS_cmpxchg_badaddr, "SYS_cmpxchg_badaddr"},
#endif
#ifdef SYS_connect
    {SYS_connect, "SYS_connect"},
#endif
#ifdef SYS_copy_file_range
    {SYS_copy_file_range, "SYS_copy_file_range"},
#endif
#ifdef SYS_creat
    {SYS_creat, "SYS_creat"},
#endif
#ifdef SYS_create_module
    {SYS_create_module, "SYS_create_module"},
#endif
#ifdef SYS_delete_module
    {SYS_delete_module, "SYS_delete_module"},
#endif
#ifdef SYS_dipc
    {SYS_dipc, "SYS_dipc"},
#endif
#ifdef SYS_dup
    {SYS_dup, "SYS_dup"},
#endif
#ifdef SYS_dup2
    {SYS_dup2, "SYS_dup2"},
#endif
#ifdef SYS_dup3
    {SYS_dup3, "SYS_dup3"},
#endif
#ifdef SYS_epoll_create
    {SYS_epoll_create, "SYS_epoll_create"},
#endif
#ifdef SYS_epoll_create1
    {SYS_epoll_create1, "SYS_epoll_create1"},
#endif
#ifdef SYS_epoll_ctl
    {SYS_epoll_ctl, "SYS_epoll_ctl"},
#endif
#ifdef SYS_epoll_ctl_old
    {SYS_epoll_ctl_old, "SYS_epoll_ctl_old"},
#endif
#ifdef SYS_epoll_pwait
    {SYS_epoll_pwait, "SYS_epoll_pwait"},
#endif
#ifdef SYS_epoll_pwait2
    {SYS_epoll_pwait2, "SYS_epoll_pwait2"},
#endif
#ifdef SYS_epoll_wait
    {SYS_epoll_wait, "SYS_epoll_wait"},
#endif
#ifdef SYS_epoll_wait_old
    {SYS_epoll_wait_old, "SYS_epoll_wait_old"},
#endif
#ifdef SYS_eventfd
    {SYS_eventfd, "SYS_eventfd"},
#endif
#ifdef SYS_eventfd2
    {SYS_eventfd2, "SYS_eventfd2"},
#endif
#ifdef SYS_exec_with_loader
    {SYS_exec_with_loader, "SYS_exec_with_loader"},
#endif
#ifdef SYS_execv
    {SYS_execv, "SYS_execv"},
#endif
#ifdef SYS_execve
    {SYS_execve, "SYS_execve"},
#endif
#ifdef SYS_execveat
    {SYS_execveat, "SYS_execveat"},
#endif
#ifdef SYS_exit
    {SYS_exit, "SYS_exit"},
#endif
#ifdef SYS_exit_group
    {SYS_exit_group, "SYS_exit_group"},
#endif
#ifdef SYS_faccessat
    {SYS_faccessat, "SYS_faccessat"},
#endif
#ifdef SYS_faccessat2
    {SYS_faccessat2, "SYS_faccessat2"},
#endif
#ifdef SYS_fadvise64
    {SYS_fadvise64, "SYS_fadvise64"},
#endif
#ifdef SYS_fadvise64_64
    {SYS_fadvise64_64, "SYS_fadvise64_64"},
#endif
#ifdef SYS_fallocate
    {SYS_fallocate, "SYS_fallocate"},
#endif
#ifdef SYS_fanotify_init
    {SYS_fanotify_init, "SYS_fanotify_init"},
#endif
#ifdef SYS_fanotify_mark
    {SYS_fanotify_mark, "SYS_fanotify_mark"},
#endif
#ifdef SYS_fchdir
    {SYS_fchdir, "SYS_fchdir"},
#endif
#ifdef SYS_fchmod
    {SYS_fchmod, "SYS_fchmod"},
#endif
#ifdef SYS_fchmodat
    {SYS_fchmodat, "SYS_fchmodat"},
#endif
#ifdef SYS_fchmodat2
    {SYS_fchmodat2, "SYS_fchmodat2"},
#endif
#ifdef SYS_fchown
    {SYS_fchown, "SYS_fchown"},
#endif
#ifdef SYS_fchown32
    {SYS_fchown32, "SYS_fchown32"},
#endif
#ifdef SYS_fchownat
    {SYS_fchownat, "SYS_fchownat"},
#endif
#ifdef SYS_fcntl
    {SYS_fcntl, "SYS_fcntl"},
#endif
#ifdef SYS_fcntl64
    {SYS_fcntl64, "SYS_fcntl64"},
#endif
#ifdef SYS_fdatasync
    {SYS_fdatasync, "SYS_fdatasync"},
#endif
#ifdef SYS_fgetxattr
    {SYS_fgetxattr, "SYS_fgetxattr"},
#endif
#ifdef SYS_finit_module
    {SYS_finit_module, "SYS_finit_module"},
#endif
#ifdef SYS_flistxattr
    {SYS_flistxattr, "SYS_flistxattr"},
#endif
#ifdef SYS_flock
    {SYS_flock, "SYS_flock"},
#endif
#ifdef SYS_fork
    {SYS_fork, "SYS_fork"},
#endif
#ifdef SYS_fp_udfiex_crtl
    {SYS_fp_udfiex_crtl, "SYS_fp_udfiex_crtl"},
#endif
#ifdef SYS_free_hugepages
    {SYS_free_hugepages, "SYS_free_hugepages"},
#endif
#ifdef SYS_fremovexattr
    {SYS_fremovexattr, "SYS_fremovexattr"},
#endif
#ifdef SYS_fsconfig
    {SYS_fsconfig, "SYS_fsconfig"},
#endif
#ifdef SYS_fsetxattr
    {SYS_fsetxattr, "SYS_fsetxattr"},
#endif
#ifdef SYS_fsmount
    {SYS_fsmount, "SYS_fsmount"},
#endif
#ifdef SYS_fsopen
    {SYS_fsopen, "SYS_fsopen"},
#endif
#ifdef SYS_fspick
    {SYS_fspick, "SYS_fspick"},
#endif
#ifdef SYS_fstat
    {SYS_fstat, "SYS_fstat"},
#endif
#ifdef SYS_fstat64
    {SYS_fstat64, "SYS_fstat64"},
#endif
#ifdef SYS_fstatat64
    {SYS_fstatat64, "SYS_fstatat64"},
#endif
#ifdef SYS_fstatfs
    {SYS_fstatfs, "SYS_fstatfs"},
#endif
#ifdef SYS_fstatfs64
    {SYS_fstatfs64, "SYS_fstatfs64"},
#endif
#ifdef SYS_fsync
    {SYS_fsync, "SYS_fsync"},
#endif
#ifdef SYS_ftime
    {SYS_ftime, "SYS_ftime"},
#endif
#ifdef SYS_ftruncate
    {SYS_ftruncate, "SYS_ftruncate"},
#endif
#ifdef SYS_ftruncate64
    {SYS_ftruncate64, "SYS_ftruncate64"},
#endif
#ifdef SYS_futex
    {SYS_futex, "SYS_futex"},
#endif
#ifdef SYS_futex_requeue
    {SYS_futex_requeue, "SYS_futex_requeue"},
#endif
#ifdef SYS_futex_time64
    {SYS_futex_time64, "SYS_futex_time64"},
#endif
#ifdef SYS_futex_wait
    {SYS_futex_wait, "SYS_futex_wait"},
#endif
#ifdef SYS_futex_waitv
    {SYS_futex_waitv, "SYS_futex_waitv"},
#endif
#ifdef SYS_futex_wake
    {SYS_futex_wake, "SYS_futex_wake"},
#endif
#ifdef SYS_futimesat
    {SYS_futimesat, "SYS_futimesat"},
#endif
#ifdef SYS_get_kernel_syms
    {SYS_get_kernel_syms, "SYS_get_kernel_syms"},
#endif
#ifdef SYS_get_mempolicy
    {SYS_get_mempolicy, "SYS_get_mempolicy"},
#endif
#ifdef SYS_get_robust_list
    {SYS_get_robust_list, "SYS_get_robust_list"},
#endif
#ifdef SYS_get_thread_area
    {SYS_get_thread_area, "SYS_get_thread_area"},
#endif
#ifdef SYS_get_tls
    {SYS_get_tls, "SYS_get_tls"},
#endif
#ifdef SYS_getcpu
    {SYS_getcpu, "SYS_getcpu"},
#endif
#ifdef SYS_getcwd
    {SYS_getcwd, "SYS_getcwd"},
#endif
#ifdef SYS_getdents
    {SYS_getdents, "SYS_getdents"},
#endif
#ifdef SYS_getdents64
    {SYS_getdents64, "SYS_getdents64"},
#endif
#ifdef SYS_getdomainname
    {SYS_getdomainname, "SYS_getdomainname"},
#endif
#ifdef SYS_getdtablesize
    {SYS_getdtablesize, "SYS_getdtablesize"},
#endif
#ifdef SYS_getegid
    {SYS_getegid, "SYS_getegid"},
#endif
#ifdef SYS_getegid32
    {SYS_getegid32, "SYS_getegid32"},
#endif
#ifdef SYS_geteuid
    {SYS_geteuid, "SYS_geteuid"},
#endif
#ifdef SYS_geteuid32
    {SYS_geteuid32, "SYS_geteuid32"},
#endif
#ifdef SYS_getgid
    {SYS_getgid, "SYS_getgid"},
#endif
#ifdef SYS_getgid32
    {SYS_getgid32, "SYS_getgid32"},
#endif
#ifdef SYS_getgroups
    {SYS_getgroups, "SYS_getgroups"},
#endif
#ifdef SYS_getgroups32
    {SYS_getgroups32, "SYS_getgroups32"},
#endif
#ifdef SYS_gethostname
    {SYS_gethostname, "SYS_gethostname"},
#endif
#ifdef SYS_getitimer
    {SYS_getitimer, "SYS_getitimer"},
#endif
#ifdef SYS_getpagesize
    {SYS_getpagesize, "SYS_getpagesize"},
#endif
#ifdef SYS_getpeername
    {SYS_getpeername, "SYS_getpeername"},
#endif
#ifdef SYS_getpgid
    {SYS_getpgid, "SYS_getpgid"},
#endif
#ifdef SYS_getpgrp
    {SYS_getpgrp, "SYS_getpgrp"},
#endif
#ifdef SYS_getpid
    {SYS_getpid, "SYS_getpid"},
#endif
#ifdef SYS_getpmsg
    {SYS_getpmsg, "SYS_getpmsg"},
#endif
#ifdef SYS_getppid
    {SYS_getppid, "SYS_getppid"},
#endif
#ifdef SYS_getpriority
    {SYS_getpriority, "SYS_getpriority"},
#endif
#ifdef SYS_getrandom
    {SYS_getrandom, "SYS_getrandom"},
#endif
#ifdef SYS_getresgid
    {SYS_getresgid, "SYS_getresgid"},
#endif
#ifdef SYS_getresgid32
    {SYS_getresgid32, "SYS_getresgid32"},
#endif
#ifdef SYS_getresuid
    {SYS_getresuid, "SYS_getresuid"},
#endif
#ifdef SYS_getresuid32
    {SYS_getresuid32, "SYS_getresuid32"},
#endif
#ifdef SYS_getrlimit
    {SYS_getrlimit, "SYS_getrlimit"},
#endif
#ifdef SYS_getrusage
    {SYS_getrusage, "SYS_getrusage"},
#endif
#ifdef SYS_getsid
    {SYS_getsid, "SYS_getsid"},
#endif
#ifdef SYS_getsockname
    {SYS_getsockname, "SYS_getsockname"},
#endif
#ifdef SYS_getsockopt
    {SYS_getsockopt, "SYS_getsockopt"},
#endif
#ifdef SYS_gettid
    {SYS_gettid, "SYS_gettid"},
#endif
#ifdef SYS_gettimeofday
    {SYS_gettimeofday, "SYS_gettimeofday"},
#endif
#ifdef SYS_getuid
    {SYS_getuid, "SYS_getuid"},
#endif
#ifdef SYS_getuid32
    {SYS_getuid32, "SYS_getuid32"},
#endif
#ifdef SYS_getunwind
    {SYS_getunwind, "SYS_getunwind"},
#endif
#ifdef SYS_getxattr
    {SYS_getxattr, "SYS_getxattr"},
#endif
#ifdef SYS_getxgid
    {SYS_getxgid, "SYS_getxgid"},
#endif
#ifdef SYS_getxpid
    {SYS_getxpid, "SYS_getxpid"},
#endif
#ifdef SYS_getxuid
    {SYS_getxuid, "SYS_getxuid"},
#endif
#ifdef SYS_gtty
    {SYS_gtty, "SYS_gtty"},
#endif
#ifdef SYS_idle
    {SYS_idle, "SYS_idle"},
#endif
#ifdef SYS_init_module
    {SYS_init_module, "SYS_init_module"},
#endif
#ifdef SYS_inotify_add_watch
    {SYS_inotify_add_watch, "SYS_inotify_add_watch"},
#endif
#ifdef SYS_inotify_init
    {SYS_inotify_init, "SYS_inotify_init"},
#endif
#ifdef SYS_inotify_init1
    {SYS_inotify_init1, "SYS_inotify_init1"},
#endif
#ifdef SYS_inotify_rm_watch
    {SYS_inotify_rm_watch, "SYS_inotify_rm_watch"},
#endif
#ifdef SYS_io_cancel
    {SYS_io_cancel, "SYS_io_cancel"},
#endif
#ifdef SYS_io_destroy
    {SYS_io_destroy, "SYS_io_destroy"},
#endif
#ifdef SYS_io_getevents
    {SYS_io_getevents, "SYS_io_getevents"},
#endif
#ifdef SYS_io_pgetevents
    {SYS_io_pgetevents, "SYS_io_pgetevents"},
#endif
#ifdef SYS_io_pgetevents_time64
    {SYS_io_pgetevents_time64, "SYS_io_pgetevents_time64"},
#endif
#ifdef SYS_io_setup
    {SYS_io_setup, "SYS_io_setup"},
#endif
#ifdef SYS_io_submit
    {SYS_io_submit, "SYS_io_submit"},
#endif
#ifdef SYS_io_uring_enter
    {SYS_io_uring_enter, "SYS_io_uring_enter"},
#endif
#ifdef SYS_io_uring_register
    {SYS_io_uring_register, "SYS_io_uring_register"},
#endif
#ifdef SYS_io_uring_setup
    {SYS_io_uring_setup, "SYS_io_uring_setup"},
#endif
#ifdef SYS_ioctl
    {SYS_ioctl, "SYS_ioctl"},
#endif
#ifdef SYS_ioperm
    {SYS_ioperm, "SYS_ioperm"},
#endif
#ifdef SYS_iopl
    {SYS_iopl, "SYS_iopl"},
#endif
#ifdef SYS_ioprio_get
    {SYS_ioprio_get, "SYS_ioprio_get"},
#endif
#ifdef SYS_ioprio_set
    {SYS_ioprio_set, "SYS_ioprio_set"},
#endif
#ifdef SYS_ipc
    {SYS_ipc, "SYS_ipc"},
#endif
#ifdef SYS_kcmp
    {SYS_kcmp, "SYS_kcmp"},
#endif
#ifdef SYS_kern_features
    {SYS_kern_features, "SYS_kern_features"},
#endif
#ifdef SYS_kexec_file_load
    {SYS_kexec_file_load, "SYS_kexec_file_load"},
#endif
#ifdef SYS_kexec_load
    {SYS_kexec_load, "SYS_kexec_load"},
#endif
#ifdef SYS_keyctl
    {SYS_keyctl, "SYS_keyctl"},
#endif
#ifdef SYS_kill
    {SYS_kill, "SYS_kill"},
#endif
#ifdef SYS_landlock_add_rule
    {SYS_landlock_add_rule, "SYS_landlock_add_rule"},
#endif
#ifdef SYS_landlock_create_ruleset
    {SYS_landlock_create_ruleset, "SYS_landlock_create_ruleset"},
#endif
#ifdef SYS_landlock_restrict_self
    {SYS_landlock_restrict_self, "SYS_landlock_restrict_self"},
#endif
#ifdef SYS_lchown
    {SYS_lchown, "SYS_lchown"},
#endif
#ifdef SYS_lchown32
    {SYS_lchown32, "SYS_lchown32"},
#endif
#ifdef SYS_lgetxattr
    {SYS_lgetxattr, "SYS_lgetxattr"},
#endif
#ifdef SYS_link
    {SYS_link, "SYS_link"},
#endif
#ifdef SYS_linkat
    {SYS_linkat, "SYS_linkat"},
#endif
#ifdef SYS_listen
    {SYS_listen, "SYS_listen"},
#endif
#ifdef SYS_listmount
    {SYS_listmount, "SYS_listmount"},
#endif
#ifdef SYS_listxattr
    {SYS_listxattr, "SYS_listxattr"},
#endif
#ifdef SYS_llistxattr
    {SYS_llistxattr, "SYS_llistxattr"},
#endif
#ifdef SYS_llseek
    {SYS_llseek, "SYS_llseek"},
#endif
#ifdef SYS_lock
    {SYS_lock, "SYS_lock"},
#endif
#ifdef SYS_lookup_dcookie
    {SYS_lookup_dcookie, "SYS_lookup_dcookie"},
#endif
#ifdef SYS_lremovexattr
    {SYS_lremovexattr, "SYS_lremovexattr"},
#endif
#ifdef SYS_lseek
    {SYS_lseek, "SYS_lseek"},
#endif
#ifdef SYS_lsetxattr
    {SYS_lsetxattr, "SYS_lsetxattr"},
#endif
#ifdef SYS_lsm_get_self_attr
    {SYS_lsm_get_self_attr, "SYS_lsm_get_self_attr"},
#endif
#ifdef SYS_lsm_list_modules
    {SYS_lsm_list_modules, "SYS_lsm_list_modules"},
#endif
#ifdef SYS_lsm_set_self_attr
    {SYS_lsm_set_self_attr, "SYS_lsm_set_self_attr"},
#endif
#ifdef SYS_lstat
    {SYS_lstat, "SYS_lstat"},
#endif
#ifdef SYS_lstat64
    {SYS_lstat64, "SYS_lstat64"},
#endif
#ifdef SYS_madvise
    {SYS_madvise, "SYS_madvise"},
#endif
#ifdef SYS_map_shadow_stack
    {SYS_map_shadow_stack, "SYS_map_shadow_stack"},
#endif
#ifdef SYS_mbind
    {SYS_mbind, "SYS_mbind"},
#endif
#ifdef SYS_membarrier
    {SYS_membarrier, "SYS_membarrier"},
#endif
#ifdef SYS_memfd_create
    {SYS_memfd_create, "SYS_memfd_create"},
#endif
#ifdef SYS_memfd_secret
    {SYS_memfd_secret, "SYS_memfd_secret"},
#endif
#ifdef SYS_memory_ordering
    {SYS_memory_ordering, "SYS_memory_ordering"},
#endif
#ifdef SYS_migrate_pages
    {SYS_migrate_pages, "SYS_migrate_pages"},
#endif
#ifdef SYS_mincore
    {SYS_mincore, "SYS_mincore"},
#endif
#ifdef SYS_mkdir
    {SYS_mkdir, "SYS_mkdir"},
#endif
#ifdef SYS_mkdirat
    {SYS_mkdirat, "SYS_mkdirat"},
#endif
#ifdef SYS_mknod
    {SYS_mknod, "SYS_mknod"},
#endif
#ifdef SYS_mknodat
    {SYS_mknodat, "SYS_mknodat"},
#endif
#ifdef SYS_mlock
    {SYS_mlock, "SYS_mlock"},
#endif
#ifdef SYS_mlock2
    {SYS_mlock2, "SYS_mlock2"},
#endif
#ifdef SYS_mlockall
    {SYS_mlockall, "SYS_mlockall"},
#endif
#ifdef SYS_mmap
    {SYS_mmap, "SYS_mmap"},
#endif
#ifdef SYS_mmap2
    {SYS_mmap2, "SYS_mmap2"},
#endif
#ifdef SYS_modify_ldt
    {SYS_modify_ldt, "SYS_modify_ldt"},
#endif
#ifdef SYS_mount
    {SYS_mount, "SYS_mount"},
#endif
#ifdef SYS_mount_setattr
    {SYS_mount_setattr, "SYS_mount_setattr"},
#endif
#ifdef SYS_move_mount
    {SYS_move_mount, "SYS_move_mount"},
#endif
#ifdef SYS_move_pages
    {SYS_move_pages, "SYS_move_pages"},
#endif
#ifdef SYS_mprotect
    {SYS_mprotect, "SYS_mprotect"},
#endif
#ifdef SYS_mpx
    {SYS_mpx, "SYS_mpx"},
#endif
#ifdef SYS_mq_getsetattr
    {SYS_mq_getsetattr, "SYS_mq_getsetattr"},
#endif
#ifdef SYS_mq_notify
    {SYS_mq_notify, "SYS_mq_notify"},
#endif
#ifdef SYS_mq_open
    {SYS_mq_open, "SYS_mq_open"},
#endif
#ifdef SYS_mq_timedreceive
    {SYS_mq_timedreceive, "SYS_mq_timedreceive"},
#endif
#ifdef SYS_mq_timedreceive_time64
    {SYS_mq_timedreceive_time64, "SYS_mq_timedreceive_time64"},
#endif
#ifdef SYS_mq_timedsend
    {SYS_mq_timedsend, "SYS_mq_timedsend"},
#endif
#ifdef SYS_mq_timedsend_time64
    {SYS_mq_timedsend_time64, "SYS_mq_timedsend_time64"},
#endif
#ifdef SYS_mq_unlink
    {SYS_mq_unlink, "SYS_mq_unlink"},
#endif
#ifdef SYS_mremap
    {SYS_mremap, "SYS_mremap"},
#endif
#ifdef SYS_mseal
    {SYS_mseal, "SYS_mseal"},
#endif
#ifdef SYS_msgctl
    {SYS_msgctl, "SYS_msgctl"},
#endif
#ifdef SYS_msgget
    {SYS_msgget, "SYS_msgget"},
#endif
#ifdef SYS_msgrcv
    {SYS_msgrcv, "SYS_msgrcv"},
#endif
#ifdef SYS_msgsnd
    {SYS_msgsnd, "SYS_msgsnd"},
#endif
#ifdef SYS_msync
    {SYS_msync, "SYS_msync"},
#endif
#ifdef SYS_multiplexer
    {SYS_multiplexer, "SYS_multiplexer"},
#endif
#ifdef SYS_munlock
    {SYS_munlock, "SYS_munlock"},
#endif
#ifdef SYS_munlockall
    {SYS_munlockall, "SYS_munlockall"},
#endif
#ifdef SYS_munmap
    {SYS_munmap, "SYS_munmap"},
#endif
#ifdef SYS_name_to_handle_at
    {SYS_name_to_handle_at, "SYS_name_to_handle_at"},
#endif
#ifdef SYS_nanosleep
    {SYS_nanosleep, "SYS_nanosleep"},
#endif
#ifdef SYS_newfstatat
    {SYS_newfstatat, "SYS_newfstatat"},
#endif
#ifdef SYS_nfsservctl
    {SYS_nfsservctl, "SYS_nfsservctl"},
#endif
#ifdef SYS_ni_syscall
    {SYS_ni_syscall, "SYS_ni_syscall"},
#endif
#ifdef SYS_nice
    {SYS_nice, "SYS_nice"},
#endif
#ifdef SYS_old_adjtimex
    {SYS_old_adjtimex, "SYS_old_adjtimex"},
#endif
#ifdef SYS_old_getpagesize
    {SYS_old_getpagesize, "SYS_old_getpagesize"},
#endif
#ifdef SYS_oldfstat
    {SYS_oldfstat, "SYS_oldfstat"},
#endif
#ifdef SYS_oldlstat
    {SYS_oldlstat, "SYS_oldlstat"},
#endif
#ifdef SYS_oldolduname
    {SYS_oldolduname, "SYS_oldolduname"},
#endif
#ifdef SYS_oldstat
    {SYS_oldstat, "SYS_oldstat"},
#endif
#ifdef SYS_oldumount
    {SYS_oldumount, "SYS_oldumount"},
#endif
#ifdef SYS_olduname
    {SYS_olduname, "SYS_olduname"},
#endif
#ifdef SYS_open
    {SYS_open, "SYS_open"},
#endif
#ifdef SYS_open_by_handle_at
    {SYS_open_by_handle_at, "SYS_open_by_handle_at"},
#endif
#ifdef SYS_open_tree
    {SYS_open_tree, "SYS_open_tree"},
#endif
#ifdef SYS_openat
    {SYS_openat, "SYS_openat"},
#endif
#ifdef SYS_openat2
    {SYS_openat2, "SYS_openat2"},
#endif
#ifdef SYS_or1k_atomic
    {SYS_or1k_atomic, "SYS_or1k_atomic"},
#endif
#ifdef SYS_osf_adjtime
    {SYS_osf_adjtime, "SYS_osf_adjtime"},
#endif
#ifdef SYS_osf_afs_syscall
    {SYS_osf_afs_syscall, "SYS_osf_afs_syscall"},
#endif
#ifdef SYS_osf_alt_plock
    {SYS_osf_alt_plock, "SYS_osf_alt_plock"},
#endif
#ifdef SYS_osf_alt_setsid
    {SYS_osf_alt_setsid, "SYS_osf_alt_setsid"},
#endif
#ifdef SYS_osf_alt_sigpending
    {SYS_osf_alt_sigpending, "SYS_osf_alt_sigpending"},
#endif
#ifdef SYS_osf_asynch_daemon
    {SYS_osf_asynch_daemon, "SYS_osf_asynch_daemon"},
#endif
#ifdef SYS_osf_audcntl
    {SYS_osf_audcntl, "SYS_osf_audcntl"},
#endif
#ifdef SYS_osf_audgen
    {SYS_osf_audgen, "SYS_osf_audgen"},
#endif
#ifdef SYS_osf_chflags
    {SYS_osf_chflags, "SYS_osf_chflags"},
#endif
#ifdef SYS_osf_execve
    {SYS_osf_execve, "SYS_osf_execve"},
#endif
#ifdef SYS_osf_exportfs
    {SYS_osf_exportfs, "SYS_osf_exportfs"},
#endif
#ifdef SYS_osf_fchflags
    {SYS_osf_fchflags, "SYS_osf_fchflags"},
#endif
#ifdef SYS_osf_fdatasync
    {SYS_osf_fdatasync, "SYS_osf_fdatasync"},
#endif
#ifdef SYS_osf_fpathconf
    {SYS_osf_fpathconf, "SYS_osf_fpathconf"},
#endif
#ifdef SYS_osf_fstat
    {SYS_osf_fstat, "SYS_osf_fstat"},
#endif
#ifdef SYS_osf_fstatfs
    {SYS_osf_fstatfs, "SYS_osf_fstatfs"},
#endif
#ifdef SYS_osf_fstatfs64
    {SYS_osf_fstatfs64, "SYS_osf_fstatfs64"},
#endif
#ifdef SYS_osf_fuser
    {SYS_osf_fuser, "SYS_osf_fuser"},
#endif
#ifdef SYS_osf_getaddressconf
    {SYS_osf_getaddressconf, "SYS_osf_getaddressconf"},
#endif
#ifdef SYS_osf_getdirentries
    {SYS_osf_getdirentries, "SYS_osf_getdirentries"},
#endif
#ifdef SYS_osf_getdomainname
    {SYS_osf_getdomainname, "SYS_osf_getdomainname"},
#endif
#ifdef SYS_osf_getfh
    {SYS_osf_getfh, "SYS_osf_getfh"},
#endif
#ifdef SYS_osf_getfsstat
    {SYS_osf_getfsstat, "SYS_osf_getfsstat"},
#endif
#ifdef SYS_osf_gethostid
    {SYS_osf_gethostid, "SYS_osf_gethostid"},
#endif
#ifdef SYS_osf_getitimer
    {SYS_osf_getitimer, "SYS_osf_getitimer"},
#endif
#ifdef SYS_osf_getlogin
    {SYS_osf_getlogin, "SYS_osf_getlogin"},
#endif
#ifdef SYS_osf_getmnt
    {SYS_osf_getmnt, "SYS_osf_getmnt"},
#endif
#ifdef SYS_osf_getrusage
    {SYS_osf_getrusage, "SYS_osf_getrusage"},
#endif
#ifdef SYS_osf_getsysinfo
    {SYS_osf_getsysinfo, "SYS_osf_getsysinfo"},
#endif
#ifdef SYS_osf_gettimeofday
    {SYS_osf_gettimeofday, "SYS_osf_gettimeofday"},
#endif
#ifdef SYS_osf_kloadcall
    {SYS_osf_kloadcall, "SYS_osf_kloadcall"},
#endif
#ifdef SYS_osf_kmodcall
    {SYS_osf_kmodcall, "SYS_osf_kmodcall"},
#endif
#ifdef SYS_osf_lstat
    {SYS_osf_lstat, "SYS_osf_lstat"},
#endif
#ifdef SYS_osf_memcntl
    {SYS_osf_memcntl, "SYS_osf_memcntl"},
#endif
#ifdef SYS_osf_mincore
    {SYS_osf_mincore, "SYS_osf_mincore"},
#endif
#ifdef SYS_osf_mount
    {SYS_osf_mount, "SYS_osf_mount"},
#endif
#ifdef SYS_osf_mremap
    {SYS_osf_mremap, "SYS_osf_mremap"},
#endif
#ifdef SYS_osf_msfs_syscall
    {SYS_osf_msfs_syscall, "SYS_osf_msfs_syscall"},
#endif
#ifdef SYS_osf_msleep
    {SYS_osf_msleep, "SYS_osf_msleep"},
#endif
#ifdef SYS_osf_mvalid
    {SYS_osf_mvalid, "SYS_osf_mvalid"},
#endif
#ifdef SYS_osf_mwakeup
    {SYS_osf_mwakeup, "SYS_osf_mwakeup"},
#endif
#ifdef SYS_osf_naccept
    {SYS_osf_naccept, "SYS_osf_naccept"},
#endif
#ifdef SYS_osf_nfssvc
    {SYS_osf_nfssvc, "SYS_osf_nfssvc"},
#endif
#ifdef SYS_osf_ngetpeername
    {SYS_osf_ngetpeername, "SYS_osf_ngetpeername"},
#endif
#ifdef SYS_osf_ngetsockname
    {SYS_osf_ngetsockname, "SYS_osf_ngetsockname"},
#endif
#ifdef SYS_osf_nrecvfrom
    {SYS_osf_nrecvfrom, "SYS_osf_nrecvfrom"},
#endif
#ifdef SYS_osf_nrecvmsg
    {SYS_osf_nrecvmsg, "SYS_osf_nrecvmsg"},
#endif
#ifdef SYS_osf_nsendmsg
    {SYS_osf_nsendmsg, "SYS_osf_nsendmsg"},
#endif
#ifdef SYS_osf_ntp_adjtime
    {SYS_osf_ntp_adjtime, "SYS_osf_ntp_adjtime"},
#endif
#ifdef SYS_osf_ntp_gettime
    {SYS_osf_ntp_gettime, "SYS_osf_ntp_gettime"},
#endif
#ifdef SYS_osf_old_creat
    {SYS_osf_old_creat, "SYS_osf_old_creat"},
#endif
#ifdef SYS_osf_old_fstat
    {SYS_osf_old_fstat, "SYS_osf_old_fstat"},
#endif
#ifdef SYS_osf_old_getpgrp
    {SYS_osf_old_getpgrp, "SYS_osf_old_getpgrp"},
#endif
#ifdef SYS_osf_old_killpg
    {SYS_osf_old_killpg, "SYS_osf_old_killpg"},
#endif
#ifdef SYS_osf_old_lstat
    {SYS_osf_old_lstat, "SYS_osf_old_lstat"},
#endif
#ifdef SYS_osf_old_open
    {SYS_osf_old_open, "SYS_osf_old_open"},
#endif
#ifdef SYS_osf_old_sigaction
    {SYS_osf_old_sigaction, "SYS_osf_old_sigaction"},
#endif
#ifdef SYS_osf_old_sigblock
    {SYS_osf_old_sigblock, "SYS_osf_old_sigblock"},
#endif
#ifdef SYS_osf_old_sigreturn
    {SYS_osf_old_sigreturn, "SYS_osf_old_sigreturn"},
#endif
#ifdef SYS_osf_old_sigsetmask
    {SYS_osf_old_sigsetmask, "SYS_osf_old_sigsetmask"},
#endif
#ifdef SYS_osf_old_sigvec
    {SYS_osf_old_sigvec, "SYS_osf_old_sigvec"},
#endif
#ifdef SYS_osf_old_stat
    {SYS_osf_old_stat, "SYS_osf_old_stat"},
#endif
#ifdef SYS_osf_old_vadvise
    {SYS_osf_old_vadvise, "SYS_osf_old_vadvise"},
#endif
#ifdef SYS_osf_old_vtrace
    {SYS_osf_old_vtrace, "SYS_osf_old_vtrace"},
#endif
#ifdef SYS_osf_old_wait
    {SYS_osf_old_wait, "SYS_osf_old_wait"},
#endif
#ifdef SYS_osf_oldquota
    {SYS_osf_oldquota, "SYS_osf_oldquota"},
#endif
#ifdef SYS_osf_pathconf
    {SYS_osf_pathconf, "SYS_osf_pathconf"},
#endif
#ifdef SYS_osf_pid_block
    {SYS_osf_pid_block, "SYS_osf_pid_block"},
#endif
#ifdef SYS_osf_pid_unblock
    {SYS_osf_pid_unblock, "SYS_osf_pid_unblock"},
#endif
#ifdef SYS_osf_plock
    {SYS_osf_plock, "SYS_osf_plock"},
#endif
#ifdef SYS_osf_priocntlset
    {SYS_osf_priocntlset, "SYS_osf_priocntlset"},
#endif
#ifdef SYS_osf_profil
    {SYS_osf_profil, "SYS_osf_profil"},
#endif
#ifdef SYS_osf_proplist_syscall
    {SYS_osf_proplist_syscall, "SYS_osf_proplist_syscall"},
#endif
#ifdef SYS_osf_reboot
    {SYS_osf_reboot, "SYS_osf_reboot"},
#endif
#ifdef SYS_osf_revoke
    {SYS_osf_revoke, "SYS_osf_revoke"},
#endif
#ifdef SYS_osf_sbrk
    {SYS_osf_sbrk, "SYS_osf_sbrk"},
#endif
#ifdef SYS_osf_security
    {SYS_osf_security, "SYS_osf_security"},
#endif
#ifdef SYS_osf_select
    {SYS_osf_select, "SYS_osf_select"},
#endif
#ifdef SYS_osf_set_program_attributes
    {SYS_osf_set_program_attributes, "SYS_osf_set_program_attributes"},
#endif
#ifdef SYS_osf_set_speculative
    {SYS_osf_set_speculative, "SYS_osf_set_speculative"},
#endif
#ifdef SYS_osf_sethostid
    {SYS_osf_sethostid, "SYS_osf_sethostid"},
#endif
#ifdef SYS_osf_setitimer
    {SYS_osf_setitimer, "SYS_osf_setitimer"},
#endif
#ifdef SYS_osf_setlogin
    {SYS_osf_setlogin, "SYS_osf_setlogin"},
#endif
#ifdef SYS_osf_setsysinfo
    {SYS_osf_setsysinfo, "SYS_osf_setsysinfo"},
#endif
#ifdef SYS_osf_settimeofday
    {SYS_osf_settimeofday, "SYS_osf_settimeofday"},
#endif
#ifdef SYS_osf_shmat
    {SYS_osf_shmat, "SYS_osf_shmat"},
#endif
#ifdef SYS_osf_signal
    {SYS_osf_signal, "SYS_osf_signal"},
#endif
#ifdef SYS_osf_sigprocmask
    {SYS_osf_sigprocmask, "SYS_osf_sigprocmask"},
#endif
#ifdef SYS_osf_sigsendset
    {SYS_osf_sigsendset, "SYS_osf_sigsendset"},
#endif
#ifdef SYS_osf_sigstack
    {SYS_osf_sigstack, "SYS_osf_sigstack"},
#endif
#ifdef SYS_osf_sigwaitprim
    {SYS_osf_sigwaitprim, "SYS_osf_sigwaitprim"},
#endif
#ifdef SYS_osf_sstk
    {SYS_osf_sstk, "SYS_osf_sstk"},
#endif
#ifdef SYS_osf_stat
    {SYS_osf_stat, "SYS_osf_stat"},
#endif
#ifdef SYS_osf_statfs
    {SYS_osf_statfs, "SYS_osf_statfs"},
#endif
#ifdef SYS_osf_statfs64
    {SYS_osf_statfs64, "SYS_osf_statfs64"},
#endif
#ifdef SYS_osf_subsys_info
    {SYS_osf_subsys_info, "SYS_osf_subsys_info"},
#endif
#ifdef SYS_osf_swapctl
    {SYS_osf_swapctl, "SYS_osf_swapctl"},
#endif
#ifdef SYS_osf_swapon
    {SYS_osf_swapon, "SYS_osf_swapon"},
#endif
#ifdef SYS_osf_syscall
    {SYS_osf_syscall, "SYS_osf_syscall"},
#endif
#ifdef SYS_osf_sysinfo
    {SYS_osf_sysinfo, "SYS_osf_sysinfo"},
#endif
#ifdef SYS_osf_table
    {SYS_osf_table, "SYS_osf_table"},
#endif
#ifdef SYS_osf_uadmin
    {SYS_osf_uadmin, "SYS_osf_uadmin"},
#endif
#ifdef SYS_osf_usleep_thread
    {SYS_osf_usleep_thread, "SYS_osf_usleep_thread"},
#endif
#ifdef SYS_osf_uswitch
    {SYS_osf_uswitch, "SYS_osf_uswitch"},
#endif
#ifdef SYS_osf_utc_adjtime
    {SYS_osf_utc_adjtime, "SYS_osf_utc_adjtime"},
#endif
#ifdef SYS_osf_utc_gettime
    {SYS_osf_utc_gettime, "SYS_osf_utc_gettime"},
#endif
#ifdef SYS_osf_utimes
    {SYS_osf_utimes, "SYS_osf_utimes"},
#endif
#ifdef SYS_osf_utsname
    {SYS_osf_utsname, "SYS_osf_utsname"},
#endif
#ifdef SYS_osf_wait4
    {SYS_osf_wait4, "SYS_osf_wait4"},
#endif
#ifdef SYS_osf_waitid
    {SYS_osf_waitid, "SYS_osf_waitid"},
#endif
#ifdef SYS_pause
    {SYS_pause, "SYS_pause"},
#endif
#ifdef SYS_pciconfig_iobase
    {SYS_pciconfig_iobase, "SYS_pciconfig_iobase"},
#endif
#ifdef SYS_pciconfig_read
    {SYS_pciconfig_read, "SYS_pciconfig_read"},
#endif
#ifdef SYS_pciconfig_write
    {SYS_pciconfig_write, "SYS_pciconfig_write"},
#endif
#ifdef SYS_perf_event_open
    {SYS_perf_event_open, "SYS_perf_event_open"},
#endif
#ifdef SYS_perfctr
    {SYS_perfctr, "SYS_perfctr"},
#endif
#ifdef SYS_perfmonctl
    {SYS_perfmonctl, "SYS_perfmonctl"},
#endif
#ifdef SYS_personality
    {SYS_personality, "SYS_personality"},
#endif
#ifdef SYS_pidfd_getfd
    {SYS_pidfd_getfd, "SYS_pidfd_getfd"},
#endif
#ifdef SYS_pidfd_open
    {SYS_pidfd_open, "SYS_pidfd_open"},
#endif
#ifdef SYS_pidfd_send_signal
    {SYS_pidfd_send_signal, "SYS_pidfd_send_signal"},
#endif
#ifdef SYS_pipe
    {SYS_pipe, "SYS_pipe"},
#endif
#ifdef SYS_pipe2
    {SYS_pipe2, "SYS_pipe2"},
#endif
#ifdef SYS_pivot_root
    {SYS_pivot_root, "SYS_pivot_root"},
#endif
#ifdef SYS_pkey_alloc
    {SYS_pkey_alloc, "SYS_pkey_alloc"},
#endif
#ifdef SYS_pkey_free
    {SYS_pkey_free, "SYS_pkey_free"},
#endif
#ifdef SYS_pkey_mprotect
    {SYS_pkey_mprotect, "SYS_pkey_mprotect"},
#endif
#ifdef SYS_poll
    {SYS_poll, "SYS_poll"},
#endif
#ifdef SYS_ppoll
    {SYS_ppoll, "SYS_ppoll"},
#endif
#ifdef SYS_ppoll_time64
    {SYS_ppoll_time64, "SYS_ppoll_time64"},
#endif
#ifdef SYS_prctl
    {SYS_prctl, "SYS_prctl"},
#endif
#ifdef SYS_pread64
    {SYS_pread64, "SYS_pread64"},
#endif
#ifdef SYS_preadv
    {SYS_preadv, "SYS_preadv"},
#endif
#ifdef SYS_preadv2
    {SYS_preadv2, "SYS_preadv2"},
#endif
#ifdef SYS_prlimit64
    {SYS_prlimit64, "SYS_prlimit64"},
#endif
#ifdef SYS_process_madvise
    {SYS_process_madvise, "SYS_process_madvise"},
#endif
#ifdef SYS_process_mrelease
    {SYS_process_mrelease, "SYS_process_mrelease"},
#endif
#ifdef SYS_process_vm_readv
    {SYS_process_vm_readv, "SYS_process_vm_readv"},
#endif
#ifdef SYS_process_vm_writev
    {SYS_process_vm_writev, "SYS_process_vm_writev"},
#endif
#ifdef SYS_prof
    {SYS_prof, "SYS_prof"},
#endif
#ifdef SYS_profil
    {SYS_profil, "SYS_profil"},
#endif
#ifdef SYS_pselect6
    {SYS_pselect6, "SYS_pselect6"},
#endif
#ifdef SYS_pselect6_time64
    {SYS_pselect6_time64, "SYS_pselect6_time64"},
#endif
#ifdef SYS_ptrace
    {SYS_ptrace, "SYS_ptrace"},
#endif
#ifdef SYS_putpmsg
    {SYS_putpmsg, "SYS_putpmsg"},
#endif
#ifdef SYS_pwrite64
    {SYS_pwrite64, "SYS_pwrite64"},
#endif
#ifdef SYS_pwritev
    {SYS_pwritev, "SYS_pwritev"},
#endif
#ifdef SYS_pwritev2
    {SYS_pwritev2, "SYS_pwritev2"},
#endif
#ifdef SYS_query_module
    {SYS_query_module, "SYS_query_module"},
#endif
#ifdef SYS_quotactl
    {SYS_quotactl, "SYS_quotactl"},
#endif
#ifdef SYS_quotactl_fd
    {SYS_quotactl_fd, "SYS_quotactl_fd"},
#endif
#ifdef SYS_read
    {SYS_read, "SYS_read"},
#endif
#ifdef SYS_readahead
    {SYS_readahead, "SYS_readahead"},
#endif
#ifdef SYS_readdir
    {SYS_readdir, "SYS_readdir"},
#endif
#ifdef SYS_readlink
    {SYS_readlink, "SYS_readlink"},
#endif
#ifdef SYS_readlinkat
    {SYS_readlinkat, "SYS_readlinkat"},
#endif
#ifdef SYS_readv
    {SYS_readv, "SYS_readv"},
#endif
#ifdef SYS_reboot
    {SYS_reboot, "SYS_reboot"},
#endif
#ifdef SYS_recv
    {SYS_recv, "SYS_recv"},
#endif
#ifdef SYS_recvfrom
    {SYS_recvfrom, "SYS_recvfrom"},
#endif
#ifdef SYS_recvmmsg
    {SYS_recvmmsg, "SYS_recvmmsg"},
#endif
#ifdef SYS_recvmmsg_time64
    {SYS_recvmmsg_time64, "SYS_recvmmsg_time64"},
#endif
#ifdef SYS_recvmsg
    {SYS_recvmsg, "SYS_recvmsg"},
#endif
#ifdef SYS_remap_file_pages
    {SYS_remap_file_pages, "SYS_remap_file_pages"},
#endif
#ifdef SYS_removexattr
    {SYS_removexattr, "SYS_removexattr"},
#endif
#ifdef SYS_rename
    {SYS_rename, "SYS_rename"},
#endif
#ifdef SYS_renameat
    {SYS_renameat, "SYS_renameat"},
#endif
#ifdef SYS_renameat2
    {SYS_renameat2, "SYS_renameat2"},
#endif
#ifdef SYS_request_key
    {SYS_request_key, "SYS_request_key"},
#endif
#ifdef SYS_restart_syscall
    {SYS_restart_syscall, "SYS_restart_syscall"},
#endif
#ifdef SYS_riscv_flush_icache
    {SYS_riscv_flush_icache, "SYS_riscv_flush_icache"},
#endif
#ifdef SYS_riscv_hwprobe
    {SYS_riscv_hwprobe, "SYS_riscv_hwprobe"},
#endif
#ifdef SYS_rmdir
    {SYS_rmdir, "SYS_rmdir"},
#endif
#ifdef SYS_rseq
    {SYS_rseq, "SYS_rseq"},
#endif
#ifdef SYS_rt_sigaction
    {SYS_rt_sigaction, "SYS_rt_sigaction"},
#endif
#ifdef SYS_rt_sigpending
    {SYS_rt_sigpending, "SYS_rt_sigpending"},
#endif
#ifdef SYS_rt_sigprocmask
    {SYS_rt_sigprocmask, "SYS_rt_sigprocmask"},
#endif
#ifdef SYS_rt_sigqueueinfo
    {SYS_rt_sigqueueinfo, "SYS_rt_sigqueueinfo"},
#endif
#ifdef SYS_rt_sigreturn
    {SYS_rt_sigreturn, "SYS_rt_sigreturn"},
#endif
#ifdef SYS_rt_sigsuspend
    {SYS_rt_sigsuspend, "SYS_rt_sigsuspend"},
#endif
#ifdef SYS_rt_sigtimedwait
    {SYS_rt_sigtimedwait, "SYS_rt_sigtimedwait"},
#endif
#ifdef SYS_rt_sigtimedwait_time64
    {SYS_rt_sigtimedwait_time64, "SYS_rt_sigtimedwait_time64"},
#endif
#ifdef SYS_rt_tgsigqueueinfo
    {SYS_rt_tgsigqueueinfo, "SYS_rt_tgsigqueueinfo"},
#endif
#ifdef SYS_rtas
    {SYS_rtas, "SYS_rtas"},
#endif
#ifdef SYS_s390_guarded_storage
    {SYS_s390_guarded_storage, "SYS_s390_guarded_storage"},
#endif
#ifdef SYS_s390_pci_mmio_read
    {SYS_s390_pci_mmio_read, "SYS_s390_pci_mmio_read"},
#endif
#ifdef SYS_s390_pci_mmio_write
    {SYS_s390_pci_mmio_write, "SYS_s390_pci_mmio_write"},
#endif
#ifdef SYS_s390_runtime_instr
    {SYS_s390_runtime_instr, "SYS_s390_runtime_instr"},
#endif
#ifdef SYS_s390_sthyi
    {SYS_s390_sthyi, "SYS_s390_sthyi"},
#endif
#ifdef SYS_sched_get_affinity
    {SYS_sched_get_affinity, "SYS_sched_get_affinity"},
#endif
#ifdef SYS_sched_get_priority_max
    {SYS_sched_get_priority_max, "SYS_sched_get_priority_max"},
#endif
#ifdef SYS_sched_get_priority_min
    {SYS_sched_get_priority_min, "SYS_sched_get_priority_min"},
#endif
#ifdef SYS_sched_getaffinity
    {SYS_sched_getaffinity, "SYS_sched_getaffinity"},
#endif
#ifdef SYS_sched_getattr
    {SYS_sched_getattr, "SYS_sched_getattr"},
#endif
#ifdef SYS_sched_getparam
    {SYS_sched_getparam, "SYS_sched_getparam"},
#endif
#ifdef SYS_sched_getscheduler
    {SYS_sched_getscheduler, "SYS_sched_getscheduler"},
#endif
#ifdef SYS_sched_rr_get_interval
    {SYS_sched_rr_get_interval, "SYS_sched_rr_get_interval"},
#endif
#ifdef SYS_sched_rr_get_interval_time64
    {SYS_sched_rr_get_interval_time64, "SYS_sched_rr_get_interval_time64"},
#endif
#ifdef SYS_sched_set_affinity
    {SYS_sched_set_affinity, "SYS_sched_set_affinity"},
#endif
#ifdef SYS_sched_setaffinity
    {SYS_sched_setaffinity, "SYS_sched_setaffinity"},
#endif
#ifdef SYS_sched_setattr
    {SYS_sched_setattr, "SYS_sched_setattr"},
#endif
#ifdef SYS_sched_setparam
    {SYS_sched_setparam, "SYS_sched_setparam"},
#endif
#ifdef SYS_sched_setscheduler
    {SYS_sched_setscheduler, "SYS_sched_setscheduler"},
#endif
#ifdef SYS_sched_yield
    {SYS_sched_yield, "SYS_sched_yield"},
#endif
#ifdef SYS_seccomp
    {SYS_seccomp, "SYS_seccomp"},
#endif
#ifdef SYS_security
    {SYS_security, "SYS_security"},
#endif
#ifdef SYS_select
    {SYS_select, "SYS_select"},
#endif
#ifdef SYS_semctl
    {SYS_semctl, "SYS_semctl"},
#endif
#ifdef SYS_semget
    {SYS_semget, "SYS_semget"},
#endif
#ifdef SYS_semop
    {SYS_semop, "SYS_semop"},
#endif
#ifdef SYS_semtimedop
    {SYS_semtimedop, "SYS_semtimedop"},
#endif
#ifdef SYS_semtimedop_time64
    {SYS_semtimedop_time64, "SYS_semtimedop_time64"},
#endif
#ifdef SYS_send
    {SYS_send, "SYS_send"},
#endif
#ifdef SYS_sendfile
    {SYS_sendfile, "SYS_sendfile"},
#endif
#ifdef SYS_sendfile64
    {SYS_sendfile64, "SYS_sendfile64"},
#endif
#ifdef SYS_sendmmsg
    {SYS_sendmmsg, "SYS_sendmmsg"},
#endif
#ifdef SYS_sendmsg
    {SYS_sendmsg, "SYS_sendmsg"},
#endif
#ifdef SYS_sendto
    {SYS_sendto, "SYS_sendto"},
#endif
#ifdef SYS_set_mempolicy
    {SYS_set_mempolicy, "SYS_set_mempolicy"},
#endif
#ifdef SYS_set_mempolicy_home_node
    {SYS_set_mempolicy_home_node, "SYS_set_mempolicy_home_node"},
#endif
#ifdef SYS_set_robust_list
    {SYS_set_robust_list, "SYS_set_robust_list"},
#endif
#ifdef SYS_set_thread_area
    {SYS_set_thread_area, "SYS_set_thread_area"},
#endif
#ifdef SYS_set_tid_address
    {SYS_set_tid_address, "SYS_set_tid_address"},
#endif
#ifdef SYS_set_tls
    {SYS_set_tls, "SYS_set_tls"},
#endif
#ifdef SYS_setdomainname
    {SYS_setdomainname, "SYS_setdomainname"},
#endif
#ifdef SYS_setfsgid
    {SYS_setfsgid, "SYS_setfsgid"},
#endif
#ifdef SYS_setfsgid32
    {SYS_setfsgid32, "SYS_setfsgid32"},
#endif
#ifdef SYS_setfsuid
    {SYS_setfsuid, "SYS_setfsuid"},
#endif
#ifdef SYS_setfsuid32
    {SYS_setfsuid32, "SYS_setfsuid32"},
#endif
#ifdef SYS_setgid
    {SYS_setgid, "SYS_setgid"},
#endif
#ifdef SYS_setgid32
    {SYS_setgid32, "SYS_setgid32"},
#endif
#ifdef SYS_setgroups
    {SYS_setgroups, "SYS_setgroups"},
#endif
#ifdef SYS_setgroups32
    {SYS_setgroups32, "SYS_setgroups32"},
#endif
#ifdef SYS_sethae
    {SYS_sethae, "SYS_sethae"},
#endif
#ifdef SYS_sethostname
    {SYS_sethostname, "SYS_sethostname"},
#endif
#ifdef SYS_setitimer
    {SYS_setitimer, "SYS_setitimer"},
#endif
#ifdef SYS_setns
    {SYS_setns, "SYS_setns"},
#endif
#ifdef SYS_setpgid
    {SYS_setpgid, "SYS_setpgid"},
#endif
#ifdef SYS_setpgrp
    {SYS_setpgrp, "SYS_setpgrp"},
#endif
#ifdef SYS_setpriority
    {SYS_setpriority, "SYS_setpriority"},
#endif
#ifdef SYS_setregid
    {SYS_setregid, "SYS_setregid"},
#endif
#ifdef SYS_setregid32
    {SYS_setregid32, "SYS_setregid32"},
#endif
#ifdef SYS_setresgid
    {SYS_setresgid, "SYS_setresgid"},
#endif
#ifdef SYS_setresgid32
    {SYS_setresgid32, "SYS_setresgid32"},
#endif
#ifdef SYS_setresuid
    {SYS_setresuid, "SYS_setresuid"},
#endif
#ifdef SYS_setresuid32
    {SYS_setresuid32, "SYS_setresuid32"},
#endif
#ifdef SYS_setreuid
    {SYS_setreuid, "SYS_setreuid"},
#endif
#ifdef SYS_setreuid32
    {SYS_setreuid32, "SYS_setreuid32"},
#endif
#ifdef SYS_setrlimit
    {SYS_setrlimit, "SYS_setrlimit"},
#endif
#ifdef SYS_setsid
    {SYS_setsid, "SYS_setsid"},
#endif
#ifdef SYS_setsockopt
    {SYS_setsockopt, "SYS_setsockopt"},
#endif
#ifdef SYS_settimeofday
    {SYS_settimeofday, "SYS_settimeofday"},
#endif
#ifdef SYS_setuid
    {SYS_setuid, "SYS_setuid"},
#endif
#ifdef SYS_setuid32
    {SYS_setuid32, "SYS_setuid32"},
#endif
#ifdef SYS_setxattr
    {SYS_setxattr, "SYS_setxattr"},
#endif
#ifdef SYS_sgetmask
    {SYS_sgetmask, "SYS_sgetmask"},
#endif
#ifdef SYS_shmat
    {SYS_shmat, "SYS_shmat"},
#endif
#ifdef SYS_shmctl
    {SYS_shmctl, "SYS_shmctl"},
#endif
#ifdef SYS_shmdt
    {SYS_shmdt, "SYS_shmdt"},
#endif
#ifdef SYS_shmget
    {SYS_shmget, "SYS_shmget"},
#endif
#ifdef SYS_shutdown
    {SYS_shutdown, "SYS_shutdown"},
#endif
#ifdef SYS_sigaction
    {SYS_sigaction, "SYS_sigaction"},
#endif
#ifdef SYS_sigaltstack
    {SYS_sigaltstack, "SYS_sigaltstack"},
#endif
#ifdef SYS_signal
    {SYS_signal, "SYS_signal"},
#endif
#ifdef SYS_signalfd
    {SYS_signalfd, "SYS_signalfd"},
#endif
#ifdef SYS_signalfd4
    {SYS_signalfd4, "SYS_signalfd4"},
#endif
#ifdef SYS_sigpending
    {SYS_sigpending, "SYS_sigpending"},
#endif
#ifdef SYS_sigprocmask
    {SYS_sigprocmask, "SYS_sigprocmask"},
#endif
#ifdef SYS_sigreturn
    {SYS_sigreturn, "SYS_sigreturn"},
#endif
#ifdef SYS_sigsuspend
    {SYS_sigsuspend, "SYS_sigsuspend"},
#endif
#ifdef SYS_socket
    {SYS_socket, "SYS_socket"},
#endif
#ifdef SYS_socketcall
    {SYS_socketcall, "SYS_socketcall"},
#endif
#ifdef SYS_socketpair
    {SYS_socketpair, "SYS_socketpair"},
#endif
#ifdef SYS_splice
    {SYS_splice, "SYS_splice"},
#endif
#ifdef SYS_spu_create
    {SYS_spu_create, "SYS_spu_create"},
#endif
#ifdef SYS_spu_run
    {SYS_spu_run, "SYS_spu_run"},
#endif
#ifdef SYS_ssetmask
    {SYS_ssetmask, "SYS_ssetmask"},
#endif
#ifdef SYS_stat
    {SYS_stat, "SYS_stat"},
#endif
#ifdef SYS_stat64
    {SYS_stat64, "SYS_stat64"},
#endif
#ifdef SYS_statfs
    {SYS_statfs, "SYS_statfs"},
#endif
#ifdef SYS_statfs64
    {SYS_statfs64, "SYS_statfs64"},
#endif
#ifdef SYS_statmount
    {SYS_statmount, "SYS_statmount"},
#endif
#ifdef SYS_statx
    {SYS_statx, "SYS_statx"},
#endif
#ifdef SYS_stime
    {SYS_stime, "SYS_stime"},
#endif
#ifdef SYS_stty
    {SYS_stty, "SYS_stty"},
#endif
#ifdef SYS_subpage_prot
    {SYS_subpage_prot, "SYS_subpage_prot"},
#endif
#ifdef SYS_swapcontext
    {SYS_swapcontext, "SYS_swapcontext"},
#endif
#ifdef SYS_swapoff
    {SYS_swapoff, "SYS_swapoff"},
#endif
#ifdef SYS_swapon
    {SYS_swapon, "SYS_swapon"},
#endif
#ifdef SYS_switch_endian
    {SYS_switch_endian, "SYS_switch_endian"},
#endif
#ifdef SYS_symlink
    {SYS_symlink, "SYS_symlink"},
#endif
#ifdef SYS_symlinkat
    {SYS_symlinkat, "SYS_symlinkat"},
#endif
#ifdef SYS_sync
    {SYS_sync, "SYS_sync"},
#endif
#ifdef SYS_sync_file_range
    {SYS_sync_file_range, "SYS_sync_file_range"},
#endif
#ifdef SYS_sync_file_range2
    {SYS_sync_file_range2, "SYS_sync_file_range2"},
#endif
#ifdef SYS_syncfs
    {SYS_syncfs, "SYS_syncfs"},
#endif
#ifdef SYS_sys_debug_setcontext
    {SYS_sys_debug_setcontext, "SYS_sys_debug_setcontext"},
#endif
#ifdef SYS_sys_epoll_create
    {SYS_sys_epoll_create, "SYS_sys_epoll_create"},
#endif
#ifdef SYS_sys_epoll_ctl
    {SYS_sys_epoll_ctl, "SYS_sys_epoll_ctl"},
#endif
#ifdef SYS_sys_epoll_wait
    {SYS_sys_epoll_wait, "SYS_sys_epoll_wait"},
#endif
#ifdef SYS_syscall
    {SYS_syscall, "SYS_syscall"},
#endif
#ifdef SYS_sysfs
    {SYS_sysfs, "SYS_sysfs"},
#endif
#ifdef SYS_sysinfo
    {SYS_sysinfo, "SYS_sysinfo"},
#endif
#ifdef SYS_syslog
    {SYS_syslog, "SYS_syslog"},
#endif
#ifdef SYS_sysmips
    {SYS_sysmips, "SYS_sysmips"},
#endif
#ifdef SYS_tee
    {SYS_tee, "SYS_tee"},
#endif
#ifdef SYS_tgkill
    {SYS_tgkill, "SYS_tgkill"},
#endif
#ifdef SYS_time
    {SYS_time, "SYS_time"},
#endif
#ifdef SYS_timer_create
    {SYS_timer_create, "SYS_timer_create"},
#endif
#ifdef SYS_timer_delete
    {SYS_timer_delete, "SYS_timer_delete"},
#endif
#ifdef SYS_timer_getoverrun
    {SYS_timer_getoverrun, "SYS_timer_getoverrun"},
#endif
#ifdef SYS_timer_gettime
    {SYS_timer_gettime, "SYS_timer_gettime"},
#endif
#ifdef SYS_timer_gettime64
    {SYS_timer_gettime64, "SYS_timer_gettime64"},
#endif
#ifdef SYS_timer_settime
    {SYS_timer_settime, "SYS_timer_settime"},
#endif
#ifdef SYS_timer_settime64
    {SYS_timer_settime64, "SYS_timer_settime64"},
#endif
#ifdef SYS_timerfd
    {SYS_timerfd, "SYS_timerfd"},
#endif
#ifdef SYS_timerfd_create
    {SYS_timerfd_create, "SYS_timerfd_create"},
#endif
#ifdef SYS_timerfd_gettime
    {SYS_timerfd_gettime, "SYS_timerfd_gettime"},
#endif
#ifdef SYS_timerfd_gettime64
    {SYS_timerfd_gettime64, "SYS_timerfd_gettime64"},
#endif
#ifdef SYS_timerfd_settime
    {SYS_timerfd_settime, "SYS_timerfd_settime"},
#endif
#ifdef SYS_timerfd_settime64
    {SYS_timerfd_settime64, "SYS_timerfd_settime64"},
#endif
#ifdef SYS_times
    {SYS_times, "SYS_times"},
#endif
#ifdef SYS_tkill
    {SYS_tkill, "SYS_tkill"},
#endif
#ifdef SYS_truncate
    {SYS_truncate, "SYS_truncate"},
#endif
#ifdef SYS_truncate64
    {SYS_truncate64, "SYS_truncate64"},
#endif
#ifdef SYS_tuxcall
    {SYS_tuxcall, "SYS_tuxcall"},
#endif
#ifdef SYS_udftrap
    {SYS_udftrap, "SYS_udftrap"},
#endif
#ifdef SYS_ugetrlimit
    {SYS_ugetrlimit, "SYS_ugetrlimit"},
#endif
#ifdef SYS_ulimit
    {SYS_ulimit, "SYS_ulimit"},
#endif
#ifdef SYS_umask
    {SYS_umask, "SYS_umask"},
#endif
#ifdef SYS_umount
    {SYS_umount, "SYS_umount"},
#endif
#ifdef SYS_umount2
    {SYS_umount2, "SYS_umount2"},
#endif
#ifdef SYS_uname
    {SYS_uname, "SYS_uname"},
#endif
#ifdef SYS_unlink
    {SYS_unlink, "SYS_unlink"},
#endif
#ifdef SYS_unlinkat
    {SYS_unlinkat, "SYS_unlinkat"},
#endif
#ifdef SYS_unshare
    {SYS_unshare, "SYS_unshare"},
#endif
#ifdef SYS_uretprobe
    {SYS_uretprobe, "SYS_uretprobe"},
#endif
#ifdef SYS_uselib
    {SYS_uselib, "SYS_uselib"},
#endif
#ifdef SYS_userfaultfd
    {SYS_userfaultfd, "SYS_userfaultfd"},
#endif
#ifdef SYS_usr26
    {SYS_usr26, "SYS_usr26"},
#endif
#ifdef SYS_usr32
    {SYS_usr32, "SYS_usr32"},
#endif
#ifdef SYS_ustat
    {SYS_ustat, "SYS_ustat"},
#endif
#ifdef SYS_utime
    {SYS_utime, "SYS_utime"},
#endif
#ifdef SYS_utimensat
    {SYS_utimensat, "SYS_utimensat"},
#endif
#ifdef SYS_utimensat_time64
    {SYS_utimensat_time64, "SYS_utimensat_time64"},
#endif
#ifdef SYS_utimes
    {SYS_utimes, "SYS_utimes"},
#endif
#ifdef SYS_utrap_install
    {SYS_utrap_install, "SYS_utrap_install"},
#endif
#ifdef SYS_vfork
    {SYS_vfork, "SYS_vfork"},
#endif
#ifdef SYS_vhangup
    {SYS_vhangup, "SYS_vhangup"},
#endif
#ifdef SYS_vm86
    {SYS_vm86, "SYS_vm86"},
#endif
#ifdef SYS_vm86old
    {SYS_vm86old, "SYS_vm86old"},
#endif
#ifdef SYS_vmsplice
    {SYS_vmsplice, "SYS_vmsplice"},
#endif
#ifdef SYS_vserver
    {SYS_vserver, "SYS_vserver"},
#endif
#ifdef SYS_wait4
    {SYS_wait4, "SYS_wait4"},
#endif
#ifdef SYS_waitid
    {SYS_waitid, "SYS_waitid"},
#endif
#ifdef SYS_waitpid
    {SYS_waitpid, "SYS_waitpid"},
#endif
#ifdef SYS_write
    {SYS_write, "SYS_write"},
#endif
#ifdef SYS_writev
    {SYS_writev, "SYS_writev"},
#endif
};

long sthread_syscall(int number, long a1, long a2, long a3, long a4, long a5, long a6)
{
  switch (number) {
    case SYS_read:
      return sthread_read(a1, (void*)a2, a3);
    case SYS_write:
      return sthread_write(a1, (void*)a2, a3);
    case SYS_open:
      return sthread_open((const char*)a1, a2, a3);
    case SYS_close:
      return sthread_close(a1);
  }
  const char* name = "unknown syscall";
  for (auto [code, n] : table) {
    if (code == number)
      name = n;
  }
  XBT_ERROR("Unhandled syscall: %s (%d)", name, number);
  return syscall(number, a1, a2, a3, a4, a5, a6);
}
