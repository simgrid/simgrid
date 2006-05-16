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
 * @{
 */

/* module handling */
void amok_hm_init(void);
void amok_hm_exit(void);

void amok_hm_mainloop(double timeOut);
void amok_hm_kill_hp(char *name,int port);
void amok_hm_kill(gras_socket_t buddy);
void amok_hm_kill_sync(gras_socket_t buddy);

xbt_dynar_t amok_hm_group_new(char *group_name);
xbt_dynar_t amok_hm_group_get(gras_socket_t master,char *group_name);

void        amok_hm_group_join(gras_socket_t master, char *group_name);
void        amok_hm_group_leave(gras_socket_t master, char *group_name);


void amok_hm_group_shutdown_local(char *group_name);
void amok_hm_group_shutdown_remote(gras_socket_t master, char *group_name);


/** @} */
#endif /* AMOK_HOST_MANAGEMENT_H */
