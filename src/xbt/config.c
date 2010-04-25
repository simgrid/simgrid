/* config - Dictionnary where the type of each variable is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Copyright (c) 2001,2002,2003,2004 Martin Quinson. All rights reserved.   */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/peer.h"

#include "xbt/config.h"         /* prototypes of this module */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_cfg, xbt, "configuration support");

/* xbt_cfgelm_t: the typedef corresponding to a config variable.

   Both data and DTD are mixed, but fixing it now would prevent me to ever
   defend my thesis. */

typedef struct {
  /* Description */
  char *desc;

  /* Allowed type of the variable */
  e_xbt_cfgelm_type_t type;
  int min, max;
  int isdefault:1;

  /* Callbacks */
  xbt_cfg_cb_t cb_set;
  xbt_cfg_cb_t cb_rm;

  /* actual content
     (cannot be an union because type peer uses both str and i) */
  xbt_dynar_t content;
} s_xbt_cfgelm_t, *xbt_cfgelm_t;

static const char *xbt_cfgelm_type_name[xbt_cfgelm_type_count] =
  { "int", "double", "string", "peer", "any" };

/* Internal stuff used in cache to free a variable */
static void xbt_cfgelm_free(void *data);

/* Retrieve the variable we'll modify */
static xbt_cfgelm_t xbt_cfgelm_get(xbt_cfg_t cfg, const char *name,
                                   e_xbt_cfgelm_type_t type);

/*----[ Memory management ]-----------------------------------------------*/

/** @brief Constructor
 *
 * Initialise an config set
 */


xbt_cfg_t xbt_cfg_new(void)
{
  return (xbt_cfg_t) xbt_dict_new();
}

/** \brief Copy an existing configuration set
 *
 * \arg whereto the config set to be created
 * \arg tocopy the source data
 *
 * This only copy the registrations, not the actual content
 */

void xbt_cfg_cpy(xbt_cfg_t tocopy, xbt_cfg_t * whereto)
{
  xbt_dict_cursor_t cursor = NULL;
  xbt_cfgelm_t variable = NULL;
  char *name = NULL;

  DEBUG1("Copy cfg set %p", tocopy);
  *whereto = NULL;
  xbt_assert0(tocopy, "cannot copy NULL config");

  xbt_dict_foreach((xbt_dict_t) tocopy, cursor, name, variable) {
    xbt_cfg_register(whereto, name, variable->desc, variable->type, NULL,
                     variable->min, variable->max, variable->cb_set,
                     variable->cb_rm);
  }
}

/** @brief Destructor */
void xbt_cfg_free(xbt_cfg_t * cfg)
{
  DEBUG1("Frees cfg set %p", cfg);
  xbt_dict_free((xbt_dict_t *) cfg);
}

/** @brief Dump a config set for debuging purpose
 *
 * \arg name The name to give to this config set
 * \arg indent what to write at the begining of each line (right number of spaces)
 * \arg cfg the config set
 */
void xbt_cfg_dump(const char *name, const char *indent, xbt_cfg_t cfg)
{
  xbt_dict_t dict = (xbt_dict_t) cfg;
  xbt_dict_cursor_t cursor = NULL;
  xbt_cfgelm_t variable = NULL;
  char *key = NULL;
  int i;
  int size;
  int ival;
  char *sval;
  double dval;
  xbt_peer_t hval;

  if (name)
    printf("%s>> Dumping of the config set '%s':\n", indent, name);
  xbt_dict_foreach(dict, cursor, key, variable) {

    printf("%s  %s:", indent, key);

    size = xbt_dynar_length(variable->content);
    printf
      ("%d_to_%d_%s. Actual size=%d. prerm=%p,postset=%p, List of values:\n",
       variable->min, variable->max, xbt_cfgelm_type_name[variable->type],
       size, variable->cb_rm, variable->cb_set);

    switch (variable->type) {

    case xbt_cfgelm_int:
      for (i = 0; i < size; i++) {
        ival = xbt_dynar_get_as(variable->content, i, int);
        printf("%s    %d\n", indent, ival);
      }
      break;

    case xbt_cfgelm_double:
      for (i = 0; i < size; i++) {
        dval = xbt_dynar_get_as(variable->content, i, double);
        printf("%s    %f\n", indent, dval);
      }
      break;

    case xbt_cfgelm_string:
      for (i = 0; i < size; i++) {
        sval = xbt_dynar_get_as(variable->content, i, char *);
        printf("%s    %s\n", indent, sval);
      }
      break;

    case xbt_cfgelm_peer:
      for (i = 0; i < size; i++) {
        hval = xbt_dynar_get_as(variable->content, i, xbt_peer_t);
        printf("%s    %s:%d\n", indent, hval->name, hval->port);
      }
      break;

    default:
      printf("%s    Invalid type!!\n", indent);
    }

  }

  if (name)
    printf("%s<< End of the config set '%s'\n", indent, name);
  fflush(stdout);

  xbt_dict_cursor_free(&cursor);
  return;
}

