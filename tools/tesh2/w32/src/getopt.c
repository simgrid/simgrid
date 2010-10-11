#include <stdio.h>
#include <string.h>

#include <getopt.h>
char * optarg = NULL;
int  optind = 0;
int  optopt = '?';
int  opterr = 1;
static char * nextchar;
static  int first_nonopt;
static int  last_nonopt;
static enum  { REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER 
} ordering;
static const char * __getopt_initialize(const char *optstring);
static int 
__getopt_internal(int argc, char *const *argv, const char *optstring,
                  const struct option *longopts, int *longind,
                  int long_only);
static void  __exchange(char **argv);
int  getopt(int argc, char *const argv[], const char *optstring) 
{
  return __getopt_internal(argc, argv, optstring,
                            (const struct option *) 0, (int *) 0, 0);
} static const char * __getopt_initialize(const char *optstring) 
{
  
      /* Start processing options with ARGV-element 1 (since ARGV-element 0
         is the program name); the sequence of previously skipped
         non-option ARGV-elements is empty.  */ 
      first_nonopt = last_nonopt = optind = 1;
  nextchar = NULL;
  
      /* Determine how to handle the ordering of options and nonoptions.  */ 
      if (optstring[0] == '-')
     {
    ordering = RETURN_IN_ORDER;
    ++optstring;
    }
  
      /* si la chaîne d'options commence par un + alors la fonction getopt() s'arrête
       * dès qu'un argument de la ligne de commande n'est pas une option
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
__getopt_internal(int argc, char *const *argv, const char *optstring,
                  const struct option *longopts, int *longind,
                  int long_only) 
{
  optarg = NULL;
  if (optind == 0)
    optstring = __getopt_initialize(optstring);
  if (nextchar == NULL || *nextchar == '\0')
     {
    
        /* Advance to the next ARGV-element.  */ 
        if (ordering == PERMUTE)
       {
      
          /* If we have just processed some options following some non-options,
             __exchange them so that the options come first.  */ 
          if (first_nonopt != last_nonopt && last_nonopt != optind)
        __exchange((char **) argv);
      
      else if (last_nonopt != optind)
        first_nonopt = optind;
      
          /* Skip any additional non-options
             and extend the range of non-options previously skipped.  */ 
          while (optind < argc
                  && (argv[optind][0] != '-' || argv[optind][1] == '\0'))
        optind++;
      last_nonopt = optind;
      }
    
        /* The special ARGV-element `--' means premature end of options.
           Skip it like a null option,
           then __exchange with previous non-options as if it were an option,
           then skip everything else like a non-option.  */ 
        if (optind != argc && !strcmp(argv[optind], "--"))
       {
      optind++;
      if (first_nonopt != last_nonopt && last_nonopt != optind)
        __exchange((char **) argv);
      
      else if (first_nonopt == last_nonopt)
        first_nonopt = optind;
      last_nonopt = argc;
      optind = argc;
      }
    
        /* If we have done all the ARGV-elements, stop the scan
           and back over any non-options that we skipped and permuted.  */ 
        if (optind == argc)
       {
      
          /* Set the next-arg-index to point at the non-options
             that we previously skipped, so the caller will digest them.  */ 
          if (first_nonopt != last_nonopt)
        optind = first_nonopt;
      return EOF;
      }
    
        /* If we have come to a non-option and did not permute it,
           either stop the scan or describe it to the caller and pass it by.  */ 
        if ((argv[optind][0] != '-' || argv[optind][1] == '\0'))
       {
      if (ordering == REQUIRE_ORDER)
        return EOF;
      optarg = argv[optind++];
      return 1;
      }
    
        /* We have found another option-ARGV-element.
           Skip the initial punctuation.  */ 
        nextchar =
        (argv[optind] + 1 + (longopts != NULL && argv[optind][1] == '-'));
    }
  
      /* Decode the current option-ARGV-element.  */ 
      
      /* Check whether the ARGV-element is a long option.
         
         If long_only and the ARGV-element has the form "-f", where f is
         a valid short option, don't consider it an abbreviated form of
         a long option that starts with f.  Otherwise there would be no
         way to give the -f short option.
         
         On the other hand, if there's a long option "fubar" and
         the ARGV-element is "-fu", do consider that an abbreviation of
         the long option, just like "--fu", and not "-f" with arg "u".
         
         This distinction seems to be the most useful approach.  */ 
      if (longopts != NULL
           && (argv[optind][1] == '-'
               || (long_only
                   && (argv[optind][2]
                       || !strchr(optstring, argv[optind][1])))))
     {
    char *nameend;
    const struct option *p;
    const struct option *pfound = NULL;
    int exact = 0;
    int ambig = 0;
    int indfound = 0;
    int option_index;
    for (nameend = nextchar; *nameend != '\0' && *nameend != '=';
           nameend++)
      
          /* Do nothing.  */ ;
    
        /* Test all long options for either exact match
           or abbreviated matches.  */ 
        for (p = longopts, option_index = 0; p->name; p++, option_index++)
       {
      if (!strncmp(p->name, nextchar, nameend - nextchar))
         {
        if (nameend - nextchar == strlen(p->name))
           {
          
              /* Exact match found.  */ 
              pfound = p;
          indfound = option_index;
          exact = 1;
          break;
          }
        
        else if (pfound == NULL)
           {
          
              /* First nonexact match found.  */ 
              exact = 0;
          
              /* begin change
                 pfound = p;
                 indfound = option_index;
                 end change */ 
              break;
          }
        
        else
           {
          
              /* Second or later nonexact match found.  */ 
              ambig = 1;
          }
        }
      }
    if (ambig && !exact)
       {
      if (opterr)
        fprintf(stderr, "error   : %s: option `%s' is ambiguous\n",
                 argv[0], argv[optind]);
      nextchar += strlen(nextchar);
      optind++;
      return '?';
      }
    if (pfound != NULL)
       {
      option_index = indfound;
      optind++;
      if (*nameend)
         {
        
            /* Don't test has_arg with >, because some C compilers don't
               allow it to be used on enums.  */ 
            if (pfound->has_arg)
          optarg = nameend + 1;
        
        else
           {
          if (opterr)
             {
            if (argv[optind - 1][1] == '-')
              
                  /* --option */ 
                  fprintf(stderr,
                          "error   : %s: option `--%s' doesn't allow an argument\n",
                          argv[0], pfound->name);
            
            else
              
                  /* +option or -option */ 
                  fprintf(stderr,
                          "error   : %s: option `%c%s' doesn't allow an argument\n",
                          argv[0], argv[optind - 1][0], pfound->name);
            }
          nextchar += strlen(nextchar);
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
            fprintf(stderr,
                     "error   : %s: option `%s' requires an argument\n",
                     argv[0], argv[optind - 1]);
          nextchar += strlen(nextchar);
          return optstring[0] == ':' ? ':' : '?';
          }
        }
      nextchar += strlen(nextchar);
      if (longind != NULL)
        *longind = option_index;
      if (pfound->flag)
         {
        *(pfound->flag) = pfound->val;
        return 0;
        }
      return pfound->val;
      }
    
        /* Can't find it as a long option.  If this is not getopt_long_only,
           or the option starts with '--' or is not a valid short
           option, then it's an error.
           Otherwise interpret it as a short option.  */ 
        if (!long_only || argv[optind][1] == '-'
            || strchr(optstring, *nextchar) == NULL)
       {
      if (opterr)
         {
        if (argv[optind][1] == '-')
          
              /* --option */ 
              fprintf(stderr, "error   : %s: unrecognized option `--%s'\n",
                      argv[0], nextchar);
        
        else
          
              /* +option or -option */ 
              fprintf(stderr, "error   : %s: unrecognized option `%c%s'\n",
                      argv[0], argv[optind][0], nextchar);
        }
      nextchar = (char *) "";
      optind++;
      return '?';
      }
    }
  
      /* Look at and handle the next short option-character.  */ 
       {
    char c = *nextchar++;
    char *temp = strchr(optstring, c);
    
        /* Increment `optind' when we start to process its last character.  */ 
        if (*nextchar == '\0')
      ++optind;
    if (temp == NULL || c == ':')
       {
      if (opterr)
        fprintf(stderr, "error   : %s: invalid option -- %c\n", argv[0],
                 c);
      optopt = c;
      return '?';
      }
    if (temp[1] == ':')
       {
      if (temp[2] == ':')
         {
        
            /* This is an option that accepts an argument optionally.  */ 
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
        
            /* This is an option that requires an argument.  */ 
            if (*nextchar != '\0')
           {
          optarg = nextchar;
          
              /* If we end this ARGV-element by taking the rest as an arg,
                 we must advance to the next element now.  */ 
              optind++;
          }
        
        else if (optind == argc)
           {
          if (opterr)
             {
            
                /* 1003.2 specifies the format of this message.  */ 
                fprintf(stderr,
                        "error   : %s: option requires an argument -- %c\n",
                        argv[0], c);
            }
          optopt = c;
          if (optstring[0] == ':')
            c = ':';
          
          else
            c = '?';
          }
        
        else
          
              /* We already incremented `optind' once;
                 increment it again when taking next ARGV-elt as argument.  */ 
              optarg = argv[optind++];
        nextchar = NULL;
        }
      }
    return c;
  }
}

