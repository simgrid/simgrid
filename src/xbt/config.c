/* config - Dictionnary where the type of each variable is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#include <stdio.h>

#include "xbt/config.h"         /* prototypes of this module */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_cfg, xbt, "configuration support");

/* xbt_cfgelm_t: the typedef corresponding to a config variable.

   Both data and DTD are mixed, but fixing it now would prevent me to ever defend my thesis. */

typedef struct {
  /* Description */
  char *desc;

  /* Allowed type of the variable */
  e_xbt_cfgelm_type_t type;
  int min, max;
  unsigned isdefault:1;

  /* Callbacks */
  xbt_cfg_cb_t cb_set;

  /* actual content (could be an union or something) */
  xbt_dynar_t content;
} s_xbt_cfgelm_t, *xbt_cfgelm_t;

static const char *xbt_cfgelm_type_name[xbt_cfgelm_type_count] = { "int", "double", "string", "boolean", "any" };

const struct xbt_boolean_couple xbt_cfgelm_boolean_values[] = {
  { "yes",    "no"},
  {  "on",   "off"},
  {"true", "false"},
  {   "1",     "0"},
  {  NULL,    NULL}
};

/* Internal stuff used in cache to free a variable */
static void xbt_cfgelm_free(void *data);

/* Retrieve the variable we'll modify */
static xbt_cfgelm_t xbt_cfgelm_get(xbt_cfg_t cfg, const char *name, e_xbt_cfgelm_type_t type);

/*----[ Memory management ]-----------------------------------------------*/
/** @brief Constructor
 *
 * Initialise a config set
 */
xbt_cfg_t xbt_cfg_new(void)
{
  return (xbt_cfg_t) xbt_dict_new_homogeneous(&xbt_cfgelm_free);
}

/** \brief Copy an existing configuration set
 *
 * @param whereto the config set to be created
 * @param tocopy the source data
 *
 * This only copy the registrations, not the actual content
 */
void xbt_cfg_cpy(xbt_cfg_t tocopy, xbt_cfg_t * whereto)
{
  xbt_dict_cursor_t cursor = NULL;
  xbt_cfgelm_t variable = NULL;
  char *name = NULL;

  XBT_DEBUG("Copy cfg set %p", tocopy);
  *whereto = NULL;
  xbt_assert(tocopy, "cannot copy NULL config");

  xbt_dict_foreach((xbt_dict_t) tocopy, cursor, name, variable) {
    xbt_cfg_register(whereto, name, variable->desc, variable->type, variable->min, variable->max, variable->cb_set);
  }
}

/** @brief Destructor */
void xbt_cfg_free(xbt_cfg_t * cfg)
{
  XBT_DEBUG("Frees cfg set %p", cfg);
  xbt_dict_free((xbt_dict_t *) cfg);
}

/** @brief Dump a config set for debuging purpose
 *
 * @param name The name to give to this config set
 * @param indent what to write at the beginning of each line (right number of spaces)
 * @param cfg the config set
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

  if (name)
    printf("%s>> Dumping of the config set '%s':\n", indent, name);

  xbt_dict_foreach(dict, cursor, key, variable) {
    printf("%s  %s:", indent, key);

    size = xbt_dynar_length(variable->content);
    printf ("%d_to_%d_%s. Actual size=%d. postset=%p, List of values:\n",
            variable->min, variable->max, xbt_cfgelm_type_name[variable->type], size, variable->cb_set);

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
    case xbt_cfgelm_boolean:
      for (i = 0; i < size; i++) {
        ival = xbt_dynar_get_as(variable->content, i, int);
        printf("%s    %d\n", indent, ival);
      }
      break;
    case xbt_cfgelm_alias:
      /* no content */
      break;
    default:
      printf("%s    Invalid type!!\n", indent);
      break;
    }
  }

  if (name)
    printf("%s<< End of the config set '%s'\n", indent, name);
  fflush(stdout);

  xbt_dict_cursor_free(&cursor);
}

/*
 * free an config element
 */
void xbt_cfgelm_free(void *data)
{
  xbt_cfgelm_t c = (xbt_cfgelm_t) data;

  XBT_DEBUG("Frees cfgelm %p", c);
  if (!c)
    return;
  xbt_free(c->desc);
  if (c->type != xbt_cfgelm_alias)
    xbt_dynar_free(&(c->content));
  free(c);
}

/*----[ Registering stuff ]-----------------------------------------------*/
/** @brief Register an element within a config set
 *
 *  @param cfg the config set
 *  @param name the name of the config element
 *  @param desc a description for this item (used by xbt_cfg_help())
 *  @param type the type of the config element
 *  @param min the minimum number of values for this config element
 *  @param max the maximum number of values for this config element
 *  @param cb_set callback function called when a value is set
 */
