
#include <runner.h>
#include <fstream.h>
#include <fstreams.h>
#include <directory.h>
#include <directories.h>
#include <excludes.h>
#include <error.h>

#include <getopt.h>
#include <getpath.h>

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



XBT_LOG_NEW_DEFAULT_CATEGORY(tesh,"TEst SHell utility");

#ifdef WIN32
/* Windows specific : the previous process error mode			*/
static UINT 
prev_error_mode = 0;
#endif

directory_t
root_directory = NULL;

/*int
exit_code = 0;
*/

int 
want_detail_summary = 0;

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
number_of_jobs = -2;

/* --jobs option is not specified (use the default job count)	*/
static int 
default_number_of_jobs = 1;

/* --jobs is specified but has no arg (one job per unit)		*/
static int 
optional_number_of_jobs = -1;

/* the global timeout											*/
static int
timeout = INDEFINITE;

/* ------------------------------------------------------------ */
/* strings dlists												*/
/* ------------------------------------------------------------ */

/* --C change the directory before running the units			*/
static directories_t 
directories = NULL;

/* the include directories : see the !i metacommand				*/
vector_t 
include_dirs = NULL;

/* the list of tesh files to run								*/
static fstreams_t 
fstreams = NULL;

/* xbt logs														*/
static lstrings_t
logs = NULL;

static excludes_t
excludes = NULL;

/* the ddlist of tesh file suffixes								*/
static lstrings_t
suffixes = NULL;

/* ------------------------------------------------------------ */
/* flags														*/
/* ------------------------------------------------------------ */

/* if 1, keep going when some commands can't be found
 * default value 0 : not keep going
 */
int 
want_keep_going = 0;

/* if 1, ignore failures from commands
 * default value : do not ignore failures
 */
int 
want_keep_going_unit = 0;

/* if 1, display tesh usage										*/
static int 
want_display_usage = 0;

/* if 1, display the tesh version								*/
static int 
want_display_version = 0;

/* if 1, the syntax of all tesh files is checked 
 * before running them
 */
static int
want_check_syntax = 0;

/* if 1, the status of all the units is display at
 * the end.
 */
static int
want_verbose = 0;

/* if 1, the directories are displayed							*/
int 
dont_want_display_directory = 0;

/* if 1, just check the syntax of all the tesh files
 * do not run them.
 */
int
want_dry_run = 0;

/* if 1, display the tesh files syntax and exit					*/
static int
want_display_semantic = 0;

int 
want_silent = 0;

int 
want_just_display = 0;

static int 
env_overrides  = 0;

static int 
display_data_base = 0;

static int 
question = 0;

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

pid_t
pid =0;

