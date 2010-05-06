/*
 * src/main.c - this file contains the main function of tesh.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 *
 */
#include <runner.h>
#include <fstream.h>
#include <fstreams.h>
#include <directory.h>
#include <directories.h>
#include <excludes.h>
#include <getopt.h>
#include <getpath.h>

#include <locale.h>

/*
 * entry used to define the parameter of a tesh option.
 */
typedef struct s_optentry
{
	int c;								/* the character of the option											*/
	
	/* 
	 * the type of the argument of the option 
	 */
	enum						
	{
		flag,							/* it's a flag option, by default the flag is set to zero				*/						
		string,							/* the option has strings as argument									*/					
		number							/* the option has an integral positive number as argument				*/			
	}type;
	
	byte* value;						/* the value of the option												*/			
	byte* optional_value;				/* the optional value of the option if not specified					*/
	const char * long_name;				/* the long name of the command											*/
}s_optentry_t,* optentry_t;


/* logs */
XBT_LOG_NEW_DEFAULT_CATEGORY(tesh,"TEst SHell utility");

#ifdef WIN32
/* Windows specific : the previous process error mode			*/
static UINT 
prev_error_mode = 0;
#endif

/* this object represents the root directory */
directory_t
root_directory = NULL;

/* the current version of tesh									*/
static const char* 
version = "1.0";

/* ------------------------------------------------------------ */
/* options														*/
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/* numbers														*/
/* ------------------------------------------------------------ */


/* --jobs is specified with arg									*/
static int 
jobs_nb = -2;

/* --jobs option is not specified (use the default job count)	*/
static int 
default_jobs_nb = 1;

/* --jobs is specified but has no arg (one job per unit)		*/
static int 
optional_jobs_nb = -1;

/* the global timeout											*/
static int
timeout = INDEFINITE;

/* ------------------------------------------------------------ */
/* strings											*/
/* ------------------------------------------------------------ */

/* --C change the directory before running the units			*/
static directories_t 
directories = NULL;

/* the include directories : see the !i metacommand				*/
xbt_dynar_t
include_dirs = NULL;

/* the list of tesh files to run								*/
static fstreams_t 
fstreams = NULL;

/* the list of tesh file suffixes								*/
static xbt_dynar_t
suffixes = NULL;

static excludes_t
excludes = NULL;


/* ------------------------------------------------------------ */
/* flags														*/
/* ------------------------------------------------------------ */

/* if 1, keep going when some commands can't be found
 * default value 0 : not keep going
 */
int 
keep_going_flag = 0;

/* if 1, ignore failures from commands
 * default value : do not ignore failures
 */
int 
keep_going_unit_flag = 0;

/* if 1, display tesh usage										*/
static int 
print_usage_flag = 0;

/* if 1, display the tesh version								*/
static int 
print_version_flag = 0;

/* if 1, the status of all the units is display at
 * the end.
 */
static int
summary_flag = 0;

/* if 1 and the flag want_summay is set to 1 tesh display the detailed summary of the run */
int 
detail_summary_flag = 0;

/* if 1, the directories are displayed							*/
int 
print_directory_flag = 0;

/* if 1, just check the syntax of all the tesh files
 * do not run them.
 */
int
dry_run_flag = 0;

/* if 1, display the tesh files syntax and exit					*/
static int
print_readme_flag = 0;

int 
silent_flag = 0;

int 
just_print_flag = 0;

/* the semaphore used to synchronize the jobs */
xbt_os_sem_t
jobs_sem = NULL;

/* the semaphore used by the runner to wait the end of all the units */
xbt_os_sem_t
units_sem = NULL;

static int
loaded = 0;

int 
interrupted = 0;

int
exit_code = 0; 

int
err_kind = 0;

char* 
err_line = NULL;


pid_t
pid =0;

int 
is_tesh_root = 1;

