/* Copyright (c) 2006, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMDAG_DATATYPES_H
#define SIMDAG_DATATYPES_H

/** @brief Workstation datatype
    @ingroup SD_datatypes_management
    
    A workstation is a place where a task can be executed.
    A workstation is represented as a <em>physical
    resource with computing capabilities</em> and has a <em>name</em>.

    @see SD_workstation_management */
typedef struct SD_workstation *SD_workstation_t;

/** @brief Workstation access mode
    @ingroup SD_datatypes_management

    By default, a workstation resource is shared, i.e. several tasks
    can be executed at the same time on a workstation. The CPU power of
    the workstation is shared between the running tasks on the workstation.
    In sequential mode, only one task can use the workstation, and the other
    tasks wait in a FIFO.

    @see SD_workstation_get_access_mode(), SD_workstation_set_access_mode() */
typedef enum {
  SD_WORKSTATION_SHARED_ACCESS,     /**< @brief Several tasks can be executed at the same time */
  SD_WORKSTATION_SEQUENTIAL_ACCESS  /**< @brief Only one task can be executed, the others wait in a FIFO. */
} e_SD_workstation_access_mode_t;

typedef enum {
  SD_LINK_SHARED,
  SD_LINK_FATPIPE
} e_SD_link_sharing_policy_t;

/** @brief Link datatype
    @ingroup SD_datatypes_management

    A link is a network node represented as a <em>name</em>, a <em>current
    bandwidth</em> and a <em>current latency</em>. A route is a list of
    links between two workstations.

    @see SD_link_management */
typedef struct SD_link *SD_link_t;

/** @brief Task datatype
    @ingroup SD_datatypes_management

    A task is some <em>computing amount</em> that can be executed
    in parallel on several workstations. A task may depend on other
    tasks, this means that the task cannot start until the other tasks are done.
    Each task has a <em>\ref e_SD_task_state_t "state"</em> indicating whether 
    the task is scheduled, running, done, etc.

    @see SD_task_management */
typedef struct SD_task *SD_task_t;

/** @brief Task states
    @ingroup SD_datatypes_management

    @see SD_task_management */
typedef enum {
  SD_NOT_SCHEDULED = 0,      /**< @brief Initial state (not valid for SD_watch and SD_unwatch). */
  SD_SCHEDULABLE = 0x0001,               /**< @brief A task becomes SD_READY as soon as its dependencies are satisfied */
  SD_SCHEDULED = 0x0002,     /**< @brief A task becomes SD_SCHEDULED when you call function
								  SD_task_schedule. SD_simulate will execute it when it becomes SD_RUNNABLE. */
  SD_RUNNABLE = 0x0004,      /**< @brief A scheduled task becomes runnable is SD_simulate as soon as its dependencies are satisfied. */
  SD_IN_FIFO = 0x0008,       /**< @brief A runnable task can have to wait in a workstation fifo if the workstation is sequential */
  SD_RUNNING = 0x0010,       /**< @brief An SD_RUNNABLE or SD_IN_FIFO becomes SD_RUNNING when it is launched. */
  SD_DONE = 0x0020,          /**< @brief The task is successfully finished. */
  SD_FAILED = 0x0040         /**< @brief A problem occurred during the execution of the task. */
} e_SD_task_state_t;

/** @brief Task kinds
    @ingroup SD_datatypes_management

    @see SD_task_management */
typedef enum {
  SD_TASK_NOT_TYPED = 0,      /**< @no specified type */
  SD_TASK_COMM_E2E = 1,       /**< @brief end to end communication */
  SD_TASK_COMP_SEQ = 2,       /**< @brief sequential computation */
} e_SD_task_kind_t;


#endif
