
#include <runner.h>
#include <fstream.h>
#include <fstreams.h>
#include <directory.h>
#include <directories.h>
#include <excludes.h>
#include <error.h>

#include <getopt.h>

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

int
exit_code = 0;

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
includes = NULL;

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

/* if 1, all the tesh file of the current directory
 * are runned
 */ 
static int
want_load_directory = 0;

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
prepared = 0;


int 
interrupted = 0; 

/* the table of the entries of the options */ 
static const struct s_optentry opt_entries[] =
{
	{ 'C', string, (byte*)&directories, 0, "directory" },
	{ 'x', string, (byte*)&suffixes, 0, "suffix" },
	{ 'e', flag, (byte*)&env_overrides, 0, "environment-overrides", },
	{ 'f', string, (byte*)&fstreams, 0, "file" },
	{ 'h', flag, (byte*)&want_display_usage, 0, "help" },
	{ 'a', flag, (byte*)&want_display_semantic, 0, "semantic" },
	{ 'i', flag, (byte*)&want_keep_going_unit, 0, "keep-going-unit" },
	{ 'I', string, (byte*)&includes, 0, "include-dir" },
	{ 'j', number, (byte*)&number_of_jobs, (byte*) &optional_number_of_jobs, "jobs" },
	{ 'k', flag, (byte*)&want_keep_going, 0, "keep-going" },
	{ 'c', flag, (byte*)&want_just_display, 0, "just-display" },
	{ 'd', flag, (byte*)&display_data_base, 0,"display-data-base" },
	{ 'q', flag, (byte*)&question, 0, "question" },
	{ 's', flag, (byte*)&want_silent, 0, "silent" },
	{ 'V', flag, (byte*)&want_display_version, 0, "version" },
	{ 'w', flag, (byte*)&dont_want_display_directory, 0,"dont-display-directory" },
	{ 'n', flag, (byte*)&want_dry_run, 0, "dry-run"},
	{ 't', number, (byte*)&timeout, 0, "timeout" },
	{ 'S', flag, (byte*)&want_check_syntax, 0, "check-syntax"},
	{ 'r', flag, (byte*)&want_load_directory, 0, "load-directory"},
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

static int
load(void);

static void
display_usage(int exit_code);

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
	init();
	
	/* process the command line */
	if((exit_code = process_command_line(argc, argv)))
		finalize();
		
	/* initialize the xbt library 
	 * for thread portability layer
	 */
	 
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
	
	if(!directories_has_directories_to_load(directories) && want_load_directory)
		WARN0("--load-directory specified but no directory specified");
	
	excludes_check(excludes, fstreams);
	
	/* load tesh */
	if((exit_code = load()))
		finalize();
		
	prepared = 1;

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
		
		WARN0("number of requested jobs exceed the number of files");
		
		/* assume one job per file */
		number_of_jobs = fstreams_get_size(fstreams);
	}

	/* initialize the semaphore used to synchronize the jobs */
	jobs_sem = xbt_os_sem_init(number_of_jobs);

	/* initialize the semaphore used by the runner to wait for the end of all units */
	units_sem = xbt_os_sem_init(0);
	
	/* initialize the runner */
	if((0 != (exit_code = runner_init(
									want_check_syntax, 
									timeout, 
									fstreams))))
	{
		finalize();
	}
	
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
	
	/* then, finalize tesh */
	finalize();
	
	#ifndef WIN32
	return exit_code;
	#endif
	
}

static int
init(void)
{
	char* buffer = getcwd(NULL, 0);
	
	#ifdef WIN32
	/* Windows specific : don't display the general-protection-fault message box and
	 * the the critical-error-handler message box (instead the system send the error
	 * to the calling process : tesh)
	 */
	prev_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	#endif
	
	/* used to store the file streams to run */
	fstreams = fstreams_new(DEFAULT_FSTREAMS_CAPACITY, fstream_free);
	
	root_directory = directory_new(buffer,want_load_directory);
	free(buffer);
	/* used to store the directories to loads */
	directories = directories_new(); 
	
	/* register the current directory */
	directories_add(directories, root_directory);
	
	/* used to store the includes directories */
	includes = vector_new(DEFAULT_INCLUDES_CAPACITY, directory_free);
	
	/* xbt logs */
	logs = lstrings_new();
	
	/* used to to store all the excluded file streams */
	excludes = excludes_new();
	
	/* list of file streams suffixes */
	suffixes = lstrings_new();
	
	lstrings_push_back(suffixes,".tesh");
	
	return 0;
}

