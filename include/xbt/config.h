/* config - Dictionary where the type of each cell is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Copyright (c) 2004-2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONFIG_H_
#define _XBT_CONFIG_H_

#include "xbt/dynar.h"
#include <stdarg.h>

SG_BEGIN_DECL()

/** @addtogroup XBT_config
 *  @brief Changing the configuration of SimGrid components (grounding feature)
 *
 *  All modules of the SimGrid toolkit can be configured with this API.
 *  User modules and libraries can also use these facilities to handle their own configuration.
 *
 *  A configuration set contain several \e variables which have a unique name in the set and can take a given type of
 *  value. For example, it may contain a \a size variable, accepting \e int values.
 *
 *  It is impossible to set a value to a variable which has not been registered before.
 *  Usually, the module registers all the options it accepts in the configuration set, during its initialization and
 *  user code then set and unset values.
 *
 *  The easiest way to register a variable is to use the xbt_str_register_str function, which accepts a string
 *  representation of the config element descriptor. The syntax is the following:
 *  \verbatim <name>:<min nb>_to_<max nb>_<type>\endverbatim
 *
 *  For example, <tt>size:1_to_1_int</tt> describes a variable called \e size which must take exactly one value, and
 *  the value being an integer. Set the maximum to 0 to disable the upper bound on data count.
 *
 *  Another example could be <tt>outputfiles:0_to_10_string</tt> which describes a variable called \e outputfiles and
 *  which can take between 0 and 10 strings as value.
 *
 *  To some extend, configuration sets can be seen as typed hash structures.
 *
 *  \section XBT_cfg_ex Example of use
 *
 *  TBD
 */
/** @defgroup XBT_cfg_use User interface: changing values
 *  @ingroup XBT_config
 *
 * This is the only interface you should use unless you want to let your own code become configurable with this.
 *
 * If the variable accept at most one value, those functions replace the current value with the provided one. If max>1,
 * the provided value is appended to the list.
 *
 * string values are strdup'ed before use, so you can (and should) free your copy
 *
 * @{
 */
/** @brief Configuration set's data type is opaque. */
#ifdef __cplusplus
namespace simgrid {
namespace config {
class Config;
}
}
typedef simgrid::config::Config* xbt_cfg_t;
#else
typedef void* xbt_cfg_t;
#endif

XBT_PUBLIC(void) xbt_cfg_set_parse(const char *options);

/* Set the value of the cell \a name in \a cfg with the provided value.*/
XBT_PUBLIC(void) xbt_cfg_set_int       (const char *name, int val);
XBT_PUBLIC(void) xbt_cfg_set_double    (const char *name, double val);
XBT_PUBLIC(void) xbt_cfg_set_string    (const char *name, const char *val);
XBT_PUBLIC(void) xbt_cfg_set_boolean   (const char *name, const char *val);
XBT_PUBLIC(void) xbt_cfg_set_as_string(const char *name, const char *val);

/*
  Set the default value of the cell \a name in \a cfg with the provided value.
  If it was already set to something (possibly from the command line), do nothing.
 */
XBT_PUBLIC(void) xbt_cfg_setdefault_int    (const char *name, int val);
XBT_PUBLIC(void) xbt_cfg_setdefault_double (const char *name, double val);
XBT_PUBLIC(void) xbt_cfg_setdefault_string (const char *name, const char *val);
XBT_PUBLIC(void) xbt_cfg_setdefault_boolean(const char *name, const char *val);

/** @brief Return if configuration is set by default*/
XBT_PUBLIC(int) xbt_cfg_is_default_value(const char *name);

/* @} */

/** @defgroup XBT_cfg_decl Configuration type declaration and memory management
 *  @ingroup XBT_config
 *
 *  @{
 */

/** \brief Callback types. They get the name of the modified entry, and the position of the changed value */
typedef void (*xbt_cfg_cb_t) (const char * name);

XBT_PUBLIC(xbt_cfg_t) xbt_cfg_new(void);
XBT_PUBLIC(void) xbt_cfg_free(xbt_cfg_t * cfg);
XBT_PUBLIC(void) xbt_cfg_dump(const char *name, const char *indent, xbt_cfg_t cfg);

 /** @} */

/** @defgroup XBT_cfg_register  Registering stuff
 *  @ingroup XBT_config
 *
 *  This how to add new variables to an existing configuration set. Use it to make your code configurable.
 *
 *  @{
 */
XBT_PUBLIC(void) xbt_cfg_register_double (const char *name, double default_val,    xbt_cfg_cb_t cb_set, const char *desc);
XBT_PUBLIC(void) xbt_cfg_register_int    (const char *name, int default_val,       xbt_cfg_cb_t cb_set, const char *desc);
XBT_PUBLIC(void) xbt_cfg_register_string (const char *name, const char*default_val,xbt_cfg_cb_t cb_set, const char *desc);
XBT_PUBLIC(void) xbt_cfg_register_boolean(const char *name, const char*default_val,xbt_cfg_cb_t cb_set, const char *desc);
XBT_PUBLIC(void) xbt_cfg_register_alias(const char *newname, const char *oldname);

XBT_PUBLIC(void) xbt_cfg_aliases(void);
XBT_PUBLIC(void) xbt_cfg_help(void);

/*  @} */
/** @defgroup XBT_cfg_get Getting the stored values
 *  @ingroup XBT_config
 *
 * This is how to retrieve the values stored in the configuration set. This is only intended to configurable code,
 * naturally.
 *
 * Note that those function return a pointer to the values actually stored in the set. Do not modify them unless you
 * really know what you're doing. Likewise, do not free the strings after use, they are not copy of the data, but the
 * data themselves.
 *
 *  @{
 */

XBT_PUBLIC(int)    xbt_cfg_get_int(const char *name);
XBT_PUBLIC(double) xbt_cfg_get_double(const char *name);
XBT_PUBLIC(char *) xbt_cfg_get_string(const char *name);
XBT_PUBLIC(int)    xbt_cfg_get_boolean(const char *name);

/** @} */

SG_END_DECL()
#endif                          /* _XBT_CONFIG_H_ */
