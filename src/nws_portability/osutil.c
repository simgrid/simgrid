/* $Id$ */


#include "config_portability.h"

#include <ctype.h>      /* isspace() */
#include <errno.h>      /* errno values */
#include <pwd.h>        /* endpwent() getpwuid() */
#include <signal.h>     /* sig{add,empty}set() sigprocmask() */
#include <stdio.h>      /* file functions */
#include <stdlib.h>     /* getenv() */
#include <string.h>     /* memset() strchr() strlen() */
#include <unistd.h>     /* alarm() getuid() sysconf() (where available) */
#include <sys/stat.h>   /* stat() */
#include <sys/time.h>   /* gettimeofday() */
#include <sys/types.h>  /* size_t time_t */
#include <time.h>       /* time() */

#include "osutil.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>

/* this is the lock for this module. Every other module will use a void *
 * as a variable as mutex. */
pthread_mutex_t nwsLock = PTHREAD_MUTEX_INITIALIZER;
#endif


/* 
 * These are defined here only becasue they are not found (sometimes) in
 * the headers but only in libraries. To avoid annoying compilation warnings
 */
#ifdef HAVE_SIGHOLD
int sighold(int sig);
#endif
#ifdef HAVE_SIGRELSE
int sigrelse(int sig);
#endif


int
CPUCount( ) {
#ifdef HAVE_SYSCONF
#	ifdef _SC_CRAY_NCPU
#		define SYSCONF_PROCESSOR_COUNT_PARAMETER _SC_CRAY_NCPU
#	elif defined(_SC_NPROC_CONF)
#		define SYSCONF_PROCESSOR_COUNT_PARAMETER _SC_NPROC_CONF
#	elif defined(_SC_NPROCESSORS_CONF)
#		define SYSCONF_PROCESSOR_COUNT_PARAMETER _SC_NPROCESSORS_CONF
#	endif
#	ifdef SYSCONF_PROCESSOR_COUNT_PARAMETER 
  /* Try to gracefully handle mis-configuration. */
  int sysconfCount = sysconf(SYSCONF_PROCESSOR_COUNT_PARAMETER);
  return (sysconfCount == 0) ? 1 : sysconfCount;
#	else
  return 1;
#	endif
#else
  return 1;
#endif
}


/* It should be thread safe (time should be thread safe) */
double
CurrentTime(void) {
	return((double)time(NULL));
}


const char *
GetEnvironmentValue(const char *name,
                    const char *rcDirs,
                    const char *rcName,
                    const char *defaultValue) {

	const char *dirEnd;
	const char *dirStart;
	char *from;
	const char *homeDir;
	size_t nameLen;
	FILE *rcFile;
	static char rcLine[255 + 1];
	char rcPath[255 + 1];
	const char *returnValue;
	char *to;

	returnValue = getenv(name);
	if(returnValue != NULL) {
		/* easy way out: we got the environmental variable */
		return returnValue;
	}

	if(*rcName != '\0') {
		nameLen = strlen(name);

		for(dirStart = rcDirs, dirEnd = dirStart; dirEnd != NULL; dirStart = dirEnd + 1) {
			/* Construct a file path from the next dir in
			 * rcDirs and rcName. */
			dirEnd = strchr(dirStart, ':');
			memset(rcPath, '\0', sizeof(rcPath));
			strncpy(rcPath, dirStart, (dirEnd == NULL) ? strlen(dirStart) : (dirEnd - dirStart));
			if((strcmp(rcPath, "~") == 0) || (strcmp(rcPath, "~/") == 0)) {
				homeDir = getenv("HOME");
				if(homeDir != NULL) {
					strcpy(rcPath, homeDir);
				}
			}
			strcat(rcPath, "/");
			strcat(rcPath, rcName);
			rcFile = fopen(rcPath, "r");

			if(rcFile == NULL) {
				/* no luck, try the next one */
				continue;
			}

			while(fgets(rcLine, sizeof(rcLine), rcFile) != NULL) {
				/* Test against pattern " *#name# =". */
				for(from = rcLine; (*from != '\0') && isspace((int)*from); from++)
					; /* Nothing more to do. */
				if(strncmp(from, name, nameLen) != 0) {
					continue;
				}
				for(from += nameLen; (*from != '\0') && isspace((int)*from); from++)
					; /* Nothing more to do. */
				if(*from != '=') {
					continue;
				}

				/* We found a line that sets the variable. */
				(void)fclose(rcFile);
				for(from++; (*from != '\0') && isspace((int)*from); from++)
					; /* Nothing more to do. */

				/* Return a single word to allow for
				 * future free-format input. */
				if(*from == '"') {
					returnValue = from + 1;
					for(from++, to = from; (*from != '\0') && (*from != '"'); from++, to++) {
						if(*from == '\\') {
							from++;
							switch(*from) {
							case 'b':
								*to = '\b';
								break;
							case 'f':
								*to = '\f';
								break;
							case 'n':
								*to = '\n';
								break;
							case 'r':
								*to = '\r';
								break;
							case 't':
								*to = '\t';
								break;
							case 'v':
								*to = '\v';
								break;
							default:
								*to = *from;
								break;
							}
						} else {
							*to = *from;
						}
					}
				} else {
					returnValue = from;
					for(to = from; (*to != '\0') && !isspace((int)*to); to++)
						; /* Nothing more to do. */
				}
				*to = '\0';

				if (returnValue != NULL) 
					return(returnValue);
				else 
					break;
			}
		(void)fclose(rcFile);
		}
	}
	return(defaultValue);
}


