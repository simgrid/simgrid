/* $Id$ */

/* trp (transport) - send/receive a bunch of bytes                          */

/* This file implements the public interface of this module, exported to the*/
/*  other modules of GRAS, but not to the end user.                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_PRIVATE_H
#define GRAS_TRP_PRIVATE_H

#include "gras_private.h"
/* A low-level socket type (each plugin implements it the way it prefers */
//typedef void gras_trp_sock_t;
 
/* A plugin type */
struct gras_trp_plugin_ {
  const char *name;
 
  gras_error_t (*init)(void);
  void         (*exit)(gras_trp_plugin_t *);
 
  gras_error_t (*socket_client_open)(const char *host,
                                     unsigned short port,
                                     int raw,
                                     unsigned int bufSize,
                                     /* OUT */ gras_trp_sock_t **dst);
  gras_error_t (*socket_server_open)(unsigned short port,
                                     int raw,
                                     unsigned int bufSize,
                                     /* OUT */ gras_trp_sock_t **dst);
  void (*socket_close)(gras_trp_sock_t **sd);
 
  gras_error_t (*select)(double timeOut,
                         gras_trp_sock_t **sd);
   
  gras_error_t (*bloc_send)(gras_trp_sock_t *sd,
                            void *data,
                            size_t size,
                            double timeOut);
  gras_error_t (*bloc_recv)(gras_trp_sock_t *sd,
                            void *data,
                            size_t size,
                            double timeOut);
  gras_error_t (*flush)(gras_trp_sock_t *sd);
 
  void *specific;
};

/**********************************************************************
 * Internal stuff to the module. Other modules shouldn't fool with it *
 **********************************************************************/

/* TCP driver */
gras_error_t gras_trp_tcp_init(void);
void         gras_trp_tcp_exit(gras_trp_plugin_t *plugin);
gras_error_t gras_trp_tcp_socket_client(const char *host,
					unsigned short port,
					int raw, 
					unsigned int bufSize, 
					/* OUT */ gras_trp_sock_t **dst);
gras_error_t gras_trp_tcp_socket_server(unsigned short port,
					int raw, 
					unsigned int bufSize, 
					/* OUT */ gras_trp_sock_t **dst);
void         gras_trp_tcp_socket_close(gras_trp_sock_t **sd);
gras_error_t gras_trp_tcp_select(double timeOut,
				 gras_trp_sock_t **sd);
  
gras_error_t gras_trp_tcp_bloc_send(gras_trp_sock_t *sd,
				    void *data,
				    size_t size,
				    double timeOut);

gras_error_t gras_trp_tcp_bloc_recv(gras_trp_sock_t *sd,
				    void *data,
				    size_t size,
				    double timeOut);
gras_error_t gras_trp_tcp_flush(gras_trp_sock_t *sd);

/* SG driver */
gras_error_t gras_trp_sg_init(void);
void         gras_trp_sg_exit(gras_trp_plugin_t *plugin);
gras_error_t gras_trp_sg_socket_client(const char *host,
				       unsigned short port,
				       int raw, 
				       unsigned int bufSize, 
				       /* OUT */ gras_trp_sock_t **dst);
gras_error_t gras_trp_sg_socket_server(unsigned short port,
				       int raw, 
				       unsigned int bufSize, 
				       /* OUT */ gras_trp_sock_t **dst);
void         gras_trp_sg_socket_close(gras_trp_sock_t **sd);
gras_error_t gras_trp_sg_select(double timeOut,
				gras_trp_sock_t **sd);

gras_error_t gras_trp_sg_bloc_send(gras_trp_sock_t *sd,
				   void *data,
				   size_t size,
				   double timeOut);

gras_error_t gras_trp_sg_bloc_recv(gras_trp_sock_t *sd,
				   void *data,
				   size_t size,
				   double timeOut);
gras_error_t gras_trp_sg_flush(gras_trp_sock_t *sd);



#endif /* GRAS_TRP_PRIVATE_H */
