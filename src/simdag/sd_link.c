#include "private.h"
#include "simdag/simdag.h"
#include "surf/surf.h"
#include "xbt/sysdep.h" /* xbt_new0 */

/* Creates a link.
 */
SD_link_t __SD_link_create(void *surf_link, char *name, void *data) {
  CHECK_INIT_DONE();
  xbt_assert0(surf_link != NULL, "surf_link is NULL !");
  xbt_assert0(name != NULL, "name is NULL !");

  SD_link_data_t sd_data = xbt_new0(s_SD_link_data_t, 1); /* link private data */
  sd_data->surf_link = surf_link;
  sd_data->name = xbt_strdup(name);

  SD_link_t link = xbt_new0(s_SD_link_t, 1);
  link->sd_data = sd_data; /* private data */
  link->data = data; /* user data */

  /*link->capacity = capacity;*/
  /* link->current_bandwidth = bandwidth;
     link->current_latency = latency;*/

  /*xbt_dynar_push(sd_global->links, link);*/
  xbt_dict_set(sd_global->links, name, link, __SD_link_destroy); /* add the workstation to the dictionary */

  return link;
}

/* Returns the user data of a link. The user data can be NULL.
 */
void* SD_link_get_data(SD_link_t link) {
  CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return link->data;
}

/* Sets the user data of a link. The new data can be NULL. The old data should have been freed first if it was not NULL.
 */
void SD_link_set_data(SD_link_t link, void *data) {
  CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  link->data = data;
}

/* Returns the name of a link. The name can be NULL.
 */
const char* SD_link_get_name(SD_link_t link) {
  CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return link->sd_data->name;

  /*  return surf_network_resource->common_public->get_resource_name(link->sd_data->surf_link);*/
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
  CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");

  /* TODO */
  return 0;
  /*  return link->current_bandwidth;*/
}

/* Return the current latency of a link.
 */
double SD_link_get_current_latency(SD_link_t link) {
  CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");

  /* TODO */
  return 0;
  /*  return link->current_latency;*/
}

/* Destroys a link. The user data (if any) should have been destroyed first.
 */
void __SD_link_destroy(void *link) {
  CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");

  if (((SD_link_t) link)->sd_data != NULL)
    xbt_free(((SD_link_t) link)->sd_data);

  /* TODO: name */
  
  xbt_free(link);
}