void xbt_cfg_register(xbt_cfg_t * cfg, const char *name, const char *desc, e_xbt_cfgelm_type_t type, int min,
                      int max, xbt_cfg_cb_t cb_set)
{
  xbt_cfgelm_t res;

  if (*cfg == NULL)
    *cfg = xbt_cfg_new();
  xbt_assert(type >= xbt_cfgelm_int && type <= xbt_cfgelm_boolean,
              "type of %s not valid (%d should be between %d and %d)",
             name, (int)type, xbt_cfgelm_int, xbt_cfgelm_boolean);
  res = xbt_dict_get_or_null((xbt_dict_t) * cfg, name);
  xbt_assert(NULL == res, "Refusing to register the config element '%s' twice.", name);

  res = xbt_new(s_xbt_cfgelm_t, 1);
  XBT_DEBUG("Register cfg elm %s (%s) (%d to %d %s (=%d) @%p in set %p)",
            name, desc, min, max, xbt_cfgelm_type_name[type], (int)type, res, *cfg);

  res->desc = xbt_strdup(desc);
  res->type = type;
  res->min = min;
  res->max = max;
  res->cb_set = cb_set;
  res->isdefault = 1;

  switch (type) {
  case xbt_cfgelm_int:
    res->content = xbt_dynar_new(sizeof(int), NULL);
    break;
  case xbt_cfgelm_double:
    res->content = xbt_dynar_new(sizeof(double), NULL);
    break;
  case xbt_cfgelm_string:
    res->content = xbt_dynar_new(sizeof(char *), xbt_free_ref);
    break;
  case xbt_cfgelm_boolean:
    res->content = xbt_dynar_new(sizeof(int), NULL);
    break;
  default:
    XBT_ERROR("%d is an invalid type code", (int)type);
    break;
  }
  xbt_dict_set((xbt_dict_t) * cfg, name, res, NULL);
}

void xbt_cfg_register_alias(xbt_cfg_t * cfg, const char *newname, const char *oldname)
{
  if (*cfg == NULL)
    *cfg = xbt_cfg_new();

  xbt_cfgelm_t res = xbt_dict_get_or_null((xbt_dict_t) * cfg, oldname);
  xbt_assert(NULL == res, "Refusing to register the option '%s' twice.", oldname);

  res = xbt_dict_get_or_null((xbt_dict_t) * cfg, newname);
  xbt_assert(res, "Cannot define an alias to the non-existing option '%s'.", newname);

  res = xbt_new0(s_xbt_cfgelm_t, 1);
  XBT_DEBUG("Register cfg alias %s -> %s in set %p)",oldname,newname, *cfg);

  res->desc = bprintf("Deprecated alias for %s",newname);
  res->type = xbt_cfgelm_alias;
  res->min = 1;
  res->max = 1;
  res->isdefault = 1;
  res->content = (xbt_dynar_t)newname;

  xbt_dict_set((xbt_dict_t) * cfg, oldname, res, NULL);
}

/** @brief Unregister an element from a config set.
 *
 *  @param cfg the config set
 *  @param name the name of the element to be freed
 *
 *  Note that it removes both the description and the actual content.
 *  Throws not_found when no such element exists.
 */
void xbt_cfg_unregister(xbt_cfg_t cfg, const char *name)
{
  XBT_DEBUG("Unregister elm '%s' from set %p", name, cfg);
  xbt_dict_remove((xbt_dict_t) cfg, name);
}

/**
 * @brief Parse a string and register the stuff described.
 *
 * @param cfg the config set
 * @param entry a string describing the element to register
 *
 * The string may consist in several variable descriptions separated by a space.
 * Each of them must use the following syntax: \<name\>:\<min nb\>_to_\<max nb\>_\<type\>
 * with type being one of  'string','int' or 'double'.
 *
 * Note that this does not allow to set the description, so you should prefer the other interface
 */
