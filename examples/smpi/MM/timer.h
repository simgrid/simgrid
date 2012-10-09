#ifndef timer_H_
#define timer_H_
/*!
 * \defgroup timer.h
 * here, we provides a generic interface to time some parts of the code
 */
# include <sys/time.h>

inline double get_second(struct timespec *res);
inline double get_microsecond(struct timespec *res);
inline double get_nanosecond(struct timespec *res);



int get_time(struct timespec *tp);
double get_timediff(struct timespec *start, struct timespec * end);
#endif /*timer_H_*/
