/* $Id$ */

/* amok host management - servers main loop and remote host stopping        */

/* Copyright (c) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_HOST_MANAGEMENT_H
#define AMOK_HOST_MANAGEMENT_H

#include <gras.h>
#include <amok/base.h>

/** \addtogroup AMOK_hm
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
 *   - joining the group (with amok_hm_group_join(), so that the master now it)
 *   - entering the endless loop (with amok_hm_mainloop()). 
 
 * The <b>master</b>, on its side, should create declare the datatypes and
 * messages just like slaves. It should then create a group with
 * amok_hm_group_new().
 
 * Afterward, there is two solutions. 
 *   - If your master is a deamon which never stops itself (just like regular UNIX daemons), it should: 
 *      - register the needed callbacks to messages comming from slaves
 *      - register repetitive actions 
 *      - entering the main loop.
 *   - If the master is not a deamon, it should:
 *      - wait a moment for the slaves registration (using gras_msg_handleall())
 *      - run its algorithm. For this, it may call RPC on slaves, or explicitely wait (with gras_msg_wait()) for the answers it expects.

 *
 * @{
 */

/* module handling */
void amok_hm_init(void);
void amok_hm_exit(void);

void amok_hm_mainloop(double timeOut);

void amok_hm_kill_hp(char *name,int port);
void amok_hm_kill(gras_socket_t buddy);
void amok_hm_kill_sync(gras_socket_t buddy);

xbt_dynar_t amok_hm_group_new(const char *group_name);
xbt_dynar_t amok_hm_group_get(gras_socket_t master, const char *group_name);

void        amok_hm_group_join(gras_socket_t master, const char *group_name);
void        amok_hm_group_leave(gras_socket_t master, const char *group_name);


void amok_hm_group_shutdown(const char *group_name);
void amok_hm_group_shutdown_remote(gras_socket_t master, const char *group_name);


/** @} */
#endif /* AMOK_HOST_MANAGEMENT_H */