void xbt_cfg_register_str(xbt_cfg_t * cfg, const char *entry)
{
  char *entrycpy = xbt_strdup(entry);
  char *tok;

  int min, max;
  e_xbt_cfgelm_type_t type;
  XBT_DEBUG("Register string '%s'", entry);

  tok = strchr(entrycpy, ':');
  xbt_assert(tok, "Invalid config element descriptor: %s%s", entry, "; Should be <name>:<min nb>_to_<max nb>_<type>");
  *(tok++) = '\0';

  min = strtol(tok, &tok, 10);
  xbt_assert(tok, "Invalid minimum in config element descriptor %s", entry);

  xbt_assert(strcmp(tok, "_to_"), "Invalid config element descriptor : %s%s",
              entry, "; Should be <name>:<min nb>_to_<max nb>_<type>");
  tok += strlen("_to_");

  max = strtol(tok, &tok, 10);
  xbt_assert(tok, "Invalid maximum in config element descriptor %s", entry);

  xbt_assert(*tok == '_', "Invalid config element descriptor: %s%s", entry,
              "; Should be <name>:<min nb>_to_<max nb>_<type>");
  tok++;

  for (type = (e_xbt_cfgelm_type_t)0; type < xbt_cfgelm_type_count && strcmp(tok, xbt_cfgelm_type_name[type]); type++);
  xbt_assert(type < xbt_cfgelm_type_count, "Invalid type in config element descriptor: %s%s", entry,
              "; Should be one of 'string', 'int' or 'double'.");

  xbt_cfg_register(cfg, entrycpy, NULL, type, min, max, NULL);

  free(entrycpy);               /* strdup'ed by dict mechanism, but cannot be const */
}

/** @brief Displays the declared aliases and their description */
void xbt_cfg_aliases(xbt_cfg_t cfg)
{
  xbt_dict_cursor_t dict_cursor;
  unsigned int dynar_cursor;
  xbt_cfgelm_t variable;
  char *name;
  xbt_dynar_t names = xbt_dynar_new(sizeof(char *), NULL);

  xbt_dict_foreach((xbt_dict_t )cfg, dict_cursor, name, variable)
    xbt_dynar_push(names, &name);
  xbt_dynar_sort_strings(names);

  xbt_dynar_foreach(names, dynar_cursor, name) {
    variable = xbt_dict_get((xbt_dict_t )cfg, name);

    if (variable->type == xbt_cfgelm_alias)
      printf("   %s: %s\n", name, variable->desc);
  }
}

/** @brief Displays the declared options and their description */
void xbt_cfg_help(xbt_cfg_t cfg)
{
  xbt_dict_cursor_t dict_cursor;
  unsigned int dynar_cursor;
  xbt_cfgelm_t variable;
  char *name;
  xbt_dynar_t names = xbt_dynar_new(sizeof(char *), NULL);

  xbt_dict_foreach((xbt_dict_t )cfg, dict_cursor, name, variable)
    xbt_dynar_push(names, &name);
  xbt_dynar_sort_strings(names);

  xbt_dynar_foreach(names, dynar_cursor, name) {
    int i;
    int size;
    variable = xbt_dict_get((xbt_dict_t )cfg, name);
    if (variable->type == xbt_cfgelm_alias)
      continue;

    printf("   %s: %s\n", name, variable->desc);
    printf("       Type: %s; ", xbt_cfgelm_type_name[variable->type]);
    if (variable->min != 1 || variable->max != 1) {
      printf("Arity: min:%d to max:", variable->min);
      if (variable->max == 0)
        printf("(no bound); ");
      else
        printf("%d; ", variable->max);
    }
    size = xbt_dynar_length(variable->content);
    printf("Current value%s: ", (size <= 1 ? "" : "s"));

    if (size != 1)
      printf(size == 0 ? "n/a\n" : "{ ");
    for (i = 0; i < size; i++) {
      const char *sep = (size == 1 ? "\n" : (i < size - 1 ? ", " : " }\n"));

      switch (variable->type) {
      case xbt_cfgelm_int:
        printf("%d%s", xbt_dynar_get_as(variable->content, i, int), sep);
        break;
      case xbt_cfgelm_double:
        printf("%f%s", xbt_dynar_get_as(variable->content, i, double), sep);
        break;
      case xbt_cfgelm_string:
        printf("'%s'%s", xbt_dynar_get_as(variable->content, i, char *), sep);
        break;
      case xbt_cfgelm_boolean: {
        int b = xbt_dynar_get_as(variable->content, i, int);
        const char *bs = b ? xbt_cfgelm_boolean_values[0].true_val: xbt_cfgelm_boolean_values[0].false_val;
        if (b == 0 || b == 1)
          printf("'%s'%s", bs, sep);
        else
          printf("'%s/%d'%s", bs, b, sep);
        break;
      }
      default:
        printf("Invalid type!!%s", sep);
      }
    }
  }
  xbt_dynar_free(&names);
}

