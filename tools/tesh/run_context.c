/* run_context -- stuff in which TESH runs a command                        */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "tesh.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>               /* floor */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

int fg_job = 0;
xbt_dynar_t bg_jobs = NULL;
rctx_t armageddon_initiator = NULL;
xbt_os_mutex_t armageddon_mutex = NULL;
struct {
  int num;
  struct sigaction act;
} oldact[3];                    /* SIGINT, SIGQUIT, SIGTERM */

xbt_os_thread_t sigwaiter_thread;
xbt_os_mutex_t sigwaiter_mutex;
xbt_os_cond_t sigwaiter_cond;
int armageddon_requested = 0;
int caught_signum = 0;

/*
 * Module management
 */

static void armageddon_sighandler(int signum)
{
  xbt_os_mutex_acquire(sigwaiter_mutex);
  caught_signum = signum;
  armageddon_requested = 1;
  xbt_os_cond_signal(sigwaiter_cond);
  xbt_os_mutex_release(sigwaiter_mutex);
}

static void *armageddon_sigwaiter(_XBT_GNUC_UNUSED void *arg)
{
  xbt_os_mutex_acquire(sigwaiter_mutex);
  /* Inform main thread that it started. */
  xbt_os_cond_signal(sigwaiter_cond);
  /* Wait for ending signal... */
  xbt_os_cond_wait(sigwaiter_cond, sigwaiter_mutex);
  if (armageddon_requested) {
    XBT_ERROR("Test suite `%s': caught signal %d", testsuite_name, caught_signum);
    rctx_armageddon(rctx, 3);
  }
  xbt_os_mutex_release(sigwaiter_mutex);
  return NULL;
}

static void wait_it(rctx_t rctx)
{
  XBT_VERB("Join thread %p which were running background cmd <%s>",
        rctx->runner, rctx->filepos);
  xbt_os_thread_join(rctx->runner, NULL);
}

static void kill_it(void *r)
{
  rctx_t rctx = *(rctx_t *) r;
  wait_it(rctx);
  rctx_free(rctx);
}

void rctx_init(void)
{
  struct sigaction newact;
  int i;
  fg_job = 0;
  bg_jobs = xbt_dynar_new(sizeof(rctx_t), kill_it);
  armageddon_mutex = xbt_os_mutex_init();
  armageddon_initiator = NULL;
  sigwaiter_mutex = xbt_os_mutex_init();
  sigwaiter_cond = xbt_os_cond_init();
  xbt_os_mutex_acquire(sigwaiter_mutex);
  sigwaiter_thread = xbt_os_thread_create("Armaggedon request waiter",
                                          armageddon_sigwaiter, NULL, NULL);
  /* Wait for thread to start... */
  xbt_os_cond_wait(sigwaiter_cond, sigwaiter_mutex);
  xbt_os_mutex_release(sigwaiter_mutex);
  memset(&newact, 0, sizeof(newact));
  newact.sa_handler = armageddon_sighandler;
  oldact[0].num = SIGINT;
  oldact[1].num = SIGQUIT;
  oldact[2].num = SIGTERM;
  for (i = 0; i < 3; i++)
    sigaction(oldact[i].num, &newact, &oldact[i].act);
}

void rctx_exit(void)
{
  int i;
  for (i = 0; i < 3; i++)
    sigaction(oldact[i].num, &oldact[i].act, NULL);
  xbt_os_cond_signal(sigwaiter_cond);
  xbt_os_thread_join(sigwaiter_thread, NULL);
  xbt_dynar_free(&bg_jobs);
  xbt_os_cond_destroy(sigwaiter_cond);
  xbt_os_mutex_destroy(sigwaiter_mutex);
  xbt_os_mutex_destroy(armageddon_mutex);
}

void rctx_wait_bg(void)
{
  /* Do not use xbt_dynar_free or it will lock the dynar, preventing armageddon
   * from working */
  while (!xbt_dynar_is_empty(bg_jobs)) {
    rctx_t rctx = xbt_dynar_getlast_as(bg_jobs, rctx_t);
    wait_it(rctx);
    xbt_dynar_pop(bg_jobs, &rctx);
    rctx_free(rctx);
  }
  xbt_dynar_reset(bg_jobs);
}