static int
load(void)
{
	chdir(directory_get_name(root_directory));
	
	if(want_load_directory)
		directories_load(directories, fstreams, suffixes);
	
	/* on a aucun fichier specifie dans la ligne de commande
	 * l'option --run-current-directory n'a pas ete specifie ou aucun fichier ne se trouve dans le repertoire a charger
	 */
	if(fstreams_is_empty(fstreams))
	{
		struct stat buffer = {0};
		
		/* add the default tesh file if it exists */
		if(!stat("teshfile", &buffer) && S_ISREG(buffer.st_mode))
			fstreams_add(fstreams, fstream_new(getcwd(NULL, 0), "teshfile"));
	}
	
	if(!excludes_is_empty(excludes) && !fstreams_is_empty(fstreams))
		fstreams_exclude(fstreams, excludes);
	
	if(fstreams_is_empty(fstreams))
		fstreams_add(fstreams, fstream_new(NULL, "stdin"));	
	
	fstreams_load(fstreams);
	
	return 0;
}

static void
finalize(void)
{
	if((!exit_code && want_display_usage) || (!exit_code && !prepared))
		display_usage(exit_code);
	
	if(fstreams)
		fstreams_free((void**)&fstreams);
	
	if(excludes)	
		excludes_free((void**)&excludes);
	
	if(directories)
		directories_free((void**)&directories);
	
	if(includes)
		vector_free(&includes);
		
	if(suffixes)
		lstrings_free(&suffixes);
	
	if(logs)
		lstrings_free(&logs);
	
	/* destroy the semaphore used to synchronize the jobs */
	if(jobs_sem)
		xbt_os_sem_destroy(jobs_sem);

	if(units_sem)
		xbt_os_sem_destroy(units_sem);
	
	/* exit from the xbt framework */
	xbt_exit();
	
	#ifdef WIN32
	SetErrorMode(prev_error_mode);
	#endif
	
	if(!want_verbose && !want_dry_run && !want_silent && !want_just_display)
		INFO2("tesh terminated with exit code %d : %s",exit_code, (!exit_code ? "success" : error_to_string(exit_code)));
	
	exit(exit_code);
}