/*
 * free an config element
 */

void xbt_cfgelm_free(void *data)
{
  xbt_cfgelm_t c = (xbt_cfgelm_t) data;

  DEBUG1("Frees cfgelm %p", c);
  if (!c)
    return;
  xbt_free(c->desc);
  xbt_dynar_free(&(c->content));
  free(c);
}

/*----[ Registering stuff ]-----------------------------------------------*/

/** @brief Register an element within a config set
 *
 *  @arg cfg the config set
 *  @arg type the type of the config element
 *  @arg min the minimum
 *  @arg max the maximum
 */

void
xbt_cfg_register(xbt_cfg_t * cfg,
                 const char *name, const char *desc, e_xbt_cfgelm_type_t type,
                 void *default_value, int min, int max, xbt_cfg_cb_t cb_set,
                 xbt_cfg_cb_t cb_rm)
{
  xbt_cfgelm_t res;

  if (*cfg == NULL)
    *cfg = xbt_cfg_new();
  xbt_assert4(type >= xbt_cfgelm_int && type <= xbt_cfgelm_peer,
              "type of %s not valid (%d should be between %d and %d)",
              name, type, xbt_cfgelm_int, xbt_cfgelm_peer);
  res = xbt_dict_get_or_null((xbt_dict_t) * cfg, name);

  if (res) {
    WARN1("Config elem %s registered twice.", name);
    /* Will be removed by the insertion of the new one */
  }

  res = xbt_new(s_xbt_cfgelm_t, 1);
  DEBUG8("Register cfg elm %s (%s) (%d to %d %s (=%d) @%p in set %p)",
         name, desc, min, max, xbt_cfgelm_type_name[type], type, res, *cfg);

  res->desc = xbt_strdup(desc);
  res->type = type;
  res->min = min;
  res->max = max;
  res->cb_set = cb_set;
  res->cb_rm = cb_rm;
  res->isdefault = 1;

  switch (type) {
  case xbt_cfgelm_int:
    res->content = xbt_dynar_new(sizeof(int), NULL);
    if (default_value)
      xbt_dynar_push(res->content, default_value);
    break;

  case xbt_cfgelm_double:
    res->content = xbt_dynar_new(sizeof(double), NULL);
    if (default_value)
      xbt_dynar_push(res->content, default_value);
    break;

  case xbt_cfgelm_string:
    res->content = xbt_dynar_new(sizeof(char *), xbt_free_ref);
    if (default_value)
      xbt_dynar_push(res->content, default_value);
    break;

  case xbt_cfgelm_peer:
    res->content = xbt_dynar_new(sizeof(xbt_peer_t), xbt_peer_free_voidp);
    if (default_value)
      xbt_dynar_push(res->content, default_value);
    break;

  default:
    ERROR1("%d is an invalide type code", type);
  }

  xbt_dict_set((xbt_dict_t) * cfg, name, res, &xbt_cfgelm_free);
}

/** @brief Unregister an element from a config set.
 *
 *  @arg cfg the config set
 *  @arg name the name of the elem to be freed
 *
 *  Note that it removes both the description and the actual content.
 *  Throws not_found when no such element exists.
 */

void xbt_cfg_unregister(xbt_cfg_t cfg, const char *name)
{
  DEBUG2("Unregister elm '%s' from set %p", name, cfg);
  xbt_dict_remove((xbt_dict_t) cfg, name);
}

/**
 * @brief Parse a string and register the stuff described.
 *
 * @arg cfg the config set
 * @arg entry a string describing the element to register
 *
 * The string may consist in several variable descriptions separated by a space.
 * Each of them must use the following syntax: \<name\>:\<min nb\>_to_\<max nb\>_\<type\>
 * with type being one of  'string','int', 'peer' or 'double'.
 *
 * @fixme: this does not allow to set the description
 */

void xbt_cfg_register_str(xbt_cfg_t * cfg, const char *entry)
{
  char *entrycpy = xbt_strdup(entry);
  char *tok;

  int min, max;
  e_xbt_cfgelm_type_t type;
  DEBUG1("Register string '%s'", entry);

  tok = strchr(entrycpy, ':');
  xbt_assert2(tok, "Invalid config element descriptor: %s%s",
              entry, "; Should be <name>:<min nb>_to_<max nb>_<type>");
  *(tok++) = '\0';

  min = strtol(tok, &tok, 10);
  xbt_assert1(tok, "Invalid minimum in config element descriptor %s", entry);

  xbt_assert2(strcmp(tok, "_to_"),
              "Invalid config element descriptor : %s%s",
              entry, "; Should be <name>:<min nb>_to_<max nb>_<type>");
  tok += strlen("_to_");

  max = strtol(tok, &tok, 10);
  xbt_assert1(tok, "Invalid maximum in config element descriptor %s", entry);

  xbt_assert2(*(tok++) == '_',
              "Invalid config element descriptor: %s%s", entry,
              "; Should be <name>:<min nb>_to_<max nb>_<type>");

  for (type = 0;
       type < xbt_cfgelm_type_count
       && strcmp(tok, xbt_cfgelm_type_name[type]); type++);
  xbt_assert2(type < xbt_cfgelm_type_count,
              "Invalid type in config element descriptor: %s%s", entry,
              "; Should be one of 'string', 'int', 'peer' or 'double'.");

  xbt_cfg_register(cfg, entrycpy, NULL, type, NULL, min, max, NULL, NULL);

  free(entrycpy);               /* strdup'ed by dict mechanism, but cannot be const */
}

