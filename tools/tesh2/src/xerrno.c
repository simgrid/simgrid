#include <xerrno.h>

typedef struct s_entry
{
	const char* name;
	int code;
	const char* string;
}entry_t;


static const
entry_t err[] =
{
	
	#ifdef E2BIG
	{"E2BIG", E2BIG, "Argument list too long."},
	#endif
	
	#ifdef EACCES
	{"EACCES", EACCES, "Permission denied."},
	#endif
	
	#ifdef EADDRINUSE
	{"EADDRINUSE", EADDRINUSE, "Address in use."},
	#endif
	
	#ifdef EADDRNOTAVAIL
	{"EADDRNOTAVAIL", EADDRNOTAVAIL, "Address not available."},
	#endif
	
	#ifdef EAFNOSUPPORT
	{"EAFNOSUPPORT", EAFNOSUPPORT, "Address family not supported."},
	#endif
	
	#ifdef EAGAIN
	{"EAGAIN", EAGAIN, "Resource unavailable, try again."},
	#endif
	
	#ifdef EALREADY
	{"EALREADY", EALREADY, "Connection already in progress."},
	#endif
	
	#ifdef EBADF
	{"EBADF", EBADF, "Bad file descriptor."},
	#endif
	
	#ifdef EBADMSG
	{"EBADMSG", EBADMSG, "Bad message."},
	#endif
	
	#ifdef EBUSY
	{"EBUSY", EBUSY, "Device or resource busy."},
	#endif
	
	#ifdef ECANCELED
	{"ECANCELED", ECANCELED, "Operation canceled."},
	#endif
	
	#ifdef ECHILD
	{"ECHILD", ECHILD, "No child processes."},
	#endif
	
	#ifdef ECONNABORTED
	{"ECONNABORTED", ECONNABORTED, "Connection aborted."},
	#endif
	
	#ifdef ECONNREFUSED
	{"ECONNREFUSED", ECONNREFUSED, "Connection refused."},
	#endif
	
	#ifdef ECONNRESET
	{"ECONNRESET", ECONNRESET, "Connection reset."},
	#endif
	
	#ifdef EDEADLK
	{"EDEADLK", EDEADLK, "Resource deadlock would occur."},
	#endif
	
	#ifdef EDESTADDRREQ
	{"EDESTADDRREQ", EDESTADDRREQ, "Destination address required."},
	#endif
	
	#ifdef EDOM
	{"EDOM", EDOM, "Mathematics argument out of domain of function."},
	#endif 
	
	#ifdef EEXIST
	{"EEXIST", EEXIST, "File exists."},
	#endif
	
	#ifdef EFAULT
	{"EFAULT", EFAULT, "Bad address."},
	#endif
	
	#ifdef EFBIG
	{"EFBIG", EFBIG, "File too large."},
	#endif
	
	#ifdef EHOSTUNREACH
	{"EHOSTUNREACH", EHOSTUNREACH, "Host is unreachable."},
	#endif
	
	#ifdef EIDRM
	{"EIDRM", EIDRM, "Identifier removed."},
	#endif
	
	#ifdef EILSEQ
	{"EILSEQ", EILSEQ, "Illegal byte sequence."},
	#endif
	
	#ifdef EINPROGRESS
	{"EINPROGRESS", EINPROGRESS, "Operation in progress."},
	#endif
	
	#ifdef EINTR
	{"EINTR", EINTR, "Interrupted function."},
	#endif
	
	#ifdef EINVAL
	{"EINVAL", EINVAL, "Invalid argument."},
	#endif
	
	#ifdef EIO
	{"EIO", EIO, "I/O error."},
	#endif
	
	#ifdef EISCONN
	{"EISCONN", EISCONN, "Socket is connected."},
	#endif
	
	#ifdef EISDIR
	{"EISDIR", EISDIR, "Is a directory."},
	#endif
	
	#ifdef ELOOP
	{"ELOOP", ELOOP, "Too many levels of symbolic links."},
	#endif
	
	#ifdef EMFILE
	{"EMFILE", EMFILE, "Too many open files."},
	#endif
	
	#ifdef EMLINK
	{"EMLINK", EMLINK, "Too many links."},
	#endif
	
	#ifdef EMSGSIZE
	{"EMSGSIZE", EMSGSIZE, "Message too large."},
	#endif
	
	#ifdef ENAMETOOLONG
	{"ENAMETOOLONG", ENAMETOOLONG, "Filename too long."},
	#endif
	
	#ifdef ENETDOWN
	{"ENETDOWN", ENETDOWN, "Network is down."},
	#endif
	
	#ifdef ENETRESET
	{"ENETRESET", ENETRESET, "Connection aborted by network."},
	#endif
	
	#ifdef ENETUNREACH
	{"ENETUNREACH", ENETUNREACH, "Network unreachable."},
	#endif
	
	#ifdef ENFILE
	{"ENFILE", ENFILE, "Too many files open in system."},
	#endif
	
	#ifdef ENOBUFS
	{"ENOBUFS", ENOBUFS, "No buffer space available."},
	#endif
	
	#ifdef ENODATA
	{"ENODATA", ENODATA, "No message is available on the STREAM head read queue."},
	#endif
	
	#ifdef ENODEV
	{"ENODEV", ENODEV, "No such device."},
	#endif
	
	#ifdef ENOENT
	{"ENOENT", ENOENT, "No such file or directory."},
	#endif
	
	#ifdef ENOEXEC
	{"ENOEXEC", ENOEXEC, "Executable file format error."},
	#endif
	
	#ifdef ENOLCK
	{"ENOLCK", ENOLCK, "No locks available."},
	#endif
	
	#ifdef ENOMEM
	{"ENOMEM", ENOMEM, "Not enough space."},
	#endif
	
	#ifdef ENOMSG
	{"ENOMSG", ENOMSG, "No message of the desired type."},
	#endif
	
	#ifdef ENOPROTOOPT
	{"ENOPROTOOPT", ENOPROTOOPT, "Protocol not available."},
	#endif
	
	#ifdef ENOSPC
	{"ENOSPC", ENOSPC, "No space left on device."},
	#endif
	
	#ifdef ENOSR
	{"ENOSR", ENOSR, "No stream resources."},
	#endif
	
	#ifdef ENOSTR
	{"ENOSTR", ENOSTR, "Not a stream."},
	#endif
	
	#ifdef ENOSYS
	{"ENOSYS", ENOSYS, "Function not supported."},
	#endif
	
	#ifdef ENOTCONN
	{"ENOTCONN", ENOTCONN, "The socket is not connected."},
	#endif
	
	#ifdef ENOTDIR
	{"ENOTDIR", ENOTDIR, "Not a directory."},
	#endif
	
	#ifdef ENOTEMPTY
	{"ENOTEMPTY", ENOTEMPTY, "Directory not empty."},
	#endif
	
	#ifdef ENOTSOCK
	{"ENOTSOCK", ENOTSOCK, "Not a socket."},
	#endif
	
	#ifdef ENOTSUP
	{"ENOTSUP", ENOTSUP, "Not supported."},
	#endif
	
	#ifdef ENOTTY
	{"ENOTTY", ENOTTY, "Inappropriate I/O control operation."},
	#endif
	
	#ifdef ENXIO
	{"ENXIO", ENXIO, "No such device or address."},
	#endif
	
	#ifdef EOPNOTSUPP
	{"EOPNOTSUPP", EOPNOTSUPP, "Operation not supported on socket."},
	#endif
	
	#ifdef EOVERFLOW
	{"EOVERFLOW", EOVERFLOW, "Value too large to be stored in data type."},
	#endif
	
	#ifdef EPERM
	{"EPERM", EPERM, "Operation not permitted."},
	#endif
	
	#ifdef EPIPE
	{"EPIPE", EPIPE, "Broken pipe."},
	#endif
	
	#ifdef EPROTO
	{"EPROTO", EPROTO, "Protocol error."},
	#endif
	
	#ifdef EPROTONOSUPPORT
	{"EPROTONOSUPPORT", EPROTONOSUPPORT, "Protocol not supported."},
	#endif
	
	#ifdef EPROTOTYPE
	{"EPROTOTYPE", EPROTOTYPE, "Protocol wrong type for socket."},
	#endif
	
	#ifdef ERANGE
	{"ERANGE", ERANGE, "Result too large."},
	#endif
	
	#ifdef EROFS
	{"EROFS", EROFS, "Read-only file system."},
	#endif
	
	#ifdef ESPIPE
	{"ESPIPE", ESPIPE, "Invalid seek."},
	#endif
	
	#ifdef ESRCH
	{"ESRCH", ESRCH, "No such process."},
	#endif
	
	#ifdef ETIME
	{"ETIME", ETIME, "Stream ioctl() timeout."},
	#endif
	
	#ifdef ETIMEDOUT
	{"ETIMEDOUT", ETIMEDOUT, "Connection timed out."},
	#endif
	
	#ifdef ETXTBSY
	{"ETXTBSY", ETXTBSY, "Text file busy."},
	#endif
	
	#ifdef EWOULDBLOCK
	{"EWOULDBLOCK", EWOULDBLOCK, "Operation would block ."},
	#endif
	
	#ifdef EXDEV
	{"EXDEV", EXDEV, "Cross-device link ."},
	#endif
	
	{"ECMDTIMEDOUT", ECMDTIMEDOUT, "Command timed out"},
	
	{"EEXEC", EEXEC, "can't execute a command"},
	{"EWAIT", EWAIT, "wait function failed"},
	{"ECMDNOTFOUND", ECMDNOTFOUND, "command is not found"},
	{"EEXITCODENOTMATCH", EEXITCODENOTMATCH, "Exit code does not match"},
	{"EOUTPUTNOTMATCH", EOUTPUTNOTMATCH, "Output does not match"},
	{"ESIGNOTMATCH", ESIGNOTMATCH, "Signal does not match"},
	{"EUNXPSIG", EUNXPSIG, "Unexpected signal caught"},
	{"ESIGNOTRECEIPT", ESIGNOTRECEIPT, "Expected signal not receipt"},
	{"EFILENOTFOUND", EFILENOTFOUND, "specified tesh file not found"},
	{"EGETCWD", EGETCWD, "system error : the getcwd() function failed"},
	{"EDIRNOTFOUND", EDIRNOTFOUND, "specified directory not found"},
	{"EPROCCMDLINE", EPROCCMDLINE, "process_command_line() failed : internal error"},
	{"ENOARG", ENOARG, "none optional argument not specified"},
	{"ENOTPOSITIVENUM", ENOTPOSITIVENUM, "argument option not strictly positive"},
	{"ESYNTAX", ESYNTAX, "Syntax error"},
	{"EINVALIDTIMEOUT", EINVALIDTIMEOUT, "timeout value specified by metacommand invalid"},
	{"EINVALIDEXITCODE", EINVALIDEXITCODE, "expected exit code value specified by the metacommand invalid"},
	{"ESIGNOTSUPP", ESIGNOTSUPP, "signal specified by the metacommand not supported (Windows specific)"},
	{"ELEADTIME", ELEADTIME, "lead time"},
	{"EREADMENOTFOUND", EREADMENOTFOUND, "unable to locate the README.txt file"},
	{"EINCLUDENOTFOUND", EINCLUDENOTFOUND, "include file specified by a metacommand is not found"},
	{"ESUFFIXTOOLONG", ESUFFIXTOOLONG, "suffix is too long"},
	{"EFILENOTINSPECDIR", EFILENOTINSPECDIR,"file not found in the specified directories"},
	{"EFILENOTINCURDIR", EFILENOTINCURDIR,"file not found in the current directory"},
	{"EINVCMDLINE", EINVCMDLINE, "invalid command line"},
	{"unkwown", -1, "unknown"}
};

#include <stdio.h>

#ifdef WIN32
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
error_to_string(int errcode)
{
	int i;
	
	for(i = 0; err[i].code != -1; i++)
		if(err[i].code == errcode)
			return err[i].string;

	#ifdef WIN32
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

void
error_register(const char* reason, int errcode, const char* command, const char* unit)
{
	xerror_t error;
	
	xbt_os_mutex_acquire(err_mutex);
	
	if(!exit_code)
		exit_code = errcode;
	
	error = xbt_new0(s_xerror_t, 1);
	
	error->reason = reason;
	error->command = command;
	error->unit = unit;
	error->errcode = errcode;
	
	xbt_dynar_push(errors, &error);
	
	xbt_os_mutex_release(err_mutex);
}
