#ifndef __GETOPT_H
#define	__GETOPT_H

#define	no_argument			0
#define required_argument	1
#define optional_argument	2
struct option  {
  const char *name;            /* name of the long option      */
    int has_arg;              /*
                                 * has_arg is : no_argument (or 0), if the option doesn't take an argument, 
                                 * required_argument (or 1) if the option takes an argument,
                                 * optional_argument (or 2) if the option takes an optional argument.
                                 */
    int *flag;                /* specify the mean used to return a result for a long option:
                                 * if flag is NULL, then getopt_long() returns val
                                 * in the other case getopt_long() returns 0, and flag points to the  
                                 * variable specified bay the content of the field val when the option 
                                 * is found but it is not update if the option is not found.
                                 */
    int val;                  /* val is the value returned by getopt_long() when the pointer flag
                                 * is NULL or the value of the variable referenced by the pointer flag
                                 * when the option is found.
                                 */
};
extern int  optind;
extern char * optarg;
extern int  opterr;
extern int  optopt;
int  getopt(int argc, char *const argv[], const char *optstring);
int 
getopt_long(int argc, char *const argv[], const char *optstring,
            const struct option *longopts, int *longindex);
int  getopt_long_only(int argc, char *const argv[],
                         const char *optstring,
                         const struct option *longopts, int *longindex);

#endif  /* !__GETOPT_H */
