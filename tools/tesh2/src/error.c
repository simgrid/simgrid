#include <error.h>

typedef struct s_entry
{
	const char* name;
	int code;
	const char* string;
}entry_t;


static const
entry_t err[] =
{
	{"ENOENT", ENOENT, "No such file of directory."},
	{"ENOMEM", ENOMEM,"Insufficient memory is available."},
	{"EACCES", EACCES, "Read or search permission was denied for a component of the pathname."},
	{"ENOTDIR", ENOTDIR, "Not a directory."},
	{"EREAD", EREAD, "a read pipe operation failed"},
	{"EREADPIPE", EREADPIPE, "a pipe used to read from the stdout of a command is broken"},
	{"ETIMEOUT", ETIMEOUT, "a command is timeouted"},
	{"EWRITE", EWRITE, "a write operation failed"},
	{"EWRITEPIPE", EWRITEPIPE, "a pipe used to write to the stdin of a command is broken"},
	{"EEXEC", EEXEC, "can't execute a command"},
	{"EWAIT", EWAIT, "wait function failed"},
	{"ECMDNOTFOUND", ECMDNOTFOUND, "command is not found"},
	{"EEXITCODENOTMATCH", EEXITCODENOTMATCH, "exit codes don't match"},
	{"EOUTPUTNOTMATCH", EOUTPUTNOTMATCH, "outputs don't match"},
	{"ESIGNOTMATCH", ESIGNOTMATCH, "signals don't match"},
	{"EUNEXPECTEDSIG", EUNEXPECTEDSIG, "unexpected signal caught"},
	{"ESIGNOTRECEIPT", ESIGNOTRECEIPT, "expected signal not receipt"},
	{"EFILENOTFOUND", EFILENOTFOUND, "specified tesh file not found"},
	{"EGETCWD", EGETCWD, "system error : the getcwd() function failed"},
	{"EDIRNOTFOUND", EDIRNOTFOUND, "specified directory not found"},
	{"ECHDIR", ECHDIR, "system error : the chdir() function failed"},
	{"EPROCCMDLINE", EPROCCMDLINE, "process_command_line() failed : internal error"},
	{"ENOARG", ENOARG, "none optional argument not specified"},
	{"ENOTPOSITIVENUM", ENOTPOSITIVENUM, "argument option not strictly positive"},
	{"ESYNTAX", ESYNTAX, "syntax error"},
	{"EINVALIDTIMEOUT", EINVALIDTIMEOUT, "timeout value specified by metacommand invalid"},
	{"EINVALIDEXITCODE", EINVALIDEXITCODE, "expected exit code value specified by the metacommand invalid"},
	{"ESIGNOTSUPP", ESIGNOTSUPP, "signal specified by the metacommand not supported (Windows specific)"},
	{"ELEADTIME", ELEADTIME, "lead time"},
	{"EREADMENOTFOUND", EREADMENOTFOUND, "unable to locate the README.txt file"},
	{"EINCLUDENOTFOUND", EINCLUDENOTFOUND, "include file specified by a metacommand is not found"},
	{"ESUFFIXTOOLONG", ESUFFIXTOOLONG, "suffix is too long"},
	{"EFILENOTINSPECDIR", EFILENOTINSPECDIR,"file not found in the specified directories"},
	{"EFILENOTINCURDIR", EFILENOTINCURDIR,"file not found in the current directory"},
	{"unkwown", -1, "unknown"}
};

#include <stdio.h>

const char*
error_to_string(int errcode)
{
	int i;
	
	for(i = 0; err[i].code != -1; i++)
		if(err[i].code == errcode)
			return err[i].string;
	
	return "unknow error";	
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
