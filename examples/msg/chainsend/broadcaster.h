#ifndef KADEPLOY_BROADCASTER_H
#define KADEPLOY_BROADCASTER_H

#include "msg/msg.h"
#include "xbt/sysdep.h"

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

#include "messages.h"
#include "iterator.h"
#include "common.h"

/* Connection parameters */
#define MAX_PENDING_SENDS 10

/* Default values for the ``file'' details */
#define PIECE_SIZE 65536
#define PIECE_COUNT 16384

/* Broadcaster struct */
typedef struct s_broadcaster {
  const char *first;
  int piece_count;
  int current_piece;
  xbt_dynar_t host_list;
  xbt_dynar_iterator_t it;
  int max_pending_sends;
  xbt_dynar_t pending_sends;
} s_broadcaster_t, *broadcaster_t;

xbt_dynar_t build_hostlist_from_hostcount(int hostcount); 

/* Broadcaster: helper functions */
broadcaster_t broadcaster_init(xbt_dynar_t host_list, unsigned int piece_count);
int broadcaster_build_chain(broadcaster_t bc);
int broadcaster_send_file(broadcaster_t bc);
int broadcaster_finish(broadcaster_t bc);
static void broadcaster_destroy(broadcaster_t bc);

/* Tasks */
int broadcaster(int argc, char *argv[]);

#endif /* KADEPLOY_BROADCASTER_H */
