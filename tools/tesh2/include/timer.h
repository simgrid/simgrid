#ifndef __TIMER_H
#define __TIMER_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

ttimer_t
timer_new(command_t command);

void
timer_free(ttimer_t* timer);

void
timer_time(ttimer_t timer);

void
timer_wait(ttimer_t timer);

#ifdef __cplusplus
}
#endif


#endif /* !__TIMER_H */
