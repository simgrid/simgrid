#include <runner.h>
#include <lstrings.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(tesh,"TEst SHell utility");

#ifdef WIN32
/* Windows specific : the previous process error mode			*/
static UINT 
prev_error_mode = 0;
#endif

int
exit_code = 0;

/* the current version of tesh									*/
static const char* 
version = "1.0";

/* the root directory											*/
static char* 
root_directory = NULL;

/* ------------------------------------------------------------ */
/* options														*/
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/* numbers														*/
/* ------------------------------------------------------------ */

/* the number of tesh files to run								*/
static int
number_of_files = 0;

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
timeout = INDEFINITE_TIMEOUT;

/* ------------------------------------------------------------ */
/* strings dlists												*/
/* ------------------------------------------------------------ */

/* --C change the directory before running the units			*/
static strings_t 
directories = NULL;

/* the include directories : see the !i metacommand				*/
strings_t 
include_directories = NULL;

/* the ddlist of tesh files to run								*/
static lstrings_t 
files = NULL;

/* xbt log														*/
static strings_t
log = NULL;

static strings_t
ignored_files = NULL;

/* the ddlist of tesh file suffixes								*/
static strings_t
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
want_run_current_directory = 0;

/* if 1, the status of all the units is display at
 * the end.
 */
static int
want_verbose = 0;

/* if 1, the directories are displayed							*/
static int 
dont_want_display_directory = 0;

/* if 1, just check the syntax of all the tesh files :
 * do not run them.
 */
int
want_dry_run = 0;

/* if 1, display the tesh files syntax and exit					*/
static int
want_display_semantic = 0;

/* vector of the stream of the tesh files to process			*/
static FILE**
streams = NULL;

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
	{ 'f', string, (byte*)&files, 0, "file" },
	{ 'h', flag, (byte*)&want_display_usage, 0, "help" },
	{ 'a', flag, (byte*)&want_display_semantic, 0, "semantic" },
	{ 'i', flag, (byte*)&want_keep_going_unit, 0, "keep-going-unit" },
	{ 'I', string, (byte*)&include_directories, 0, "include-dir" },
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
	{ 'r', flag, (byte*)&want_run_current_directory, 0, "run-current-directory"},
	{ 'v', flag, (byte*)&want_verbose, 0, "verbose"},
	{ 'F', string,(byte*)&ignored_files, 0, "ignore-file"},
	{ 'l', string,(byte*)&log,0,"log"},
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
	" -r, --run-current-directory            Run all the tesh files located in the current directory.\n",
	" -v, --verbose                          Display the status of the commands.\n",
	" -F file , --ignore-file=FILE           Ignore the tesh file FILE.\n",
	" -l format, --log                       Format of the xbt log.\n",
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
prepare(void);

static void
display_usage(int exit_code);

static void
display_version(void);

static void
finalize(void);

int
is_valid_file(const char* file_name);

static void
display_semantic(void);

