/* $Id$ */

/* config - Dictionary where the type of each cell is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Copyright (c) 2001,2002,2003,2004 Martin Quinson. All rights reserved.   */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONFIG_H_
#define _XBT_CONFIG_H_

#include "xbt/dynar.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_config
 *  @brief Changing the configuration of SimGrid components (grounding feature)
 *
 *  All modules of the SimGrid toolkit can be configured with this API. 
 *  User modules and libraries can also use these facilities to handle 
 *  their own configuration.
 *
 *  A configuration set contain several \e variables which have a unique name
 *  in the set and can take a given type of value. For example, it may 
 *  contain a \a size variable, accepting \e int values. 
 *
 *  It is impossible to set a value to a variable which has not been registered before.
 *  Usually, the module registers all the options it accepts in the configuration set,
 *  during its initialization and user code then set and unset values.
 *
 *  The easiest way to register a variable is to use the xbt_str_register_str function, 
 *  which accepts a string representation of the config element descriptor. The syntax 
 *  is the following: \verbatim <name>:<min nb>_to_<max nb>_<type>\endverbatim
 *
 *  For example, <tt>size:1_to_1_int</tt> describes a variable called \e size which 
 *  must take exactly one value, and the value being an integer. Set the maximum to 0 to 
 *  disable the upper bound on data count.
 *
 *  Another example could be <tt>outputfiles:0_to_10_string</tt> which describes a variable
 *  called \e outputfiles and which can take between 0 and 10 strings as value.
 *
 *  To some extend, configuration sets can be seen as typed hash structures.
 * 
 *  \todo This great mechanism is not used in SimGrid yet...
 *
 *
 *  \section XBT_cfg_ex Example of use
 *
 *  \dontinclude config.c
 *
 *  First, let's create a configuration set with some registered variables.
 *  This must be done by the configurable library before the user interactions.
 *
 *  \skip make_set
 *  \until end_of_make_set
 *
 *  Now, set and get a single value
 *  \skip get_single_value
 *  \skip int
 *  \until cfg_free
 *
 *  And now, set and get a multiple value
 *  \skip get_multiple_value
 *  \skip dyn
 *  \until cfg_free
 *
 *  All those functions throws mismatch_error if asked to deal with an 
 *  unregistered variable.
 *  \skip myset
 *  \until cfg_free
 * 
 */
/** @defgroup XBT_cfg_use User interface: changing values
 *  @ingroup XBT_config
 *
 * This is the only interface you should use unless you want to let your 
 * own code become configurable with this.
 *
 * If the variable accept at most one value, those functions replace the 
 * current value with the provided one. If max>1, the provided value is 
 * appended to the list.
 *
 * string values are strdup'ed before use, so you can (and should) free
 * your copy  
 *
 * @{
 */
  /** @brief Configuration set are only special dynars. But don't rely on it, it may change. */
     typedef xbt_dynar_t xbt_cfg_t;

XBT_PUBLIC(void) xbt_cfg_set(xbt_cfg_t cfg, const char *name, ...);
XBT_PUBLIC(void) xbt_cfg_set_vargs(xbt_cfg_t cfg, const char *name,
                                   va_list pa);
XBT_PUBLIC(void) xbt_cfg_set_parse(xbt_cfg_t cfg, const char *options);


/*
  Set the value of the cell \a name in \a cfg with the provided value.
 */
XBT_PUBLIC(void) xbt_cfg_set_int(xbt_cfg_t cfg, const char *name, int val);
XBT_PUBLIC(void) xbt_cfg_set_double(xbt_cfg_t cfg, const char *name,
                                    double val);
XBT_PUBLIC(void) xbt_cfg_set_string(xbt_cfg_t cfg, const char *name,
                                    const char *val);
XBT_PUBLIC(void) xbt_cfg_set_peer(xbt_cfg_t cfg, const char *name,
                                  const char *peer, int port);

/*
 Remove the provided value from the cell @name in @cfg.
 */
XBT_PUBLIC(void) xbt_cfg_rm_int(xbt_cfg_t cfg, const char *name, int val);
XBT_PUBLIC(void) xbt_cfg_rm_double(xbt_cfg_t cfg, const char *name,
                                   double val);
XBT_PUBLIC(void) xbt_cfg_rm_string(xbt_cfg_t cfg, const char *name,
                                   const char *val);
