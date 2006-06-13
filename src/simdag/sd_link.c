#include "simdag/simdag.h"
#include "xbt/sysdep.h" /* xbt_new0 */

/* Creates a link.
 */
SG_link_t SG_link_create(void *data, const char *name, double capacity, double bandwidth, double latency) {
  xbt_assert0(capacity >= 0, "Invalid parameter"); /* or capacity > 0 ? */

  SG_link_t link = xbt_new0(s_SG_link_t, 1);

  link->data = data;
  link->name = xbt_strdup(name);
  link->capacity = capacity;
  link->current_bandwidth = bandwidth;
  link->current_latency = latency;

  return link;
}

/* Returns the user data of a link. The user data can be NULL.
 */
void* SG_link_get_data(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->data;
}

/* Sets the user data of a link. The data can be NULL.
 */
void SG_link_set_data(SG_link_t link, void *data) {
  xbt_assert0(link, "Invalid parameter");
  link->data = data;
}

/* Returns the name of a link. The name can be NULL.
 */
const char* SG_link_get_name(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->name;
}

/* Returns the capacity of a link.
 */
double SG_link_get_capacity(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->capacity;
}

/* Return the current bandwidth of a link.
 */
double SG_link_get_current_bandwidth(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->current_bandwidth;
}

/* Return the current latency of a link.
 */
double SG_link_get_current_latency(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->current_latency;
}

/* Destroys a link. The user data (if any) should have been destroyed first.
 */
void SG_link_destroy(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");

  if (link->name)
    free(link->name);

  free(link);
}
