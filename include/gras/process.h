/* gras/process.h - Manipulating data related to an host.                   */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PROCESS_H
#define GRAS_PROCESS_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/dict.h"

SG_BEGIN_DECL()

     void gras_agent_spawn(const char *name, void *data, xbt_main_func_t code,
                           int argc, char *argv[], xbt_dict_t properties);


/****************************************************************************/
/* Manipulating User Data                                                   */
/****************************************************************************/

/** \addtogroup GRAS_globals
 *  \brief Handling global variables so that it works on simulator.
 * 
 * In GRAS, using globals is forbidden since the "processes" will
 * sometimes run as a thread inside the same process (namely, in
 * simulation mode). So, you have to put all globals in a structure, and
 * let GRAS handle it.
 * 
 * Use the \ref gras_userdata_new macro to create a new user data (or malloc it
 * and use \ref gras_userdata_set yourself), and \ref gras_userdata_get to
 * retrieve a reference to it.
 * 
 * 
 * For more info on this, you may want to check the relevant lesson of the GRAS tutorial:
 * \ref GRAS_tut_tour_globals
 */
/* @{ */

/**
 * \brief Get the data associated with the current process.
 * \ingroup GRAS_globals
 */
XBT_PUBLIC(void *) gras_userdata_get(void);

/**
 * \brief Set the data associated with the current process.
 * \ingroup GRAS_globals
 */
XBT_PUBLIC(void *) gras_userdata_set(void *ud);

/** \brief Malloc and set the data associated with the current process.
 *
 * @warning gras_userdata_new() expects the pointed type, not the
 * pointer type. We know it'a a bit troublesome, but it seems like
 * the only solution since this macro has to compute the size to
 * malloc and should thus know the pointed type. 
 *
 * You'll find an example in the tutorial:  \ref GRAS_tut_tour_globals
 */
#define gras_userdata_new(type) ((type*)gras_userdata_set(xbt_new0(type,1)))
/* @} */

SG_END_DECL()
#endif /* GRAS_PROCESS_H */