/* the table of the entries of the options */ 
static const struct s_optentry opt_entries[] =
{
	{ 'C', string, (byte*)NULL, 0, "directory" },
	{ 'x', string, (byte*)&suffixes, 0, "suffix" },
	{ 'f', string, (byte*)&fstreams, 0, "file" },
	{ 'h', flag, (byte*)&print_usage_flag, 0, "help" },
	{ 'a', flag, (byte*)&print_readme_flag, 0, "README" },
	{ 'k', flag, (byte*)&keep_going_flag, 0, "keep-going" },
	{ 'i', flag, (byte*)&keep_going_unit_flag, 0, "keep-going-unit"},
	{ 'I', string, (byte*)&include_dirs, 0, "include-dir" },
	{ 'j', number, (byte*)&jobs_nb, (byte*) &optional_jobs_nb, "jobs" },
	{ 'm', flag, (byte*)&detail_summary_flag, 0, "detail-summary" },
	{ 'c', flag, (byte*)&just_print_flag, 0, "just-print" },
	{ 's', flag, (byte*)&silent_flag, 0, "silent" },
	{ 'V', flag, (byte*)&print_version_flag, 0, "version" },
	{ 'w', flag, (byte*)&print_directory_flag, 0,"dont-print-directory" },
	{ 'n', flag, (byte*)&dry_run_flag, 0, "dry-run"},
	{ 't', number, (byte*)&timeout, 0, "timeout" },
	{ 'r', string, (byte*)&directories, 0, "load-directory"},
	{ 'v', flag, (byte*)&summary_flag, 0, "summary"},
	{ 'F', string,(byte*)&excludes, 0, "exclude"},
	{ 'l', string, (byte*)NULL, 0, "log" },
	{ 0, 0, 0, 0, 0}
};

/* the tesh usage												*/
static const char* usage[] =
{
	"Options:\n",
	"  -C DIRECTORY, --directory=DIRECTORY   Change to DIRECTORY before running any commands.\n",
	"  -e, --environment-overrides           Environment variables override files.\n",
	"  -f FILE, --file=FILE                  Read FILE as a teshfile.\n",
	"                                           remark :\n",
	"                                           all argument of the command line without\n",
	"                                           option is dealed as a tesh file.\n",
	"  -h, --help                            Print this message and exit.\n",
	"  -i, --keep-going-unit                 Ignore failures from commands.\n",
	"                                        The possible failures are :\n",
	"                                         - the exit code differ from the expected\n",
	"                                         - the signal throw differ from the expected\n",
	"                                         - the output differ from the expected\n",
	"                                         - the read pipe is broken\n",
	"                                         - the write pipe is broken\n",
	"                                         - the command assigned delay is outdated\n",
	"  -I DIRECTORY, --include-dir=DIRECTORY Search DIRECTORY for included files.\n",
	"  -j [N], --jobs[=N]                    Allow N units at once; infinite units with\n"
	"                                        no arg.\n",
	"  -k, --keep-going                      Keep going when some commands can't be made or\n",
	"                                        failed.\n",
	"  -c, --just-print                      Don't actually run any commands; just print them.\n",
	"  -s, --silent,                         Don't echo commands.\n",
	"  -V, --version                         Print the version number of tesh and exit.\n",
	"  -d, --dont-print-directory            Don't display the current directory.\n",
	"  -n, --dry-run                         Check the syntax of the specified tesh files, display the result and exit.\n",
	"  -t, --timeout                         Wait the end of the commands at most timeout seconds.\n",
	"  -x, --suffix                          Consider the new suffix for the tesh files.\n"
	"                                           remark :\n",
	"                                           the default suffix for the tesh files is \".tesh\".\n",
	" -a, --README                           Print the read me file and exit.\n",
	" -r, --load-directory                   Run all the tesh files located in the directories specified by the option --directory.\n",
	" -v, --summary                          Display the status of the commands.\n",
	" -m, --detail-summary                   Detail the summary of the run.\n",
	" -F file , --exclude=FILE               Ignore the tesh file FILE.\n",
	" -l format, --log                       Format of the xbt logs.\n",
	NULL
};

/* the string of options of tesh								*/								
static char 
optstring[1 + sizeof (opt_entries) / sizeof (opt_entries[0]) * 3];

/* the option table of tesh										*/
static struct 
option longopts[(sizeof (opt_entries) / sizeof (s_optentry_t))];

static void
init_options(void);

static int
process_command_line(int argc, char** argv);

static void
load(void);

static void
print_usage(void);

static void
print_version(void);

static void
finalize(void);

static void
print_readme(void);

static int
init(void);

static int
screen_cleaned;

static int
finalized = 0;

static int 
sig_int = 0;

#ifdef WIN32
static void 
sig_int_handler(int signum)
{

	if(!finalized)
	{
		sig_int = 1;
		runner_interrupt();
		while(!finalized);
	}
	
}
#endif

static void 
free_string(void* str)
{
	free(*(void**)str);
}

