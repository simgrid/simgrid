#include <xerrno.h>

typedef struct s_entry
{
	const char* name;
	int code;
	unsigned kind : 1;		/* if 1 it's an error of the application else assume it's a system error */
	const char* string;
	
}entry_t;


static const
entry_t err[] =
{
	
	#ifdef E2BIG
	{"E2BIG", E2BIG, 0, "argument list too long"},
	#endif
	
	#ifdef EACCES
	{"EACCES", EACCES, 0, "permission denied"},
	#endif
	
	#ifdef EADDRINUSE
	{"EADDRINUSE", EADDRINUSE, 0, "address in use"},
	#endif
	
	#ifdef EADDRNOTAVAIL
	{"EADDRNOTAVAIL", EADDRNOTAVAIL, 0, "address not available"},
	#endif
	
	#ifdef EAFNOSUPPORT
	{"EAFNOSUPPORT", EAFNOSUPPORT, 0, "address family not supported"},
	#endif
	
	#ifdef EAGAIN
	{"EAGAIN", EAGAIN, 0, "resource unavailable, try again"},
	#endif
	
	#ifdef EALREADY
	{"EALREADY", EALREADY, 0, "connection already in progress"},
	#endif
	
	#ifdef EBADF
	{"EBADF", EBADF, 0, "bad file descriptor"},
	#endif
	
	#ifdef EBADMSG
	{"EBADMSG", EBADMSG, 0, "bad message"},
	#endif
	
	#ifdef EBUSY
	{"EBUSY", EBUSY, 0, "device or resource busy"},
	#endif
	
	#ifdef ECANCELED
	{"ECANCELED", ECANCELED, 0, "operation canceled"},
	#endif
	
	#ifdef ECHILD
	{"ECHILD", ECHILD, 0, "no child processes"},
	#endif
	
	#ifdef ECONNABORTED
	{"ECONNABORTED", ECONNABORTED, 0, "connection aborted"},
	#endif
	
	#ifdef ECONNREFUSED
	{"ECONNREFUSED", ECONNREFUSED, 0, "connection refused"},
	#endif
	
	#ifdef ECONNRESET
	{"ECONNRESET", ECONNRESET, 0, "connection reset"},
	#endif
	
	#ifdef EDEADLK
	{"EDEADLK", EDEADLK, 0, "resource deadlock would occur"},
	#endif
	
	#ifdef EDESTADDRREQ
	{"EDESTADDRREQ", EDESTADDRREQ, 0, "destination address required"},
	#endif
	
	#ifdef EDOM
	{"EDOM", EDOM, 0, "mathematics argument out of domain of function"},
	#endif 
	
	#ifdef EEXIST
	{"EEXIST", EEXIST, 0, "file exists"},
	#endif
	
	#ifdef EFAULT
	{"EFAULT", EFAULT, 0, "bad address"},
	#endif
	
	#ifdef EFBIG
	{"EFBIG", EFBIG, 0, "file too large"},
	#endif
	
	#ifdef EHOSTUNREACH
	{"EHOSTUNREACH", EHOSTUNREACH, 0, "host is unreachable"},
	#endif
	
	#ifdef EIDRM
	{"EIDRM", EIDRM, 0, "identifier removed"},
	#endif
	
	#ifdef EILSEQ
	{"EILSEQ", EILSEQ, 0, "illegal byte sequence"},
	#endif
	
	#ifdef EINPROGRESS
	{"EINPROGRESS", EINPROGRESS, 0, "operation in progress"},
	#endif
	
	#ifdef EINTR
	{"EINTR", EINTR, 0, "interrupted function"},
	#endif
	
	#ifdef EINVAL
	{"EINVAL", EINVAL, 0, "invalid argument"},
	#endif
	
	#ifdef EIO
	{"EIO", EIO, 0, "I/O error"},
	#endif
	
	#ifdef EISCONN
	{"EISCONN", EISCONN, 0, "socket is connected"},
	#endif
	
	#ifdef EISDIR
	{"EISDIR", EISDIR, 0, "is a directory"},
	#endif
	
	#ifdef ELOOP
	{"ELOOP", ELOOP, 0, "too many levels of symbolic links"},
	#endif
	
	#ifdef EMFILE
	{"EMFILE", EMFILE, 0, "too many open files"},
	#endif
	
	#ifdef EMLINK
	{"EMLINK", EMLINK, 0, "too many links"},
	#endif
	
	#ifdef EMSGSIZE
	{"EMSGSIZE", EMSGSIZE, 0, "message too large"},
	#endif
	
	#ifdef ENAMETOOLONG
	{"ENAMETOOLONG", ENAMETOOLONG, 0, "filename too long"},
	#endif
	
	#ifdef ENETDOWN
	{"ENETDOWN", ENETDOWN, 0, "network is down"},
	#endif
	
	#ifdef ENETRESET
	{"ENETRESET", ENETRESET, 0, "connection aborted by network"},
	#endif
	
	#ifdef ENETUNREACH
	{"ENETUNREACH", ENETUNREACH, 0, "network unreachable"},
	#endif
	
	#ifdef ENFILE
	{"ENFILE", ENFILE, 0, "too many files open in system"},
	#endif
	
	#ifdef ENOBUFS
	{"ENOBUFS", ENOBUFS, 0, "no buffer space available"},
	#endif
	
	#ifdef ENODATA
	{"ENODATA", ENODATA, 0, "no message is available on the STREAM head read queue"},
	#endif
	
	#ifdef ENODEV
	{"ENODEV", ENODEV, 0, "no such device"},
	#endif
	
	#ifdef ENOENT
	{"ENOENT", ENOENT, 0, "no such file or directory"},
	#endif
	
	#ifdef ENOEXEC
	{"ENOEXEC", ENOEXEC, 0, "executable file format error"},
	#endif
	
	#ifdef ENOLCK
	{"ENOLCK", ENOLCK, 0, "no locks available"},
	#endif
	
	#ifdef ENOMEM
	{"ENOMEM", ENOMEM, 0, "not enough space"},
	#endif
	
	#ifdef ENOMSG
	{"ENOMSG", ENOMSG, 0, "no message of the desired type"},
	#endif
	
	#ifdef ENOPROTOOPT
	{"ENOPROTOOPT", ENOPROTOOPT, 0, "protocol not available"},
	#endif
	
	#ifdef ENOSPC
	{"ENOSPC", ENOSPC, 0, "no space left on device"},
	#endif
	
	#ifdef ENOSR
	{"ENOSR", ENOSR, 0, "no stream resources"},
	#endif
	
	#ifdef ENOSTR
	{"ENOSTR", ENOSTR, 0, "not a stream"},
	#endif
	
	#ifdef ENOSYS
	{"ENOSYS", ENOSYS, 0, "function not supported"},
	#endif
	
	#ifdef ENOTCONN
	{"ENOTCONN", ENOTCONN, 0, "the socket is not connected"},
	#endif
	
	#ifdef ENOTDIR
	{"ENOTDIR", ENOTDIR, 0, "not a directory"},
	#endif
	
	#ifdef ENOTEMPTY
	{"ENOTEMPTY", ENOTEMPTY, 0, "directory not empty"},
	#endif
	
	#ifdef ENOTSOCK
	{"ENOTSOCK", ENOTSOCK, 0, "not a socket"},
	#endif
	
	#ifdef ENOTSUP
	{"ENOTSUP", ENOTSUP, 0, "not supported"},
	#endif
	
	#ifdef ENOTTY
	{"ENOTTY", ENOTTY, 0, "inappropriate I/O control operation"},
	#endif
	
	#ifdef ENXIO
	{"ENXIO", ENXIO, 0, "no such device or address"},
	#endif
	
	#ifdef EOPNOTSUPP
	{"EOPNOTSUPP", EOPNOTSUPP, 0, "operation not supported on socket"},
	#endif
	
	#ifdef EOVERFLOW
	{"EOVERFLOW", EOVERFLOW, 0, "value too large to be stored in data type"},
	#endif
	
	#ifdef EPERM
	{"EPERM", EPERM, 0, "operation not permitted"},
	#endif
	
	#ifdef EPIPE
	{"EPIPE", EPIPE, 0, "broken pipe"},
	#endif
	
	#ifdef EPROTO
	{"EPROTO", EPROTO, 0, "protocol error"},
	#endif
	
	#ifdef EPROTONOSUPPORT
	{"EPROTONOSUPPORT", EPROTONOSUPPORT, 0, "protocol not supported"},
	#endif
	
	#ifdef EPROTOTYPE
	{"EPROTOTYPE", EPROTOTYPE, 0, "protocol wrong type for socket"},
	#endif
	
	#ifdef ERANGE
	{"ERANGE", ERANGE, 0, "result too large"},
	#endif
	
	#ifdef EROFS
	{"EROFS", EROFS, 0, "read-only file system"},
	#endif
	
	#ifdef ESPIPE
	{"ESPIPE", ESPIPE, 0, "invalid seek"},
	#endif
	
	#ifdef ESRCH
	{"ESRCH", ESRCH, 0, "no such process"},
	#endif
	
	#ifdef ETIME
	{"ETIME", ETIME, 0, "stream ioctl() timeout"},
	#endif
	
	#ifdef ETIMEDOUT
	{"ETIMEDOUT", ETIMEDOUT, 0, "connection timed out"},
	#endif
	
	#ifdef ETXTBSY
	{"ETXTBSY", ETXTBSY, 0, "text file busy"},
	#endif
	
	#ifdef EWOULDBLOCK
	{"EWOULDBLOCK", EWOULDBLOCK, 0, "operation would block"},
	#endif
	
	#ifdef EXDEV
	{"EXDEV", EXDEV, 0, "cross-device link"},
	#endif
	
	{"ECMDTIMEDOUT", ECMDTIMEDOUT, 1, "command timed out"},

	{"ECMDNOTFOUND", ECMDNOTFOUND,1,  "command not found"},

	{"EEXITCODENOTMATCH", EEXITCODENOTMATCH,1,  "exit code mismatch"},

	{"EOUTPUTNOTMATCH", EOUTPUTNOTMATCH,1,  "output mismatch"},

	{"ESIGNOTMATCH", ESIGNOTMATCH,1,  "signal mismatch"},

	{"EUNXPSIG", EUNXPSIG,1,  "unexpected signal caught"},

	{"ESIGNOTRECEIPT", ESIGNOTRECEIVED,1,  "expected signal not receipt"},

	{"EPROCCMDLINE", EPROCCMDLINE, 1, "command line processing failed"},

	{"ENOARG", ENOARG, 1, "none optional argument not specified"},

	{"ENOTPOSITIVENUM", ENOTPOSITIVENUM, 1, "argument option not strictly positive"},

	{"ESYNTAX", ESYNTAX,1,  "syntax error"},

	{"ELEADTIME", ELEADTIME, 1, "timed out"},

	{"EREADMENOTFOUND", EREADMENOTFOUND,1,  "unable to locate the README.txt file"},

	{"EINCLUDENOTFOUND", EINCLUDENOTFOUND, 1,  "include file not found"},

	{"ESUFFIXTOOLONG", ESUFFIXTOOLONG,1,  "suffix too long"},

	{"EINVCMDLINE", EINVCMDLINE,1,  "invalid command line"},

	{"unkwown", -1, 0, "unknown"}
};

