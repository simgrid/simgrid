#ifndef __DEF_H
#define __DEF_H

#if (defined(__BUILTIN) && !defined(__CHKCMD) && !defined(WARN_DEF_MISMATCH))
#ifdef _XBT_WIN32
#pragma message(Macro definition mismatch : __BUILTIN defined but __CHKCMD not defined)
#else
#warning "Macro definition mismatch : __BUILTIN defined but __CHKCMD not defined"
#endif
#define WARN_DEF_MISMATCH	1
#endif

/* must be defined first */
#ifdef _XBT_WIN32

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

#ifdef _XBT_WIN32
	
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
#define	MAX_SUFFIX 						((unsigned int)9)


#ifdef __cplusplus
}
#endif

#endif /* !__DEF_H */

