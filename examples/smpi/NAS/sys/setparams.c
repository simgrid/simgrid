/* 
 * This utility configures a NPB to be built for a specific number
 * of nodes and a specific class. It creates a file "npbparams.h" 
 * in the source directory. This file keeps state information about 
 * which size of benchmark is currently being built (so that nothing
 * if unnecessarily rebuilt) and defines (through PARAMETER statements)
 * the number of nodes and class for which a benchmark is being built. 

 * The utility takes 3 arguments: 
 *    setparams benchmark-name nprocs class
 *      benchmark-name is "ep", "dt", or "is"
 *      nprocs is the number of processors to run on
 *      class is the size of the benchmark
 * These parameters are checked for the current benchmark. If they
 * are invalid, this program prints a message and aborts. 
 * If the parameters are ok, the current npbsize.h (actually just
 * the first line) is read in. If the new parameters are the same as 
 * the old, nothing is done, but an exit code is returned to force the
 * user to specify (otherwise the make procedure succeeds but builds a
 * binary of the wrong name).  Otherwise the file is rewritten. 
 * Errors write a message (to stdout) and abort. 
 * 
 * This program makes use of two extra benchmark "classes"
 * class "X" means an invalid specification. It is returned if
 * there is an error parsing the config file. 
 * class "U" is an external specification meaning "unknown class"
 * 
 * Unfortunately everything has to be case sensitive. This is
 * because we can always convert lower to upper or v.v. but
 * can't feed this information back to the makefile, so typing
 * make CLASS=a and make CLASS=A will produce different binaries.
 *
 * 
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

/*
 * This is the master version number for this set of NPB benchmarks. It is in an obscure place so people
 * won't accidentally change it. 
 */

#define VERSION "3.3"

/* controls verbose output from setparams */
/* #define VERBOSE */

#define FILENAME "npbparams.h"
#define DESC_LINE "c NPROCS = %d CLASS = %c\n"
#define DEF_CLASS_LINE     "#define CLASS '%c'\n"
#define DEF_NUM_PROCS_LINE "#define NUM_PROCS %d\n"
#define FINDENT  "        "
#define CONTINUE "     > "

void get_info(int argc, char *argv[], int *typep, int *nprocsp, char *classp, int* subtypep);
void check_info(int type, int nprocs, char class);
void read_info(int type, int *nprocsp, char *classp, int *subtypep);
void write_info(int type, int nprocs, char class, int subtype);
void write_ep_info_C(FILE *fp, int nprocs, char class);  /* after C translation */
void write_is_info(FILE *fp, int nprocs, char class);
void write_dt_info(FILE *fp, int nprocs, char class);
void write_compiler_info(int type, FILE *fp);
void check_line(char *line, char *label, char *val);
int  check_include_line(char *line, char *filename);
void put_string(FILE *fp, char *name, char *val);
void put_def_string(FILE *fp, char *name, char *val);
void put_def_variable(FILE *fp, char *name, char *val);
int isqrt(int i);
int ilog2(int i);
int ipow2(int i);

enum benchmark_types {IS, DT, EP};

int main(int argc, char *argv[])
{
  int nprocs, nprocs_old, type;
  char class, class_old;
  int subtype = -1, old_subtype = -1;

  /* Get command line arguments. Make sure they're ok. */
  get_info(argc, argv, &type, &nprocs, &class, &subtype);
  if (class != 'U') {
#ifdef VERBOSE
    printf("setparams: For benchmark %s: number of processors = %d class = %c\n", 
     argv[1], nprocs, class); 
#endif
    check_info(type, nprocs, class);
  }

  /* Get old information. */
  read_info(type, &nprocs_old, &class_old, &old_subtype);
  if (class != 'U') {
    if (class_old != 'X') {
#ifdef VERBOSE
      printf("setparams:     old settings: number of processors = %d class = %c\n", 
       nprocs_old, class_old); 
#endif
    }
  } else {
    printf("setparams:\n\
  *********************************************************************\n\
  * You must specify NPROCS and CLASS to build this benchmark         *\n\
  * For example, to build a class A benchmark for 4 processors, type  *\n\
  *       make {benchmark-name} NPROCS=4 CLASS=A                      *\n\
  *********************************************************************\n\n"); 

    if (class_old != 'X') {
#ifdef VERBOSE
      printf("setparams: Previous settings were CLASS=%c NPROCS=%d\n", 
       class_old, nprocs_old); 
#endif
    }
    exit(1); /* exit on class==U */
  }

  /* Write out new information if it's different. */
  if (nprocs != nprocs_old || class != class_old || subtype != old_subtype) {
#ifdef VERBOSE
    printf("setparams: Writing %s\n", FILENAME); 
#endif
    write_info(type, nprocs, class, subtype);
  } else {
#ifdef VERBOSE
    printf("setparams: Settings unchanged. %s unmodified\n", FILENAME); 
#endif
  }

  return 0;
}

