#include "private.h"
#include "simdag/simdag.h"
#include "xbt/sysdep.h" /* xbt_new0 */

/* Creates a link.
 */
SD_link_t __SD_link_create(const char *name, void *surf_link, void *data) {
  xbt_assert0(surf_link != NULL, "surf_link is NULL !");
  SD_link_data_t sd_data = xbt_new0(s_SD_link_data_t, 1); /* link private data */
  sd_data->surf_link = surf_link;

  SD_link_t link = xbt_new0(s_SD_link_t, 1);
  link->name = xbt_strdup(name);
  link->data = data;
  link->sd_data = sd_data;

  /*link->capacity = capacity;*/
  /* link->current_bandwidth = bandwidth;
     link->current_latency = latency;*/

  return link;
}

/* Returns the user data of a link. The user data can be NULL.
 */
void* SD_link_get_data(SD_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->data;
}

/* Sets the user data of a link. The new data can be NULL. The old data should have been freed first if it was not NULL.
 */
void SD_link_set_data(SD_link_t link, void *data) {
  xbt_assert0(link, "Invalid parameter");
  link->data = data;
}

/* Returns the name of a link. The name can be NULL.
 */
const char* SD_link_get_name(SD_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->name;
}

/* Returns the capacity of a link.
 */
/*
double SD_link_get_capacity(SD_link_t link) {
  xbt_assert0(link, "Invalid parameter");
  return link->capacity;
}*/

/* Return the current bandwidth of a link.
 */
double SD_link_get_current_bandwidth(SD_link_t link) {
  xbt_assert0(link, "Invalid parameter");

  /* TODO */
  return 0;
  /*  return link->current_bandwidth;*/
}

/* Return the current latency of a link.
 */
double SD_link_get_current_latency(SD_link_t link) {
  xbt_assert0(link, "Invalid parameter");

  /* TODO */
  return 0;
  /*  return link->current_latency;*/
}

/* Destroys a link. The user data (if any) should have been destroyed first.
 */
void __SD_link_destroy(SD_link_t link) {
  xbt_assert0(link, "Invalid parameter");

  if (link->sd_data != NULL)
    xbt_free(link->sd_data);
  
  if (link->name != NULL)
    xbt_free(link->name);

  xbt_free(link);
}