/** @brief Check that each variable have the right amount of values */
void xbt_cfg_check(xbt_cfg_t cfg)
{
  xbt_dict_cursor_t cursor;
  xbt_cfgelm_t variable;
  char *name;
  int size;

  xbt_assert(cfg, "NULL config set.");
  XBT_DEBUG("Check cfg set %p", cfg);

  xbt_dict_foreach((xbt_dict_t) cfg, cursor, name, variable) {
    if (variable->type == xbt_cfgelm_alias)
      continue;

    size = xbt_dynar_length(variable->content);
    if (variable->min > size) {
      xbt_dict_cursor_free(&cursor);
      THROWF(mismatch_error, 0, "Config elem %s needs at least %d %s, but there is only %d values.",
             name, variable->min, xbt_cfgelm_type_name[variable->type], size);
    }

    if (variable->isdefault && size > variable->min) {
      xbt_dict_cursor_free(&cursor);
      THROWF(mismatch_error, 0, "Config elem %s theoretically accepts %d %s, but has a default of %d values.",
             name, variable->min, xbt_cfgelm_type_name[variable->type], size);
    }

    if (variable->max > 0 && variable->max < size) {
      xbt_dict_cursor_free(&cursor);
      THROWF(mismatch_error, 0, "Config elem %s accepts at most %d %s, but there is %d values.",
             name, variable->max, xbt_cfgelm_type_name[variable->type], size);
    }
  }
  xbt_dict_cursor_free(&cursor);
}

static xbt_cfgelm_t xbt_cfgelm_get(xbt_cfg_t cfg, const char *name, e_xbt_cfgelm_type_t type)
{
  xbt_cfgelm_t res = xbt_dict_get_or_null((xbt_dict_t) cfg, name);

  // The user used the old name. Switch to the new one after a short warning
  while (res && res->type == xbt_cfgelm_alias) {
    const char* newname = (const char *)res->content;
    XBT_INFO("Option %s has been renamed to %s. Consider switching.", name, newname);
    res = xbt_cfgelm_get(cfg, newname, type);
  }

  if (!res) {
    xbt_cfg_help(cfg);
    fflush(stdout);
    THROWF(not_found_error, 0, "No registered variable '%s' in this config set.", name);
  }

  xbt_assert(type == xbt_cfgelm_any || res->type == type,
              "You tried to access to the config element %s as an %s, but its type is %s.",
              name, xbt_cfgelm_type_name[type], xbt_cfgelm_type_name[res->type]);
  return res;
}

/** @brief Get the type of this variable in that configuration set
 *
 * @param cfg the config set
 * @param name the name of the element
 *
 * @return the type of the given element
 */
e_xbt_cfgelm_type_t xbt_cfg_get_type(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = NULL;

  variable = xbt_dict_get_or_null((xbt_dict_t) cfg, name);
  if (!variable)
    THROWF(not_found_error, 0, "Can't get the type of '%s' since this variable does not exist", name);

  XBT_DEBUG("type in variable = %d", (int)variable->type);
  return variable->type;
}

/*----[ Setting ]---------------------------------------------------------*/
/**  @brief va_args version of xbt_cfg_set
 *
 * @param cfg config set to fill
 * @param name  variable name
 * @param pa  variable value
 *
 * Add some values to the config set.
 */
void xbt_cfg_set_vargs(xbt_cfg_t cfg, const char *name, va_list pa)
{
  char *str;
  int i;
  double d;
  e_xbt_cfgelm_type_t type = xbt_cfgelm_any; /* Set a dummy value to make gcc happy. It cannot get uninitialized */

  xbt_ex_t e;

  TRY {
    type = xbt_cfg_get_type(cfg, name);
  }
  CATCH(e) {
    if (e.category == not_found_error) {
      xbt_ex_free(e);
      THROWF(not_found_error, 0, "Can't set the property '%s' since it's not registered", name);
    }
    RETHROW;
  }

  switch (type) {
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
  case xbt_cfgelm_boolean:
    str = va_arg(pa, char *);
    xbt_cfg_set_boolean(cfg, name, str);
    break;
  default:
    xbt_die("Config element variable %s not valid (type=%d)", name, (int)type);
  }
}

/** @brief Add a NULL-terminated list of pairs {(char*)key, value} to the set
 *
 * @param cfg config set to fill
 * @param name variable name
 * @param ... variable value
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
 * @param cfg config set to fill
 * @param options a string containing the content to add to the config set. This is a '\\t',' ' or '\\n' or ','
 * separated list of variables. Each individual variable is like "[name]:[value]" where [name] is the name of an
 * already registered variable, and [value] conforms to the data type under which this variable was registered.
 *
 * @todo This is a crude manual parser, it should be a proper lexer.
 */
