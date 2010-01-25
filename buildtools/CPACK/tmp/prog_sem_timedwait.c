#include <semaphore.h>

int main()
{
	sem_t *s;
	const struct timespec * t;
	sem_timedwait(s, t);
}