static void
init_options (void)
{
	char *p;
	unsigned int i;
	
	if(optstring[0] != '\0')
	/* déjà traité.  */
		return;
	
	p = optstring;
	
	/* Return switch and non-switch args in order, regardless of
	POSIXLY_CORRECT.  Non-switch args are returned as option 1.  */
	
	/* le premier caractère de la chaîne d'options vaut -.
	 * les arguments ne correspondant pas à une option sont 
	 * manipulés comme s'ils étaient des arguments d'une option
	 *  dont le caractère est le caractère de code 1
	 */
	*p++ = '-';
	
	for (i = 0; opt_entries[i].c != '\0'; ++i)
	{
		/* initialize le nom de l'option longue*/
		longopts[i].name = (opt_entries[i].long_name == 0 ? "" : opt_entries[i].long_name);
		
		/* getopt_long() retourne la valeur de val */
		longopts[i].flag = 0;
		
		/* la valeur de l'option courte est le caractère spécifié dans  opt_entries[i].c */
		longopts[i].val = opt_entries[i].c;
		
		/* on l'ajoute à la chaine des optstring */
		*p++ = opt_entries[i].c;
		
		switch (opt_entries[i].type)
		{
			/* si c'est une option qui sert a positionner un flag ou que l'on doit ignorée, elle n'a pas d'argument */
			case flag:
			longopts[i].has_arg = no_argument;
			break;
		
			/* c'est une option qui attent un argument : 
			 * une chaine de caractères, un nombre flottant, 
			 * ou un entier positif
			 */
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

static int
process_command_line(int argc, char** argv)
{
	register const struct s_optentry* entry;
	register int c;
	directory_t directory;
	fstream_t fstream;
	
	/* initialize the options table of tesh */
	init_options();
	
	/* display the errors of the function getopt_long() */
	opterr = 1;
	
	optind = 0;
	
	while (optind < argc)
	{
		c = getopt_long (argc, argv, optstring, longopts, (int *) 0);
		
		if(c == EOF)
		{
			/* end of the command line or "--".  */
			break;
		}
		else if (c == 1)
		{
			/* the argument of the command line is not an option (no "-"), assume it's a tesh file */
			/*struct stat buffer = {0};
			char* prev = getcwd(NULL, 0);
			
			directory = directories_get_back(directories);
			
			chdir(directory->name);
			
			if(stat(optarg, &buffer) || !S_ISREG(buffer.st_mode))
			{
				chdir(prev);
				free(prev);
				ERROR1("file %s not found", optarg);
				return EFILENOTFOUND;
			}
			
			chdir(prev);
			free(prev);*/
			
			directory = directories_search_fstream_directory(directories, optarg);
			
			if(!directory)
			{
				if(1 == directories_get_size(directories))
				{
					ERROR1("file %s not found in the current directory",optarg);
					return EFILENOTINCURDIR;
				}
				else
				{
					ERROR1("file %s not found in the specified directories",optarg);
					return EFILENOTINSPECDIR;
				}
			}
			
			if(!(fstream = fstream_new(directory_get_name(directory), optarg)))
			{
				ERROR1("command line processing failed with the error code %d", errno);
				return EPROCESSCMDLINE;
			}
			else
			{
				if(fstreams_contains(fstreams, fstream))
				{
					fstream_free((void**)&fstream);
					WARN1("file %s already specified", optarg);
				}
				else
				{
					if((errno = fstreams_add(fstreams, fstream)))
					{
						fstream_free((void**)&fstream);
						ERROR1("command line processing failed with the error code %d", errno);
						return EPROCESSCMDLINE;
					}
				}
			}					
		}
		else if (c == '?')
		{
			/* unknown option, getopt_long() displays the error */
			return 1;
		}
		else
		{
			for (entry = opt_entries; entry->c != '\0'; ++entry)
				
				if(c == entry->c)
				{
		
					switch (entry->type)
					{
						/* impossible */
						default:
						ERROR0("command line processing failed : internal error");
						return EPROCESSCMDLINE;
						
						
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
							ERROR2("the option %c \"%s\"requires an argument",entry->c,entry->long_name);
							return EARGNOTSPEC;
						}
						
						/* --directory option */
						if(!strcmp(entry->long_name,"directory"))
						{
							if(!(directory = directory_new(optarg, want_load_directory)))
							{
								if(ENOTDIR == errno)
								{
									ERROR1("directory %s not found",optarg);
									return EDIRNOTFOUND;
								}
								else
								{
									ERROR1("command line processing failed with the error code %d", errno);
									return EPROCESSCMDLINE;
								}
							}
							else
							{
								if(directories_contains(directories, directory))
								{
									directory_free((void**)&directory);
									WARN1("directory %s already specified",optarg);
								}
								else
								{
									if((errno = directories_add(directories, directory)))
									{
										directory_free((void**)&directory);
										ERROR1("command line processing failed with the error code %d", errno);
										return EPROCESSCMDLINE;
									}
								}
							}			
						}
						/* --suffix option */
						else if(!strcmp(entry->long_name,"suffix"))
						{
							if(strlen(optarg) > MAX_SUFFIX)
							{
								ERROR1("suffix %s too long",optarg);
								return ESUFFIXTOOLONG;	
							}
							
							if(optarg[0] == '.')
							{
								char suffix[MAX_SUFFIX + 2] = {0};
								sprintf(suffix,".%s",optarg);
								
								if(lstrings_contains(suffixes, suffix))
									WARN1("suffix %s already specified", optarg);
								else
									lstrings_push_back(suffixes, suffix);
							}
							else
							{
								if(lstrings_contains(suffixes, optarg))
									WARN1("suffix %s already specified", optarg);
								else
									lstrings_push_back(suffixes, optarg);	
							}
						}
						/* --file option */
						else if(!strcmp(entry->long_name,"file"))
						{
							
							/* the argument of the command line is not an option (no "-"), assume it's a tesh file */
							/*struct stat buffer = {0};
							char* prev = getcwd(NULL, 0);
							
							directory = directories_get_back(directories);
							
							chdir(directory->name);
			
							if(stat(optarg, &buffer) || !S_ISREG(buffer.st_mode))
							{
								chdir(prev);
								free(prev);
								ERROR1("file %s not found", optarg);
								return EFILENOTFOUND;
							}
							
							chdir(prev);
							free(prev);*/
							
							directory = directories_search_fstream_directory(directories, optarg);
			
							if(!directory)
							{
								if(1 == directories_get_size(directories))
								{
									ERROR1("file %s not found in the current directory",optarg);
									return EFILENOTINCURDIR;
								}
								else
								{
									ERROR1("file %s not found in the specified directories",optarg);
									return EFILENOTINSPECDIR;
								}
							}
							
							if(!(fstream = fstream_new(directory_get_name(directory),optarg)))
							{
								ERROR1("command line processing failed with the error code %d", errno);
								return EPROCESSCMDLINE;
							}
							else
							{
								if(fstreams_contains(fstreams, fstream))
								{
									fstream_free((void**)&fstream);
									WARN1("file %s already specified", optarg);
								}
								else
								{
									if((errno = fstreams_add(fstreams, fstream)))
									{
										fstream_free((void**)&fstream);
										ERROR1("command line processing failed with the error code %d", errno);
										return EPROCESSCMDLINE;
									}
								}
							}		
						}
						/* --include-dir option */
						else if(!strcmp(entry->long_name,"include-dir"))
						{
							if(!(directory = directory_new(optarg, want_load_directory)))
							{
								if(ENOTDIR == errno)
								{
									ERROR1("%s is not a directory",optarg);
									return EDIRNOTFOUND;
								}
								else
								{
									ERROR1("command line processing failed with the error code %d", errno);
									return EPROCESSCMDLINE;
								}
							}
							else
							{
								if(vector_contains(includes, directory))
								{
									directory_free((void**)&directory);
									WARN1("include directory %s already specified",optarg);
									
								}
								else
								{
									if((errno = vector_push_back(includes, directory)))
									{
										directory_free((void**)&directory);
										ERROR1("command line processing failed with the error code %d", errno);
										return EPROCESSCMDLINE;
									}
								}
							}
						}
						/* --exclude option */ 
						else if(!strcmp(entry->long_name,"exclude"))
						{
							directory = directories_get_back(directories);
							
							if(!(fstream = fstream_new(directory_get_name(directory), optarg)))
							{
								if(ENOENT == errno)
								{
									ERROR1("file to exclude %s not found", optarg);
									return EFILENOTFOUND;
								}
								else
								{
									ERROR1("command line processing failed with the error code %d", errno);
									return EPROCESSCMDLINE;
								}
							}
							else
							{
								if(excludes_contains(excludes, fstream))
								{
									fstream_free((void**)&fstream);
									WARN1("file to exclude %s already specified", optarg);
								}
								else
								{
									if((errno = excludes_add(excludes, fstream)))
									{
										fstream_free((void**)&fstream);
										ERROR1("command line processing failed with the error code %d", errno);
										return EPROCESSCMDLINE;
									}
								}
							}				
						}
						/* --log option */
						else if(!strcmp(entry->long_name,"log"))
						{
							lstrings_push_back(logs, optarg);
						}
						else
						{
							/* TODO */
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
								ERROR2("option %c \"%s\" requires an strictly positive integer as argument",entry->c, entry->long_name);
								return ENOTPOSITIVENUM;
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
display_usage(int exit_code)
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


