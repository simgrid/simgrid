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

#include "portable.h"
#include "xbt/sysdep.h"
#include "xbt/function_types.h"
#include "xbt/log.h"

#include <sys/types.h>
#include <sys/wait.h>

/**
 ** Buffer code
 **/
typedef struct {
  char *data;
  int used,size;
} buff_t;

static void buff_empty(buff_t *b) {
  b->used=0;
  b->data[0]='\n';
  b->data[1]='\0';
}
static buff_t *buff_new(void) {
  buff_t *res=malloc(sizeof(buff_t));
  res->data=malloc(512);
  res->size=512;
  buff_empty(res);
  return res;
}
static void buff_free(buff_t *b) {
  if (b) {
    if (b->data)
      free(b->data);
    free(b);
  }
}
static void buff_append(buff_t *b, char *toadd) {
  int addlen=strlen(toadd);
  int needed_space=b->used+addlen+1;

  if (needed_space > b->size) {
    b->data = realloc(b->data, needed_space);
    b->size = needed_space;
  }
  strcpy(b->data+b->used, toadd);
  b->used += addlen;  
}
static void buff_chomp(buff_t *b) {
  while (b->data[b->used] == '\n') {
    b->data[b->used] = '\0';
    if (b->used)
      b->used--;
  }
}
/**
 ** Options
 **/
int timeout_value = 5; /* child timeout value */
int expected_signal=0; /* !=0 if the following command should raise a signal */
int expected_return=0; /* the exepeted return code of following command */
int verbose=0; /* wheather we should wine on problems */

/**
 ** Dealing with timeouts
 **/
int timeouted;
static void timeout_handler(int sig) {
  timeouted = 1;
}
/**
 ** Dealing with timeouts
 **/
int brokenpipe;
static void pipe_handler(int sig) {
  brokenpipe = 1;
}

/**
 ** Launching a child
 **/
buff_t *input;
buff_t *output_wanted;
buff_t *output_got;

static void check_output() {
  if (output_wanted->used==0 
      && output_got->used==0)
    return;
  buff_chomp(output_got);
  buff_chomp(output_wanted);

  if (   output_got->used != output_wanted->used
      || strcmp(output_got->data, output_wanted->data)) {
    fprintf(stderr,"Output don't match expectations\n");
    fprintf(stderr,">>>>> Expected %d chars:\n%s\n<<<<< Expected\n",
	    output_wanted->used,output_wanted->data);
    fprintf(stderr,">>>>> Got %d chars:\n%s\n<<<<< Got\n",
	    output_got->used,output_got->data);
    exit(2);
  }
  buff_empty(output_wanted);
  buff_empty(output_got);
  
}