/*  get_info(): Get parameters from command line */
void get_info(int argc, char *argv[], int *typep, int *nprocsp, char *classp, int *subtypep)
{
  if (argc < 4) {
    printf("Usage: %s (%d) benchmark-name nprocs class\n", argv[0], argc);
    exit(1);
  }

  *nprocsp = atoi(argv[2]);
  *classp = *argv[3];

  if      (!strcmp(argv[1], "is") || !strcmp(argv[1], "IS")) *typep = IS;
  else if (!strcmp(argv[1], "dt") || !strcmp(argv[1], "DT")) *typep = DT;
  else if (!strcmp(argv[1], "ep") || !strcmp(argv[1], "EP")) *typep = EP;
  else {
    printf("setparams: Error: unknown benchmark type %s\n", argv[1]);
    exit(1);
  }
}

/*
 *  check_info(): Make sure command line data is ok for this benchmark 
 */

void check_info(int type, int nprocs, char class) 
{
  int rootprocs, logprocs; 

  /* check number of processors */
  if (nprocs <= 0) {
    printf("setparams: Number of processors must be greater than zero\n");
    exit(1);
  }
  switch(type) {
  case IS:
    logprocs = ilog2(nprocs);
    if (logprocs < 0) {
      printf("setparams: Number of processors must be a power of two (1,2,4,...) for this benchmark\n");
      exit(1);
    }
    break;

  case EP:
  case DT:
    break;

  default:
    /* never should have gotten this far with a bad name */
    printf("setparams: (Internal Error) Benchmark type %d unknown to this program\n", type); 
    exit(1);
  }

  /* check class */
  if (class != 'S' && 
      class != 'W' && 
      class != 'A' && 
      class != 'B' && 
      class != 'C' && 
      class != 'D' && 
      class != 'E') {
    printf("setparams: Unknown benchmark class %c\n", class); 
    printf("setparams: Allowed classes are \"S\", \"W\", and \"A\" through \"E\"\n");
    exit(1);
  }

  if (class == 'E' && (type == IS || type == DT)) {
    printf("setparams: Benchmark class %c not defined for IS or DT\n", class);
    exit(1);
  }

  if (class == 'D' && type == IS && nprocs < 4) {
    printf("setparams: IS class D size cannot be run on less than 4 processors\n");
    exit(1);
  }
}

/* 
 * read_info(): Read previous information from file. 
 *              Not an error if file doesn't exist, because this may be the first time we're running.
 *              Assumes the first two lines of the file is in a special format that we understand (since we wrote it).
 */

void read_info(int type, int *nprocsp, char *classp, int *subtypep)
{
  int nread = 0;
  FILE *fp;
  fp = fopen(FILENAME, "r");
  if (fp == NULL) {
#ifdef VERBOSE
    printf("setparams: INFO: configuration file %s does not exist (yet)\n", FILENAME); 
#endif
    goto abort;
  }

  /* first two lines of file contains info */
  nread = fscanf(fp, DEF_CLASS_LINE, classp);
  nread += fscanf(fp, DEF_NUM_PROCS_LINE, nprocsp);
  if (nread != 2) {
    printf("setparams: Error line %d parsing config file %s. Ignoring previous settings\n", __LINE__,FILENAME);
    goto abort;
  }

  fclose(fp);
  return;

 abort:
  *nprocsp = -1;
  *classp = 'X';
  *subtypep = -1;
  return;
}