/** @brief Displays the declared options and their description */
void xbt_cfg_help(xbt_cfg_t cfg)
{
  xbt_dict_cursor_t cursor;
  xbt_cfgelm_t variable;
  char *name;

  int i;
  int size;

  xbt_dict_foreach((xbt_dict_t) cfg, cursor, name, variable) {
    printf("   %s: %s\n", name, variable->desc);
    printf("       Type: %s; ", xbt_cfgelm_type_name[variable->type]);
    if (variable->min != 1 || variable->max != 1) {
      if (variable->min == 0 && variable->max == 0)
        printf("Arity: no bound; ");
      else
        printf("Arity: min:%d to max:%d; ", variable->min, variable->max);
    }
    printf("Current value: ");
    size = xbt_dynar_length(variable->content);

    switch (variable->type) {
      int ival;
      char *sval;
      double dval;
      xbt_peer_t hval;

    case xbt_cfgelm_int:
      for (i = 0; i < size; i++) {
        ival = xbt_dynar_get_as(variable->content, i, int);
        printf("%s%d\n", (i == 0 ? "" : "              "), ival);
      }
      break;

    case xbt_cfgelm_double:
      for (i = 0; i < size; i++) {
        dval = xbt_dynar_get_as(variable->content, i, double);
        printf("%s%f\n", (i == 0 ? "" : "              "), dval);
      }
      break;

    case xbt_cfgelm_string:
      for (i = 0; i < size; i++) {
        sval = xbt_dynar_get_as(variable->content, i, char *);
        printf("%s'%s'\n", (i == 0 ? "" : "              "), sval);
      }
      break;

    case xbt_cfgelm_peer:
      for (i = 0; i < size; i++) {
        hval = xbt_dynar_get_as(variable->content, i, xbt_peer_t);
        printf("%s%s:%d\n", (i == 0 ? "" : "              "), hval->name,
               hval->port);
      }
      break;

    default:
      printf("Invalid type!!\n");
    }

  }
}

/** @brief Check that each variable have the right amount of values */
void xbt_cfg_check(xbt_cfg_t cfg)
{
  xbt_dict_cursor_t cursor;
  xbt_cfgelm_t variable;
  char *name;
  int size;

  xbt_assert0(cfg, "NULL config set.");
  DEBUG1("Check cfg set %p", cfg);

  xbt_dict_foreach((xbt_dict_t) cfg, cursor, name, variable) {
    size = xbt_dynar_length(variable->content);
    if (variable->min > size) {
      xbt_dict_cursor_free(&cursor);
      THROW4(mismatch_error, 0,
             "Config elem %s needs at least %d %s, but there is only %d values.",
             name, variable->min, xbt_cfgelm_type_name[variable->type], size);
    }

    if (variable->max > 0 && variable->max < size) {
      xbt_dict_cursor_free(&cursor);
      THROW4(mismatch_error, 0,
             "Config elem %s accepts at most %d %s, but there is %d values.",
             name, variable->max, xbt_cfgelm_type_name[variable->type], size);
    }
  }

  xbt_dict_cursor_free(&cursor);
}

static xbt_cfgelm_t xbt_cfgelm_get(xbt_cfg_t cfg,
                                   const char *name, e_xbt_cfgelm_type_t type)
{
  xbt_cfgelm_t res = NULL;

  res = xbt_dict_get_or_null((xbt_dict_t) cfg, name);
  if (!res) {
    xbt_cfg_help(cfg);
    THROW1(not_found_error, 0,
           "No registered variable '%s' in this config set", name);
  }

  xbt_assert3(type == xbt_cfgelm_any || res->type == type,
              "You tried to access to the config element %s as an %s, but its type is %s.",
              name,
              xbt_cfgelm_type_name[type], xbt_cfgelm_type_name[res->type]);

  return res;
}

/** @brief Get the type of this variable in that configuration set
 *
 * \arg cfg the config set
 * \arg name the name of the element
 * \arg type the result
 *
 */

e_xbt_cfgelm_type_t xbt_cfg_get_type(xbt_cfg_t cfg, const char *name)
{

  xbt_cfgelm_t variable = NULL;

  variable = xbt_dict_get_or_null((xbt_dict_t) cfg, name);
  if (!variable)
    THROW1(not_found_error, 0,
           "Can't get the type of '%s' since this variable does not exist",
           name);

  INFO1("type in variable = %d", variable->type);

  return variable->type;
}

