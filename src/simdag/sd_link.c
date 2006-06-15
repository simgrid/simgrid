#include "private.h"
#include "simdag/simdag.h"
#include "xbt/sysdep.h" /* xbt_new0 */

/* Creates a link.
 */
SG_link_t __SG_link_create(const char *name, void *surf_link, void *data) {
  xbt_assert0(surf_link != NULL, "surf_link is NULL !");
  SG_link_data_t sgdata = xbt_new0(s_SG_link_data_t, 1); /* link private data */
  sgdata->surf_link = surf_link;

  SG_link_t link = xbt_new0(s_SG_link_t, 1);
  link->name = xbt_strdup(name);
  link->data = data;
  link->sgdata = sgdata;

  /*link->capacity = capacity;*/
  /* link->current_bandwidth = bandwidth;
     link->current_latency = latency;*/

  return link;
}

/* Returns the user data of a link. The user data can be NULL.
 */
void* SG_link_get_data(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->data;
}

/* Sets the user data of a link. The new data can be NULL. The old data should have been freed first if it was not NULL.
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
/*
double SG_link_get_capacity(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->capacity;
}*/

/* Return the current bandwidth of a link.
 */
double SG_link_get_current_bandwidth(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");

  /* TODO */
  return 0;
  /*  return link->current_bandwidth;*/
}

/* Return the current latency of a link.
 */
double SG_link_get_current_latency(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");

  /* TODO */
  return 0;
  /*  return link->current_latency;*/
}

/* Destroys a link. The user data (if any) should have been destroyed first.
 */
void __SG_link_destroy(SG_link_t link) {
  xbt_assert0(link, "Invalid parameter");

  if (link->sgdata != NULL)
    xbt_free(link->sgdata);
  
  if (link->name != NULL)
    xbt_free(link->name);

  xbt_free(link);
}
