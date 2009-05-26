/* $Id$ */

/* TESH (Test Shell) -- mini shell specialized in running test units        */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* specific to Borland Compiler */
#ifdef __BORLANDDC__
#pragma hdrstop
#endif

#include "tesh.h"
#include "xbt.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(tesh, "TEst SHell utility");

/*** Options ***/
int timeout_value = 5;          /* child timeout value */

char *testsuite_name;
static void handle_line(const char *filepos, char *line)
{
  /* Search end */
  xbt_str_rtrim(line + 2, "\n");

  /*
     DEBUG7("rctx={%s,in={%d,>>%10s<<},exp={%d,>>%10s<<},got={%d,>>%10s<<}}",
     rctx->cmd,
     rctx->input->used,        rctx->input->data,
     rctx->output_wanted->used,rctx->output_wanted->data,
     rctx->output_got->used,   rctx->output_got->data);
   */
  DEBUG2("[%s] %s", filepos, line);

  switch (line[0]) {
  case '#':
    break;

  case '$':
    /* further trim useless chars which are significant for in/output */
    xbt_str_rtrim(line + 2, " \t");

    /* Deal with CD commands here, not in rctx */
    if (!strncmp("cd ", line + 2, 3)) {
      char *dir = line + 4;

      if (rctx->cmd)
        rctx_start();

      /* search beginning */
      while (*(dir++) == ' ');
      dir--;
      VERB1("Saw cd '%s'", dir);
      if (chdir(dir)) {
        ERROR2("Chdir to %s failed: %s", dir, strerror(errno));
        ERROR1("Test suite `%s': NOK (system error)", testsuite_name);
        rctx_armageddon(rctx, 4);
      }
      break;
    }                           /* else, pushline */
  case '&':
  case '<':
  case '>':
  case '!':
    rctx_pushline(filepos, line[0], line + 2 /* pass '$ ' stuff */ );
    break;

  case 'p':
    INFO2("[%s] %s", filepos, line + 2);
    break;
  case 'P':
    CRITICAL2("[%s] %s", filepos, line + 2);
    break;

  default:
    ERROR2("[%s] Syntax error: %s", filepos, line);
    ERROR1("Test suite `%s': NOK (syntax error)", testsuite_name);
    rctx_armageddon(rctx, 1);
    break;
  }
}

static void handle_suite(const char *filename, FILE * IN)
{
  size_t len;
  char *line = NULL;
  int line_num = 0;
  char file_pos[256];

  xbt_strbuff_t buff = xbt_strbuff_new();
  int buffbegin = 0;

  rctx = rctx_new();

  while (getline(&line, &len, IN) != -1) {
    line_num++;

    /* Count the line length while checking wheather it's blank */
    int blankline = 1;
    int linelen = 0;
    while (line[linelen] != '\0') {
      if (line[linelen] != ' ' && line[linelen] != '\t'
          && line[linelen] != '\n')
        blankline = 0;
      linelen++;
    }

    if (blankline) {
      if (!rctx->cmd && !rctx->is_empty) {
        ERROR1("[%d] Error: no command found in this chunk of lines.",
               buffbegin);
        ERROR1("Test suite `%s': NOK (syntax error)", testsuite_name);
        rctx_armageddon(rctx, 1);
      }
      if (rctx->cmd)
        rctx_start();

      continue;
    }

    /* Deal with \ at the end of the line, and call handle_line on result */
    int to_be_continued = 0;
    if (linelen > 1 && line[linelen - 2] == '\\') {
      if (linelen > 2 && line[linelen - 3] == '\\') {
        /* Damn. Escaped \ */
        line[linelen - 2] = '\n';
        line[linelen - 1] = '\0';
      } else {
        to_be_continued = 1;
        line[linelen - 2] = '\0';
        linelen -= 2;
        if (!buff->used)
          buffbegin = line_num;
      }
    }

    if (buff->used || to_be_continued) {
      xbt_strbuff_append(buff, line);

      if (!to_be_continued) {
        snprintf(file_pos, 256, "%s:%d", filename, buffbegin);
        handle_line(file_pos, buff->data);
        xbt_strbuff_empty(buff);
      }

    } else {
      snprintf(file_pos, 256, "%s:%d", filename, line_num);
      handle_line(file_pos, line);
    }
  }
  /* Check that last command of the file ran well */
  if (rctx->cmd)
    rctx_start();

  /* Wait all background commands */

  rctx_free(rctx);

  /* Clear buffers */
  if (line)
    free(line);
  xbt_strbuff_free(buff);

}

static void parse_environ()
{
  char *p;
  int i;
  env = xbt_dict_new();
  for (i = 0; environ[i]; i++) {
    p = environ[i];
    char *eq = strchr(p, '=');
    char *key = bprintf("%.*s", (int) (eq - p), p);
    xbt_dict_set(env, key, xbt_strdup(eq + 1), xbt_free_f);
    free(key);
  }
}

int main(int argc, char *argv[])
{

  FILE *IN;
  int i;

  /* Ignore pipe issues.
     They will show up when we try to send data to dead buddies,
     but we will stop doing so when we're done with provided input */
  struct sigaction newact;
  memset(&newact, 0, sizeof(newact));
  newact.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &newact, NULL);

  xbt_init(&argc, argv);
  rctx_init();
  parse_environ();

  /* Get args */
  for (i = 1; i < argc; i++) {
    if (!strncmp(argv[i], "--cd", strlen("--cd") + 1)) {
      if (i == argc - 1) {
        ERROR0("--cd argument requires an argument");
        exit(1);
      }
      if (chdir(argv[i + 1])) {
        ERROR2("Cannot change directory to %s: %s", argv[i + 1],
               strerror(errno));
        exit(1);
      }
      INFO1("Change directory to %s", argv[i + 1]);
      memmove(argv + i, argv + i + 2, argc - i - 1);
      argc -= 2;
    }
  }

  /* Find the description file */
  if (argc == 1) {
    INFO0("Test suite from stdin");
    testsuite_name = xbt_strdup("(stdin)");
    handle_suite("stdin", stdin);
    INFO0("Test suite from stdin OK");

  } else {
    for (i = 1; i < argc; i++) {
      char *suitename = xbt_strdup(argv[i]);
      if (!strcmp("./", suitename))
        memmove(suitename, suitename + 2, strlen(suitename + 2));

      if (!strcmp(".tesh", suitename + strlen(suitename) - 5))
        suitename[strlen(suitename) - 5] = '\0';

      INFO1("Test suite `%s'", suitename);
      testsuite_name = suitename;
      IN = fopen(argv[i], "r");
      if (!IN) {
        perror(bprintf("Impossible to open the suite file `%s'", argv[i]));
        ERROR1("Test suite `%s': NOK (system error)", testsuite_name);
        rctx_armageddon(rctx, 1);
      }
      handle_suite(suitename, IN);
      rctx_wait_bg();
      fclose(IN);
      INFO1("Test suite `%s' OK", suitename);
      free(suitename);
    }
  }

  rctx_exit();
  xbt_exit();
  return 0;
}