/*----[ Setting ]---------------------------------------------------------*/
/**  @brief va_args version of xbt_cfg_set
 *
 * \arg cfg config set to fill
 * \arg n   variable name
 * \arg pa  variable value
 *
 * Add some values to the config set.
 */
void xbt_cfg_set_vargs(xbt_cfg_t cfg, const char *name, va_list pa)
{
  char *str;
  int i;
  double d;
  e_xbt_cfgelm_type_t type = 0; /* Set a dummy value to make gcc happy. It cannot get uninitialized */

  xbt_ex_t e;

  TRY {
    type = xbt_cfg_get_type(cfg, name);
  } CATCH(e) {
    if (e.category == not_found_error) {
      xbt_ex_free(e);
      THROW1(not_found_error, 0,
             "Can't set the property '%s' since it's not registered", name);
    }
    RETHROW;
  }

  switch (type) {
  case xbt_cfgelm_peer:
    str = va_arg(pa, char *);
    i = va_arg(pa, int);
    xbt_cfg_set_peer(cfg, name, str, i);
    break;

  case xbt_cfgelm_string:
    str = va_arg(pa, char *);
    xbt_cfg_set_string(cfg, name, str);
    break;

  case xbt_cfgelm_int:
    i = va_arg(pa, int);
    xbt_cfg_set_int(cfg, name, i);
    break;

  case xbt_cfgelm_double:
    d = va_arg(pa, double);
    xbt_cfg_set_double(cfg, name, d);
    break;

  default:
    xbt_assert2(0, "Config element variable %s not valid (type=%d)", name,
                type);
  }
}

/** @brief Add a NULL-terminated list of pairs {(char*)key, value} to the set
 *
 * \arg cfg config set to fill
 * \arg name variable name
 * \arg varargs variable value
 *
 */
void xbt_cfg_set(xbt_cfg_t cfg, const char *name, ...)
{
  va_list pa;

  va_start(pa, name);
  xbt_cfg_set_vargs(cfg, name, pa);
  va_end(pa);
}

/** @brief Add values parsed from a string into a config set
 *
 * \arg cfg config set to fill
 * \arg options a string containing the content to add to the config set. This
 * is a '\\t',' ' or '\\n' or ',' separated list of variables. Each individual variable is
 * like "[name]:[value]" where [name] is the name of an already registred
 * variable, and [value] conforms to the data type under which this variable was
 * registred.
 *
 * @todo This is a crude manual parser, it should be a proper lexer.
 */

void xbt_cfg_set_parse(xbt_cfg_t cfg, const char *options)
{
  xbt_ex_t e;

  int i;
  double d;
  char *str;

  xbt_cfgelm_t variable = NULL;
  char *optionlist_cpy;
  char *option, *name, *val;

  int len;

  XBT_IN;
  if (!options || !strlen(options)) {   /* nothing to do */
    return;
  }
  optionlist_cpy = xbt_strdup(options);

  DEBUG1("List to parse and set:'%s'", options);
  option = optionlist_cpy;
  while (1) {                   /* breaks in the code */

    if (!option)
      break;
    name = option;
    len = strlen(name);
    DEBUG3("Still to parse and set: '%s'. len=%d; option-name=%ld",
           name, len, (long) (option - name));

    /* Pass the value */
    while (option - name <= (len - 1) && *option != ' ' && *option != '\n'
           && *option != '\t' && *option != ',') {
      DEBUG1("Take %c.", *option);
      option++;
    }
    if (option - name == len) {
      DEBUG0("Boundary=EOL");
      option = NULL;            /* don't do next iteration */

    } else {
      DEBUG3("Boundary on '%c'. len=%d;option-name=%ld",
             *option, len, (long) (option - name));

      /* Pass the following blank chars */
      *(option++) = '\0';
      while (option - name < (len - 1) &&
             (*option == ' ' || *option == '\n' || *option == '\t')) {
        /*      fprintf(stderr,"Ignore a blank char.\n"); */
        option++;
      }
      if (option - name == len - 1)
        option = NULL;          /* don't do next iteration */
    }
    DEBUG2("parse now:'%s'; parse later:'%s'", name, option);

    if (name[0] == ' ' || name[0] == '\n' || name[0] == '\t')
      continue;
    if (!strlen(name))
      break;

    val = strchr(name, ':');
    if (!val) {
      free(optionlist_cpy);
      xbt_assert1(FALSE,
                  "Option '%s' badly formated. Should be of the form 'name:value'",
                  name);
    }
    *(val++) = '\0';

    INFO2("Configuration change: Set '%s' to '%s'", name, val);

    TRY {
      variable = xbt_dict_get((xbt_dict_t) cfg, name);
    }
    CATCH(e) {
      /* put it back on what won't get freed, ie within "options" and out of "optionlist_cpy" */
      name = (char *) (optionlist_cpy - name + options);
      free(optionlist_cpy);
      if (e.category == not_found_error) {
        xbt_ex_free(e);
        THROW1(not_found_error, 0,
               "No registered variable corresponding to '%s'.", name);
      }
      RETHROW;
    }

    TRY {
      switch (variable->type) {
      case xbt_cfgelm_string:
        xbt_cfg_set_string(cfg, name, val);     /* throws */
        break;

      case xbt_cfgelm_int:
        i = strtol(val, &val, 0);
        if (val == NULL) {
          free(optionlist_cpy);
          xbt_assert1(FALSE,
                      "Value of option %s not valid. Should be an integer",
                      name);
        }

        xbt_cfg_set_int(cfg, name, i);  /* throws */
        break;

      case xbt_cfgelm_double:
        d = strtod(val, &val);
        if (val == NULL) {
          free(optionlist_cpy);
          xbt_assert1(FALSE,
                      "Value of option %s not valid. Should be a double",
                      name);
        }

        xbt_cfg_set_double(cfg, name, d);       /* throws */
        break;

      case xbt_cfgelm_peer:
        str = val;
        val = strchr(val, ':');
        if (!val) {
          free(optionlist_cpy);
          xbt_assert1(FALSE,
                      "Value of option %s not valid. Should be an peer (machine:port)",
                      name);
        }

        *(val++) = '\0';
        i = strtol(val, &val, 0);
        if (val == NULL) {
          free(optionlist_cpy);
          xbt_assert1(FALSE,
                      "Value of option %s not valid. Should be an peer (machine:port)",
                      name);
        }

        xbt_cfg_set_peer(cfg, name, str, i);    /* throws */
        break;

      default:
        THROW1(unknown_error, 0, "Type of config element %s is not valid.",
               name);
      }
    }
    CATCH(e) {
      free(optionlist_cpy);
      RETHROW;
    }
  }
  free(optionlist_cpy);

}