int
main(int argc, char* argv[])
{
	#ifdef WIN32
	/* Windows specific : don't display the general-protection-fault message box and
	 * the the critical-error-handler message box (instead the system send the error
	 * to the calling process : tesh)
	 */
	prev_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	#endif
	
	files = lstrings();

	/* process the command line */
	if(0 != (exit_code = process_command_line(argc, argv)))
		finalize();
		
	/* initialize the xbt library 
	 * for thread portability layer
	 */
	 
	if(log)
		xbt_init(&(log->number), log->items);
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

	/* prepare tesh */
	if(0 != (exit_code = prepare()))
		finalize();
		
	prepared = 1;

	if(-2 == number_of_jobs)
	{/* --jobs is not specified (use the default value) */
		number_of_jobs = default_number_of_jobs;
	}
	else if(optional_number_of_jobs == number_of_jobs)
	{/* --jobs option is specified with no args (use one job per unit) */
		number_of_jobs = lstrings_get_size(files);
	}

	if(number_of_jobs > lstrings_get_size(files))
	{/* --jobs = N is specified and N is more than the number of tesh files */
		
		WARN0("number of requested jobs exceed the number of files");
		
		/* assume one job per file */
		number_of_jobs = lstrings_get_size(files);
	}

	/* initialize the semaphore used to synchronize the jobs */
	jobs_sem = xbt_os_sem_init(number_of_jobs);

	/* initialize the semaphore used by the runner to wait for the end of all units */
	units_sem = xbt_os_sem_init(0);
	
	
	/* initialize the runner */
	if((0 != (exit_code = runner_init(
									want_check_syntax, 
									timeout, 
									number_of_files,
									files,
									streams))))
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
prepare(void)
{
	struct dirent* entry ={0};
	DIR* dir;
	
	/* get the current directory and save it */
	if(NULL == (root_directory = getcwd(NULL, 0)))
	{
		ERROR1("system error - %d : getcwd() function failed",errno);
		return E_GETCWD_FAILED;
	}
	else if(!dont_want_display_directory)
		INFO1("entering directory \"%s\"",root_directory);

	/* first, if the option --directory is specified change
	 * the directory
	 */
	if(directories)
	{
		unsigned int i;

		/* assume that the current directory and the new directory are not different */
		int diff = 0;
			
		/* if length the current directory and the directory specified in the command line 
		 * are not equal the to directories are different.
		 */
		if(strlen(root_directory) != strlen(directories->items[0]))
			diff = 1;
		
		/* if the two directories have the same length 
		 * compare them
		 */
		if(!diff)
		{
			for(i = 0; i < strlen(root_directory); i++)
			{
				if(toupper(root_directory[i]) != toupper((directories->items[0])[i]))
				{
					diff = 1;
					break;
				}
			}
		}
		
		/* the directory specified in the command line is the current directory
		 * if the want_warn_on_mismatch_syntax flag is set to 1, warn the user and
		 * do nothing else
		 */
		if(!diff)
		{
			WARN1("already in the directory %s",root_directory);
		}
		else
		{
			/* the directory specified in the command line is not the current directory
			 * change the current directory
			 */
			if(-1 == chdir(directories->items[0]))
			{
				if(ENOENT == errno)
				{	
					ERROR1("the directory %s does not exist", directories->items[0]);
					return E_DIR_DOES_NOT_EXIST;
				}
				else
				{
					ERROR1("system error - errno : %d : chdir() failed", errno);
					return E_CHDIR_FAILED;
				}
			}
			
			/* if the want_display_directory flag is set to 1, display the change */
			if(!dont_want_display_directory)
				INFO1("entering directory \"%s\"",directories->items[0]);
		}
	}

	/* if the --run-current-directory flag is specified laod all the tesh file contained in the current directory
	 * the tesh files having the default suffix ".tesh"
	 * the tesh files having a suffix specified by an option --suffix if any
	 *  remark if the tesh file of the current directory is specified in the command line, it is not laoded a second
	 * time and if the flag --warn-mismatch-syntax is specified a warning is displayed.
	 */
	if(want_run_current_directory)
	{

		/* try to find the default tesh file teshfile or the file having the default suffix ".tesh" or a suffix
		 * specified in the command line with the option -x in the current directory 
		 */
		dir = opendir(getcwd(NULL, 0));
	
		

		while(NULL != (entry = readdir(dir)))
		{
			/* there is a teshfile in the current directory add it in the ddlist of tesh files to process */
			if(is_valid_file(entry->d_name))
				lstrings_push_back(files, entry->d_name);
		}

		closedir(dir);
	}
	
	/* if no tesh files are specified on the command line and if the option --run-current-directory
	 * is specified and that the current directory does not contain any file, try to load the default
	 * tesh file named teshfile.
	 */
	if(lstrings_is_empty(files))
	{
		FILE* stream = NULL;

		stream = fopen("teshfile","r");

		if(stream)
		{
			streams = xbt_new0(FILE*, 1);
			streams[0] = stream;
			
			lstrings_push_back(files, "teshfile");
		}
		else
		{
			/* no tesh file and can't locate the default tesh file teshfile */
			
			/*ERROR0("no tesh file specified and no teshfile found");
			return E_NO_TESH_FILE;*/
			
			/* use stdin */
			
			streams = xbt_new0(FILE*, 1);

			lstrings_push_back(files, "stdin");
		}
	}
	else	
	{
		/* the user has specified some tesh files or the option --run-current-directory is specified */
		int i;

		streams = xbt_new0(FILE*, files->number);

		for(i = 0; i < files->number; i++)
		{
			streams[i] = fopen(files->items[i], "r");
		
			/* cannot open the tesh files, display the error and exit */
			if(!streams[i])
			{
				perror(bprintf("tesh file `%s' not found",files->items[i]));
				ERROR1("tesh file `%s': not found",files->items[i]);
				return E_TESH_FILE_NOT_FOUND;
			}
		}
	}

	number_of_files = lstrings_get_size(files);

	/* deal with the ignored tesh files now */
	if(ignored_files)
	{
		int i, j, ignored, exists;
		
		for(j = 0; j < ignored_files->number; j++)
		{
			exists = 0;
			
			for(i = 0; i < files->number; i++)
			{
				if(!strcmp(files->items[i], ignored_files->items[j]))
				{
					exists = 1;
					break;
				}
			}
			
			if(!exists)
				WARN1("the file %s cannot be ignored",ignored_files->items[j]);
		}

		for(i = 0; i < files->number; i++)
		{
			ignored = 0;

			for(j = 0; j < ignored_files->number; j++)
			{
				if(!strcmp(files->items[i],ignored_files->items[j]))
				{
					ignored = 1;
					break;
				}
			}
		
			if(ignored)
			{
				number_of_files--;
				fclose(streams[i]);
				streams[i] = NULL;
			}
			
		}
		
		if(!number_of_files)
		{
			/*ERROR0("no tesh file to run");
			return E_NO_TESH_FILE;*/
			
			/* use stdin */
			
			streams = xbt_new0(FILE*, 1);

			files = (strings_t)malloc (sizeof (s_strings_t));
			files->capacity = 1;
			files->items = (char **) malloc (1 * sizeof (char *));

			streams[0] = stdin;
			files->items[0] = strdup("stdin");
			files->number = 1;
		}
	}
	
	chdir(root_directory);

	return 0;
}

int
is_valid_file(const char* file_name)
{
	int j;

	/* test if the file is specified in the command line */
	if(files)
	{
		for(j = 0; j < files->number; j++)
		{
			if(!strcmp(files->items[j], file_name))
			{
				WARN1("the file %s specified in the command line is in the current directory",file_name);
				return 0;
			}
		}
	}

	if(!strncmp(".tesh", file_name + (strlen(file_name) - 5), 5))
		return 1;

	if(suffixes)
	{
		int i;

		for(i = 0; i < suffixes->number; i++)
		{
			if(!strncmp(suffixes->items[i], file_name + (strlen(file_name) - strlen(suffixes->items[i])), strlen(suffixes->items[i])))
				return 1;
		}
	}

	return 0;
}

static void
unprepare(void)
{
	
	/* close all file streams and free the stream allocated table */
	if(streams)
	{	
		int i;

		for(i = 0; i < files->number; i++)
			if(NULL != streams[i])
				fclose(streams[i]);

		free(streams);
	}

	/* release the files allocated memory */
	if(files)
	{
		int i;

		for(i = 0; i < files->number; i++)
			free(files->items[i]);

		free(files->items);
		free(files);
	}

	if(ignored_files)
	{
		int i;

		for(i = 0; i < ignored_files->number; i++)
			free(ignored_files->items[i]);

		free(ignored_files->items);
		free(ignored_files);
	}

	/* free the root directory */
	if(root_directory)
		free(root_directory);

}

static void
finalize(void)
{
	if(((0 == exit_code) && want_display_usage) || ((0 != exit_code) && !prepared))
		display_usage(exit_code);

	/* close the openned tesh files */
	unprepare();
	
	/* clenup the directory string list */
	if(directories)
	{
		int i;

		for(i = 0; i < directories->number; i++)
			free(directories->items[i]);

		free(directories->items);
		free(directories);
	}
	
	/* clenup the include directory string list */
	if(include_directories)
	{
		int i;

		for(i = 0; i < include_directories->number; i++)
			free(include_directories->items[i]);

		free(include_directories->items);
		free(include_directories);
	}
	
	/* clenup the suffix string ddlist */
	if(suffixes)
	{
		int i;

		for(i = 0; i < suffixes->number; i++)
			free(suffixes->items[i]);

		free(suffixes->items);
		free(suffixes);
	}
	
	/* clenup the log string ddlist */
	if(log)
	{
		int i;

		for(i = 0; i < log->number; i++)
			free(log->items[i]);

		free(log->items);
		free(log);
	}
	
	/* destroy the semaphore used to synchronize the jobs */
	if(NULL != jobs_sem)
		xbt_os_sem_destroy(jobs_sem);

	/* destroy the semaphore used by the runner to wait for 
	 * the end of all the units 
	 */
	if(NULL != units_sem)
		xbt_os_sem_destroy(units_sem);
		
		
	
	/* exit from the xbt framework */
	xbt_exit();
	
	#ifdef WIN32
	SetErrorMode(prev_error_mode);
	#endif
	
	if(!want_verbose && !want_dry_run && !want_silent && !want_just_display)
		INFO1("tesh terminated with exit code %d",exit_code);
		
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
	register strings_t v;
	register int c;
	
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
						
			if(!files)
			{
				files = (strings_t)malloc (sizeof (s_strings_t));
				files->capacity = DEFAULT_CAPACITY;
				files->number = 0;
				files->items = (char **) malloc (DEFAULT_CAPACITY * sizeof (char *));
			}
			else if (files->number == files->capacity - 1)
			{
				files->capacity += DEFAULT_CAPACITY;
				files->items = (char **)realloc ((char *) files->items,files->capacity * sizeof (char *));
			}
			
			/* special case of suffix : add a point at the beginning */
			files->items[files->number++] = strdup(optarg);
			files->items[files->number] = NULL;
			
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
						ERROR0("\ninternal error - process_command_line() function failed");
						return E_PROCESS_COMMAND_LINE_FAILED;
		
						case flag:
						
						*(int *) entry->value = 1;
						
						break;
		
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
							return E_ARG_NOT_SPECIFIED;
						}
		
						v = *(strings_t*) entry->value;
						
						if(!v)
						{
							v = (strings_t)malloc (sizeof (s_strings_t));
							v->capacity = DEFAULT_CAPACITY;
							v->number = 0;
							v->items = (char **) malloc (DEFAULT_CAPACITY * sizeof (char *));
							*(struct s_strings **) entry->value = v;
						}
						else if (v->number == v->capacity - 1)
						{
							v->capacity += DEFAULT_CAPACITY;
							v->items = (char **)realloc ((char *) v->items,v->capacity * sizeof (char *));
						}
						
						/* special case of suffix : add a point at the beginning */
						if(c == 'x' && optarg[0])
						{
							char buffer[MAX_PATH + 1] = {0};
							sprintf(buffer,".%s",optarg);
							v->items[v->number++] = strdup(buffer);
						}
						else
							v->items[v->number++] = strdup(optarg);
						
						v->items[v->number] = NULL;

						break;
		
						case number:
						
						if ((NULL == optarg) && (argc > optind))
						{
							const char* cp;
							
							for (cp = argv[optind]; isdigit(cp[0]); ++cp)
								if(cp[0] == '\0')
									optarg = argv[optind++];
						}
		
						/* the number option is specified and has no arg */
						if(NULL != optarg)
						{
							int i = atoi(optarg);
							const char *cp;

							for (cp = optarg; isdigit(cp[0]); ++cp);
		
							if (i < 1 || cp[0] != '\0')
							{
								ERROR2("\nthe option %c \"%s\" requires an strictly positive integer as argument",entry->c, entry->long_name);
								return E_NOT_POSITIVE_NUMBER;
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
		exit_code = E_README_NOT_FOUND;
		return;
	}
	
	while(getline(&line, &len, stream) != -1)
		printf("%s",line);
		
	fclose(stream);
}