void xbt_cfg_set_parse(xbt_cfg_t cfg, const char *options) {
  char *optionlist_cpy;
  char *option, *name, *val;
  int len;

  XBT_IN();
  if (!options || !strlen(options)) {   /* nothing to do */
    return;
  }
  optionlist_cpy = xbt_strdup(options);

  XBT_DEBUG("List to parse and set:'%s'", options);
  option = optionlist_cpy;
  while (1) {                   /* breaks in the code */
    if (!option)
      break;
    name = option;
    len = strlen(name);
    XBT_DEBUG("Still to parse and set: '%s'. len=%d; option-name=%ld", name, len, (long) (option - name));

    /* Pass the value */
    while (option - name <= (len - 1) && *option != ' ' && *option != '\n' && *option != '\t' && *option != ',') {
      XBT_DEBUG("Take %c.", *option);
      option++;
    }
    if (option - name == len) {
      XBT_DEBUG("Boundary=EOL");
      option = NULL;            /* don't do next iteration */
    } else {
      XBT_DEBUG("Boundary on '%c'. len=%d;option-name=%ld", *option, len, (long) (option - name));
      /* Pass the following blank chars */
      *(option++) = '\0';
      while (option - name < (len - 1) && (*option == ' ' || *option == '\n' || *option == '\t')) {
        /*      fprintf(stderr,"Ignore a blank char.\n"); */
        option++;
      }
      if (option - name == len - 1)
        option = NULL;          /* don't do next iteration */
    }
    XBT_DEBUG("parse now:'%s'; parse later:'%s'", name, option);

    if (name[0] == ' ' || name[0] == '\n' || name[0] == '\t')
      continue;
    if (!strlen(name))
      break;

    val = strchr(name, ':');
    if (!val) {
      /* don't free(optionlist_cpy) here, 'name' points inside it */
      xbt_die("Option '%s' badly formatted. Should be of the form 'name:value'", name);
    }
    *(val++) = '\0';

    if (strncmp(name, "contexts/", strlen("contexts/")) && strncmp(name, "path", strlen("path")))
      XBT_INFO("Configuration change: Set '%s' to '%s'", name, val);

    TRY {
      xbt_cfg_set_as_string(cfg,name,val);
    } CATCH_ANONYMOUS {
      free(optionlist_cpy);
      RETHROW;
    }
  }
  free(optionlist_cpy);
}

/** @brief Set the value of a variable, using the string representation of that value
 *
 * @param cfg config set to modify
 * @param key name of the variable to modify
 * @param value string representation of the value to set
 *
 * @return the first char after the parsed value in val
 */

void *xbt_cfg_set_as_string(xbt_cfg_t cfg, const char *key, const char *value) {
  xbt_ex_t e;

  char *ret;
  volatile xbt_cfgelm_t variable = NULL;
  int i;
  double d;

  TRY {
    while (variable == NULL) {
      variable = xbt_dict_get((xbt_dict_t) cfg, key);
      if (variable->type == xbt_cfgelm_alias) {
        const char *newname = (const char*)variable->content;
        XBT_INFO("Note: configuration '%s' is deprecated. Please use '%s' instead.", key, newname);
        key = newname;
        variable = NULL;
      }
    }
  } CATCH(e) {
    if (e.category == not_found_error) {
      xbt_ex_free(e);
      THROWF(not_found_error, 0, "No registered variable corresponding to '%s'.", key);
    }
    RETHROW;
  }

  switch (variable->type) {
  case xbt_cfgelm_string:
    xbt_cfg_set_string(cfg, key, value);     /* throws */
    break;
  case xbt_cfgelm_int:
    i = strtol(value, &ret, 0);
    if (ret == value) {
      xbt_die("Value of option %s not valid. Should be an integer", key);
    }
    xbt_cfg_set_int(cfg, key, i);  /* throws */
    break;
  case xbt_cfgelm_double:
    d = strtod(value, &ret);
    if (ret == value) {
      xbt_die("Value of option %s not valid. Should be a double", key);
    }
    xbt_cfg_set_double(cfg, key, d);       /* throws */
    break;
  case xbt_cfgelm_boolean:
    xbt_cfg_set_boolean(cfg, key, value);  /* throws */
    ret = (char *)value + strlen(value);
    break;
  default:
    THROWF(unknown_error, 0, "Type of config element %s is not valid.", key);
    break;
  }
  return ret;
}

/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_int(xbt_cfg_t cfg, const char *name, int val)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (variable->isdefault){
    xbt_cfg_set_int(cfg, name, val);
    variable->isdefault = 1;
  } else
    XBT_DEBUG("Do not override configuration variable '%s' with value '%d' because it was already set.", name, val);
}

/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_double(xbt_cfg_t cfg, const char *name, double val)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (variable->isdefault) {
    xbt_cfg_set_double(cfg, name, val);
    variable->isdefault = 1;
  } else
    XBT_DEBUG("Do not override configuration variable '%s' with value '%f' because it was already set.", name, val);
}