static void rctx_armageddon_kill_one(rctx_t initiator, const char *filepos,
                                     rctx_t rctx)
{
  if (rctx != initiator) {
    XBT_INFO("Kill <%s> because <%s> failed", rctx->filepos, filepos);
    xbt_os_mutex_acquire(rctx->interruption);
    if (!rctx->reader_done) {
      rctx->interrupted = 1;
      kill(rctx->pid, SIGTERM);
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = (100e-6 - floor(100e-6)) * 1e9;
      nanosleep (&ts, NULL);
      kill(rctx->pid, SIGKILL);
    }
    xbt_os_mutex_release(rctx->interruption);
  }
}

void rctx_armageddon(rctx_t initiator, int exitcode)
{
  unsigned int cursor;
  rctx_t job;
  const char *filepos = initiator && initiator->filepos ?
      initiator->filepos : "(master)";

  XBT_DEBUG("Armageddon request by <%s> (exit=%d)", filepos, exitcode);
  xbt_os_mutex_acquire(armageddon_mutex);
  if (armageddon_initiator != NULL) {
    XBT_VERB("Armageddon already started. Let it go");
    xbt_os_mutex_release(armageddon_mutex);
    return;
  }
  XBT_DEBUG("Armageddon request by <%s> got the lock. Let's go amok",
         filepos);
  armageddon_initiator = initiator;
  xbt_os_mutex_release(armageddon_mutex);

  /* Kill foreground command */
  if (fg_job)
    rctx_armageddon_kill_one(initiator, filepos, rctx);

  /* Kill any background commands */
  xbt_dynar_foreach(bg_jobs, cursor, job) {
    rctx_armageddon_kill_one(initiator, filepos, job);
  }

  /* Give runner threads a chance to acknowledge the processes deaths */
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = (10000e-6 - floor(10000e-6)) * 1e9;
  nanosleep (&ts, NULL);
  /* Ensure that nobody is running rctx_wait on exit */
  if (fg_job)
    xbt_os_mutex_acquire(rctx->interruption);
  xbt_dynar_foreach(bg_jobs, cursor, job)
    xbt_os_mutex_acquire(job->interruption);
  XBT_VERB("Shut everything down!");
  exit(exitcode);
}

/*
 * Memory management
 */

void rctx_empty(rctx_t rc)
{
  int i;
  char **env_it;
  void *filepos;

  free(rc->cmd);
  rc->cmd = NULL;
  /* avoid race with rctx_armageddon log messages */
  filepos = rc->filepos;
  rc->filepos = NULL;
  free(filepos);
  for (i = 0, env_it = environ; *env_it; i++, env_it++);
  if (rc->env) {
    for (env_it = rctx->env + i; *env_it; env_it++)
      free(*env_it);
    free(rc->env);
  }
  rc->env_size = i + 1;
  rc->env = malloc(rc->env_size * sizeof(char *));
  memcpy(rc->env, environ, rc->env_size * sizeof(char *));

  rc->is_empty = 1;
  rc->is_background = 0;
  rc->is_stoppable = 0;
  rc->output = e_output_check;
  rc->output_sort = 0;
  rc->brokenpipe = 0;
  rc->timeout = 0;
  rc->interrupted = 0;
  xbt_strbuff_empty(rc->input);
  xbt_strbuff_empty(rc->output_wanted);
  xbt_strbuff_empty(rc->output_got);
}


rctx_t rctx_new()
{
  rctx_t res = xbt_new0(s_rctx_t, 1);

  res->input = xbt_strbuff_new();
  res->output_sort = 0;
  res->output_wanted = xbt_strbuff_new();
  res->output_got = xbt_strbuff_new();
  res->interruption = xbt_os_mutex_init();
  rctx_empty(res);
  return res;
}

void rctx_free(rctx_t rctx)
{
  XBT_DEBUG("RCTX: Free %p", rctx);
  rctx_dump(rctx, "free");
  if (!rctx)
    return;

  free(rctx->cmd);
  free(rctx->filepos);
  if (rctx->env) {
    int i;
    char **env_it;
    for (i = 0, env_it = environ; *env_it; i++, env_it++);
    for (env_it = rctx->env + i; *env_it; env_it++)
      free(*env_it);
    free(rctx->env);
  }
  xbt_os_mutex_destroy(rctx->interruption);
  xbt_strbuff_free(rctx->input);
  xbt_strbuff_free(rctx->output_got);
  xbt_strbuff_free(rctx->output_wanted);
  free(rctx);
}