/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_int(xbt_cfg_t cfg, const char *name, int val) {
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (variable->isdefault)
    xbt_cfg_set_int(cfg, name,val);
  else
    DEBUG2("Do not override configuration variable '%s' with value '%d' because it was already set.",name,val);
}
/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_double(xbt_cfg_t cfg, const char *name, double val) {
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (variable->isdefault)
    xbt_cfg_set_double(cfg, name, val);
  else
    DEBUG2("Do not override configuration variable '%s' with value '%lf' because it was already set.",name,val);
}
/** @brief Set a string value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_string(xbt_cfg_t cfg, const char *name, const char* val) {
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);

  if (variable->isdefault)
    xbt_cfg_set_string(cfg, name,val);
  else
    DEBUG2("Do not override configuration variable '%s' with value '%s' because it was already set.",name,val);
}
/** @brief Set a peer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_peer(xbt_cfg_t cfg, const char *name, const char* host, int port) {
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_peer);

  if (variable->isdefault)
    xbt_cfg_set_peer(cfg, name, host, port);
  else
    DEBUG3("Do not override configuration variable '%s' with value '%s:%d' because it was already set.",
        name,host,port);
}

/** @brief Set or add an integer value to \a name within \a cfg
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value of the variable
 */
void xbt_cfg_set_int(xbt_cfg_t cfg, const char *name, int val)
{
  xbt_cfgelm_t variable;

  VERB2("Configuration setting: %s=%d", name, val);
  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (variable->max == 1) {
    if (variable->cb_rm && xbt_dynar_length(variable->content))
      (*variable->cb_rm) (name, 0);

    xbt_dynar_set(variable->content, 0, &val);
  } else {
    if (variable->max
        && xbt_dynar_length(variable->content) ==
        (unsigned long) variable->max)
      THROW3(mismatch_error, 0,
             "Cannot add value %d to the config element %s since it's already full (size=%d)",
             val, name, variable->max);

    xbt_dynar_push(variable->content, &val);
  }

  if (variable->cb_set)
    (*variable->cb_set) (name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/** @brief Set or add a double value to \a name within \a cfg
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the doule to set
 */

void xbt_cfg_set_double(xbt_cfg_t cfg, const char *name, double val)
{
  xbt_cfgelm_t variable;

  VERB2("Configuration setting: %s=%f", name, val);
  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (variable->max == 1) {
    if (variable->cb_rm && xbt_dynar_length(variable->content))
      (*variable->cb_rm) (name, 0);

    xbt_dynar_set(variable->content, 0, &val);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      THROW3(mismatch_error, 0,
             "Cannot add value %f to the config element %s since it's already full (size=%d)",
             val, name, variable->max);

    xbt_dynar_push(variable->content, &val);
  }

  if (variable->cb_set)
    (*variable->cb_set) (name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/** @brief Set or add a string value to \a name within \a cfg
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value to be added
 *
 */

void xbt_cfg_set_string(xbt_cfg_t cfg, const char *name, const char *val)
{
  xbt_cfgelm_t variable;
  char *newval = xbt_strdup(val);

  VERB2("Configuration setting: %s=%s", name, val);
  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);
  DEBUG5("Variable: %d to %d %s (=%d) @%p",
         variable->min, variable->max, xbt_cfgelm_type_name[variable->type],
         variable->type, variable);

  if (variable->max == 1) {
    if (xbt_dynar_length(variable->content)) {
      if (variable->cb_rm)
        (*variable->cb_rm) (name, 0);
      else if (variable->type == xbt_cfgelm_string) {
        char *sval = xbt_dynar_get_as(variable->content, 0, char *);
        free(sval);
      }
    }

    xbt_dynar_set(variable->content, 0, &newval);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      THROW3(mismatch_error, 0,
             "Cannot add value %s to the config element %s since it's already full (size=%d)",
             name, val, variable->max);

    xbt_dynar_push(variable->content, &newval);
  }

  if (variable->cb_set)
    (*variable->cb_set) (name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/** @brief Set or add an peer value to \a name within \a cfg
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg peer the peer
 * \arg port the port number
 *
 * \e peer values are composed of a string (peername) and an integer (port)
 */

void
xbt_cfg_set_peer(xbt_cfg_t cfg, const char *name, const char *peer, int port)
{
  xbt_cfgelm_t variable;
  xbt_peer_t val = xbt_peer_new(peer, port);

  VERB3("Configuration setting: %s=%s:%d", name, peer, port);

  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_peer);

  if (variable->max == 1) {
    if (variable->cb_rm && xbt_dynar_length(variable->content))
      (*variable->cb_rm) (name, 0);

    xbt_dynar_set(variable->content, 0, &val);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      THROW4(mismatch_error, 0,
             "Cannot add value %s:%d to the config element %s since it's already full (size=%d)",
             peer, port, name, variable->max);

    xbt_dynar_push(variable->content, &val);
  }

  if (variable->cb_set)
    (*variable->cb_set) (name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/* ---- [ Removing ] ---- */

/** @brief Remove the provided \e val integer value from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value to be removed
 */
void xbt_cfg_rm_int(xbt_cfg_t cfg, const char *name, int val)
{

  xbt_cfgelm_t variable;
  unsigned int cpt;
  int seen;

  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROW3(mismatch_error, 0,
           "Cannot remove value %d from the config element %s since it's already at its minimal size (=%d)",
           val, name, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (seen == val) {
      if (variable->cb_rm)
        (*variable->cb_rm) (name, cpt);
      xbt_dynar_cursor_rm(variable->content, &cpt);
      return;
    }
  }

  THROW2(not_found_error, 0,
         "Can't remove the value %d of config element %s: value not found.",
         val, name);
}

/** @brief Remove the provided \e val double value from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value to be removed
 */

void xbt_cfg_rm_double(xbt_cfg_t cfg, const char *name, double val)
{
  xbt_cfgelm_t variable;
  unsigned int cpt;
  double seen;

  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROW3(mismatch_error, 0,
           "Cannot remove value %f from the config element %s since it's already at its minimal size (=%d)",
           val, name, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (seen == val) {
      xbt_dynar_cursor_rm(variable->content, &cpt);
      if (variable->cb_rm)
        (*variable->cb_rm) (name, cpt);
      return;
    }
  }

  THROW2(not_found_error, 0,
         "Can't remove the value %f of config element %s: value not found.",
         val, name);
}

/** @brief Remove the provided \e val string value from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value of the string which will be removed
 */
void xbt_cfg_rm_string(xbt_cfg_t cfg, const char *name, const char *val)
{
  xbt_cfgelm_t variable;
  unsigned int cpt;
  char *seen;

  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROW3(mismatch_error, 0,
           "Cannot remove value %s from the config element %s since it's already at its minimal size (=%d)",
           name, val, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (!strcpy(seen, val)) {
      if (variable->cb_rm)
        (*variable->cb_rm) (name, cpt);
      xbt_dynar_cursor_rm(variable->content, &cpt);
      return;
    }
  }

  THROW2(not_found_error, 0,
         "Can't remove the value %s of config element %s: value not found.",
         val, name);
}

/** @brief Remove the provided \e val peer value from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg peer the peername
 * \arg port the port number
 */

void
xbt_cfg_rm_peer(xbt_cfg_t cfg, const char *name, const char *peer, int port)
{
  xbt_cfgelm_t variable;
  unsigned int cpt;
  xbt_peer_t seen;

  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_peer);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROW4(mismatch_error, 0,
           "Cannot remove value %s:%d from the config element %s since it's already at its minimal size (=%d)",
           peer, port, name, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (!strcpy(seen->name, peer) && seen->port == port) {
      if (variable->cb_rm)
        (*variable->cb_rm) (name, cpt);
      xbt_dynar_cursor_rm(variable->content, &cpt);
      return;
    }
  }

  THROW3(not_found_error, 0,
         "Can't remove the value %s:%d of config element %s: value not found.",
         peer, port, name);
}

/** @brief Remove the \e pos th value from the provided variable */

void xbt_cfg_rm_at(xbt_cfg_t cfg, const char *name, int pos)
{

  xbt_cfgelm_t variable;

  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_any);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROW3(mismatch_error, 0,
           "Cannot remove %dth value from the config element %s since it's already at its minimal size (=%d)",
           pos, name, variable->min);

  if (variable->cb_rm)
    (*variable->cb_rm) (name, pos);
  xbt_dynar_remove_at(variable->content, pos, NULL);
}

/** @brief Remove all the values from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 */

void xbt_cfg_empty(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = NULL;
  xbt_ex_t e;

  TRY {
    variable = xbt_dict_get((xbt_dict_t) cfg, name);
  } CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;

    xbt_ex_free(e);
    THROW1(not_found_error, 0,
           "Can't empty  '%s' since this config element does not exist",
           name);
  }

  if (variable) {
    if (variable->cb_rm) {
      unsigned int cpt;
      void *ignored;
      xbt_dynar_foreach(variable->content, cpt, ignored) {
        (*variable->cb_rm) (name, cpt);
      }
    }
    xbt_dynar_reset(variable->content);
  }
}

/*----[ Getting ]---------------------------------------------------------*/

/** @brief Retrieve an integer value of a variable (get a warning if not uniq)
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using
 * xbt_cfg_get_dynar() instead.
 *
 * \warning the returned value is the actual content of the config set
 */
int xbt_cfg_get_int(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2
      ("You asked for the first value of the config element '%s', but there is %lu values",
       name, xbt_dynar_length(variable->content));
  }

  return xbt_dynar_get_as(variable->content, 0, int);
}

/** @brief Retrieve a double value of a variable (get a warning if not uniq)
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using
 * xbt_cfg_get_dynar() instead.
 *
 * \warning the returned value is the actual content of the config set
 */

double xbt_cfg_get_double(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2
      ("You asked for the first value of the config element '%s', but there is %lu values\n",
       name, xbt_dynar_length(variable->content));
  }

  return xbt_dynar_get_as(variable->content, 0, double);
}

/** @brief Retrieve a string value of a variable (get a warning if not uniq)
 *
 * \arg th the config set
 * \arg name the name of the variable
 * \arg val the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using
 * xbt_cfg_get_dynar() instead. Returns NULL if there is no value.
 *
 * \warning the returned value is the actual content of the config set
 */

char *xbt_cfg_get_string(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2
      ("You asked for the first value of the config element '%s', but there is %lu values\n",
       name, xbt_dynar_length(variable->content));
  } else if (xbt_dynar_length(variable->content) == 0) {
    return NULL;
  }

  return xbt_dynar_get_as(variable->content, 0, char *);
}