/** @brief Set a string value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_string(xbt_cfg_t cfg, const char *name, const char *val)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);

  if (variable->isdefault){
    xbt_cfg_set_string(cfg, name, val);
    variable->isdefault = 1;
  } else
    XBT_DEBUG("Do not override configuration variable '%s' with value '%s' because it was already set.", name, val);
}

/** @brief Set an boolean value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_boolean(xbt_cfg_t cfg, const char *name, const char *val)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_boolean);

  if (variable->isdefault){
    xbt_cfg_set_boolean(cfg, name, val);
    variable->isdefault = 1;
  }
   else
    XBT_DEBUG("Do not override configuration variable '%s' with value '%s' because it was already set.", name, val);
}

/** @brief Set or add an integer value to \a name within \a cfg
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the value of the variable
 */
void xbt_cfg_set_int(xbt_cfg_t cfg, const char *name, int val)
{
  XBT_VERB("Configuration setting: %s=%d", name, val);
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (variable->max == 1) {
    xbt_dynar_set(variable->content, 0, &val);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == (unsigned long) variable->max)
      THROWF(mismatch_error, 0, "Cannot add value %d to the config element %s since it's already full (size=%d)",
             val, name, variable->max);

    xbt_dynar_push(variable->content, &val);
  }

  if (variable->cb_set)
    variable->cb_set(name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/** @brief Set or add a double value to \a name within \a cfg
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the doule to set
 */
void xbt_cfg_set_double(xbt_cfg_t cfg, const char *name, double val)
{
  XBT_VERB("Configuration setting: %s=%f", name, val);
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (variable->max == 1) {
    xbt_dynar_set(variable->content, 0, &val);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      THROWF(mismatch_error, 0, "Cannot add value %f to the config element %s since it's already full (size=%d)",
             val, name, variable->max);

    xbt_dynar_push(variable->content, &val);
  }

  if (variable->cb_set)
    variable->cb_set(name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/** @brief Set or add a string value to \a name within \a cfg
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the value to be added
 *
 */
void xbt_cfg_set_string(xbt_cfg_t cfg, const char *name, const char *val)
{
  char *newval = xbt_strdup(val);

  XBT_VERB("Configuration setting: %s=%s", name, val);
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);

  XBT_DEBUG("Variable: %d to %d %s (=%d) @%p",
         variable->min, variable->max, xbt_cfgelm_type_name[variable->type], (int)variable->type, variable);

  if (variable->max == 1) {
    if (!xbt_dynar_is_empty(variable->content)) {
      char *sval = xbt_dynar_get_as(variable->content, 0, char *);
      free(sval);
    }

    xbt_dynar_set(variable->content, 0, &newval);
  } else {
    if (variable->max
        && xbt_dynar_length(variable->content) == variable->max)
      THROWF(mismatch_error, 0, "Cannot add value %s to the config element %s since it's already full (size=%d)",
             name, val, variable->max);

    xbt_dynar_push(variable->content, &newval);
  }

  if (variable->cb_set)
    variable->cb_set(name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/** @brief Set or add a boolean value to \a name within \a cfg
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the value of the variable
 */
void xbt_cfg_set_boolean(xbt_cfg_t cfg, const char *name, const char *val)
{
  int i, bval;

  XBT_VERB("Configuration setting: %s=%s", name, val);
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_boolean);

  for (i = 0; xbt_cfgelm_boolean_values[i].true_val != NULL; i++) {
    if (strcmp(val, xbt_cfgelm_boolean_values[i].true_val) == 0){
      bval = 1;
      break;
    }
    if (strcmp(val, xbt_cfgelm_boolean_values[i].false_val) == 0){
      bval = 0;
      break;
    }
  }
  if (xbt_cfgelm_boolean_values[i].true_val == NULL) {
    xbt_die("Value of option '%s' not valid. Should be a boolean (yes,no,on,off,true,false,0,1)", val);
  }

  if (variable->max == 1) {
    xbt_dynar_set(variable->content, 0, &bval);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == (unsigned long) variable->max)
      THROWF(mismatch_error, 0, "Cannot add value %s to the config element %s since it's already full (size=%d)",
             val, name, variable->max);

    xbt_dynar_push(variable->content, &bval);
  }

  if (variable->cb_set)
    variable->cb_set(name, xbt_dynar_length(variable->content) - 1);
  variable->isdefault = 0;
}

/* ---- [ Removing ] ---- */
/** @brief Remove the provided \e val integer value from a variable
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the value to be removed
 */
void xbt_cfg_rm_int(xbt_cfg_t cfg, const char *name, int val)
{
  unsigned int cpt;
  int seen;

  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROWF(mismatch_error, 0,
           "Cannot remove value %d from the config element %s since it's already at its minimal size (=%d)",
           val, name, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (seen == val) {
      xbt_dynar_cursor_rm(variable->content, &cpt);
      return;
    }
  }
  THROWF(not_found_error, 0, "Can't remove the value %d of config element %s: value not found.", val, name);
}

/** @brief Remove the provided \e val double value from a variable
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the value to be removed
 */
void xbt_cfg_rm_double(xbt_cfg_t cfg, const char *name, double val)
{
  unsigned int cpt;
  double seen;

  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROWF(mismatch_error, 0,
           "Cannot remove value %f from the config element %s since it's already at its minimal size (=%d)",
           val, name, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (seen == val) {
      xbt_dynar_cursor_rm(variable->content, &cpt);
      return;
    }
  }

  THROWF(not_found_error, 0,"Can't remove the value %f of config element %s: value not found.", val, name);
}

/** @brief Remove the provided \e val string value from a variable
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the value of the string which will be removed
 */
void xbt_cfg_rm_string(xbt_cfg_t cfg, const char *name, const char *val)
{
  unsigned int cpt;
  char *seen;
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_string);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROWF(mismatch_error, 0,
           "Cannot remove value %s from the config element %s since it's already at its minimal size (=%d)",
           name, val, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (!strcpy(seen, val)) {
      xbt_dynar_cursor_rm(variable->content, &cpt);
      return;
    }
  }

  THROWF(not_found_error, 0, "Can't remove the value %s of config element %s: value not found.", val, name);
}

/** @brief Remove the provided \e val boolean value from a variable
 *
 * @param cfg the config set
 * @param name the name of the variable
 * @param val the value to be removed
 */
void xbt_cfg_rm_boolean(xbt_cfg_t cfg, const char *name, int val)
{
  unsigned int cpt;
  int seen;
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_boolean);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROWF(mismatch_error, 0,
           "Cannot remove value %d from the config element %s since it's already at its minimal size (=%d)",
           val, name, variable->min);

  xbt_dynar_foreach(variable->content, cpt, seen) {
    if (seen == val) {
      xbt_dynar_cursor_rm(variable->content, &cpt);
      return;
    }
  }

  THROWF(not_found_error, 0, "Can't remove the value %d of config element %s: value not found.", val, name);
}