int
main(int argc, char* argv[])
{
	/* set the locale to the default*/
	setlocale(LC_ALL,"");

	/* xbt initialization */
	xbt_init(&argc, argv);
	
	/* initialize tesh */
	if(init() < 0)
		finalize();
		
	/* process the command line */
	if(process_command_line(argc, argv) < 0)
		finalize();
	
	/* move to the root directory (the directory may change during the command line processing) */
	chdir(root_directory->name);
		
	/* the user wants to display the usage of tesh */
	if(print_version_flag)
	{
		print_version();

		if(!print_usage_flag)
			finalize();
	}
	
	/* the user wants to display the usage of tesh */
	if(print_usage_flag)
	{
		print_usage();
		finalize();
	}
	
	/* the user wants to display the semantic of the tesh file metacommands */
	if(print_readme_flag)
	{
		print_readme();
		finalize();
	}
	
	/* load tesh files */
	load();
	
	
	/* use by the finalize function to known if it must display the tesh usage */	
	loaded = 1;

	if(-2 == jobs_nb)
	{/* --jobs is not specified (use the default value) */
		jobs_nb = default_jobs_nb;
	}
	else if(optional_jobs_nb == jobs_nb)
	{/* --jobs option is specified with no args (use one job per unit) */
		jobs_nb = fstreams_get_size(fstreams);
	}

	if(jobs_nb > fstreams_get_size(fstreams))
	{/* --jobs = N is specified and N is more than the number of tesh files */
		
		WARN0("Number of requested jobs exceed the number of files to run");
		
		/* assume one job per file */
		jobs_nb = fstreams_get_size(fstreams);
	}

	if(jobs_nb != 1 && dry_run_flag)
	{
		jobs_nb = 1;
		INFO0("Dry run specified : force jobs count to 1");
	}

	/* initialize the semaphore used to synchronize all the units */
	jobs_sem = xbt_os_sem_init(jobs_nb);

	/* initialize the semaphore used by the runner to wait for the end of all units */
	units_sem = xbt_os_sem_init(0);
	
	/* initialize the runner */
	if(runner_init(/*check_syntax_flag,*/ timeout, fstreams) < 0)
		finalize();
		
	if(just_print_flag && silent_flag)
		silent_flag = 0;
		
	if(just_print_flag && dry_run_flag)
		WARN0("mismatch in the syntax : --just-check-syntax and --just-display options at same time");
	
	/* run all the units */
	runner_run();
	
	if(runner_is_timedout())
		ERROR1("Tesh timed out after `(%d)' seconds", timeout);

	/* show the result of the units */
	if(detail_summary_flag ||summary_flag || dry_run_flag)
		runner_summarize();
		
	/* all the test are runned, destroy the runner */
	runner_destroy();
	
	/* then, finalize tesh (release all the allocated memory and exits) */
	finalize();
	
	#ifndef WIN32
	return exit_code;
	#endif
	
}

/* init --	initialize tesh : allocated all the objects needed by tesh to run
 *			the tesh files specified in the command line.
 *
 * return	If successful the function returns zero. Otherwise the function returns
 *			-1 and sets the global variable errno to the appropriate error code.
 */



static int
init(void)
{
	char* buffer;
	char* suffix = strdup(".tesh");
	
	#ifdef WIN32
	/* Windows specific : don't display the general-protection-fault message box and
	 * the the critical-error-handler message box (instead the system send the error
	 * to the calling process : tesh)
	 */
	prev_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
 	
 	/* handle the interrupt signal */
 	signal(SIGINT, sig_int_handler);
	#else
	/* Ignore pipe issues.
	 * They will show up when we try to send data to dead buddies, 
     * but we will stop doing so when we're done with provided input 
     */
  	/*
	struct sigaction act;
	memset(&act,0, sizeof(struct sigaction));
 	act.sa_handler = SIG_IGN;
 	sigaction(SIGPIPE, &act, NULL);
 	
 	memset(&act,0, sizeof(struct sigaction));
 	act.sa_handler = sig_int_handler;
 	sigaction(SIGINT, &act, NULL);*/
 	
 	#endif
 	
 	
 	
	
	/* used to store the files to run */
	if(!(fstreams = fstreams_new((void_f_pvoid_t)fstream_free)))
	{
		ERROR1("(system error) %s", strerror(errno));
		return -1;
	}
	
	/* register the current directory */
	if(!(buffer  = getcwd(NULL, 0)))
	{
		ERROR1("(system error) %s", strerror(errno));
		return -1;
	}
	
	/* save the root directory */
	if(!(root_directory = directory_new(buffer)))
	{
		ERROR1("(system error) %s", strerror(errno));
		return -1;
	}
	
	free(buffer);
	
	/* the directories to loads */
	if(!(directories = directories_new()))
	{
		ERROR1("(system error) %s", strerror(errno));
		return -1;
	}
	
	/* the include directories */
	include_dirs = xbt_dynar_new(sizeof(directory_t), (void_f_pvoid_t)directory_free);
	
	/* the excluded files */
	if(!(excludes = excludes_new()))
	{
		ERROR1("(system error) %s", strerror(errno));
		return -1;
	}
	
	/* the suffixes */
	suffixes = xbt_dynar_new(sizeof(char*),free_string);
	
	/* register the default suffix ".tesh" */
	xbt_dynar_push(suffixes, &suffix);
	
	return 0;
}

