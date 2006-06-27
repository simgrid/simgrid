#ifndef SIMDAG_DATATYPES_H
#define SIMDAG_DATATYPES_H

/** @brief Workstation datatype
    @ingroup SD_datatypes_management
    @{ */
typedef struct SD_workstation *SD_workstation_t;
/** @} */

/** @brief Link datatype
    @ingroup SD_datatypes_management
    @{ */
typedef struct SD_link *SD_link_t;
/** @} */

/** @brief Task datatype
    @ingroup SD_datatypes_management
    @{ */
typedef struct SD_task *SD_task_t;
/** @} */

/** @brief Task states
    @ingroup SD_datatypes_management
    @{ */
typedef enum {
  SD_NOT_SCHEDULED = 0,      /**< @brief Initial state (not valid for SD_watch and SD_unwatch). */
  SD_SCHEDULED =     0x0001, /**< @brief A task becomes SD_SCHEDULED when you call function
				SD_task_schedule. SD_simulate will execute it when it becomes SD_READY. */
  SD_READY =         0x0002, /**< @brief A scheduled task becomes ready as soon as its dependencies are satisfied. */
  SD_RUNNING =       0x0004, /**< @brief A ready task becomes running in SD_simulate. */
  SD_DONE =          0x0008, /**< @brief The task is successfuly finished. */
  SD_FAILED =        0x0010  /**< @brief A problem occured. */
} e_SD_task_state_t;
/** @} */

#endif