XBT_PUBLIC(void) xbt_cfg_rm_peer(xbt_cfg_t cfg, const char *name,
                                 const char *peer, int port);

/*
 Remove the value at position \e pos from the config \e cfg
 */
XBT_PUBLIC(void) xbt_cfg_rm_at(xbt_cfg_t cfg, const char *name, int pos);

/* rm every values */
XBT_PUBLIC(void) xbt_cfg_empty(xbt_cfg_t cfg, const char *name);

/* @} */

/** @defgroup XBT_cfg_decl Configuration type declaration and memory management
 *  @ingroup XBT_config
 *
 *  @{
 */

  /** @brief possible content of each configuration cell */
     typedef enum {
       xbt_cfgelm_int = 0,
                       /**< int */
       xbt_cfgelm_double,
                       /**< double */
       xbt_cfgelm_string,
                       /**< char* */
       xbt_cfgelm_peer,/**< both a char* (representing the peername) and an integer (representing the port) */

       xbt_cfgelm_any,          /* not shown to users to prevent errors */
       xbt_cfgelm_type_count
     } e_xbt_cfgelm_type_t;

  /** \brief Callback types. They get the name of the modified entry, and the position of the changed value */
     typedef void (*xbt_cfg_cb_t) (const char *, int);

XBT_PUBLIC(xbt_cfg_t) xbt_cfg_new(void);
XBT_PUBLIC(void) xbt_cfg_cpy(xbt_cfg_t tocopy, /* OUT */ xbt_cfg_t * whereto);
XBT_PUBLIC(void) xbt_cfg_free(xbt_cfg_t * cfg);
XBT_PUBLIC(void) xbt_cfg_dump(const char *name, const char *indent,
                              xbt_cfg_t cfg);

 /** @} */

/** @defgroup XBT_cfg_register  Registering stuff
 *  @ingroup XBT_config
 *
 *  This how to add new variables to an existing configuration set. Use it to make your code 
 *  configurable.
 *
 *  @{
 */
XBT_PUBLIC(void) xbt_cfg_register(xbt_cfg_t cfg,
                                  const char *name, e_xbt_cfgelm_type_t type,
                                  int min, int max,
                                  xbt_cfg_cb_t cb_set, xbt_cfg_cb_t cb_rm);
XBT_PUBLIC(void) xbt_cfg_unregister(xbt_cfg_t cfg, const char *name);
XBT_PUBLIC(void) xbt_cfg_register_str(xbt_cfg_t cfg, const char *entry);
XBT_PUBLIC(void) xbt_cfg_check(xbt_cfg_t cfg);
XBT_PUBLIC(e_xbt_cfgelm_type_t) xbt_cfg_get_type(xbt_cfg_t cfg,
                                                 const char *name);
/*  @} */
/** @defgroup XBT_cfg_get Getting the stored values
 *  @ingroup XBT_config
 *
 * This is how to retrieve the values stored in the configuration set. This is only 
 * intended to configurable code, naturally.
 *
 * Note that those function return a pointer to the values actually stored 
 * in the set. Do not modify them unless you really know what you're doing. 
 * Likewise, do not free the strings after use, they are not copy of the data,
 * but the data themselves.
 *
 *  @{
 */

XBT_PUBLIC(int) xbt_cfg_get_int(xbt_cfg_t cfg, const char *name);
XBT_PUBLIC(double) xbt_cfg_get_double(xbt_cfg_t cfg, const char *name);
XBT_PUBLIC(char *) xbt_cfg_get_string(xbt_cfg_t cfg, const char *name);
XBT_PUBLIC(void) xbt_cfg_get_peer(xbt_cfg_t cfg, const char *name,
                                  char **peer, int *port);
XBT_PUBLIC(xbt_dynar_t) xbt_cfg_get_dynar(xbt_cfg_t cfg, const char *name);

XBT_PUBLIC(int) xbt_cfg_get_int_at(xbt_cfg_t cfg, const char *name, int pos);
XBT_PUBLIC(double) xbt_cfg_get_double_at(xbt_cfg_t cfg, const char *name,
                                         int pos);
XBT_PUBLIC(char *) xbt_cfg_get_string_at(xbt_cfg_t cfg, const char *name,
                                         int pos);
XBT_PUBLIC(void) xbt_cfg_get_peer_at(xbt_cfg_t cfg, const char *name, int pos,
                                     char **peer, int *port);

/** @} */

SG_END_DECL()
#endif /* _XBT_CONFIG_H_ */