void rctx_dump(rctx_t rctx, const char *str)
{
  XBT_DEBUG("%s RCTX %p={in%p={%d,%10s}, want={%d,%10s}, out={%d,%10s}}",
         str, rctx,
         rctx->input, rctx->input->used, rctx->input->data,
         rctx->output_wanted->used, rctx->output_wanted->data,
         rctx->output_got->used, rctx->output_got->data);
  XBT_DEBUG("%s RCTX %p=[cmd%p=%10s, pid=%d]",
         str, rctx, rctx->cmd, rctx->cmd, rctx->pid);

}

/*
 * Getting instructions from the file
 */

void rctx_pushline(const char *filepos, char kind, char *line)
{

  switch (kind) {
  case '$':
  case '&':
    if (rctx->cmd) {
      if (!rctx->is_empty) {
        XBT_ERROR
            ("[%s] More than one command in this chunk of lines (previous: %s).\n"
             " Cannot guess which input/output belongs to which command.",
             filepos, rctx->cmd);
        XBT_ERROR("Test suite `%s': NOK (syntax error)", testsuite_name);
        rctx_armageddon(rctx, 1);
        return;
      }
      rctx_start();
      XBT_VERB("[%s] More than one command in this chunk of lines", filepos);
    }
    if (kind == '&')
      rctx->is_background = 1;
    else
      rctx->is_background = 0;

    rctx->cmd = xbt_strdup(line);
    rctx->filepos = xbt_strdup(filepos);
    if (option){
      char *newcmd = bprintf("%s %s", rctx->cmd, option);
      free(rctx->cmd);
      rctx->cmd = newcmd;
    }
    XBT_INFO("[%s] %s%s", filepos, rctx->cmd,
          ((rctx->is_background) ? " (background command)" : ""));

    break;

  case '<':
    rctx->is_empty = 0;
    xbt_strbuff_append(rctx->input, line);
    xbt_strbuff_append(rctx->input, "\n");
    break;

  case '>':
    rctx->is_empty = 0;
    xbt_strbuff_append(rctx->output_wanted, line);
    xbt_strbuff_append(rctx->output_wanted, "\n");
    XBT_DEBUG("wanted:%s",rctx->output_wanted->data);
    break;

  case '!':
    if (rctx->cmd)
      rctx_start();

    if (!strncmp(line, "timeout no", strlen("timeout no"))) {
      XBT_VERB("[%s] (disable timeout)", filepos);
      timeout_value = -1;
    } else if (!strncmp(line, "timeout ", strlen("timeout "))) {
      timeout_value = atoi(line + strlen("timeout"));
      XBT_VERB("[%s] (new timeout value: %d)", filepos, timeout_value);

    } else if (!strncmp(line, "expect signal ", strlen("expect signal "))) {
      rctx->expected_signal = strdup(line + strlen("expect signal "));
      xbt_str_trim(rctx->expected_signal, " \n");
      XBT_VERB("[%s] (next command must raise signal %s)",
            filepos, rctx->expected_signal);

    } else if (!strncmp(line, "expect return ", strlen("expect return "))) {
      rctx->expected_return = atoi(line + strlen("expect return "));
      XBT_VERB("[%s] (next command must return code %d)",
            filepos, rctx->expected_return);

    } else if (!strncmp(line, "output sort", strlen("output sort"))) {
      sort_len = atoi(line + strlen("output sort"));
      if (sort_len==0)
        sort_len=SORT_LEN_DEFAULT;
      rctx->output_sort = 1;
      XBT_VERB("[%s] (sort output of next command)", filepos);

    } else if (!strncmp(line, "output ignore", strlen("output ignore"))) {
      rctx->output = e_output_ignore;
      XBT_VERB("[%s] (ignore output of next command)", filepos);

    } else if (!strncmp(line, "output display", strlen("output display"))) {
      rctx->output = e_output_display;
      XBT_VERB("[%s] (ignore output of next command)", filepos);

    } else if (!strncmp(line, "setenv ", strlen("setenv "))) {
      int len = strlen("setenv ");
      char *eq = strchr(line + len, '=');
      char *key = bprintf("%.*s", (int) (eq - line - len), line + len);
      xbt_dict_set(env, key, xbt_strdup(eq + 1), NULL);
      free(key);

      rctx->env = realloc(rctx->env, ++(rctx->env_size) * sizeof(char *));
      rctx->env[rctx->env_size - 2] = xbt_strdup(line + len);
      rctx->env[rctx->env_size - 1] = NULL;
      XBT_VERB("[%s] setenv %s", filepos, line + len);

    } else {
      XBT_ERROR("%s: Malformed metacommand: %s", filepos, line);
      XBT_ERROR("Test suite `%s': NOK (syntax error)", testsuite_name);
      rctx_armageddon(rctx, 1);
      return;
    }
    break;
  }
}

