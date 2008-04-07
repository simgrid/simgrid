#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <types.h>

#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif


/* 
 * if 1, keep going when some commands can't be founded	
 */
extern int 
want_keep_going;

/* if 1, ignore failures from commands					
 * the possibles failures are :
 *
 *	- exit code	!= expected exit code
 *	- signal	!= expected signal
 *	- output	!= expected output
 *  - read pipe broken
 *  - write pipe broken
 *  - timeout
 *
 * remark :
 *
 *	a command not found is not a failure, it's an error
 *  to keep going when a command is not found specify the
 *  option --keep-going
 */
extern int
want_keep_going_unit;

/* 
 * the semaphore used to synchronize the jobs 
 */
extern xbt_os_sem_t
jobs_sem;

/* 
 * the semaphore used by the runner to wait the end of all the units 
 */
extern xbt_os_sem_t
units_sem;

/* the dlist of tesh include directories */
extern vector_t 
include_dirs;

extern int
interrupted;

/*extern int
exit_code;
*/

extern int
want_silent;

extern int
want_dry_run;

extern int 
want_just_display;

extern int
dont_want_display_directory;

extern directory_t
root_directory;

extern int
want_detail_summary;

extern int
exit_code;

extern pid_t
pid;



#ifdef __cplusplus
}
#endif

#endif /* !__GLOBAL_H */

