/*
 * include/types.h - types representing the tesh concepts.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file declares the types used to run a set of tesh files.
 *
 */


#ifndef __TYPES_H
#define __TYPES_H

#include <def.h>
#include <portable.h>
#include <xbt/xbt_os_thread.h>
#include <xbt/strbuff.h>
#include <xbt.h>

#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * byte type definition 
 */
#ifndef __BYTE_T_DEFINED
  typedef unsigned char byte;
#define __BYTE_T_DEFINED
#endif

/*
 * file descriptor and pid types for portability.
 */

#ifdef _XBT_WIN32

#ifndef __FD_T_DEFINED
  typedef HANDLE fd_t;
#define __FD_T_DEFINED
#endif

#ifndef __PID_T_DEFINED
  typedef HANDLE pid_t;
#define __PID_T_DEFINED
#endif

#else

#ifndef __FD_T_DEFINED
  typedef int fd_t;
#define __FD_T_DEFINED
#endif

#ifndef __PID_T_DEFINED
  typedef int pid_t;
#define __PID_T_DEFINED
#endif
#endif

/* forward declarations */

  struct s_unit;                /* this structure represents a tesh unit        */

  struct s_runner;
  struct s_units;
  struct s_unit;
  struct s_excludes;
  struct s_fstreams;
  struct s_fstream;
  struct s_directories;
  struct s_directory;
  struct s_writer;
  struct s_reader;
  struct s_timer;
  struct s_context;
  struct s_command;
  struct s_variable;
  struct s_error;


/* 
 * declaration of the tesh unit type which represents a tesh file
 * to run. 	
 */
  typedef struct s_unit {
    char *description;          /* an optional description of the unit                                                                  */
    struct s_fstream *fstream;  /* the file stream used to parse the tesh file                                                  */
    struct s_runner *runner;    /* the runner of the unit                                                                                           */
    xbt_dynar_t commands;       /* a vector containing all the commands parsed from the tesh file               */
    int cmd_nb;                 /* number of created commands of the unit                                                               */
    int started_cmd_nb;         /* number of started commands of the unit                                                               */
    int interrupted_cmd_nb;     /* number of interrupted commands of the unit                                                   */
    int failed_cmd_nb;          /* number of failed commands of the unit                                                                */
    int successeded_cmd_nb;     /* number of successeded commands of the unit                                                   */
    int terminated_cmd_nb;      /* number of ended commands                                                                                             */
    int waiting_cmd_nb;         /* REMOVE THIS FIELD LATER                                                                                              */
    xbt_os_thread_t thread;     /* all the units run in its own thread                                                                  */
    xbt_os_sem_t sem;           /* used by the last command of the unit to signal the end of the unit   */
    xbt_os_mutex_t mutex;       /* used to synchronously access to the properties of the runner                 */
    unsigned interrupted:1;     /* if 1, the unit is interrupted by the runner                                                  */
    unsigned failed:1;          /* if 1, the unit is failed                                                                                             */
    unsigned successeded:1;     /* if 1, the unit is successeded                                                                                */
    unsigned parsed:1;          /* if 1, the tesh file of the unit is parsed                                                    */
    unsigned released:1;        /* if 1, the unit is released                                                                                   */
    unsigned is_running_suite:1;        /* if 1, the unit is running a suite                                                                    */
    struct s_unit *owner;       /* the unit owned the unit is included                                                                  */
    struct s_unit *root;        /* the root unit                                                                                                                */
    xbt_dynar_t suites;         /* the suites contained by the unit                                                                             */
    int exit_code;              /* the exit code of the unit                                                                                    */
    int err_kind;
    char *err_line;
    xbt_dynar_t includes;
    char *filepos;
  } s_unit_t, *unit_t;

/* 
 * declaration of the tesh file stream type used to manage the tesh file.	
 */
  typedef struct s_fstream {
    char *name;                 /* the name of the file stream (same as the name of the tesh file)              */
    char *directory;            /* the directory containing the tesh file                                                               */
    FILE *stream;               /* the system file stream                                                                                               */
    struct s_unit *unit;        /* a reference to the unit using the file stream object                                 */
    unsigned parsed:1;
  } s_fstream_t, *fstream_t;