/*
 * Actually doing the job
 */

/* The IO of the childs are handled by the two following threads
   (one pair per child) */

static void *thread_writer(void *r)
{
  int posw;
  rctx_t rctx = (rctx_t) r;
  for (posw = 0; posw < rctx->input->used && !rctx->brokenpipe;) {
    int got;
    XBT_DEBUG("Still %d chars to write", rctx->input->used - posw);
    got =
        write(rctx->child_to, rctx->input->data + posw,
              rctx->input->used - posw);
    if (got > 0)
      posw += got;
    if (got < 0) {
      if (errno == EPIPE) {
        rctx->brokenpipe = 1;
      } else if (errno != EINTR && errno != EAGAIN && errno != EPIPE) {
        perror("Error while writing input to child");
        XBT_ERROR("Test suite `%s': NOK (system error)", testsuite_name);
        rctx_armageddon(rctx, 4);
        return NULL;
      }
    }
    XBT_DEBUG("written %d chars so far", posw);

    if (got <= 0){
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = (100e-6 - floor(100e-6)) * 1e9;
      nanosleep (&ts, NULL);
    }
  }
  rctx->input->data[0] = '\0';
  rctx->input->used = 0;
  close(rctx->child_to);

  return NULL;
}

static void *thread_reader(void *r)
{
  rctx_t rctx = (rctx_t) r;
  char *buffout = malloc(4096);
  int posr, got_pid;

  do {
    posr = read(rctx->child_from, buffout, 4095);
    if (posr < 0 && errno != EINTR && errno != EAGAIN) {
      perror("Error while reading output of child");
      XBT_ERROR("Test suite `%s': NOK (system error)", testsuite_name);
      rctx_armageddon(rctx, 4);
      return NULL;
    }
    if (posr > 0) {
      buffout[posr] = '\0';
      xbt_strbuff_append(rctx->output_got, buffout);
    } else {
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = (100e-6 - floor(100e-6)) * 1e9;
      nanosleep (&ts, NULL);
    }
  } while (!rctx->timeout && posr != 0);
  free(buffout);

  /* let this thread wait for the child so that the main thread can detect the timeout without blocking on the wait */
  got_pid = waitpid(rctx->pid, &rctx->status, 0);
  if (got_pid != rctx->pid) {
    perror(bprintf
           ("(%s) Cannot wait for the child %s (got pid %d where pid %d were expected;status=%d)",
            xbt_thread_self_name(), rctx->cmd, (int) got_pid,
            (int) rctx->pid, rctx->status));
    XBT_ERROR("Test suite `%s': NOK (system error)", testsuite_name);
    rctx_armageddon(rctx, 4);
    return NULL;
  }

  rctx->reader_done = 1;
  return NULL;
}

/* Special command: mkfile is a built-in creating a file with the input data as content */
static void rctx_mkfile(void)
{
  char *filename = xbt_strdup(rctx->cmd + strlen("mkfile "));
  FILE *OUT;
  int err;
  xbt_str_trim(filename, NULL);
  OUT = fopen(filename, "w");
  if (!OUT) {
    THROWF(system_error, errno, "%s: Cannot create file %s: %s",
           rctx->filepos, filename, strerror(errno));
  }
  err = (fprintf(OUT, "%s", rctx->input->data) < 0);
  err = (fclose(OUT) == -1) || err;
  if (err) {
    THROWF(system_error, errno, "%s: Cannot write file %s: %s",
           rctx->filepos, filename, strerror(errno));
  }
  free(filename);
}

