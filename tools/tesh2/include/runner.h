#ifndef __RUNNER_H	
#define __RUNNER_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif
				
int
runner_init(int want_check_syntax, int timeout, fstreams_t fstreams);

void
runner_destroy(void);

void
runner_run(void);

void
runner_display_status(void);

void
runner_interrupt(void);


#ifdef __cplusplus
}
#endif


#endif /* !__RUNNER_H */