static void exec_cmd(char *cmd) {
  int child_stdin[2];
  int child_stdout[2];

  if (pipe(child_stdin) || pipe(child_stdout)) {
    perror("Cannot open the pipes");
    exit(4);
  }

  int pid=fork();
  if (pid<0) {
    perror("Cannot fork the command");
    exit(4);
  }

  if (pid) { /* father */
    char buffout[4096];
    int posw,posr;
    int status;
    close(child_stdin[0]);
    fcntl(child_stdin[1], F_SETFL, O_NONBLOCK);
    close(child_stdout[1]);
    fcntl(child_stdout[0], F_SETFL, O_NONBLOCK);

    brokenpipe = 0;
    for (posw=0; posw<input->used && !brokenpipe; ) {
      int got;
      //      fprintf(stderr,"Still %d chars to write\n",input->used-posw);
      got=write(child_stdin[1],input->data+posw,input->used-posw);
      if (got>0)
	posw+=got;
      if (got<0 && errno!=EINTR && errno!=EAGAIN && errno!=EPIPE) {
	perror("Error while writing input to child");
	exit(4);
      }
      //      fprintf(stderr,"written %d chars so far\n",posw);

      posr=read(child_stdout[0],&buffout,4096);
      //      fprintf(stderr,"got %d chars\n",posr);
      if (posr<0 && errno!=EINTR && errno!=EAGAIN) {
	perror("Error while reading output of child");
	exit(4);
      }
      if (posr>0) {
	buffout[posr]='\0';      
	buff_append(output_got,buffout);
      }
       
      if (got <= 0 && posr <= 0)
	 usleep(100);
    }
    input->data[0]='\0';
    input->used=0;
    close(child_stdin[1]);

    timeouted = 0;
    alarm(timeout_value);
    do {
      posr=read(child_stdout[0],&buffout,4096);
      if (posr<0 && errno!=EINTR && errno!=EAGAIN) {
	perror("Error while reading output of child");
	exit(4);
      }
      if (posr>0) {
	buffout[posr]='\0';
	buff_append(output_got,buffout);
      } else {
	usleep(100);
      }
    } while (!timeouted && posr!=0);

    /* Check for broken pipe */
    if (brokenpipe && verbose) {
      fprintf(stderr,"Warning: Child did not consume all its input (I got broken pipe)\n");
    }

    /* Check for timeouts */
    if (timeouted) {
      fprintf(stderr,"Child timeouted (waited %d sec)\n",timeout_value);
      exit(3);
    }
    alarm(0);
      

    /* Wait for child, and check why it terminated */
    wait(&status);

    if (WIFSIGNALED(status) && WTERMSIG(status) != expected_signal) {
      fprintf(stderr,"Child got signal %d instead of signal %d\n",
	      WTERMSIG(status), expected_signal);
      exit(WTERMSIG(status)+4);
    }
    if (!WIFSIGNALED(status) && expected_signal != 0) {
      fprintf(stderr,"Child didn't got expected signal %d\n",
	      expected_signal);
      exit(5);
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) != expected_return ) {
      if (expected_return) 
	fprintf(stderr,"Child returned code %d instead of %d\n",
		WEXITSTATUS(status), expected_return);
      else
	fprintf(stderr,"Child returned code %d\n", WEXITSTATUS(status));
      exit(40+WEXITSTATUS(status));
    }
    expected_return = expected_signal = 0;

  } else { /* child */

    close(child_stdin[1]);
    close(child_stdout[0]);
    dup2(child_stdin[0],0);
    close(child_stdin[0]);
    dup2(child_stdout[1],1);
    dup2(child_stdout[1],2);
    close(child_stdout[1]);

    execlp ("/bin/sh", "sh", "-c", cmd, NULL);
  }
}

static void run_cmd(char *cmd) {
  if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
    int pos = 2;
    /* Search end */
    pos = strlen(cmd)-1;
    while (cmd[pos] == '\n' || cmd[pos] == ' ' || cmd[pos] == '\t')
      cmd[pos--] = '\0';
    /* search begining */
    pos = 2;
    while (cmd[pos++] == ' ');
    pos--;
    //    fprintf(stderr,"Saw cd '%s'\n",cmd+pos);
    if (chdir(cmd+pos)) {
      perror("Chdir failed");
      exit(4);
    }
    
  } else {
    exec_cmd(cmd);
  }
}

