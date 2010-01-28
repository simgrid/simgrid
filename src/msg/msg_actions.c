/* Copyright (c) 2009 The SimGrid team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"     /* getline */
#include "msg/private.h"
#include "xbt/str.h"
#include "xbt/dynar.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_action,msg,"MSG actions for trace driven simulation");

static xbt_dict_t action_funs;
static xbt_dict_t action_queues;

static xbt_dynar_t action_get_action(char *name);

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
  xbt_dynar_t evt=NULL;

  while ((evt = action_get_action(argv[0]))) {
    msg_action_fun function =
        xbt_dict_get(action_funs, xbt_dynar_get_as(evt, 1, char *));
    (*function) (evt);
    xbt_dynar_free(&evt);
  }

  return 0;
}

void _MSG_action_init()
{
  action_funs = xbt_dict_new();
  action_queues = xbt_dict_new();
  MSG_function_register_default(MSG_action_runner);
}

void _MSG_action_exit() {
  xbt_dict_free(&action_queues);
  xbt_dict_free(&action_funs);
}

static FILE *action_fp=NULL;
static char *action_line = NULL;
static size_t action_len = 0;

static xbt_dynar_t action_get_action(char *name) {
  ssize_t read;
  xbt_dynar_t evt=NULL;


  xbt_dynar_t myqueue = xbt_dict_get_or_null(action_queues,name);
  if (myqueue==NULL || xbt_dynar_length(myqueue)==0) { // nothing stored for me. Read the file further

    if (action_fp==NULL) { // File closed now. There's nothing more to read. I'm out of here
      goto todo_done;
    }

    // Read lines until I reach something for me (which breaks in loop body) or end of file
    while ((read = getline(&action_line, &action_len, action_fp)) != -1) {
      // cleanup and split the string I just read
      char *comment = strchr(action_line, '#');
      if (comment != NULL)
        *comment = '\0';
      xbt_str_trim(action_line, NULL);
      if (action_line[0] == '\0')
        continue;
      evt = xbt_str_split_quoted(action_line);

      // if it's for me, I'm done
      char *evtname = xbt_dynar_get_as(evt, 0, char *);
      if (!strcmp(name,evtname)) {
        return evt;
      } else {
        // Else, I have to store it for the relevant colleague
        xbt_dynar_t otherqueue = xbt_dict_get_or_null(action_queues,evtname);
        if (otherqueue == NULL) { // Damn. Create the queue of that guy
          otherqueue = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
          xbt_dict_set(action_queues ,evtname, otherqueue, NULL);
        }
        xbt_dynar_push(otherqueue,&evt);
      }
    }
    goto todo_done; // end of file reached in vain while searching for more work
  } else {
    // Get something from my queue and return it
    xbt_dynar_shift(myqueue,&evt);
    return evt;
  }


  todo_done: // I did all my actions for me in the file. cleanup before leaving
  if (myqueue != NULL) {
    xbt_dynar_free(&myqueue);
    xbt_dict_remove(action_queues,name);
  }
  return NULL;
}

/** \ingroup msg_actions
 * \brief A trace loader
 *
 *  Load a trace file containing actions, and execute them.
 */
MSG_error_t MSG_action_trace_run(char *path)
{
  MSG_error_t res;

  action_fp = fopen(path, "r");
  xbt_assert2(action_fp != NULL, "Cannot open %s: %s", path, strerror(errno));

  res = MSG_main();

  if (xbt_dict_size(action_queues)) {
    WARN0("Not all actions got consumed. If the simulation ended successfully (without deadlock), you may want to add new processes to your deployment file.");
    xbt_dict_cursor_t cursor;
    char *name;
    xbt_dynar_t todo;

    xbt_dict_foreach(action_queues,cursor,name,todo) {
      WARN2("Still %lu actions for %s",xbt_dynar_length(todo),name);
    }
  }

  if (action_line)
    free(action_line);
  fclose(action_fp);
  xbt_dict_free(&action_queues);
  action_queues = xbt_dict_new();

  return res;
}