/** @brief Remove the \e pos th value from the provided variable */
void xbt_cfg_rm_at(xbt_cfg_t cfg, const char *name, int pos)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_any);

  if (xbt_dynar_length(variable->content) == variable->min)
    THROWF(mismatch_error, 0,
           "Cannot remove %dth value from the config element %s since it's already at its minimal size (=%d)",
           pos, name, variable->min);

  xbt_dynar_remove_at(variable->content, pos, NULL);
}

/** @brief Remove all the values from a variable
 *
 * @param cfg the config set
 * @param name the name of the variable
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
    THROWF(not_found_error, 0, "Can't empty  '%s' since this option does not exist", name);
  }

  if (variable)
    xbt_dynar_reset(variable->content);
}

/* Say if the value is the default value */
int xbt_cfg_is_default_value(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_any);
  return variable->isdefault;
}

/*----[ Getting ]---------------------------------------------------------*/
/** @brief Retrieve an integer value of a variable (get a warning if not uniq)
 *
 * @param cfg the config set
 * @param name the name of the variable
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using xbt_cfg_get_dynar() instead.
 */
int xbt_cfg_get_int(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_int);

  if (xbt_dynar_length(variable->content) > 1) {
    XBT_WARN("You asked for the first value of the config element '%s', but there is %lu values",
         name, xbt_dynar_length(variable->content));
  }

  return xbt_dynar_get_as(variable->content, 0, int);
}

/** @brief Retrieve a double value of a variable (get a warning if not uniq)
 *
 * @param cfg the config set
 * @param name the name of the variable
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using xbt_cfg_get_dynar() instead.
 */
double xbt_cfg_get_double(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_double);

  if (xbt_dynar_length(variable->content) > 1) {
    XBT_WARN ("You asked for the first value of the config element '%s', but there is %lu values\n",
         name, xbt_dynar_length(variable->content));
  }

  return xbt_dynar_get_as(variable->content, 0, double);
}

