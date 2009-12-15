/* Copyright (c) 2009 The SimGrid team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"     /* getline */
#include "msg/private.h"
#include "xbt/str.h"
#include "xbt/dynar.h"

static xbt_dict_t action_funs;
static xbt_dynar_t action_list;


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
  xbt_dynar_t evt;
  unsigned int cursor;

  xbt_dynar_foreach(action_list, cursor, evt) {
    if (!strcmp(xbt_dynar_get_as(evt, 0, char *), argv[0])) {
      msg_action_fun function =
        xbt_dict_get(action_funs, xbt_dynar_get_as(evt, 1, char *));
      (*function) (evt);
    }
  }

  return 0;
}

void _MSG_action_init()
{
  action_funs = xbt_dict_new();
  action_list = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  MSG_function_register_default(MSG_action_runner);
}

/** \ingroup msg_actions
 * \brief A trace loader
 *
 *  Load a trace file containing actions, and execute them.
 */
MSG_error_t MSG_action_trace_run(char *path)
{
  MSG_error_t res;

  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  xbt_dynar_t evt;

  fp = fopen(path, "r");
  xbt_assert2(fp != NULL, "Cannot open %s: %s", path, strerror(errno));

  while ((read = getline(&line, &len, fp)) != -1) {
    char *comment = strchr(line, '#');
    if (comment != NULL)
      *comment = '\0';
    xbt_str_trim(line, NULL);
    if (line[0] == '\0')
      continue;
    evt = xbt_str_split_quoted(line);
    xbt_dynar_push(action_list, &evt);
  }

  if (line)
    free(line);
  fclose(fp);

  res = MSG_main();
  xbt_dynar_free(&action_list);
  action_list = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);

  return res;
}