/* 
 * write_info(): Write new information to config file. 
 *               First line is in a special format so we can read
 *               it in again. Then comes a warning. The rest is all
 *               specific to a particular benchmark. 
 */

void write_info(int type, int nprocs, char class, int subtype) 
{
  FILE *fp;
  char *BT_TYPES[] = {"NONE", "FULL", "SIMPLE", "EPIO", "FORTRAN"};

  fp = fopen(FILENAME, "w");
  if (fp == NULL) {
    printf("setparams: Can't open file %s for writing\n", FILENAME);
    exit(1);
  }

  fprintf(fp, DEF_CLASS_LINE, class);
  fprintf(fp, DEF_NUM_PROCS_LINE, nprocs);
  fprintf(fp, "\
/*\n\
   This file is generated automatically by the setparams utility.\n\
   It sets the number of processors and the class of the NPB\n\
   in this directory. Do not modify it by hand.   */\n\
   \n");

  /* Now do benchmark-specific stuff */
  switch(type) {
  case IS:
    write_is_info(fp, nprocs, class);  
    break;
  case DT:
    write_dt_info(fp, nprocs, class);  
    break;
  case EP:
    write_ep_info_C(fp, nprocs, class);
    break;
  default:
    printf("setparams: (Internal error): Unknown benchmark type %d\n", type);
    exit(1);
  }
  write_compiler_info(type, fp);
  fclose(fp);
  return;
}

/* write_dt_info(): Write DT specific info to config file */

void write_dt_info(FILE *fp, int nprocs, char class) 
{
  int num_samples,deviation,num_sources;
  if      (class == 'S') { num_samples=1728; deviation=128; num_sources=4; }
  else if (class == 'W') { num_samples=1728*8; deviation=128*2; num_sources=4*2; }
  else if (class == 'A') { num_samples=1728*64; deviation=128*4; num_sources=4*4; }
  else if (class == 'B') { num_samples=1728*512; deviation=128*8; num_sources=4*8; }
  else if (class == 'C') { num_samples=1728*4096; deviation=128*16; num_sources=4*16; }
  else if (class == 'D') { num_samples=1728*4096*8; deviation=128*32; num_sources=4*32; }
  else {
    printf("setparams: Internal error: invalid class type %c\n", class);
    exit(1);
  }
  fprintf(fp, "#define NUM_SAMPLES %d\n", num_samples);
  fprintf(fp, "#define STD_DEVIATION %d\n", deviation);
  fprintf(fp, "#define NUM_SOURCES %d\n", num_sources);
}

/* write_is_info(): Write IS specific info to config file */
void write_is_info(FILE *fp, int nprocs, char class)
{
  if( class != 'S' && class != 'W' && class != 'A' && class != 'B' && class != 'C' && class != 'D' )
  {
    printf("setparams: Internal error: invalid class type %c\n", class);
    exit(1);
  }
}

/* write_ep_info_C(): Write EP specific info to config file */
void write_ep_info_C(FILE *fp, int nprocs, char class)
{
  /* easiest way (given the way the benchmark is written) is to specify log of number of grid points in each
   * direction m1, m2, m3. nt is the number of iterations
   */
  int m;
  if      (class == 'S') { m = 24; }
  else if (class == 'W') { m = 25; }
  else if (class == 'A') { m = 28; }
  else if (class == 'B') { m = 30; }
  else if (class == 'C') { m = 32; }
  else if (class == 'D') { m = 36; }
  else if (class == 'E') { m = 40; }
  else {
    printf("setparams: Internal error: invalid class type %c\n", class);
    exit(1);
  }

  /* number of processors given by "npm" */
  fprintf(fp, "%schar *_class=\"%c\";\n",FINDENT,class);
  fprintf(fp, "%sint m=%d;\n", FINDENT,m);
  fprintf(fp, "%sint npm=%d;\n", FINDENT,nprocs);
}

/* 
 * This is a gross hack to allow the benchmarks to  print out how they were compiled. Various other ways
 * of doing this have been tried and they all fail on some machine - due to a broken "make" program, or
 * F77 limitations, of whatever. Hopefully this will always work because it uses very portable C. Unfortunately
 * it relies on parsing the make.def file - YUK. 
 * If your machine doesn't have <string.h> or <ctype.h>, happy hacking!
 */