/* load -- load the tesh files to run */
static void
load(void)
{
	
	/* if the directories object is not empty load all the tesh files contained in
	 * the directories specified in the command line (this tesh files must have the
	 * a suffix specified in the suffixes object.
	 */
	if(!directories_is_empty(directories))
		directories_load(directories, fstreams, suffixes);
	
	/* if no tesh file has been specified in the command line try to load the default tesh file
	 * teshfile from the current directory
	 */
	if(fstreams_is_empty(fstreams))
	{
		struct stat buffer = {0};
		
		/* add the default tesh file if it exists in the current directory */
		if(!stat("teshfile", &buffer) && S_ISREG(buffer.st_mode))
			fstreams_add(fstreams, fstream_new(getcwd(NULL, 0), "teshfile"));
	}
	
	/* excludes the files specified in the command line and stored in the excludes object */
	if(!excludes_is_empty(excludes) && !fstreams_is_empty(fstreams))
	{
		/* check the files to excludes before */	
		excludes_check(excludes, fstreams);
		
		/* exclude the specified tesh files */
		fstreams_exclude(fstreams, excludes);
	}
	
	/* if the fstreams object is empty use the stdin */
	if(fstreams_is_empty(fstreams))
		fstreams_add(fstreams, fstream_new(NULL, "stdin"));
	
	/* load the tesh files (open them) */
	fstreams_load(fstreams);
	
}

/* finalize -- cleanup all the allocated objects and display the tesh usage if needed */
static void
finalize(void)
{
	/* delete the fstreams object */
	if(fstreams)
		fstreams_free((void**)&fstreams);
	
	/* delete the excludes object */
	if(excludes)	
		excludes_free((void**)&excludes);
	
	/* delete the directories object */
	if(directories)
		directories_free((void**)&directories);
		
	/* delete the root directory object */
	if(root_directory)
		directory_free((void**)&root_directory);
	
	/* delete the include directories object */
	if(include_dirs)
		xbt_dynar_free(&include_dirs);
	
	/* delete the list of tesh files suffixes */	
	if(suffixes)
		xbt_dynar_free(&suffixes);
		
	/* destroy the semaphore used to synchronize the units */
	if(jobs_sem)
		xbt_os_sem_destroy(jobs_sem);
	
	/* destroy the semaphore used by the runner used to wait for the end of the units */
	if(units_sem)
		xbt_os_sem_destroy(units_sem);
	
	/* Windows specific (restore the previouse error mode */
	#ifdef WIN32
	SetErrorMode(prev_error_mode);
	#endif

	if(sig_int)
		INFO0("Tesh interrupted (receive a SIGINT)");
	else if(!summary_flag && !detail_summary_flag && !dry_run_flag && !silent_flag && !just_print_flag && !print_version_flag && !print_usage_flag && is_tesh_root)
	{
		if(!exit_code)
			INFO2("Tesh terminated with exit code %d : %s",exit_code, "success");
		else
		{
			if(err_line)
				ERROR3("Tesh terminated with exit code `(<%s> %s)' (%d)",err_line, error_to_string(exit_code, err_kind), exit_code);
			else
				ERROR2("Tesh terminated with exit code `(%s)' (%d)", error_to_string(exit_code, err_kind), exit_code);
			
		}
	}

	if(err_line)
		free(err_line);
	
	/* exit from the xbt framework */

	finalized = 1;
	
	/* exit with the last error code */
	if(!sig_int)
		exit(exit_code);
}

