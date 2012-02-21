/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h" //For getline, keep that include first

#include "msg_private.h"
#include "xbt/str.h"
#include "xbt/dynar.h"
#include "xbt/replay_trace_reader.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_action, msg,
                                "MSG actions for trace driven simulation");

static xbt_dict_t action_funs;
static xbt_dict_t action_queues;

/* To split the file if a unique one is given (specific variable for the other case live in runner()) */
static FILE *action_fp = NULL;
static char *action_line = NULL;
static size_t action_len = 0;

static const char **action_get_action(char *name);

/** \ingroup msg_actions
 * \brief Registers a function to handle a kind of action
 *
 * Registers a function to handle a kind of action
 * This table is then used by #MSG_action_trace_run
 *
 * The argument of the function is the line describing the action, splitted on spaces with xbt_str_split_quoted()
 *
 * \param name the reference name of the action.
 * \param code the function; prototype given by the type: void...(xbt_dynar_t action)
 */
void MSG_action_register(const char *action_name, msg_action_fun function)
{
  xbt_dict_set(action_funs, action_name, function, NULL);
}

/** \ingroup msg_actions
 * \brief Unregisters a function, which handled a kind of action
 *
 * \param name the reference name of the action.
 */
void MSG_action_unregister(const char *action_name)
{
  xbt_dict_remove(action_funs, action_name);
}

static int MSG_action_runner(int argc, char *argv[])
{
  const char **evt;
  if (action_fp) {              // A unique trace file

    while ((evt = action_get_action(argv[0]))) {
      msg_action_fun function =
        (msg_action_fun)xbt_dict_get(action_funs, evt[1]);
      function(evt);
      free(evt);
    }
  } else {                      // Should have got my trace file in argument
    xbt_assert(argc >= 2,
                "No '%s' agent function provided, no simulation-wide trace file provided to MSG_action_trace_run(), "
                "and no process-wide trace file provided in deployment file. Aborting.",
                argv[0]
        );
    xbt_replay_trace_reader_t reader = xbt_replay_trace_reader_new(argv[1]);
    while ((evt=xbt_replay_trace_reader_get(reader))) {
      if (!strcmp(argv[0],evt[0])) {
        msg_action_fun function =
          (msg_action_fun)xbt_dict_get(action_funs, evt[1]);
        function(evt);
        free(evt);
      } else {
        XBT_WARN("%s: Ignore trace element not for me",
              xbt_replay_trace_reader_position(reader));
      }
    }
    xbt_replay_trace_reader_free(&reader);
  }
  return 0;
}

void _MSG_action_init()
{
  action_funs = xbt_dict_new_homogeneous(NULL);
  action_queues = xbt_dict_new_homogeneous(NULL);
  MSG_function_register_default(MSG_action_runner);
}

void _MSG_action_exit()
{
  xbt_dict_free(&action_queues);
  xbt_dict_free(&action_funs);
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
    while (getline(&action_line, &action_len, action_fp) != -1) {
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

/** \ingroup msg_actions
 * \brief A trace loader
 *
 *  If path!=NULL, load a trace file containing actions, and execute them.
 *  Else, assume that each process gets the path in its deployment file
 */
MSG_error_t MSG_action_trace_run(char *path)
{
  MSG_error_t res;
  char *name;
  xbt_dynar_t todo;
  xbt_dict_cursor_t cursor;

  if (path) {
    action_fp = fopen(path, "r");
    xbt_assert(action_fp != NULL, "Cannot open %s: %s", path,
                strerror(errno));
  }
  res = MSG_main();

  if (!xbt_dict_is_empty(action_queues)) {
    XBT_WARN
        ("Not all actions got consumed. If the simulation ended successfully (without deadlock), you may want to add new processes to your deployment file.");


    xbt_dict_foreach(action_queues, cursor, name, todo) {
      XBT_WARN("Still %lu actions for %s", xbt_dynar_length(todo), name);
    }
  }

  free(action_line);
  if (path)
    fclose(action_fp);
  xbt_dict_free(&action_queues);
  action_queues = xbt_dict_new_homogeneous(NULL);

  return res;
}