#include <stdio.h>

#ifdef _XBT_WIN32
static char *
w32error_to_string(DWORD errcode) 
{
	static char buffer[128];

	/*
	 *  Special code for winsock error handling.
	 */
	if (errcode > WSABASEERR) 
	{
		HMODULE hModule = GetModuleHandle("wsock32");
		
		if(hModule) 
		{
			FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,hModule, errcode, LANG_NEUTRAL, buffer, 128, NULL);
			FreeLibrary(hModule);
		}
	} 
	else 
	{
		/*
		 *  Default system message handling
		 */
    	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errcode, LANG_NEUTRAL, buffer, 128, NULL);
	}

    return buffer;
}
#endif

const char*
error_to_string(int errcode, int kind)
{
	int i;
	
	for(i = 0; err[i].code != -1; i++)
		if(err[i].code == errcode && err[i].kind == kind)
			return err[i].string;

	#ifdef _XBT_WIN32

	/* assume it's a W32 error */
	return w32error_to_string((DWORD)errcode);
	#else
	return "unknow error";	
	#endif
	
}

const char*
error_get_at(int pos, int* code)
{
	if(pos < 0 || (pos > (sizeof(err)/sizeof(entry_t)) - 2))
	{
		errno = ERANGE;
		return NULL;
	}
	
	*code = err[pos].code;
	return err[pos].name;
}