/* function to be called from the child to start the actual process */
static void start_command(rctx_t rctx)
{
  xbt_dynar_t cmd;
  char *binary_name = NULL;
  unsigned int it;
  char *str;
  char **args;
  int errcode;

  if (!strncmp(rctx->cmd, "mkfile ", strlen("mkfile "))) {
    rctx_mkfile();
    /* Valgrind detects memory leaks here.
     * To correct those leaks, we must free objects allocated in main() or in
     * handle_suite(), but we have no more reference to them at this point.
     * A quick and dirty hack to make valgrind happy it to uncomment the
     * following line.
     */
    /* execlp("true", "true", (const char *)0); */
    exit(0);                    /* end the working child */
  }

  cmd = xbt_str_split_quoted(rctx->cmd);
  xbt_dynar_get_cpy(cmd, 0, &binary_name);
  args = xbt_new(char *, xbt_dynar_length(cmd) + 1);
  xbt_dynar_foreach(cmd, it, str) {
    args[it] = xbt_strdup(str);
  }
  args[it] = NULL;
  xbt_dynar_free_container(&cmd);

  /* To search for the right executable path when not trivial */
  struct stat stat_buf;

  /* build the command line */
  if (stat(binary_name, &stat_buf)) {
    /* Damn. binary not in current dir. We'll have to dig the PATH to find it */
    int i;

    for (i = 0; environ[i]; i++) {
      if (!strncmp("PATH=", environ[i], 5)) {
        xbt_dynar_t path = xbt_str_split(environ[i] + 5, ":");

        xbt_dynar_foreach(path, it, str) {
          free(binary_name);
          binary_name = bprintf("%s/%s", str, args[0]);
          if (!stat(binary_name, &stat_buf)) {
            /* Found. */
            XBT_DEBUG("Looked in the PATH for the binary. Found %s",
                   binary_name);
            xbt_dynar_free(&path);
            break;
          }
        }
        xbt_dynar_free(&path);
        if (stat(binary_name, &stat_buf)) {
          /* not found */
          printf("TESH_ERROR Command %s not found\n", args[0]);
          exit(127);
        }
        break;
      }
    }
  } else {
    binary_name = xbt_strdup(args[0]);
  }

  errcode = execve(binary_name, args, rctx->env);
  printf("TESH_ERROR %s: Cannot start %s: %s\n", rctx->filepos, rctx->cmd,
         strerror(errcode));
  exit(127);
}

/* Start a new child, plug the pipes as expected and fire up the
   helping threads. Is also waits for the child to end if this is a
   foreground job, or fire up a thread to wait otherwise. */
void rctx_start(void)
{
  int child_in[2];
  int child_out[2];

  XBT_DEBUG("Cmd before rewriting %s", rctx->cmd);
  char *newcmd = xbt_str_varsubst(rctx->cmd, env);
  free(rctx->cmd);
  rctx->cmd = newcmd;
  XBT_VERB("Start %s %s", rctx->cmd,
        (rctx->is_background ? "(background job)" : ""));
  xbt_os_mutex_acquire(armageddon_mutex);
  if (armageddon_initiator) {
    XBT_VERB("Armageddon in progress. Do not start job.");
    xbt_os_mutex_release(armageddon_mutex);
    return;
  }
  if (pipe(child_in) || pipe(child_out)) {
    perror("Cannot open the pipes");
    XBT_ERROR("Test suite `%s': NOK (system error)", testsuite_name);
    xbt_os_mutex_release(armageddon_mutex);
    rctx_armageddon(rctx, 4);
  }

  rctx->pid = fork();
  if (rctx->pid < 0) {
    perror("Cannot fork the command");
    XBT_ERROR("Test suite `%s': NOK (system error)", testsuite_name);
    xbt_os_mutex_release(armageddon_mutex);
    rctx_armageddon(rctx, 4);
    return;
  }

  if (rctx->pid) {              /* father */
    close(child_in[0]);
    rctx->child_to = child_in[1];

    close(child_out[1]);
    rctx->child_from = child_out[0];

    if (timeout_value > 0)
      rctx->end_time = time(NULL) + timeout_value;
    else
      rctx->end_time = -1;

    rctx->reader_done = 0;
    rctx->reader =
        xbt_os_thread_create("reader", thread_reader, (void *) rctx, NULL);
    rctx->writer =
        xbt_os_thread_create("writer", thread_writer, (void *) rctx, NULL);

  } else {                      /* child */
    close(child_in[1]);
    dup2(child_in[0], 0);
    close(child_in[0]);

    close(child_out[0]);
    dup2(child_out[1], 1);
    dup2(child_out[1], 2);
    close(child_out[1]);

    start_command(rctx);
  }

  rctx->is_stoppable = 1;

  if (!rctx->is_background) {
    fg_job = 1;
    xbt_os_mutex_release(armageddon_mutex);
    rctx_wait(rctx);
    fg_job = 0;
  } else {
    /* Damn. Copy the rctx and launch a thread to handle it */
    rctx_t old = rctx;
    xbt_os_thread_t runner;

    rctx = rctx_new();
    XBT_DEBUG("RCTX: new bg=%p, new fg=%p", old, rctx);

    XBT_DEBUG("Launch a thread to wait for %s %d", old->cmd, old->pid);
    runner = xbt_os_thread_create(old->cmd, rctx_wait, (void *) old, NULL);
    old->runner = runner;
    XBT_VERB("Launched thread %p to wait for %s %d", runner, old->cmd,
          old->pid);
    xbt_dynar_push(bg_jobs, &old);
    xbt_os_mutex_release(armageddon_mutex);
  }
}

