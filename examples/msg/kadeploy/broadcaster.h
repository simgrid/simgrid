#ifndef KADEPLOY_BROADCASTER_H
#define KADEPLOY_BROADCASTER_H

#include "msg/msg.h"
#include "xbt/sysdep.h"

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

#include "messages.h"
#include "iterator.h"

#define HOSTNAME_LENGTH 20
#define PIECE_COUNT 1000

xbt_dynar_t build_hostlist_from_hostcount(int hostcount); 
/*xbt_dynar_t build_hostlist_from_argv(int argc, char *argv[]);*/

/* Broadcaster: helper functions */
int broadcaster_build_chain(const char **first, xbt_dynar_t host_list);
int broadcaster_send_file(const char *first);
int broadcaster_finish(xbt_dynar_t host_list);

/* Tasks */
int broadcaster(int argc, char *argv[]);

#endif /* KADEPLOY_BROADCASTER_H */