/** @brief Retrieve an peer value of a variable (get a warning if not uniq)
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg peer the peer
 * \arg port the port number
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using
 * xbt_cfg_get_dynar() instead.
 *
 * \warning the returned value is the actual content of the config set
 */

void xbt_cfg_get_peer(xbt_cfg_t cfg, const char *name, char **peer, int *port)
{
  xbt_cfgelm_t variable;
  xbt_peer_t val;

  variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_peer);

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2
      ("You asked for the first value of the config element '%s', but there is %lu values\n",
       name, xbt_dynar_length(variable->content));
  }

  val = xbt_dynar_get_as(variable->content, 0, xbt_peer_t);
  *peer = val->name;
  *port = val->port;
}

/** @brief Retrieve the dynar of all the values stored in a variable
 *
 * \arg cfg where to search in
 * \arg name what to search for
 * \arg dynar result
 *
 * Get the data stored in the config set.
 *
 * \warning the returned value is the actual content of the config set
 */
xbt_dynar_t xbt_cfg_get_dynar(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = NULL;
  xbt_ex_t e;

  TRY {
    variable = xbt_dict_get((xbt_dict_t) cfg, name);
  } CATCH(e) {
    if (e.category == not_found_error) {
      xbt_ex_free(e);
      THROW1(not_found_error, 0,
             "No registered variable %s in this config set", name);
    }
    RETHROW;
  }

  return variable->content;
}