/* Helper function to sort the output */
static int cmpstringp(const void *p1, const void *p2) {
  /* Sort only using the sort_len first chars
   * If they are the same, then, sort using pointer address
   * (be stable wrt output of each process)
   */
  const char **s1 = *(const char***)p1;
  const char **s2 = *(const char***)p2;

  XBT_DEBUG("Compare strings '%s' and '%s'", *s1, *s2);

  int res = strncmp(*s1, *s2, sort_len);
  if (res == 0)
    res = s1 > s2 ? 1 : (s1 < s2 ? -1 : 0);
  return res;
}

static void stable_sort(xbt_dynar_t a)
{
  unsigned long len = xbt_dynar_length(a);
  void **b = xbt_new(void*, len);
  unsigned long i;
  for (i = 0 ; i < len ; i++)   /* fill the array b with pointers to strings */
    b[i] = xbt_dynar_get_ptr(a, i);
  qsort(b, len, sizeof *b, cmpstringp); /* sort it */
  for (i = 0 ; i < len ; i++) /* dereference the pointers to get the strings */
    b[i] = *(char**)b[i];
  for (i = 0 ; i < len ; i++)   /* put everything in place */
    xbt_dynar_set_as(a, i, char*, b[i]);
  xbt_free(b);
}

/* Waits for the child to end (or to timeout), and check its
   ending conditions. This is launched from rctx_start but either in main
   thread (for foreground jobs) or in a separate one for background jobs.
   That explains the prototype, forced by xbt_os_thread_create. */

