#include <stdio.h>
#include <string.h>

#include <getopt.h>

char* 
optarg = NULL;

int 
optind = 0;

int 
optopt = '?';

int 
opterr = 1;

static char*
nextchar;

static 
int first_nonopt;

static int 
last_nonopt;


static enum
{
	REQUIRE_ORDER, 
	PERMUTE, 
	RETURN_IN_ORDER
}ordering;


static const char *
__getopt_initialize (const char *optstring);

static int
__getopt_internal (int argc, char *const *argv, const char* optstring, const struct option *longopts, int* longind, int long_only);

static void
__exchange (char **argv);

int 
getopt (int argc, char * const argv[], const char *optstring)
{
	return __getopt_internal(argc, argv, optstring,(const struct option *) 0,(int *) 0,0);
}

static const char *
__getopt_initialize (const char *optstring)
{
	first_nonopt = last_nonopt = optind = 1;
	nextchar = NULL;
	
	if (optstring[0] == '-')
	{
		ordering = RETURN_IN_ORDER;
		++optstring;
	}
	
	/* if the optstring begining with the character +, the getopt() function
	 * stop when an argument of the command line is not an option.
	 */
	else if (optstring[0] == '+')
	{
		ordering = REQUIRE_ORDER;
		++optstring;
	}
	else
	{
		ordering = PERMUTE;
	}
	
	return optstring;
}