/* init_options -- initialize the options string */
static void
init_options (void)
{
	char *p;
	unsigned int i;
	
	/* the function has been already called */
	if(optstring[0] != '\0')
		return;
	
	p = optstring;
	
	*p++ = '-';
	
	for (i = 0; opt_entries[i].c != '\0'; ++i)
	{
		/* initialize the long name of the option*/
		longopts[i].name = (opt_entries[i].long_name == 0 ? "" : opt_entries[i].long_name);
		
		/* getopt_long returns the value of val */
		longopts[i].flag = 0;
		
		/* the short option */
		longopts[i].val = opt_entries[i].c;
		
		/* add the short option in the options string */
		*p++ = opt_entries[i].c;
		
		switch (opt_entries[i].type)
		{
			/* if this option is used to set a flag or if the argument must be ignored
			 * the option has no argument
			 */
			case flag:
			longopts[i].has_arg = no_argument;
			break;
		
			/* the option has an argument */
			case string:
			case number:
			
			*p++ = ':';
			
			if(opt_entries[i].optional_value != 0)
			{
				*p++ = ':';
				
				longopts[i].has_arg = optional_argument;
			}
			else
				longopts[i].has_arg = required_argument;
			
			break;
		}
	}
	
	*p = '\0';
	longopts[i].name = 0;
}

/* process_command_line --	process the command line
 *
 * param					argc the number of the arguments contained by the command line.
 * param					The array of C strings containing all the arguments of the command 
 *							line.
 *
 * return					If successful, the function returns 0. Otherwise -1 is returned
 *							and sets the global variable errno to indicate the error.
 *
 * errors	[ENOENT]		A file name specified in the command line does not exist
 */	