void *rctx_wait(void *r)
{
  rctx_t rctx = (rctx_t) r;
  int errcode = 0;
  int now = time(NULL);

  rctx_dump(rctx, "wait");

  if (!rctx->is_stoppable)
    THROWF(unknown_error, 0, "Cmd '%s' not started yet. Cannot wait it",
           rctx->cmd);

  /* Wait for the child to die or the timeout to happen (or an armageddon to happen) */
  while (!rctx->reader_done
         && (rctx->end_time < 0 || rctx->end_time >= now)) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (100e-6 - floor(100e-6)) * 1e9;
    nanosleep (&ts, NULL);
    now = time(NULL);
  }

  xbt_os_mutex_acquire(rctx->interruption);
  if (!rctx->interrupted && rctx->end_time > 0 && rctx->end_time < now) {
    XBT_INFO("<%s> timeouted. Kill the process.", rctx->filepos);
    rctx->timeout = 1;
    kill(rctx->pid, SIGTERM);
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (100e-6 - floor(100e-6)) * 1e9;
    nanosleep (&ts, NULL);
    kill(rctx->pid, SIGKILL);
  }

  /* Make sure helper threads die.
     Cannot block since they wait for the child we just killed
     if not already dead. */
  xbt_os_thread_join(rctx->writer, NULL);
  xbt_os_thread_join(rctx->reader, NULL);

  /*  xbt_os_mutex_release(rctx->interruption);
     if (rctx->interrupted)
     return NULL;
     xbt_os_mutex_acquire(rctx->interruption); */

  { // Sorting output got
    xbt_dynar_t a = xbt_str_split(rctx->output_got->data, "\n");
    xbt_dynar_t b = xbt_dynar_new(sizeof(char *), NULL);
    unsigned cpt;
    char *str;
    xbt_dynar_foreach(a, cpt, str) {
      if (strncmp(str, "TESH_ERROR ", (sizeof "TESH_ERROR ") - 1) == 0) {
        XBT_CRITICAL("%s", str);
        errcode = 1;
      } else if (coverage &&
                 strncmp(str, "profiling:", (sizeof "profiling:") - 1) == 0) {
        XBT_DEBUG("Remove line [%u]: '%s'", cpt, str);
      } else {
        xbt_dynar_push_as(b, char *, str);
      }
    }

    if (rctx->output_sort) {
      stable_sort(b);
      /* If empty lines moved in first position, remove them */
      while (!xbt_dynar_is_empty(b) && *xbt_dynar_getfirst_as(b, char*) == '\0')
        xbt_dynar_shift(b, NULL);
    }

    if (rctx->output_sort || xbt_dynar_length(b) != xbt_dynar_length(a)) {
      char *newbuf = xbt_str_join(b, "\n");
      strcpy(rctx->output_got->data, newbuf);
      rctx->output_got->used = strlen(newbuf);
      xbt_free(newbuf);
    }

    xbt_dynar_free(&b);
    xbt_dynar_free(&a);
  }

  { // Sorting output wanted
    xbt_dynar_t a = xbt_str_split(rctx->output_wanted->data, "\n");
    xbt_dynar_t b = xbt_dynar_new(sizeof(char *), NULL);
    unsigned cpt;
    char *str;
    xbt_dynar_foreach(a, cpt, str) {
      if (strncmp(str, "TESH_ERROR ", (sizeof "TESH_ERROR ") - 1) == 0) {
        XBT_CRITICAL("%s", str);
        errcode = 1;
      } else if (coverage &&
                 strncmp(str, "profiling:", (sizeof "profiling:") - 1) == 0) {
        XBT_DEBUG("Remove line [%u]: '%s'", cpt, str);
      } else {
        xbt_dynar_push_as(b, char *, str);
      }
    }

    if (rctx->output_sort) {
      stable_sort(b);
      /* If empty lines moved in first position, remove them */
      while (!xbt_dynar_is_empty(b) && *xbt_dynar_getfirst_as(b, char*) == '\0')
        xbt_dynar_shift(b, NULL);
    }

    if (rctx->output_sort || xbt_dynar_length(b) != xbt_dynar_length(a)) {
      char *newbuf = xbt_str_join(b, "\n");
      strcpy(rctx->output_wanted->data, newbuf);
      rctx->output_wanted->used = strlen(newbuf);
      xbt_free(newbuf);
    }

    xbt_dynar_free(&b);
    xbt_dynar_free(&a);
  }
  xbt_strbuff_chomp(rctx->output_got);
  xbt_strbuff_chomp(rctx->output_wanted);
  xbt_strbuff_trim(rctx->output_got);
  xbt_strbuff_trim(rctx->output_wanted);

  /* Check for broken pipe */
  if (rctx->brokenpipe)
    XBT_VERB
        ("Warning: Child did not consume all its input (I got broken pipe)");

  /* Check for timeouts */
  if (rctx->timeout) {
    if (rctx->output_got->data[0])
      XBT_INFO("<%s> Output on timeout:\n%s",
            rctx->filepos, rctx->output_got->data);
    else
      XBT_INFO("<%s> No output before timeout", rctx->filepos);
    XBT_ERROR("Test suite `%s': NOK (<%s> timeout after %d sec)",
           testsuite_name, rctx->filepos, timeout_value);
    XBT_DEBUG("<%s> Interrupted = %d", rctx->filepos, (int)rctx->interrupted);
    if (!rctx->interrupted) {
      xbt_os_mutex_release(rctx->interruption);
      rctx_armageddon(rctx, 3);
      return NULL;
    }
  }

  XBT_DEBUG("RCTX=%p (pid=%d)", rctx, rctx->pid);
  XBT_DEBUG("Status(%s|%d)=%d", rctx->cmd, rctx->pid, rctx->status);

  if (!rctx->interrupted) {
    if (WIFSIGNALED(rctx->status) && !rctx->expected_signal) {
      XBT_ERROR("Test suite `%s': NOK (<%s> got signal %s)",
             testsuite_name, rctx->filepos,
             signal_name(WTERMSIG(rctx->status), NULL));
      errcode = WTERMSIG(rctx->status) + 4;
    }

    if (WIFSIGNALED(rctx->status) && rctx->expected_signal &&
        strcmp(signal_name(WTERMSIG(rctx->status), rctx->expected_signal),
               rctx->expected_signal)) {
      XBT_ERROR("Test suite `%s': NOK (%s got signal %s instead of %s)",
             testsuite_name, rctx->filepos,
             signal_name(WTERMSIG(rctx->status), rctx->expected_signal),
             rctx->expected_signal);
      errcode = WTERMSIG(rctx->status) + 4;
    }

    if (!WIFSIGNALED(rctx->status) && rctx->expected_signal) {
      XBT_ERROR("Test suite `%s': NOK (child %s expected signal %s)",
             testsuite_name, rctx->filepos, rctx->expected_signal);
      errcode = 5;
    }

    if (WIFEXITED(rctx->status)
        && WEXITSTATUS(rctx->status) != rctx->expected_return) {
      if (rctx->expected_return)
        XBT_ERROR
            ("Test suite `%s': NOK (<%s> returned code %d instead of %d)",
             testsuite_name, rctx->filepos, WEXITSTATUS(rctx->status),
             rctx->expected_return);
      else
        XBT_ERROR("Test suite `%s': NOK (<%s> returned code %d)",
               testsuite_name, rctx->filepos, WEXITSTATUS(rctx->status));
      errcode = 40 + WEXITSTATUS(rctx->status);

    }
    rctx->expected_return = 0;

    free(rctx->expected_signal);
    rctx->expected_signal = NULL;
  }

  if ((errcode && errcode != 1) || rctx->interrupted) {
    /* checking output, and matching */
    xbt_dynar_t a = xbt_str_split(rctx->output_got->data, "\n");
    char *out = xbt_str_join(a, "\n||");
    xbt_dynar_free(&a);
    XBT_INFO("Output of <%s> so far: \n||%s", rctx->filepos, out);
    free(out);
  } else if (rctx->output == e_output_check
             && (rctx->output_got->used != rctx->output_wanted->used
                 || strcmp(rctx->output_got->data,
                           rctx->output_wanted->data))) {
    if (XBT_LOG_ISENABLED(tesh, xbt_log_priority_info)) {
      char *diff =
          xbt_str_diff(rctx->output_wanted->data, rctx->output_got->data);
      XBT_ERROR("Output of <%s> mismatch:\n%s", rctx->filepos, diff);
      free(diff);
    }
    XBT_ERROR("Test suite `%s': NOK (<%s> output mismatch)",
           testsuite_name, rctx->filepos);

    errcode = 2;
  } else if (rctx->output == e_output_ignore) {
    XBT_INFO("(ignoring the output of <%s> as requested)", rctx->filepos);
  } else if (rctx->output == e_output_display) {
    xbt_dynar_t a = xbt_str_split(rctx->output_got->data, "\n");
    char *out = xbt_str_join(a, "\n||");
    xbt_dynar_free(&a);
    XBT_INFO("Here is the (ignored) command output: \n||%s", out);
    free(out);
  }

  if (!rctx->is_background) {
    xbt_os_mutex_acquire(armageddon_mutex);
    /* Don't touch rctx if armageddon is in progress. */
    if (!armageddon_initiator)
      rctx_empty(rctx);
    xbt_os_mutex_release(armageddon_mutex);
  }
  if (errcode) {
    if (!rctx->interrupted) {
      xbt_os_mutex_release(rctx->interruption);
      rctx_armageddon(rctx, errcode);
      return NULL;
    }
  }

  xbt_os_mutex_release(rctx->interruption);
  return NULL;
}
