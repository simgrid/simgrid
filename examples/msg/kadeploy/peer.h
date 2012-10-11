#ifndef KADEPLOY_PEER_H
#define KADEPLOY_PEER_H

#include "msg/msg.h"
#include "xbt/sysdep.h"

#include "messages.h"

#define PEER_SHUTDOWN_DEADLINE 6000

/* Peer struct */
typedef struct s_peer {
  int init;
  const char *prev;
  const char *next;
  const char *me;
  int pieces;
  xbt_dynar_t pending_sends;
  int close_asap; /* TODO: unused */
} s_peer_t, *peer_t;

/* Peer: helper functions */
msg_error_t peer_wait_for_message(peer_t peer);
int peer_execute_task(peer_t peer, msg_task_t task);
void peer_init_chain(peer_t peer, message_t msg);
void peer_shutdown(peer_t p);
void peer_init(peer_t p);

int peer(int argc, char *argv[]);

#endif /* KADEPLOY_PEER_H */