int
__getopt_internal (int argc, char *const *argv, const char* optstring, const struct option *longopts, int* longind, int long_only)
{
	optarg = NULL;
	
	if (optind == 0)
		optstring = __getopt_initialize (optstring);
	
	if (nextchar == NULL || *nextchar == '\0')
	{
		if (ordering == PERMUTE)
		{
			if (first_nonopt != last_nonopt && last_nonopt != optind)
				__exchange ((char **) argv);
			else if (last_nonopt != optind)
				first_nonopt = optind;
	
			
	
			while (optind < argc && (argv[optind][0] != '-' || argv[optind][1] == '\0'))
				optind++;
			
			last_nonopt = optind;
		}

	
		if (optind != argc && !strcmp (argv[optind], "--"))
		{
			optind++;
	
			if (first_nonopt != last_nonopt && last_nonopt != optind)
				__exchange ((char **) argv);
			else if (first_nonopt == last_nonopt)
				first_nonopt = optind;
			
			last_nonopt = argc;
	
			optind = argc;
		}
	
		if (optind == argc)
		{
			if (first_nonopt != last_nonopt)
				optind = first_nonopt;
			
			return EOF;
		}
	
		if ((argv[optind][0] != '-' || argv[optind][1] == '\0'))
		{
			if (ordering == REQUIRE_ORDER)
				return EOF;
			optarg = argv[optind++];
				return 1;
		}
	
		nextchar = (argv[optind] + 1 + (longopts != NULL && argv[optind][1] == '-'));
	}
	
	if (longopts != NULL && (argv[optind][1] == '-' || (long_only && (argv[optind][2] || !strchr (optstring, argv[optind][1])))))
	{
		char *nameend;
		const struct option *p;
		const struct option *pfound = NULL;
		int exact = 0;
		int ambig = 0;
		int indfound = 0;
		int option_index;
	
		for (nameend = nextchar; *nameend !='\0' && *nameend != '='; nameend++)
			
		for (p = longopts, option_index = 0; p->name; p++, option_index++)
		{
			if(!strncmp (p->name, nextchar, nameend - nextchar))
			{

				if ((nameend - nextchar) == strlen (p->name))
				{
					pfound = p;
					indfound = option_index;
					exact = 1;
					break;
				}
				else if (pfound == NULL)
				{
					exact = 0;
					break;
				}
				else
					ambig = 1;
				
			}
		}
	
		if (ambig && !exact)
		{
			if (opterr)
				fprintf (stderr, "ERROR   : %s: option `%s' is ambiguous\n",argv[0], argv[optind]);
			
			nextchar += strlen (nextchar);
			optind++;
			return '?';
		}
	
		if (pfound != NULL)
		{
			option_index = indfound;
			optind++;
			
			if (*nameend)
			{
				if (pfound->has_arg)
					optarg = nameend + 1;
				else
				{
					if (opterr)
					{
						if (argv[optind - 1][1] == '-')
							/* --option */
							fprintf (stderr,"error   : %s: option `--%s' doesn't allow an argument\n",argv[0], pfound->name);
						else
							/* +option or -option */
							fprintf (stderr,"error   : %s: option `%c%s' doesn't allow an argument\n",argv[0], argv[optind - 1][0], pfound->name);
					}
					
					nextchar += strlen (nextchar);
					return '?';
				}
			}
			else if (pfound->has_arg == 1)
			{
				if (optind < argc)
					optarg = argv[optind++];
				else
				{
					if (opterr)
						fprintf (stderr, "error   : %s: option `%s' requires an argument\n",argv[0], argv[optind - 1]);
					
					nextchar += strlen (nextchar);
					return optstring[0] == ':' ? ':' : '?';
				}
			}
			
			nextchar += strlen (nextchar);
			
			if (longind != NULL)
				*longind = option_index;
			
			if (pfound->flag)
			{
				*(pfound->flag) = pfound->val;
				return 0;
			}
			
			return pfound->val;
		}
	
		if (!long_only || argv[optind][1] == '-'|| strchr (optstring, *nextchar) == NULL)
		{
			if (opterr)
			{
				if (argv[optind][1] == '-')
					/* --option */
					fprintf (stderr, "error   : %s: unrecognized option `--%s'\n",argv[0], nextchar);
				else
					/* +option or -option */
					fprintf (stderr, "error   : %s: unrecognized option `%c%s'\n",argv[0], argv[optind][0], nextchar);
			}
			
			nextchar = (char *) "";
			optind++;
			return '?';
		}
	}
	
	{
		char c = *nextchar++;
		char *temp = strchr (optstring, c);
	
		/* Increment `optind' when we start to process its last character.  */
		if (*nextchar == '\0')
			++optind;
	
		if (temp == NULL || c == ':')
		{
			if (opterr)
					fprintf (stderr, "error   : %s: invalid option -- %c\n", argv[0], c);
			
			optopt = c;
			return '?';
		}
		
		if (temp[1] == ':')
		{
			if (temp[2] == ':')
			{
				/* it's an option that accepts an argument optionally.  */
				if (*nextchar != '\0')
				{
					optarg = nextchar;
					optind++;
				}
				else
					optarg = NULL;
				
				nextchar = NULL;
			}
			else
			{
				/* it's an option that requires an argument.  */
				if (*nextchar != '\0')
				{
					optarg = nextchar;
					optind++;
				}
				else if (optind == argc)
				{
					if (opterr)
					{
						/* 1003.2 specifies the format of this message.  */
						fprintf (stderr, "ERROR   : %s: option requires an argument -- %c\n",argv[0], c);
					}
					optopt = c;
					
					if (optstring[0] == ':')
						c = ':';
					else
						c = '?';
				}
				else
					optarg = argv[optind++];
				
				nextchar = NULL;
			}
		}
	return c;
	
	}
}


static void
__exchange (char **argv)
{
	int bottom = first_nonopt;
	int middle = last_nonopt;
	int top = optind;
	char *tem;
	
	while (top > middle && middle > bottom)
	{
		if (top - middle > middle - bottom)
		{
			int len = middle - bottom;
			register int i;
	
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[top - (middle - bottom) + i];
				argv[top - (middle - bottom) + i] = tem;
			}
			
			top -= len;
		}
		else
		{
			int len = top - middle;
			register int i;
		
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[middle + i];
				argv[middle + i] = tem;
			}

			bottom += len;
		}
	}
	
	
	first_nonopt += (optind - last_nonopt);
	last_nonopt = optind;
}

int
getopt_long (int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index)
{
	return __getopt_internal (argc, argv, options, long_options, opt_index, 0);
}


int
getopt_long_only(int argc, char *const *argv, const char *options, const struct option *long_options,int *opt_index)
{
	return __getopt_internal (argc, argv, options, long_options, opt_index, 1);
}