static void  __exchange(char **argv) 
{
  int bottom = first_nonopt;
  int middle = last_nonopt;
  int top = optind;
  char *tem;
  
      /* Exchange the shorter segment with the far end of the longer segment.
         That puts the shorter segment into the right place.
         It leaves the longer segment in the right place overall,
         but it consists of two parts that need to be swapped next.  */ 
      while (top > middle && middle > bottom)
     {
    if (top - middle > middle - bottom)
       {
      
          /* Bottom segment is the short one.  */ 
      int len = middle - bottom;
      register int i;
      
          /* Swap it with the top part of the top segment.  */ 
          for (i = 0; i < len; i++)
         {
        tem = argv[bottom + i];
        argv[bottom + i] = argv[top - (middle - bottom) + i];
        argv[top - (middle - bottom) + i] = tem;
        }
      
          /* Exclude the moved bottom segment from further swapping.  */ 
          top -= len;
      }
    
    else
       {
      
          /* Top segment is the short one.  */ 
      int len = top - middle;
      register int i;
      
          /* Swap it with the bottom part of the bottom segment.  */ 
          for (i = 0; i < len; i++)
         {
        tem = argv[bottom + i];
        argv[bottom + i] = argv[middle + i];
        argv[middle + i] = tem;
        }
      
          /* Exclude the moved top segment from further swapping.  */ 
          bottom += len;
      }
    }
  
      /* Update records for the slots the non-options now occupy.  */ 
      first_nonopt += (optind - last_nonopt);
  last_nonopt = optind;
}

int 
getopt_long(int argc, char *const *argv, const char *options,
            const struct option *long_options, int *opt_index) 
{
  return __getopt_internal(argc, argv, options, long_options, opt_index,
                            0);
}

int 
getopt_long_only(int argc, char *const *argv, const char *options,
                 const struct option *long_options, int *opt_index) 
{
  return __getopt_internal(argc, argv, options, long_options, opt_index,
                            1);
}