static int
process_command_line(int argc, char** argv)
{
	register const struct s_optentry* entry;
	register int c;
	directory_t directory;
	fstream_t fstream;
	
	/* initialize the options string of tesh */
	init_options();
	
	/* let the function getopt_long display the errors if any */
	opterr = 1;
	
	/* set option index to zero */
	optind = 0;
	
	while (optind < argc)
	{
		c = getopt_long(argc, argv, optstring, longopts, (int *) 0);
		
		if(c == EOF)
		{
			/* end of the command line or "--".  */
			break;
		}
		else if (c == 1)
		{
			/* no option specified, assume it's a tesh file to run */
			char* path;
			char* delimiter;
			
			/* getpath returns -1 when the file to get the path doesn't exist */
			if(getpath(optarg, &path) < 0)
			{
				exit_code = errno;
				err_kind = 0;
				
				if(ENOENT == errno)
					ERROR1("File %s does not exist", optarg);
				else
					ERROR0("Insufficient memory is available to parse the command line : system error");
					
				return -1;
			}
			
			/* get to the last / (if any) to get the short name of the file */
			delimiter = strrchr(optarg,'/');
			
			#ifdef WIN32
			if(!delimiter)
				delimiter = strrchr(optarg,'\\');
			#endif
			
			/* create a new file stream which represents the tesh file to run */
			fstream = fstream_new(path, delimiter ? delimiter + 1 : optarg);
			
			free(path);
			
			/* if the list of all tesh files to run already contains this file
			 * destroy it and display a warning, otherwise add it in the list.
			 */
			if(fstreams_contains(fstreams, fstream))
			{
				fstream_free(&fstream);
				WARN1("File %s already specified to be run", optarg);
			}
			else
				fstreams_add(fstreams, fstream);
			
		}
		else if (c == '?')
		{
			/* unknown option, let getopt_long() displays the error */
			ERROR0("Command line processing failed : invalid command line");
			exit_code = EINVCMDLINE;
			err_kind = 1;
			return -1;
		}
		else
		{
			for(entry = opt_entries; entry->c != '\0'; ++entry)
				
				if(c == entry->c)
				{
		
					switch (entry->type)
					{
						/* impossible */
						default:
						ERROR0("Command line processing failed : internal error");
						exit_code = EPROCCMDLINE;
						err_kind = 1;
						return -1;
						
						
						/* flag options */
						case flag:
						/* set the flag to one */
						*(int*) entry->value = 1;
						
						break;
						
						/* string options */
						case string:
		
						if(!optarg)
						{
							/* an option with a optional arg is specified use the entry->optional_value */
							optarg = (char*)entry->optional_value;
						}
						else if (*optarg == '\0')
						{
							/* a non optional argument is not specified */
							ERROR2("Option %c \"%s\"requires an argument",entry->c,entry->long_name);
							exit_code = ENOARG;
							err_kind = 1;
							return -1;
						}
						
						/* --load-directory option */
						if(!strcmp(entry->long_name,"load-directory"))
						{
							char* path;
							
							if(translatepath(optarg, &path) < 0)
							{
								exit_code = errno;
								err_kind = 0;
								
								if(ENOTDIR == errno)
									ERROR1("%s is not a directory",optarg);
								else
									ERROR0("Insufficient memory is available to process the command line - system error");
									
								return -1;
								
							}
							else
							{
								
								directory = directory_new(path);
								free(path);
						
								if(directories_contains(directories, directory))
								{
									directory_free((void**)&directory);
									WARN1("Directory %s already specified to be load",optarg);
								}
								else
									 directories_add(directories, directory);
									 
								
							}			
						}
						else if(!strcmp(entry->long_name,"directory"))
						{
							char* path ;
							
							if(translatepath(optarg, &path) < 0)
							{
								exit_code = errno;
								err_kind = 0;
								
								if(ENOTDIR == errno)
									ERROR1("%s is not a directory",optarg);
								else
									ERROR0("Insufficient memory is available to process the command line - system error");
									
								return -1;
							}
							else
							{
								char* buffer = getcwd(NULL, 0);

								
								if(!strcmp(buffer, path))
									WARN1("Already in the directory %s", optarg);
								else if(!print_directory_flag)
									INFO1("Entering directory \"%s\"",path);

								chdir(path);
								free(path);
						
								free(buffer);
							}	
						}
						
						/* --suffix option */
						else if(!strcmp(entry->long_name,"suffix"))
						{
							if(strlen(optarg) > MAX_SUFFIX)
							{
								ERROR1("Suffix %s too long",optarg);
								exit_code = ESUFFIXTOOLONG;
								err_kind = 1;
								return -1;
							}
							
							if(optarg[0] == '.')
							{
								char* cur;
								unsigned int i;
								int exists = 0;

								char* suffix = xbt_new0(char, MAX_SUFFIX + 2);
								sprintf(suffix,".%s",optarg);
								
								xbt_dynar_foreach(suffixes, i, cur)
								{
									if(!strcmp(suffix, cur))
									{
										exists = 1;
										break;
									}
								}

								if(exists)
									WARN1("Suffix %s already specified to be used", optarg);
								else
									xbt_dynar_push(suffixes, &suffix);
							}
							else
							{
								char* cur;
								unsigned int i;
								int exists = 0;

								xbt_dynar_foreach(suffixes, i, cur)
								{
									if(!strcmp(optarg, cur))
									{
										exists = 1;
										break;
									}
								}

								if(exists)
									WARN1("Suffix %s already specified to be used", optarg);
								else
								{
									char* suffix = strdup(optarg);
									xbt_dynar_push(suffixes, &suffix);
								}
							}
						}
						/* --file option */
						else if(!strcmp(entry->long_name,"file"))
						{
							char* path;
							char* delimiter;
							
							if(getpath(optarg, &path) < 0)
							{
								exit_code = errno;
								err_kind = 0;
								
								if(ENOENT == errno)
									ERROR1("File %s does not exist", optarg);
								else
									ERROR1("System error :`(%s)'", strerror(errno));
								
								return -1;
							}
							
							delimiter = strrchr(optarg,'/');

							#ifdef WIN32
							if(!delimiter)
								delimiter = strrchr(optarg,'\\');
							#endif
							
							fstream = fstream_new(path, delimiter ? delimiter + 1 : optarg);
							
							free(path);
							
							if(fstreams_contains(fstreams, fstream))
							{
								fstream_free(&fstream);
								WARN1("File %s already specified to run", optarg);
							}
							else
								fstreams_add(fstreams, fstream);
						}
						/* --include-dir option */
						else if(!strcmp(entry->long_name,"include-dir"))
						{
							
							char* path ;
							
							if(translatepath(optarg, &path) < 0)
							{
								exit_code = errno;
								err_kind = 0;
								
								if(ENOTDIR == errno)
									ERROR1("%s is not a directory",optarg);
								else
									ERROR0("Insufficient memory is available to process the command line - system error");
									
								return -1;
							}
							
							else
							{
								int exists = 0;
								unsigned int i;
								directory_t cur;
								directory = directory_new(path);
								free(path);

								xbt_dynar_foreach(include_dirs, i , cur)
								{
									if(!strcmp(cur->name, optarg))
									{
										exists = 1;
										break;
									}
								}

								if(exists)
								{
									directory_free((void**)&directory);
									WARN1("Include directory %s already specified to be used",optarg);
									
								}
								else
									xbt_dynar_push(include_dirs, &directory);

							}
						}
						/* --exclude option */ 
						else if(!strcmp(entry->long_name,"exclude"))
						{
							char* path;
							char* delimiter;
			
							if(getpath(optarg, &path) < 0)
							{
								exit_code = errno;
								err_kind = 0;
								
								if(ENOENT == errno)
									ERROR1("file %s does not exist", optarg);
								else
									ERROR0("Insufficient memory is available to process the command line - system error");
									
								return -1;
							}
			
							delimiter = strrchr(optarg,'/');

							#ifdef WIN32
							if(!delimiter)
								delimiter = strrchr(optarg,'\\');
							#endif
			
							fstream = fstream_new(path, delimiter ? delimiter + 1 : optarg);
							free(path);
			
							if(excludes_contains(excludes, fstream))
							{
								fstream_free(&fstream);
								WARN1("File %s already specified to be exclude", optarg);
							}
							else
								excludes_add(excludes, fstream);
									
						}
						else if(!strcmp(entry->long_name,"log"))
						{
							/* NOTHING TODO : log for xbt */
						}
						else
						{
							INFO1("Unexpected option %s", optarg);
							return -1;
						}
						
						
						break;
						
						/* strictly positve number options */
						case number:
							
						if ((NULL == optarg) && (argc > optind))
						{
							const char* cp;
							
							for (cp = argv[optind]; isdigit(cp[0]); ++cp)
								if(cp[0] == '\0')
									optarg = argv[optind++];
						}
		
						/* the number option is specified */
						if(NULL != optarg)
						{
							int i = atoi(optarg);
							const char *cp;

							for (cp = optarg; isdigit(cp[0]); ++cp);
		
							if (i < 1 || cp[0] != '\0')
							{
								ERROR2("Option %c \"%s\" requires an strictly positive integer as argument",entry->c, entry->long_name);
								exit_code = ENOTPOSITIVENUM;
								err_kind = 1;
								return -1;
							}
							else
								*(int*)entry->value = i;
						}
						/* the number option is specified but has no arg, use the optional value*/
						else
							*(int*)entry->value = *(int*) entry->optional_value;
						
						break;
		
				}
				break;
			}
		}
	}

	return 0;
}