/* the table of the entries of the options */ 
static const struct s_optentry opt_entries[] =
{
	{ 'C', string, (byte*)NULL, 0, "directory" },
	{ 'x', string, (byte*)&suffixes, 0, "suffix" },
	{ 'e', flag, (byte*)&env_overrides, 0, "environment-overrides", },
	{ 'f', string, (byte*)&fstreams, 0, "file" },
	{ 'h', flag, (byte*)&want_display_usage, 0, "help" },
	{ 'a', flag, (byte*)&want_display_semantic, 0, "semantic" },
	{ 'i', flag, (byte*)&want_keep_going_unit, 0, "keep-going-unit" },
	{ 'I', string, (byte*)&include_dirs, 0, "include-dir" },
	{ 'j', number, (byte*)&number_of_jobs, (byte*) &optional_number_of_jobs, "jobs" },
	{ 'k', flag, (byte*)&want_keep_going, 0, "keep-going" },
	{ 'm', flag, (byte*)&want_detail_summary, 0, "detail-summary" },
	{ 'c', flag, (byte*)&want_just_display, 0, "just-display" },
	{ 'd', flag, (byte*)&display_data_base, 0,"display-data-base" },
	{ 'q', flag, (byte*)&question, 0, "question" },
	{ 's', flag, (byte*)&want_silent, 0, "silent" },
	{ 'V', flag, (byte*)&want_display_version, 0, "version" },
	{ 'w', flag, (byte*)&dont_want_display_directory, 0,"dont-display-directory" },
	{ 'n', flag, (byte*)&want_dry_run, 0, "dry-run"},
	{ 't', number, (byte*)&timeout, 0, "timeout" },
	{ 'S', flag, (byte*)&want_check_syntax, 0, "check-syntax"},
	{ 'r', string, (byte*)&directories, 0, "load-directory"},
	{ 'v', flag, (byte*)&want_verbose, 0, "verbose"},
	{ 'F', string,(byte*)&excludes, 0, "exclude"},
	{ 'l', string,(byte*)&logs,0,"log"},
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
	"  -h, --help                            Display this message and exit.\n",
	"  -i, --keep-going-unit                 Ignore failures from commands.\n",
	"                                        The possible failures are :\n",
	"                                         - the exit code differ from the expected\n",
	"                                         - the signal throw differ from the expected\n",
	"                                         - the output differ from the expected\n",
	"                                         - the read pipe is broken\n",
	"                                         - the write pipe is broken\n",
	"                                         - the command assigned delay is outdated\n",
	"  -I DIRECTORY, --include-dir=DIRECTORY Search DIRECTORY for included files.\n",
	"  -j [N], --jobs[=N]                    Allow N commands at once; infinite commands with\n"
	"                                        no arg.\n",
	"  -k, --keep-going                      Keep going when some commands can't be made or\n",
	"                                        failed.\n",
	"  -c, --just-display                    Don't actually run any commands; just display them.\n",
	"  -p, --display-data-base               Display tesh's internal database.\n",
	"  -q, --question                        Run no commands; exit status says if up to date.\n",
	"  -s, --silent,                         Don't echo commands.\n",
	"  -V, --version                         Display the version number of tesh and exit.\n",
	"  -d, --dont-display-directory          Don't display the current directory.\n",
	"  -n, --dry-run                         Check the syntax of the specified tesh files, display the result and exit.\n",
	"  -t, --timeout                         Wait the end of the commands at most timeout seconds.\n",
	"  -S, --check-syntax                    Check the syntax of the tesh files before run them. \n",
	"  -x, --suffix                          Consider the new suffix for the tesh files.\n"
	"                                           remark :\n",
	"                                           the default suffix for the tesh files is \".tesh\".\n",
	" -a, --semantic                         Display the tesh file metacommands syntax and exit.\n",
	" -b, --build-file                       Build a tesh file.\n",
	" -r, --load-directory                   Run all the tesh files located in the directories specified by the option --directory.\n",
	" -v, --verbose                          Display the status of the commands.\n",
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
display_usage(void);

static void
display_version(void);

static void
finalize(void);

static void
display_semantic(void);

static int
init(void);



int
main(int argc, char* argv[])
{
	if(init() < 0)
		finalize();
		
	/* process the command line */
	if(process_command_line(argc, argv) < 0)
		finalize();
	
	/* move to the root directory (the directory may change during the command line processing) */
	chdir(root_directory->name);
		
	/* initialize the xbt library 
	 * for thread portability layer
	 */
	 
	/* xbt initialization */
	if(!lstrings_is_empty(logs))
	{
		int size = lstrings_get_size(logs);
		char** cstr = lstrings_to_cstr(logs);
		
		xbt_init(&size, cstr);
		
		free(cstr);
		
	}
	else
		xbt_init(&argc, argv);
	
	/* the user wants to display the usage of tesh */
	if(want_display_usage)
		finalize();
	
	/* the user wants to display the version of tesh */
	if(want_display_version)
	{
		display_version();
		finalize();
	}
	
	/* the user wants to display the semantic of the tesh file metacommands */
	if(want_display_semantic)
	{
		display_semantic();
		finalize();
	}
	
	/* load tesh files */
	load();
	
	
	/* use by the finalize function to known if it must display the tesh usage */	
	loaded = 1;

	if(-2 == number_of_jobs)
	{/* --jobs is not specified (use the default value) */
		number_of_jobs = default_number_of_jobs;
	}
	else if(optional_number_of_jobs == number_of_jobs)
	{/* --jobs option is specified with no args (use one job per unit) */
		number_of_jobs = fstreams_get_size(fstreams);
	}

	if(number_of_jobs > fstreams_get_size(fstreams))
	{/* --jobs = N is specified and N is more than the number of tesh files */
		
		WARN0("Number of requested jobs exceed the number of files to run");
		
		/* assume one job per file */
		number_of_jobs = fstreams_get_size(fstreams);
	}

	/* initialize the semaphore used to synchronize all the units */
	jobs_sem = xbt_os_sem_init(number_of_jobs);

	/* initialize the semaphore used by the runner to wait for the end of all units */
	units_sem = xbt_os_sem_init(0);
	
	/* initialize the runner */
	if(runner_init(want_check_syntax, timeout, fstreams))
		finalize();
		
	if(want_just_display && want_silent)
		want_silent = 0;
		
	if(want_just_display && want_dry_run)
		WARN0("mismatch in the syntax : --just-check-syntax and --just-display options at same time");
	
	/* run all the units */
	runner_run();
	
	
	/* show the result of the units */
	if(want_verbose || want_dry_run)
		runner_display_status();
		
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
	
	
	
	#ifdef WIN32
	/* Windows specific : don't display the general-protection-fault message box and
	 * the the critical-error-handler message box (instead the system send the error
	 * to the calling process : tesh)
	 */
	prev_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	#endif
	
	/* used to store the files to run */
	if(!(fstreams = fstreams_new(DEFAULT_FSTREAMS_CAPACITY, fstream_free)))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	/* register the current directory */
	if(!(buffer  = getcwd(NULL, 0)))
	{
		exit_code = errno;
		
		if(EACCES == errno)
			ERROR0("tesh initialization failed - Insufficient permission to read the current directory");
		else
			ERROR0("Insufficient memory is available to initialize tesh : system error");	
		
		return -1;
	}
	
	/* save the root directory */
	if(!(root_directory = directory_new(buffer)))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	free(buffer);
	
	/* the directories to loads */
	if(!(directories = directories_new()))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	/* the include directories */
	if(!(include_dirs = vector_new(DEFAULT_INCLUDE_DIRS_CAPACITY, directory_free)))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	/* xbt logs option */
	if(!(logs = lstrings_new()))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	/* the excluded files */
	if(!(excludes = excludes_new()))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	/* the suffixes */
	if(!(suffixes = lstrings_new()))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	/* register the default suffix ".tesh" */
	if(lstrings_push_back(suffixes,".tesh"))
	{
		ERROR0("Insufficient memory is available to initialize tesh : system error");
		return -1;
	}
	
	pid = getpid();
	
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
	/* if there is not an error and the user wants display the usage or
	 * if there is an error and all the files to load are loaded, display the usage
	 */
	if((!exit_code && want_display_usage) || (!exit_code && !loaded))
		display_usage();
	
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
		vector_free(&include_dirs);
	
	/* delete the list of tesh files suffixes */	
	if(suffixes)
		lstrings_free(&suffixes);
	
	/* delete the xbt log options list */
	if(logs)
		lstrings_free(&logs);
		
	
	/* destroy the semaphore used to synchronize the units */
	if(jobs_sem)
		xbt_os_sem_destroy(jobs_sem);
	
	/* destroy the semaphore used by the runner used to wait for the end of the units */
	if(units_sem)
		xbt_os_sem_destroy(units_sem);
	
	/* exit from the xbt framework */
	xbt_exit();
	
	/* Windows specific (restore the previouse error mode */
	#ifdef WIN32
	SetErrorMode(prev_error_mode);
	#endif
	
	if(!want_verbose && !want_dry_run && !want_silent && !want_just_display)
		INFO2("tesh terminated with exit code %d : %s",exit_code, (!exit_code ? "success" : error_to_string(exit_code)));
	
	/* exit with the last error code */
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
				
				if(ENOENT == errno)
					ERROR1("File %s does not exist", optarg);
				else
					ERROR0("Insufficient memory is available to parse the command line : system error");
					
				return -1;
			}
			
			/* get to the last / (if any) to get the short name of the file */
			delimiter = strrchr(optarg,'/');
			
			/* create a new file stream which represents the tesh file to run */
			fstream = fstream_new(path, delimiter ? delimiter + 1 : optarg);
			
			free(path);
			
			/* if the list of all tesh files to run already contains this file
			 * destroy it and display a warning, otherwise add it in the list.
			 */
			if(fstreams_contains(fstreams, fstream))
			{
				fstream_free((void**)&fstream);
				WARN1("File %s already specified to be run", optarg);
			}
			else
				fstreams_add(fstreams, fstream);
				
			
				
			
		}
		else if (c == '?')
		{
			/* unknown option, let getopt_long() displays the error */
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
							return -1;
						}
						
						/* --load-directory option */
						if(!strcmp(entry->long_name,"load-directory"))
						{
							char* path;
							
							if(translatepath(optarg, &path) < 0)
							{
								exit_code = errno;
								
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
								
								if(ENOTDIR == errno)
									ERROR1("%s is not a directory",optarg);
								else
									ERROR0("Insufficient memory is available to process the command line - system error");
									
								return -1;
							}
							else
							{
								if(!dont_want_display_directory)
									INFO1("Entering directory \"%s\"",path);
								
								chdir(path);
								free(path);
								
								
							}	
						}
						
						/* --suffix option */
						else if(!strcmp(entry->long_name,"suffix"))
						{
							if(strlen(optarg) > MAX_SUFFIX)
							{
								ERROR1("Suffix %s too long",optarg);
								exit_code = ESUFFIXTOOLONG;	
								return -1;
							}
							
							if(optarg[0] == '.')
							{
								char suffix[MAX_SUFFIX + 2] = {0};
								sprintf(suffix,".%s",optarg);
								
								if(lstrings_contains(suffixes, suffix))
									WARN1("Suffix %s already specified to be used", optarg);
								else
									lstrings_push_back(suffixes, suffix);
							}
							else
							{
								if(lstrings_contains(suffixes, optarg))
									WARN1("Suffix %s already specified to be used", optarg);
								else
									lstrings_push_back(suffixes, optarg);	
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
								
								if(ENOENT == errno)
									ERROR1("File %s does not exist", optarg);
								else
									ERROR0("Insufficient memory is available to process the command line - system error");
								
								return -1;
							}
							
							delimiter = strrchr(optarg,'/');
							
							fstream = fstream_new(path, delimiter ? delimiter + 1 : optarg);
							
							free(path);
							
							if(fstreams_contains(fstreams, fstream))
							{
								fstream_free((void**)&fstream);
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
							
								if(vector_contains(include_dirs, directory))
								{
									directory_free((void**)&directory);
									WARN1("Include directory %s already specified to be used",optarg);
									
								}
								else
									vector_push_back(include_dirs, directory);
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
								
								if(ENOENT == errno)
									ERROR1("file %s does not exist", optarg);
								else
									ERROR0("Insufficient memory is available to process the command line - system error");
									
								return -1;
							}
			
							delimiter = strrchr(optarg,'/');
			
							fstream = fstream_new(path, delimiter ? delimiter + 1 : optarg);
							free(path);
			
							if(excludes_contains(excludes, fstream))
							{
								fstream_free((void**)&fstream);
								WARN1("File %s already specified to be exclude", optarg);
							}
							else
								excludes_add(excludes, fstream);
									
						}
						/* --log option */
						else if(!strcmp(entry->long_name,"log"))
						{
							lstrings_push_back(logs, optarg);
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
display_usage(void)
{
	const char **cpp;
	FILE* stream;
	
	if (want_display_version)
		display_version();
	
	stream = exit_code ? stderr : stdout;
	
	fprintf (stream, "Usage: tesh [options] [file] ...\n");
	
	for (cpp = usage; *cpp; ++cpp)
		fputs (*cpp, stream);
	
	fprintf(stream, "\nReport bugs to <martin.quinson@loria.fr | malek.cherier@loria.fr>\n");
}

static void
display_version(void)
{
	/* TODO : display the version of tesh */
	printf("Version :\n");
	printf("  tesh version %s : Mini shell specialized in running test units by Martin Quinson \n", version);
	printf("  and Malek Cherier\n");
	printf("  Copyright (c) 2007, 2008 Martin Quinson, Malek Cherier\n");
	printf("  All rights reserved\n");
	printf("  This program is free software; you can redistribute it and/or modify it\n");
	printf("  under the terms of the license (GNU LGPL) which comes with this package.\n\n");
	
	if(!want_display_usage)
		printf("Report bugs to <martin.quinson@loria.fr | malek.cherier@loria.fr>");
}

static void
display_semantic(void)
{
	size_t len;
	char * line = NULL;
	
	FILE* stream = fopen("README.txt", "r");
	
	if(!stream)
	{
		ERROR0("Unable to locate the README.txt file");
		exit_code = EREADMENOTFOUND;
		return;
	}
	
	while(getline(&line, &len, stream) != -1)
		printf("%s",line);
		
	fclose(stream);
}