#define VERBOSE
#define LL 400
#include <stdio.h>
#define DEFFILE "../config/make.def"
#define DEFAULT_MESSAGE "(none)"
FILE *deffile;
void write_compiler_info(int type, FILE *fp)
{
  char line[LL];
  char compiletime[LL], randfile[LL];
  char mpicc[LL], cflags[LL], clink[LL], clinkflags[LL],
       cmpi_lib[LL], cmpi_inc[LL];
  struct tm *tmp;
  time_t t;
  deffile = fopen(DEFFILE, "r");
  if (deffile == NULL) {
    printf("\n\
setparams: File %s doesn't exist. To build the NAS benchmarks\n\
           you need to create is according to the instructions\n\
           in the README in the main directory and comments in \n\
           the file config/make.def.template\n", DEFFILE);
    exit(1);
  }
  strcpy(randfile, DEFAULT_MESSAGE);
  strcpy(mpicc, DEFAULT_MESSAGE);
  strcpy(cflags, DEFAULT_MESSAGE);
  strcpy(clink, DEFAULT_MESSAGE);
  strcpy(clinkflags, DEFAULT_MESSAGE);
  strcpy(cmpi_lib, DEFAULT_MESSAGE);
  strcpy(cmpi_inc, DEFAULT_MESSAGE);

  while (fgets(line, LL, deffile) != NULL) {
    if (*line == '#') continue;
    /* yes, this is inefficient. but it's simple! */
    check_line(line, "RAND", randfile);
    check_line(line, "MPICC", mpicc);
    check_line(line, "CFLAGS", cflags);
    check_line(line, "CLINK", clink);
    check_line(line, "CLINKFLAGS", clinkflags);
    check_line(line, "CMPI_LIB", cmpi_lib);
    check_line(line, "CMPI_INC", cmpi_inc);
  }

  (void) time(&t);
  tmp = localtime(&t);
  (void) strftime(compiletime, (size_t)LL, "%d %b %Y", tmp);

  put_def_string(fp, "COMPILETIME", compiletime);
  put_def_string(fp, "NPBVERSION", VERSION);
  put_def_string(fp, "MPICC", mpicc);
  put_def_string(fp, "CFLAGS", cflags);
  put_def_string(fp, "CLINK", clink);
  put_def_string(fp, "CLINKFLAGS", clinkflags);
  put_def_string(fp, "CMPI_LIB", cmpi_lib);
  put_def_string(fp, "CMPI_INC", cmpi_inc);
}

void check_line(char *line, char *label, char *val)
{
  char *original_line;
  int n;
  original_line = line;
  /* compare beginning of line and label */
  while (*label != '\0' && *line == *label) {
    line++; label++; 
  }
  /* if *label is not EOS, we must have had a mismatch */
  if (*label != '\0') return;
  /* if *line is not a space, actual label is longer than test label */
  if (!isspace(*line) && *line != '=') return ; 
  /* skip over white space */
  while (isspace(*line)) line++;
  /* next char should be '=' */
  if (*line != '=') return;
  /* skip over white space */
  while (isspace(*++line));
  /* if EOS, nothing was specified */
  if (*line == '\0') return;
  /* finally we've come to the value */
  strcpy(val, line);
  /* chop off the newline at the end */
  n = strlen(val)-1;
  if (n >= 0 && val[n] == '\n')
    val[n--] = '\0';
  if (n >= 0 && val[n] == '\r')
    val[n--] = '\0';
  /* treat continuation */
  while (val[n] == '\\' && fgets(original_line, LL, deffile)) {
     line = original_line;
     while (isspace(*line)) line++;
     if (isspace(*original_line)) val[n++] = ' ';
     while (*line && *line != '\n' && *line != '\r' && n < LL-1)
       val[n++] = *line++;
     val[n] = '\0';
     n--;
  }
/*  if (val[strlen(val) - 1] == '\\') {
    printf("\n\
setparams: Error in file make.def. Because of the way in which\n\
           command line arguments are incorporated into the\n\
           executable benchmark, you can't have any continued\n\
           lines in the file make.def, that is, lines ending\n\
           with the character \"\\\". Although it may be ugly, \n\
           you should be able to reformat without continuation\n\
           lines. The offending line is\n\
  %s\n", original_line);
    exit(1);
  } */
}

