/* $Id$ */

/* gras_dummy - dummy implementation of function depending on the RL or SG, */
/*              so that we can build a dummy library to test the Utils      */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

xbt_error_t gras_process_init() {
  return unknown_error;
}

xbt_error_t gras_process_exit() {
  return unknown_error;
}

xbt_error_t gras_sock_client_open(const char *host, short port, 
				   /* OUT */ gras_sock_t **sock) {
  return unknown_error;
}


xbt_error_t gras_sock_server_open(unsigned short startingPort, 
				   unsigned short endingPort,
				   /* OUT */ gras_sock_t **sock) {

  return unknown_error;
}

xbt_error_t gras_sock_close(gras_sock_t *sock) {
  return unknown_error;
}

unsigned short gras_sock_get_my_port(gras_sock_t *sd) {
  return -1;
}

unsigned short gras_sock_get_peer_port(gras_sock_t *sd) {
  return -1;
}

char * gras_sock_get_peer_name(gras_sock_t *sd) {
  return NULL;
}

xbt_error_t grasMsgRecv(gras_msg_t **msg,
			 double timeOut) {
  return unknown_error;
}
xbt_error_t gras_msg_send(gras_sock_t *sd,
			   gras_msg_t *msg,
			   e_xbt_free_directive_t freeDirective) {
  return unknown_error;
}
xbt_error_t gras_rawsock_server_open(unsigned short startingPort, 
                                  unsigned short endingPort,
                                  unsigned int bufSize, gras_rawsock_t **sock) {
  return unknown_error;
}
xbt_error_t gras_rawsock_client_open(const char *host, short port, 
                                  unsigned int bufSize, gras_rawsock_t **sock) {
  return unknown_error;
}
xbt_error_t gras_rawsock_close(gras_rawsock_t *sd) {
  return unknown_error;
}
unsigned short gras_rawsock_get_peer_port(gras_rawsock_t *sd) {
  return -1;
}
xbt_error_t
gras_rawsock_recv(gras_rawsock_t *sd, unsigned int expSize, unsigned int msgSize, 
		  unsigned int timeout) {
  return unknown_error;
}
xbt_error_t
gras_rawsock_send(gras_rawsock_t *sd, unsigned int expSize, unsigned int msgSize){
  return unknown_error;
}
gras_process_data_t *gras_process_data_get() {
  return NULL;
}

double gras_time() {
  return 0;
}

void gras_sleep(unsigned long sec,unsigned long usec) {}