/** @brief Retrieve a string value of a variable (get a warning if not uniq)
 *
 * @param cfg the config set
 * @param name the name of the variable
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
    XBT_WARN("You asked for the first value of the config element '%s', but there is %lu values\n",
         name, xbt_dynar_length(variable->content));
  } else if (xbt_dynar_is_empty(variable->content)) {
    return NULL;
  }

  return xbt_dynar_get_as(variable->content, 0, char *);
}

/** @brief Retrieve a boolean value of a variable (get a warning if not uniq)
 *
 * @param cfg the config set
 * @param name the name of the variable
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using xbt_cfg_get_dynar() instead.
 */
int xbt_cfg_get_boolean(xbt_cfg_t cfg, const char *name)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_boolean);

  if (xbt_dynar_length(variable->content) > 1) {
    XBT_WARN("You asked for the first value of the config element '%s', but there is %lu values",
         name, xbt_dynar_length(variable->content));
  }

  return xbt_dynar_get_as(variable->content, 0, int);
}

/** @brief Retrieve the dynar of all the values stored in a variable
 *
 * @param cfg where to search in
 * @param name what to search for
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
      THROWF(not_found_error, 0, "No registered variable %s in this config set", name);
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

/** @brief Retrieve one of the boolean value of a variable */
int xbt_cfg_get_boolean_at(xbt_cfg_t cfg, const char *name, int pos)
{
  xbt_cfgelm_t variable = xbt_cfgelm_get(cfg, name, xbt_cfgelm_boolean);
  return xbt_dynar_get_as(variable->content, pos, int);
}

#ifdef SIMGRID_TEST
#include "xbt.h"
#include "xbt/ex.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_cfg);

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
  xbt_test_add("Alloc and free a config set");
  xbt_cfg_set_parse(set, "peername:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);
}

XBT_TEST_UNIT("validation", test_config_validation, "Validation tests")
{
  xbt_cfg_t set = set = make_set();
  xbt_ex_t e;

  xbt_test_add("Having too few elements for speed");
  xbt_cfg_set_parse(set, "peername:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  TRY {
    xbt_cfg_check(set);
  } CATCH(e) {
    if (e.category != mismatch_error || strncmp(e.msg, "Config elem speed needs", strlen("Config elem speed needs")))
      xbt_test_fail("Got an exception. msg=%s", e.msg);
    xbt_ex_free(e);
  }
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  xbt_test_add("Having too much values of 'speed'");
  set = make_set();
  xbt_cfg_set_parse(set, "peername:toto:42 user:alegrand");
  TRY {
    xbt_cfg_set_parse(set, "speed:42 speed:24 speed:34");
  } CATCH(e) {
    if (e.category != mismatch_error ||
        strncmp(e.msg, "Cannot add value 34 to the config elem speed", strlen("Config elem speed needs")))
      xbt_test_fail("Got an exception. msg=%s", e.msg);
    xbt_ex_free(e);
  }
  xbt_cfg_check(set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);
}

XBT_TEST_UNIT("use", test_config_use, "Data retrieving tests")
{
  xbt_test_add("Get a single value");
  {
    /* get_single_value */
    int ival;
    xbt_cfg_t myset = make_set();

    xbt_cfg_set_parse(myset, "peername:toto:42 speed:42");
    ival = xbt_cfg_get_int(myset, "speed");
    if (ival != 42)
      xbt_test_fail("Speed value = %d, I expected 42", ival);
    xbt_cfg_free(&myset);
  }

  xbt_test_add("Get multiple values");
  {
    /* get_multiple_value */
    xbt_dynar_t dyn;
    xbt_cfg_t myset = make_set();

    xbt_cfg_set_parse(myset, "peername:veloce user:foo\nuser:bar\tuser:toto");
    xbt_cfg_set_parse(myset, "speed:42");
    xbt_cfg_check(myset);
    dyn = xbt_cfg_get_dynar(myset, "user");

    if (xbt_dynar_length(dyn) != 3)
      xbt_test_fail("Dynar length = %lu, I expected 3", xbt_dynar_length(dyn));

    if (strcmp(xbt_dynar_get_as(dyn, 0, char *), "foo"))
       xbt_test_fail("Dynar[0] = %s, I expected foo", xbt_dynar_get_as(dyn, 0, char *));

    if (strcmp(xbt_dynar_get_as(dyn, 1, char *), "bar"))
       xbt_test_fail("Dynar[1] = %s, I expected bar", xbt_dynar_get_as(dyn, 1, char *));

    if (strcmp(xbt_dynar_get_as(dyn, 2, char *), "toto"))
       xbt_test_fail("Dynar[2] = %s, I expected toto", xbt_dynar_get_as(dyn, 2, char *));
    xbt_cfg_free(&myset);
  }

  xbt_test_add("Access to a non-existant entry");
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
#endif                          /* SIMGRID_TEST */
