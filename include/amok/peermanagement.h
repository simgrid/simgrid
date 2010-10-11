/* amok peer management - servers main loop and remote peer stopping        */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_PEER_MANAGEMENT_H
#define AMOK_PEER_MANAGEMENT_H

#include <gras.h>

/** \addtogroup AMOK_pm
 *  \brief Managing remote servers
 * 
 * This module provide the main loop of servers designed to answer to
 * callbacks. They can furthermore be stopped remotely.  
 * 
 * This module is especially useful to program following the centralized
 * master/slave architecture. In this architecture, one of the processes
 * acts as a master dispatching orders to the others, which are called
 * slaves. 
  
 * The source code of the <b>slaves</b> then only consists in:
 *   - declaring the gras datatypes (see \ref GRAS_dd)
 *   - declaring the messages (with gras_msgtype_declare() or gras_msgtype_declare_rpc())
 *   - attaching the right callbacks to the messages (with gras_cb_register())
 *   - declaring the right repetitive actions (see \ref GRAS_timer)
 *   - joining the group (with amok_pm_group_join(), so that the master now it)
 *   - entering the endless loop (with amok_pm_mainloop()). 
 
 * The <b>master</b>, on its side, should create declare the datatypes and
 * messages just like slaves. It should then create a group with
 * amok_pm_group_new().
 
 * Afterward, there is two solutions. 
 *   - If your master is a deamon which never stops itself (just like regular UNIX daemons), it should: 
 *      - register the needed callbacks to messages comming from slaves
 *      - register repetitive actions 
 *      - entering the main loop.
 *   - If the master is not a deamon, it should:
 *      - wait a moment for the slaves registration (using gras_msg_handleall())
 *      - run its algoritpm. For this, it may call RPC on slaves, or explicitely wait (with gras_msg_wait()) for the answers it expects.

 * \section AMOK_pm_compat Compatibility issues
 * 
 * The API described here is as of SimGrid 3.2 and higher. In version 3.1
 * (where this module were introduced), all functions were named amok_hm_*
 * This was because the module used to be named HostManagement, but it was
 * renamed before being released to betterly express its purpose.
 * Unfortunately, the rename was not done properly before version 3.2.
 * 
 * @{
 */

/* module handling */
XBT_PUBLIC(void) amok_pm_init(void);
XBT_PUBLIC(void) amok_pm_exit(void);

XBT_PUBLIC(void) amok_pm_mainloop(double timeOut);

XBT_PUBLIC(void) amok_pm_kill_hp(char *name, int port);
XBT_PUBLIC(void) amok_pm_kill(gras_socket_t buddy);
XBT_PUBLIC(void) amok_pm_kill_sync(gras_socket_t buddy);

XBT_PUBLIC(xbt_dynar_t) amok_pm_group_new(const char *group_name);
XBT_PUBLIC(xbt_dynar_t) amok_pm_group_get(gras_socket_t master,
                                          const char *group_name);

XBT_PUBLIC(int) amok_pm_group_join(gras_socket_t master,
                                   const char *group_name);
XBT_PUBLIC(void) amok_pm_group_leave(gras_socket_t master,
                                     const char *group_name);


XBT_PUBLIC(void) amok_pm_group_shutdown(const char *group_name);
XBT_PUBLIC(void) amok_pm_group_shutdown_remote(gras_socket_t master,
                                               const char *group_name);


/** @} */
#endif                          /* AMOK_peer_MANAGEMENT_H */