/** @brief Retrieve one of the integer value of a variable */
int xbt_cfg_get_int_at(xbt_cfg_t cfg, const char *name, int pos)
{

  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);
  return xbt_dynar_get_as(variable->content, pos, int);
}

/** @brief Retrieve one of the double value of a variable */
double xbt_cfg_get_double_at(xbt_cfg_t cfg, const char *name, int pos)
{

  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);
  return xbt_dynar_get_as(variable->content, pos, double);
}


/** @brief Retrieve one of the string value of a variable */
char *xbt_cfg_get_string_at(xbt_cfg_t cfg, const char *name, int pos)
{

  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);
  return xbt_dynar_get_as(variable->content, pos, char *);
}

/** @brief Retrieve one of the peer value of a variable */
void
xbt_cfg_get_peer_at(xbt_cfg_t cfg, const char *name, int pos,
                    char **peer, int *port)
{

  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);
  xbt_peer_t val = xbt_dynar_get_ptr(variable->content, pos);

  *port = val->port;
  *peer = val->name;
}


#ifdef SIMGRID_TEST
#include "xbt.h"
#include "xbt/ex.h"

XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);

XBT_TEST_SUITE("config", "Configuration support");

static xbt_cfg_t make_set()
{
  xbt_cfg_t set = NULL;

  xbt_log_threshold_set(&_XBT_LOGV(xbt_cfg), xbt_log_priority_critical);
  xbt_cfg_register_str(&set, "speed:1_to_2_int");
  xbt_cfg_register_str(&set, "peername:1_to_1_string");
  xbt_cfg_register_str(&set, "user:1_to_10_string");

  return set;
}                               /* end_of_make_set */