/* 
 * command status 
 */
  typedef enum e_command_status {
    cs_initialized = 0,         /* the is initialized                                                                                           */
    cs_started = 1,             /* the command is started                                                                                       */
    cs_in_progress = 2,         /* the command is execited                                                                                      */
    cs_waiting = 3,             /* the command is waiting the writer, the reader and the timer          */
    cs_interrupted = 4,         /* the command is interrupted                                                                           */
    cs_failed = 5,              /* the command is failed                                                                                        */
    cs_successeded = 6,         /* the command is successeded                                                                           */
    cs_killed = 7,              /* the command is killed                                                                                        */
    csr_fcntl_function_failed = 8       /* the fcntl function failed                                                                            */
  } command_status_t;

/* 
 * reason of the status of the command 
 */
  typedef enum e_command_status_raison {
    csr_unknown = 0,            /* unknown reason                                                                                                       */
    csr_read_failure = 1,       /* a read operation failed                                                                                      */
    csr_read_pipe_broken = 2,   /* the pipe used to read from the stdout of the command is broken       */
    csr_timeout = 3,            /* timeout                                                                                                                      */
    csr_write_failure = 4,      /* a write operation failed                                                                                     */
    csr_write_pipe_broken = 5,  /* the pipe used to write to the stdin of the command is broken         */
    csr_fork_function_failure = 6,      /* can't execute the command                                                                            */
    csr_wait_failure = 8,       /* the wait process function failed                                                                     */
    csr_interruption_request = 9,       /* the command has received an interruption request                                     */
    csr_command_not_found = 10, /* the command is not found                                                                                     */
    csr_exit_codes_dont_match = 11,
    csr_outputs_dont_match = 12,
    csr_signals_dont_match = 13,
    csr_unexpected_signal_caught = 14,
    csr_expected_signal_not_received = 15,
    csr_pipe_function_failed = 16,      /* the function pipe() or CreatePipe() fails                                            */
    csr_dup2_function_failure = 17,
    csr_execlp_function_failure = 18,
    csr_create_process_function_failure = 19,
    csr_waitpid_function_failure = 20,
    csr_get_exit_code_process_function_failure = 21,
    csr_shell_failed = 22
  } cs_reason_t;




  typedef struct s_variable {
    char *name;
    char *val;
    int used;
    int env;
    int err;
  } s_variable_t, *variable_t;

/* 
 * declaration of the tesh timer type 
 */
  typedef struct s_timer {
    xbt_os_thread_t thread;     /* asynchronous timer                                                                                                   */
    struct s_command *command;  /* the timed command                                                                                                    */
    int timeouted;              /* if 1, the timer is timeouted                                                                                 */
    xbt_os_sem_t started;
  } s_timer_t, *ttimer_t;

/* 
 * declaration of the tesh reader type 
 */
  typedef struct s_reader {
    xbt_os_thread_t thread;     /* asynchonous reader                                                                                                   */
    struct s_command *command;  /* the command of the reader                                                                                    */
    int failed;                 /* if 1, the reader failed                                                                                              */
    int broken_pipe;            /* if 1, the pipe used by the reader is broken                                                  */
    int done;
    xbt_os_sem_t started;
  } s_reader_t, *reader_t;

/* 
 * declaration of the tesh writer type 
 */
  typedef struct s_writer {
    xbt_os_thread_t thread;     /* asynchronous writer                                                                                                  */
    struct s_command *command;  /* the command of the writer                                                                                    */
    int failed;                 /* if 1, the writer failed                                                                                              */
    int broken_pipe;            /* if 1, the pipe used by the writer is broken                                                  */
    xbt_os_sem_t written;
    xbt_os_sem_t can_write;
    int done;
  } s_writer_t, *writer_t;


  typedef struct s_units {
    xbt_dynar_t items;
    int number_of_runned_units; /* the number of units runned                                                                                   */
    int number_of_ended_units;  /* the number of units over                                                                                             */

  } s_units_t, *units_t;