int check_include_line(char *line, char *filename)
{
  char *include_string = "include";
  /* compare beginning of line and "include" */
  while (*include_string != '\0' && *line == *include_string) {
    line++; include_string++; 
  }
  /* if *include_string is not EOS, we must have had a mismatch */
  if (*include_string != '\0') return(0);
  /* if *line is not a space, first word is not "include" */
  if (!isspace(*line)) return(0); 
  /* skip over white space */
  while (isspace(*++line));
  /* if EOS, nothing was specified */
  if (*line == '\0') return(0);
  /* next keyword should be name of include file in *filename */
  while (*filename != '\0' && *line == *filename) {
    line++; filename++; 
  }  
  if (*filename != '\0' || 
      (*line != ' ' && *line != '\0' && *line !='\n')) return(0);
  else return(1);
}

#define MAXL 46
void put_string(FILE *fp, char *name, char *val)
{
  int len;
  len = strlen(val);
  if (len > MAXL) {
    val[MAXL] = '\0';
    val[MAXL-1] = '.';
    val[MAXL-2] = '.';
    val[MAXL-3] = '.';
    len = MAXL;
  }
  fprintf(fp, "%scharacter*%d %s\n", FINDENT, len, name);
  fprintf(fp, "%sparameter (%s=\'%s\')\n", FINDENT, name, val);
}

/* NOTE: is the ... stuff necessary in C? */
void put_def_string(FILE *fp, char *name, char *val)
{
  int len;
  len = strlen(val);
  if (len > MAXL) {
    val[MAXL] = '\0';
    val[MAXL-1] = '.';
    val[MAXL-2] = '.';
    val[MAXL-3] = '.';
    len = MAXL;
  }
  fprintf(fp, "#define %s \"%s\"\n", name, val);
}

void put_def_variable(FILE *fp, char *name, char *val)
{
  int len;
  len = strlen(val);
  if (len > MAXL) {
    val[MAXL] = '\0';
    val[MAXL-1] = '.';
    val[MAXL-2] = '.';
    val[MAXL-3] = '.';
    len = MAXL;
  }
  fprintf(fp, "#define %s %s\n", name, val);
}

#if 0
/* this version allows arbitrarily long lines but some compilers don't like that and they're rarely useful */

#define LINELEN 65
void put_string(FILE *fp, char *name, char *val)
{
  int len, nlines, pos, i;
  char line[100];
  len = strlen(val);
  nlines = len/LINELEN;
  if (nlines*LINELEN < len) nlines++;
  fprintf(fp, "%scharacter*%d %s\n", FINDENT, nlines*LINELEN, name);
  fprintf(fp, "%sparameter (%s = \n", FINDENT, name);
  for (i = 0; i < nlines; i++) {
    pos = i*LINELEN;
    if (i == 0) fprintf(fp, "%s\'", CONTINUE);
    else        fprintf(fp, "%s", CONTINUE);
    /* number should be same as LINELEN */
    fprintf(fp, "%.65s", val+pos);
    if (i == nlines-1) fprintf(fp, "\')\n");
    else             fprintf(fp, "\n");
  }
}
#endif


/* integer square root. Return error if argument isn't a perfect square or is less than or equal to zero */
int isqrt(int i)
{
  int root, square;
  if (i <= 0) return(-1);
  square = 0;
  for (root = 1; square <= i; root++) {
    square = root*root;
    if (square == i) return(root);
  }
  return(-1);
}

/* integer log base two. Return error is argument isn't a power of two or is less than or equal to zero */
int ilog2(int i)
{
  int log2;
  int exp2 = 1;
  if (i <= 0) return(-1);

  for (log2 = 0; log2 < 20; log2++) {
    if (exp2 == i) return(log2);
    exp2 *= 2;
  }
  return(-1);
}

int ipow2(int i)
{
  int pow2 = 1;
  if (i < 0) return(-1);
  if (i == 0) return(1);
  while(i--) pow2 *= 2;
  return(pow2);
}