static void handle_line(int nl, char *line) {
  
  // printf("%d: %s",nl,line);  fflush(stdout);
  switch (line[0]) {
  case '#': break;
  case '$': 
    check_output(); /* Check that last command ran well */
     
    printf("[%d] %s",nl,line);
    fflush(stdout);
    run_cmd(line+2);
    break;
    
  case '<':
    buff_append(input,line+2);
    break;

  case '>':
    buff_append(output_wanted,line+2);
    break;

  case '!':
    if (!strncmp(line+2,"set timeout ",strlen("set timeout "))) {
      timeout_value=atoi(line+2+strlen("set timeout"));
      printf("[%d] (new timeout value: %d)\n",
	     nl,timeout_value);

    } else if (!strncmp(line+2,"expect signal ",strlen("expect signal "))) {
      expected_signal = atoi(line+2+strlen("expect signal "));
      printf("[%d] (next command must raise signal %d)\n", 
	     nl, expected_signal);

    } else if (!strncmp(line+2,"expect return ",strlen("expect return "))) {
      expected_return = atoi(line+2+strlen("expect return "));
      printf("[%d] (next command must return code %d)\n",
	     nl, expected_return);
       
    } else if (!strncmp(line+2,"verbose on",strlen("verbose on"))) {
      verbose = 1;
      printf("[%d] (increase verbosity)\n", nl);
       
    } else if (!strncmp(line+2,"verbose off",strlen("verbose off"))) {
      verbose = 1;
      printf("[%d] (decrease verbosity)\n", nl);

    } else {
      fprintf(stderr,"%d: Malformed metacommand: %s",nl,line);
      exit(1);
    }
    break;

  case 'p':
    printf("[%d] %s",nl,line+2);
    break;

  default:
    fprintf(stderr,"Syntax error line %d: %s",nl, line);
    exit(1);
    break;
  }
}

static int handle_suite(FILE* IN) {
  int len;
  char * line = NULL;
  int line_num=0;

  buff_t *buff=buff_new();
  int buffbegin = 0;   

  while (getline(&line,(size_t*) &len, IN) != -1) {
    line_num++;

    /* Count the line length while checking wheather it's blank */
    int blankline=1;
    int linelen = 0;    
    while (line[linelen] != '\0') {
      if (line[linelen] != ' ' && line[linelen] != '\t' && line[linelen]!='\n')
	blankline = 0;
      linelen++;
    }
    
    if (blankline)
      continue;

    /* Deal with \ at the end of the line, and call handle_line on result */
    int to_be_continued = 0;
    if (linelen>1 && line[linelen-2]=='\\') {
      if (linelen>2 && line[linelen-3] == '\\') {
	/* Damn. Escaped \ */
	line[linelen-2] = '\n';
	line[linelen-1] = '\0';
      } else {
	to_be_continued = 1;
	line[linelen-2] = '\0';
	linelen -= 2;  
	if (!buff->used)
	  buffbegin = line_num;
      }
    }

    if (buff->used || to_be_continued) { 
      buff_append(buff,line);

      if (!to_be_continued) {
	handle_line(buffbegin, buff->data);    
	buff_empty(buff);
      }
	
    } else {
      handle_line(line_num,line);    
    }
  }
  check_output(); /* Check that last command ran well */

  /* Clear buffers */
  if (line)
    free(line);
  buff_free(buff);
  return 1;
}

int main(int argc,char *argv[]) {

  /* Setup the signal handlers */
  struct sigaction newact,oldact;
  memset(&newact,0,sizeof(newact));
  newact.sa_handler=timeout_handler;
  sigaction(SIGALRM,&newact,&oldact);

  newact.sa_handler=pipe_handler;
  sigaction(SIGPIPE,&newact,&oldact);
   
  /* Setup the input/output buffers */
  input=buff_new();
  output_wanted=buff_new();
  output_got=buff_new();

  /* Find the description file */
  FILE *IN;

  if (argc == 1) {
    printf("Test suite from stdin\n");fflush(stdout);
    handle_suite(stdin);
    fprintf(stderr,"Test suite from stdin OK\n");
     
  } else {
    int i;
     
    for (i=1; i<argc; i++) {
       printf("Test suite `%s'\n",argv[i]);fflush(stdout);
       IN=fopen(argv[i], "r");
       if (!IN) {
	  perror(bprintf("Impossible to open the suite file `%s'",argv[i]));
	  exit(1);
       }
       handle_suite(IN);
//       fclose(IN); ->leads to segfault on amd64...
       fprintf(stderr,"Test suite `%s' OK\n",argv[i]);
    }
  }
   
  buff_free(input);
  buff_free(output_wanted);
  buff_free(output_got);
  return 0;  
}