int
GetUserName(char *name,
            size_t length) {
  struct passwd *myPasswd;
  myPasswd = getpwuid(getuid());
  if(myPasswd != NULL) {
    strncpy(name, myPasswd->pw_name, length);
    name[length - 1] = '\0';
  }
  endpwent();
  return(myPasswd != NULL);
}

void
HoldSignal(int sig) {
#ifdef HAVE_SIGHOLD
  sighold(sig);
#else
  sigset_t set, oset;
  sigemptyset(&set);
  sigaddset(&set, sig);\
  sigprocmask(SIG_BLOCK, &set, &oset);
#endif
}


int
MakeDirectory(const char *path,
              mode_t mode,
              int makeEntirePath) {

  const char *endSubPath;
  struct stat pathStat;
  char subPath[255 + 1];

  if(makeEntirePath) {
    endSubPath = path;  /* This will ignore leading '/' */
    while((endSubPath = strchr(endSubPath + 1, '/')) != NULL) {
      memset(subPath, '\0', sizeof(subPath));
      strncpy(subPath, path, endSubPath - path);
      if((stat(subPath, &pathStat) == -1) && (errno == ENOENT)) {
        if(mkdir(subPath, mode) == -1) {
          return 0;
        }
      }
      else if(!S_ISDIR(pathStat.st_mode)) {
        return 0;
      }
    }
  }

  if((stat(path, &pathStat) == -1) && (errno == ENOENT)) {
    return(mkdir(path, mode) != -1);
  }
  else {
    return(S_ISDIR(pathStat.st_mode));
  }
}



int
GetNWSLock(void **lock) {
#ifdef HAVE_PTHREAD_H
	int ret;

	if (lock == NULL) {
		return 0;
	}
	if (*lock == NULL) {
		/* let's play it safe: let's do one mutex at the time (in
		 * case multiple threads are running on the same lock */
	 	if (pthread_mutex_lock(&nwsLock) != 0) {
			return 0;
		}
		/* there is no mutex yet: let's create it. We double
		 * check in the pretty rare condition of having 2 threads
		 * creating lock at the same time */
		if (*lock == NULL) {
			*lock = (void *)MALLOC(sizeof(pthread_mutex_t));
			pthread_mutex_init((pthread_mutex_t *)(*lock), NULL);
		}
	 	if (*lock == NULL || pthread_mutex_unlock(&nwsLock) != 0) {
			return 0;
		}
	}
	ret = pthread_mutex_lock((pthread_mutex_t *)(*lock));
	if  (ret != 0) {
		fprintf(stderr, "GetNWSLock: Unable to lock (errno = %d)!\n", ret);
		return 0;
	}
#endif
	return 1;
}

int 
ReleaseNWSLock(void **lock) {

#ifdef HAVE_PTHREAD_H
	if (lock == NULL || pthread_mutex_unlock((pthread_mutex_t *)*lock) != 0) {
		return 0;
	}
#endif
	return 1;
}

long
MicroTime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return(tv.tv_sec * 1000000 + tv.tv_usec);
}


void
ReleaseSignal(int sig) {
#ifdef HAVE_SIGRELSE
  sigrelse(sig);
#else
  sigset_t set, oset;
  sigemptyset(&set);
  sigaddset(&set, sig);
  sigprocmask(SIG_UNBLOCK, &set, &oset);
#endif
}


int
SignalAlarm(	handler h, 
		handler *old) {
#ifdef USE_ALARM_SIGNAL
	handler tmp;
	
	tmp = signal(SIGALRM, h);
	if (tmp == SIG_ERR) {
		return 0;
	}
	if (old != NULL)
		*old = tmp;

#endif
	return 1;
}

/* It should be thread safe (alarm is thread safe) */
void
SetRealTimer(unsigned int numberOfSecs) {
#ifdef USE_ALARM_SIGNAL
#ifdef HAVE_SIGINTERRUPT
	if (numberOfSecs > 0) {
		/* just interrupt a system call upon receipt of interrupt */
		siginterrupt(SIGALRM, 1);
	}
#endif

	alarm(numberOfSecs);
#endif
}