static void
print_usage(void)
{
	const char **cpp;
	FILE* stream;
	
	stream = exit_code ? stderr : stdout;

	if(!screen_cleaned)
	{
		#ifdef WIN32
		system("cls");
		#else
		system("clear");
		#endif
		screen_cleaned = 1;
	}
	
	fprintf (stream, "Usage: tesh [options] [file] ...\n");
	
	for (cpp = usage; *cpp; ++cpp)
		fputs (*cpp, stream);
	
	fprintf(stream, "\nReport bugs to <martin.quinson@loria.fr | malek.cherier@loria.fr>\n");
}

static void
print_version(void)
{
	if(!screen_cleaned)
	{
		#ifdef WIN32
		system("cls");
		#else
		system("clear");
		#endif
		screen_cleaned = 1;
	}

	/* TODO : display the version of tesh */
	printf("Version :\n");
	printf("  tesh version %s : Mini shell specialized in running test units by Martin Quinson \n", version);
	printf("  and Malek Cherier\n");
	printf("  Copyright (c) 2007, 2008 Martin Quinson, Malek Cherier\n");
	printf("  All rights reserved\n");
	printf("  This program is free software; you can redistribute it and/or modify it\n");
	printf("  under the terms of the license (GNU LGPL) which comes with this package.\n\n");
	
	if(!print_usage_flag)
		printf("Report bugs to <martin.quinson@loria.fr | malek.cherier@loria.fr>");
}

static void
print_readme(void)
{
	size_t len;
	char * line = NULL;
	
	FILE* stream = fopen("examples/README.tesh", "r");
	
	if(!stream)
	{
		ERROR0("Unable to locate the README.txt file");
		exit_code = EREADMENOTFOUND;
		err_kind = 1;
		return;
	}
	
	while(getline(&line, &len, stream) != -1)
		printf("%s",line);
		
	fclose(stream);
}


