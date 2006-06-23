#include "private.h"
#include "simdag/simdag.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

/* Creates a link.
 */
SD_link_t __SD_link_create(void *surf_link, void *data) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(surf_link != NULL, "surf_link is NULL !");


  SD_link_t link = xbt_new0(s_SD_link_t, 1);
  link->surf_link = surf_link;
  link->data = data; /* user data */

  const char *name = SD_link_get_name(link);
  xbt_dict_set(sd_global->links, name, link, __SD_link_destroy); /* add the workstation to the dictionary */

  return link;
}

/* Returns the user data of a link. The user data can be NULL.
 */
void* SD_link_get_data(SD_link_t link) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return link->data;
}

/* Sets the user data of a link. The new data can be NULL. The old data should have been freed first if it was not NULL.
 */
void SD_link_set_data(SD_link_t link, void *data) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  link->data = data;
}

/* Returns the name of a link. The name cannot be NULL.
 */
const char* SD_link_get_name(SD_link_t link) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_link_name(link->surf_link);
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_link_bandwidth(link->surf_link);
}

/* Return the current latency of a link.
 */
double SD_link_get_current_latency(SD_link_t link) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_link_latency(link->surf_link);
}

/* Destroys a link. The user data (if any) should have been destroyed first.
 */
void __SD_link_destroy(void *link) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  /* link->surf_link is freed by surf_exit and link->data is freed by the user */
  xbt_free(link);
}

