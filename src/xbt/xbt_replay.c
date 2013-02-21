/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "internal_config.h"
#include <errno.h>
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/replay.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(replay,xbt,"Replay trace reader");

typedef struct s_replay_reader {
  FILE *fp;
  char *line;
  size_t line_len;
  char *position; /* stable storage */
  char *filename; int linenum;
} s_xbt_replay_reader_t;

FILE *action_fp;

xbt_dict_t action_funs;
xbt_dict_t action_queues;

static char *action_line = NULL;
static size_t action_len = 0;

static const char **action_get_action(char *name);

xbt_replay_reader_t xbt_replay_reader_new(const char *filename)
{
  xbt_replay_reader_t res = xbt_new0(s_xbt_replay_reader_t,1);
  res->fp = fopen(filename, "r");
  xbt_assert(res->fp != NULL, "Cannot open %s: %s", filename,
      strerror(errno));
  res->filename = xbt_strdup(filename);
  return res;
}

const char *xbt_replay_reader_position(xbt_replay_reader_t reader)
{
  free(reader->position);
  reader->position = bprintf("%s:%d",reader->filename,reader->linenum);
  return reader->position;
}

const char **xbt_replay_reader_get(xbt_replay_reader_t reader)
{
  ssize_t read;
  xbt_dynar_t d;
  read = xbt_getline(&reader->line, &reader->line_len, reader->fp);
  //XBT_INFO("got from trace: %s",reader->line);
  reader->linenum++;
  if (read==-1)
    return NULL; /* end of file */
  char *comment = strchr(reader->line, '#');
  if (comment != NULL)
    *comment = '\0';
  xbt_str_trim(reader->line, NULL);
  if (reader->line[0] == '\0')
    return xbt_replay_reader_get(reader); /* Get next line */

  d=xbt_str_split_quoted_in_place(reader->line);
  if (xbt_dynar_is_empty(d)) {
    xbt_dynar_free(&d);
    return xbt_replay_reader_get(reader); /* Get next line */
  }
  return xbt_dynar_to_array(d);
}

void xbt_replay_reader_free(xbt_replay_reader_t *reader)
{
  free((*reader)->filename);
  free((*reader)->position);
  fclose((*reader)->fp);
  free((*reader)->line);
  free(*reader);
  *reader=NULL;
}

/**
 * \ingroup XBT_replay
 * \brief Registers a function to handle a kind of action
 *
 * Registers a function to handle a kind of action
 * This table is then used by \ref xbt_replay_action_runner
 *
 * The argument of the function is the line describing the action, splitted on spaces with xbt_str_split_quoted()
 *
 * \param action_name the reference name of the action.
 * \param function prototype given by the type: void...(xbt_dynar_t action)
 */
void xbt_replay_action_register(const char *action_name, action_fun function)
{
  xbt_dict_set(action_funs, action_name, function, NULL);
}

/** \ingroup XBT_replay
 * \brief Unregisters a function, which handled a kind of action
 *
 * \param action_name the reference name of the action.
 */
void xbt_replay_action_unregister(const char *action_name)
{
  xbt_dict_remove(action_funs, action_name);
}

void _xbt_replay_action_init(void)
{
  action_funs = xbt_dict_new_homogeneous(NULL);
  action_queues = xbt_dict_new_homogeneous(NULL);
}

void _xbt_replay_action_exit(void)
{
  xbt_dict_free(&action_queues);
  xbt_dict_free(&action_funs);
  free(action_line);
}

/**
 * \ingroup XBT_replay
 * \brief TODO

 * \param argc argc .
 * \param argv argv
 */
int xbt_replay_action_runner(int argc, char *argv[])
{
  const char **evt;
  int i;
  if (action_fp) {              // A unique trace file

    while ((evt = action_get_action(argv[0]))) {
      action_fun function =
        (action_fun)xbt_dict_get(action_funs, evt[1]);
      function(evt);
      for (i=0;evt[i]!= NULL;i++)
        free((char*)evt[i]);
      free(evt);
    }
  } else {                      // Should have got my trace file in argument
    xbt_assert(argc >= 2,
                "No '%s' agent function provided, no simulation-wide trace file provided, "
                "and no process-wide trace file provided in deployment file. Aborting.",
                argv[0]
        );
    xbt_replay_reader_t reader = xbt_replay_reader_new(argv[1]);
    while ((evt=xbt_replay_reader_get(reader))) {
      if (!strcmp(argv[0],evt[0])) {
        action_fun function =
          (action_fun)xbt_dict_get(action_funs, evt[1]);
        function(evt);
        free(evt);
      } else {
        XBT_WARN("%s: Ignore trace element not for me",
              xbt_replay_reader_position(reader));
      }
    }
    xbt_replay_reader_free(&reader);
  }
  return 0;
}


static const char **action_get_action(char *name)
{
  xbt_dynar_t evt = NULL;
  char *evtname = NULL;

  xbt_dynar_t myqueue = xbt_dict_get_or_null(action_queues, name);
  if (myqueue == NULL || xbt_dynar_is_empty(myqueue)) {      // nothing stored for me. Read the file further

    if (action_fp == NULL) {    // File closed now. There's nothing more to read. I'm out of here
      goto todo_done;
    }
    // Read lines until I reach something for me (which breaks in loop body)
    // or end of file reached
    while (xbt_getline(&action_line, &action_len, action_fp) != -1) {
      // cleanup and split the string I just read
      char *comment = strchr(action_line, '#');
      if (comment != NULL)
        *comment = '\0';
      xbt_str_trim(action_line, NULL);
      if (action_line[0] == '\0')
        continue;
      /* we cannot split in place here because we parse&store several lines for
       * the colleagues... */
      evt = xbt_str_split_quoted(action_line);

      // if it's for me, I'm done
      evtname = xbt_dynar_get_as(evt, 0, char *);
      if (!strcmp(name, evtname)) {
        return xbt_dynar_to_array(evt);
      } else {
        // Else, I have to store it for the relevant colleague
        xbt_dynar_t otherqueue =
            xbt_dict_get_or_null(action_queues, evtname);
        if (otherqueue == NULL) {       // Damn. Create the queue of that guy
          otherqueue =
              xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
          xbt_dict_set(action_queues, evtname, otherqueue, NULL);
        }
        xbt_dynar_push(otherqueue, &evt);
      }
    }
    goto todo_done;             // end of file reached while searching in vain for more work
  } else {
    // Get something from my queue and return it
    xbt_dynar_shift(myqueue, &evt);
    return xbt_dynar_to_array(evt);
  }


  // I did all my actions for me in the file (either I closed the file, or a colleague did)
  // Let's cleanup before leaving
todo_done:
  if (myqueue != NULL) {
    xbt_dynar_free(&myqueue);
    xbt_dict_remove(action_queues, name);
  }
  return NULL;
}