/* 
 * declaration of the tesh runner type 
 */
  typedef struct s_runner {
    struct s_units *units;
    int timeouted;              /* if 1, the runner is timeouted                                                                                */
    int timeout;                /* the timeout of the runner                                                                                    */
    int interrupted;            /* if 1, the runner failed                                                                                              */
    int number_of_runned_units; /* the number of units runned by the runner                                                             */
    int number_of_ended_units;  /* the number of ended units                                                                                    */
    int waiting;                /* if 1, the runner is waiting the end of all the units                                 */
    xbt_os_thread_t thread;     /* the timer thread                                                                                                             */
    xbt_dynar_t variables;

    int total_of_tests;
    int total_of_successeded_tests;
    int total_of_failed_tests;
    int total_of_interrupted_tests;

    int total_of_units;
    int total_of_successeded_units;
    int total_of_failed_units;
    int total_of_interrupted_units;

    int total_of_suites;
    int total_of_successeded_suites;
    int total_of_failed_suites;
    int total_of_interrupted_suites;
    char **path;
    char **builtin;
  } s_runner_t, *runner_t;


  typedef struct s_fstreams {
    xbt_dynar_t items;
  } s_fstreams_t, *fstreams_t;


  typedef struct s_excludes {
    xbt_dynar_t items;
  } s_excludes_t, *excludes_t;

  typedef struct s_directory {
    char *name;
    DIR *stream;
  } s_directory_t, *directory_t;

  typedef struct s_directories {
    xbt_dynar_t items;
  } s_directories_t, *directories_t;


  typedef enum {
    oh_check,
    oh_display,
    oh_ignore
  } output_handling_t;
/* 
 * declaration of the tesh context type 
 */
  typedef struct s_context {
    char *command_line;         /* the command line of the command to execute                                                   */
    const char *line;           /* the current parsed line                                                                                              */
    char *pos;
    int exit_code;              /* the expected exit code of the command                                                                */
    char *signal;               /* the expected signal raised by the command                                                    */
    int timeout;                /* the timeout of the test                                                                                              */
    xbt_strbuff_t input;        /* the input to write in the stdin of the command to run                                */
    xbt_strbuff_t output;       /* the expected output of the command of the test                                               */
    output_handling_t output_handling;
    int async;                  /* if 1, the command is asynchronous                                                                    */

#ifdef _XBT_WIN32
    char *t_command_line;       /* translate the command line on Windows                                                                */
    unsigned is_not_found:1;
#endif

  } s_context_t, *context_t;

/* 
 * declaration of the tesh command type 
 */
  typedef struct s_command {
    unit_t root;
    unit_t unit;                /* the unit of the command                                                                                              */
    struct s_context *context;  /* the context of the execution of the command                                                  */
    xbt_os_thread_t thread;     /* asynchronous command                                                                                                 */
    struct s_writer *writer;    /* the writer used to write in the command stdin                                                */
    struct s_reader *reader;    /* the reader used to read from the command stout                                               */
    struct s_timer *timer;      /* the timer used for the command                                                                               */
    command_status_t status;    /* the current status of the command                                                                    */
    cs_reason_t reason;         /* the reason of the state of the command                                                               */
    int successeded;            /* if 1, the command is successeded                                                                             */
    int interrupted;            /* if 1, the command is interrupted                                                                             */
    int failed;                 /* if 1, the command is failed                                                                                  */
    pid_t pid;                  /* the program id of the command                                                                                */
    xbt_strbuff_t output;       /* the output of the command                                                                                    */
    fd_t stdout_fd;             /* the stdout fd of the command                                                                                 */
    fd_t stdin_fd;              /* the stdin fd of the command                                                                                  */
    int exit_code;              /* the exit code of the command                                                                                 */
#ifdef _XBT_WIN32
    unsigned long stat_val;
#else
    int stat_val;
#endif
    char *signal;               /* the signal raised by the command if any                                                              */
    xbt_os_mutex_t mutex;

#ifndef _XBT_WIN32
    int killed;                 /* if 1, the command was killed                                                                                 */
    int execlp_errno;
#endif

  } s_command_t, *command_t;

#ifdef __cplusplus
}
#endif
#endif                          /* !__TYPES_H */
