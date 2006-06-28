#ifndef SIMDAG_DATATYPES_H
#define SIMDAG_DATATYPES_H

/** @brief Workstation datatype
    @ingroup SD_datatypes_management
    
    A workstation is a place where a task can be executed.
    A workstation is represented as a <em>physical
    resource with computing capabilities</em> and has a <em>name</em>.

    @see SD_workstation_management */
typedef struct SD_workstation *SD_workstation_t;

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
  SD_SCHEDULED =     0x0001, /**< @brief A task becomes SD_SCHEDULED when you call function
				SD_task_schedule. SD_simulate will execute it when it becomes SD_READY. */
  SD_READY =         0x0002, /**< @brief A scheduled task becomes ready as soon as its dependencies are satisfied. */
  SD_RUNNING =       0x0004, /**< @brief When a task is ready, it is launched in the function SD_simulate and becomes SD_RUNNING. */
  SD_DONE =          0x0008, /**< @brief The task is successfuly finished. */
  SD_FAILED =        0x0010  /**< @brief A problem occured during the execution of the task. */
} e_SD_task_state_t;

#endif
