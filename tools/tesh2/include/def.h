#ifndef __DEF_H
#define __DEF_H

/* must be defined first */
#ifdef WIN32

	#define _WIN32_WINNT	0x0400
	
	#if (_MSC_VER >= 1400 && !defined(_CRT_SECURE_NO_DEPRECATE))
		#define _CRT_SECURE_NO_DEPRECATE
	#endif
	
	#include <direct.h> /* for getcwd(), _chdir() */
	
#endif

#include <xerrno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
	
	#define strdup			_strdup
	#define chdir			_chdir
	#define getcwd			_getcwd
	
	#ifndef S_ISDIR
		#define	S_ISDIR(__mode)	(((__mode) & S_IFMT) == S_IFDIR)
	#endif

	#ifndef S_ISREG
		#define S_ISREG(__mode) (((__mode) & S_IFMT) == S_IFREG)
	#endif
	
	#define INDEFINITE_PID	NULL
	#define INDEFINITE_FD	NULL
#else
	#define INDEFINITE_PID	((int)-1)
	#define INDEFINITE_FD	((int)-1)	
#endif

#ifndef PATH_MAX
	#define PATH_MAX 		((unsigned int)260)
#endif

#ifndef VAR_NAME_MAX
	#define VAR_NAME_MAX	((unsigned int) 80)
#endif

#define INDEFINITE						((int)-1)
#define INDEFINITE_SIGNAL				NULL

#define DEFAULT_FSTREAMS_CAPACITY		((int)128)
#define DEFAULT_INCLUDE_DIRS_CAPACITY	DEFAULT_FSTREAMS_CAPACITY
#define DEFAULT_UNITS_CAPACITY			((int)64)
#define DEFAULT_INCLUDES				((int)8)
#define DEFAULT_COMMANDS_CAPACITY		((int)512)
#define DEFAULT_SUITES_CAPACITY			((int)32)
#define DEFAULT_ERRORS_CAPACITY			((int)32)
#define	MAX_SUFFIX 						((unsigned int)9)


#ifdef __cplusplus
}
#endif

#endif /* !__DEF_H */