XBT_TEST_UNIT("memuse", test_config_memuse, "Alloc and free a config set")
{
  xbt_cfg_t set = make_set();
  xbt_test_add0("Alloc and free a config set");
  xbt_cfg_set_parse(set,
                    "peername:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);
}

XBT_TEST_UNIT("validation", test_config_validation, "Validation tests")
{
  xbt_cfg_t set = set = make_set();
  xbt_ex_t e;

  xbt_test_add0("Having too few elements for speed");
  xbt_cfg_set_parse(set,
                    "peername:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  TRY {
    xbt_cfg_check(set);
  }
  CATCH(e) {
    if (e.category != mismatch_error ||
        strncmp(e.msg, "Config elem speed needs",
                strlen("Config elem speed needs")))
      xbt_test_fail1("Got an exception. msg=%s", e.msg);
    xbt_ex_free(e);
  }
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);



  xbt_test_add0("Having too much values of 'speed'");
  set = make_set();
  xbt_cfg_set_parse(set, "peername:toto:42 user:alegrand");
  TRY {
    xbt_cfg_set_parse(set, "speed:42 speed:24 speed:34");
  }
  CATCH(e) {
    if (e.category != mismatch_error ||
        strncmp(e.msg, "Cannot add value 34 to the config elem speed",
                strlen("Config elem speed needs")))
      xbt_test_fail1("Got an exception. msg=%s", e.msg);
    xbt_ex_free(e);
  }
  xbt_cfg_check(set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

}

XBT_TEST_UNIT("use", test_config_use, "Data retrieving tests")
{

  xbt_test_add0("Get a single value");
  {
    /* get_single_value */
    int ival;
    xbt_cfg_t myset = make_set();

    xbt_cfg_set_parse(myset, "peername:toto:42 speed:42");
    ival = xbt_cfg_get_int(myset, "speed");
    if (ival != 42)
      xbt_test_fail1("Speed value = %d, I expected 42", ival);
    xbt_cfg_free(&myset);
  }

  xbt_test_add0("Get multiple values");
  {
    /* get_multiple_value */
    xbt_dynar_t dyn;
    xbt_cfg_t myset = make_set();

    xbt_cfg_set_parse(myset, "peername:veloce user:foo\nuser:bar\tuser:toto");
    xbt_cfg_set_parse(myset, "speed:42");
    xbt_cfg_check(myset);
    dyn = xbt_cfg_get_dynar(myset, "user");

    if (xbt_dynar_length(dyn) != 3)
      xbt_test_fail1("Dynar length = %lu, I expected 3",
                     xbt_dynar_length(dyn));

    if (strcmp(xbt_dynar_get_as(dyn, 0, char *), "foo"))
        xbt_test_fail1("Dynar[0] = %s, I expected foo",
                       xbt_dynar_get_as(dyn, 0, char *));

    if (strcmp(xbt_dynar_get_as(dyn, 1, char *), "bar"))
        xbt_test_fail1("Dynar[1] = %s, I expected bar",
                       xbt_dynar_get_as(dyn, 1, char *));

    if (strcmp(xbt_dynar_get_as(dyn, 2, char *), "toto"))
        xbt_test_fail1("Dynar[2] = %s, I expected toto",
                       xbt_dynar_get_as(dyn, 2, char *));
    xbt_cfg_free(&myset);
  }

  xbt_test_add0("Access to a non-existant entry");
  {
    /* non-existant_entry */
    xbt_cfg_t myset = make_set();
    xbt_ex_t e;

    TRY {
      xbt_cfg_set_parse(myset, "color:blue");
    } CATCH(e) {
      if (e.category != not_found_error)
        xbt_test_exception(e);
      xbt_ex_free(e);
    }
    xbt_cfg_free(&myset);
  }
}
#endif /* SIMGRID_TEST */
