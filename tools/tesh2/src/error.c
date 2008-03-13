#include <error.h>

typedef struct s_entry
{
	int code;
	const char* string;
}entry_t;

static const
entry_t err[] =
{
	{EREAD, "a read pipe operation failed"},
	{EREADPIPE, "a pipe used to read from the stdout of a command is broken"},
	{ETIMEOUT, "a command is timeouted"},
	{EWRITE, "a write operation failed"},
	{EWRITEPIPE, "a pipe used to write to the stdin of a command is broken"},
	{EEXEC, "can't execute a command"},
	{EWAIT, "wait function failed"},
	{ECMDNOTFOUND, "command is not found"},
	{EEXITCODENOTMATCH, "exit codes don't match"},
	{EOUTPUTNOTMATCH, "outputs don't match"},
	{ESIGNOTMATCH, "signals don't match"},
	{EUNEXPECTEDSIG, "unexpected signal caught"},
	{ESIGNOTRECEIPT, "expected signal not receipt"},
	{EFILENOTFOUND, "specified tesh file not found"},
	{EGETCWD, "system error : the getcwd() function failed"},
	{EDIRNOTFOUND, "specified directory not found"},
	{ECHDIR, "system error : the chdir() function failed"},
	{EPROCESSCMDLINE, "internal error : the process_command_line() function failed"},
	{EARGNOTSPEC, "none optional argument not specified in the command line"},
	{ENOTPOSITIVENUM, "argument option not strictly positive"},
	{ESYNTAX, "syntax error"},
	{EINVALIDTIMEOUT, "timeout value specified by metacommand invalid"},
	{EINVALIDEXITCODE, "expected exit code value specified by the metacommand invalid"},
	{ESIGNOTSUPP, "signal specified by the metacommand not supported (Windows specific)"},
	{ELEADTIME, "lead time"},
	{EREADMENOTFOUND, "unable to locate the README.txt file"},
	{EINCLUDENOTFOUND, "include file specified by a metacommand is not found"},
	{ESUFFIXTOOLONG, "suffix is too long"},
	{EFILENOTINSPECDIR,"file not found in the specified directories"},
	{EFILENOTINCURDIR,"file not found in the current directory"},
	{-1, "unknown"}
};

const char*
error_to_string(int error)
{
	int i;
	
	for(i = 0; error >= 0 && err[i].code != -1; i++)
	{
		if(err[i].code == error)
			return err[i].string;
	} 
	
	return "unknow error";	
}
