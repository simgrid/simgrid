

/* must be defined before */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <windows.h>

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#define strdup	_strdup
#define fileno	_fileno
#define creat	_creat
#define open	_open
#define read	_read
#define write	_write
#endif

#ifdef max
#undef max
#define max(h,i) ((h) > (i) ? (h) : (i))
#endif

#ifndef S_ISREG
#define S_ISREG(__mode) (((__mode) & S_IFMT) == S_IFREG)
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO	0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO	1
#endif


static
int exit_code = EXIT_SUCCESS;

static char *rfname;

static char *wfname = NULL;

static
int wfd;

#ifndef _MSC_VER
static size_t
#else
static int
#endif
 bsize;

static char *buf = NULL;

static int
 fnumber = 0;

int cat_files(char **fnames);

int cat(int);

char **fnames = NULL;

void exit0(int errcode)
{
  if (fnames) {
    int i;

    for (i = 0; i < fnumber; i++)
      free(fnames[i]);

    free(fnames);
  }

  if (buf)
    free(buf);

  if (wfname)
    free(wfname);


  exit(errcode);
}

int main(int argc, char *argv[])
{
  int i, j;
  int success = 1;
  int redirect = 0;
  int exists = 0;

  struct stat stat_buf = { 0 };

  setlocale(LC_ALL, "");

  bsize = BUFSIZ;

  if (!(buf = (char *) malloc(bsize))) {
    fprintf(stderr, "[ERROR/cat] not enough memory\n");
    exit0(EXIT_FAILURE);
  }

  if (argc > 1) {
    if (!(fnames = (char **) calloc(argc, sizeof(char *)))) {
      fprintf(stderr, "[ERROR/cat]  not enough memory\n");
      exit0(EXIT_FAILURE);
    }

    for (i = 1, j = 0; i < argc; i++) {
      if (!strcmp(argv[i], ">")) {
        redirect = 1;
        break;
      } else if (!strcmp(argv[i], ">>")) {
        redirect = 1;
        exists = 1;
        break;
      } else {
        if (stat(argv[i], &stat_buf)) {
          fprintf(stderr,
                  "[ERROR/cat (1)] could not get information about %s\n",
                  argv[i]);
          success = 0;
          break;
        } else {
          if (S_ISREG(stat_buf.st_mode)) {
            fnames[j++] = strdup(argv[i]);
            fnumber++;
          } else {
            fprintf(stderr, "[ERROR/cat] %s is not a file\n", argv[i]);
            success = 0;
            break;
          }
        }
      }
    }

    if (!success)
      exit0(EXIT_FAILURE);
  }

  if (redirect) {
    if (i != argc - 2) {
      if (exists)
        fprintf(stderr,
                "[ERROR/cat] syntax error near `>>' (i : %d - argc : %d\n",
                i, argc);
      else
        fprintf(stderr,
                "[ERROR/cat] syntax error near `>' (i : %d - argc : %d\n",
                i, argc);

      exit0(EXIT_FAILURE);
    } else {
      wfname = strdup(argv[i + 1]);

      if (!exists) {
        if ((wfd = creat(wfname, _S_IREAD | _S_IWRITE)) < 0) {
          fprintf(stderr, "[ERROR/cat] could not create %s file\n",
                  wfname);

          exit0(EXIT_FAILURE);
        }
      } else {
        if ((wfd = open(wfname, O_WRONLY | O_APPEND, 0)) < 0) {
          fprintf(stderr, "[ERROR/cat] could not open %s file\n", wfname);

          exit0(EXIT_FAILURE);
        }
      }
    }
  } else {
    wfd = STDOUT_FILENO;

    if (fstat(wfd, &stat_buf)) {
      fprintf(stderr,
              "[ERROR/cat (3)] could not get information about stdout\n");
      exit0(EXIT_FAILURE);
    }
  }

  exit_code = cat_files(fnames);

  if (wfd == STDOUT_FILENO) {
    if (fclose(stdout)) {
      fprintf(stderr, "[ERROR/cat] could not close stdout\n");
      exit_code = EXIT_FAILURE;
    }
  } else {
    if (close(wfd)) {
      fprintf(stderr, "[ERROR/cat] could not close %s\n", wfname);
      exit_code = EXIT_FAILURE;
    }
  }

  exit0(exit_code);
}

int cat_files(char **fnames)
{
  int rfd, i;
  int failure = 0;

  rfd = STDIN_FILENO;

  rfname = "stdin";

  if (fnumber) {

    for (i = 0; i < fnumber && !failure; i++) {
      if (!strcmp(fnames[i], "-"))
        rfd = fileno(stdin);
      else if ((rfd = open(fnames[i], O_RDONLY, 0)) < 0) {
        fprintf(stderr, "[WARN/cat] could not open %s file", fnames[i]);
        exit_code = EXIT_FAILURE;
        continue;
      }

      rfname = fnames[i];

      failure = cat(rfd);

      close(rfd);

    }
  } else
    failure = cat(rfd);

  return failure ? 1 : 0;
}

int cat(int rfd)
{

#ifndef _MSC_VER
  size_t bytes_readed_nb;
#else
  int bytes_readed_nb;
#endif
  int bytes_written_nb, pos;

  while ((bytes_readed_nb = read(rfd, buf, bsize)) != -1
         && bytes_readed_nb != 0) {
    for (pos = 0; bytes_readed_nb;
         bytes_readed_nb -= bytes_written_nb, pos += bytes_written_nb) {
      if ((bytes_written_nb = write(wfd, buf + pos, bytes_readed_nb)) == 0
          || bytes_written_nb == -1) {
        fprintf(stderr, "[ERROR/cat] could not write to %s\n",
                wfd == fileno(stdout) ? "stdout" : wfname);
        return 1;
      }
    }
  }

  if (bytes_readed_nb < 0) {
    fprintf(stderr, "[WARN/cat] could not read %s file", rfname);
    return 1;
  }

  return 0;
}
