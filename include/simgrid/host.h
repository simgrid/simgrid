/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_HOST_H_
#define SIMGRID_HOST_H_

#include <xbt/dict.h>
#include <xbt/dynar.h>
#include <simgrid/forward.h>

SG_BEGIN_DECL
/** Returns an array of all existing hosts (use sg_host_count() to know the array size).
 *
 * @remark The host order in this array is generally different from the
 * creation/declaration order in the XML platform (we use a hash table
 * internally).
 */
XBT_PUBLIC sg_host_t* sg_host_list();

/** Returns the amount of hosts existing in the platform. */
XBT_PUBLIC size_t sg_host_count();

XBT_PUBLIC size_t sg_host_extension_create(void (*deleter)(void*));
XBT_PUBLIC void* sg_host_extension_get(const_sg_host_t host, size_t rank);

/** Finds a host from its name */
XBT_PUBLIC sg_host_t sg_host_by_name(const char* name);
XBT_PUBLIC sg_vm_t sg_vm_by_name(sg_host_t host, const char* name);

/** @brief Return the name of the sg_host_t. */
XBT_PUBLIC const char* sg_host_get_name(const_sg_host_t host);

// ========== User Data ==============
/** @brief Return the user data of a #sg_host_t.
 *
 * This functions returns the user data associated to @a host if any.
 */
XBT_PUBLIC void* sg_host_get_data(const_sg_host_t host);

/** @brief Set the user data of a #sg_host_t.
 *
 * This functions attach @a data to @a host.
 */
XBT_PUBLIC void sg_host_set_data(sg_host_t host, void* userdata);

// ========= storage related functions ============
XBT_PUBLIC void sg_host_get_disks(const_sg_host_t host, unsigned int* disk_count, sg_disk_t** disks);
XBT_PUBLIC sg_disk_t sg_host_get_disk_by_name(const_sg_host_t host, const char* name);

// =========== user-level functions ===============
/** @brief Return the speed of the processor (in flop/s), regardless of the current load on the machine. */
XBT_PUBLIC double sg_host_get_speed(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_pstate_speed(const_sg_host_t host, unsigned long pstate_index);

XBT_PUBLIC double sg_host_get_available_speed(const_sg_host_t host);

XBT_PUBLIC int sg_host_core_count(const_sg_host_t host);

/** @brief Returns the current computation load (in flops per second).
 * @param host a host
 */
XBT_PUBLIC double sg_host_get_load(const_sg_host_t host);

/** @brief Return the location on which the current process is running. */
XBT_PUBLIC sg_host_t sg_host_self();

XBT_PUBLIC const char* sg_host_self_get_name();

/** @brief Return the total count of pstates defined for a host.
 *
 * @beginrst
 * See also :ref:`plugin_host_energy`.
 * @endrst
 *
 * @param  host host to test
 */
XBT_PUBLIC unsigned long sg_host_get_nb_pstates(const_sg_host_t host);

XBT_PUBLIC unsigned long sg_host_get_pstate(const_sg_host_t host);
XBT_PUBLIC void sg_host_set_pstate(sg_host_t host, unsigned long pstate);

XBT_PUBLIC void sg_host_turn_on(sg_host_t host);
XBT_PUBLIC void sg_host_turn_off(sg_host_t host);
XBT_PUBLIC int sg_host_is_on(const_sg_host_t host);

#ifndef DOXYGEN
XBT_ATTRIB_DEPRECATED_v403("Please use sg_host_get_property_names instead: we want to kill xbt_dict at some point")
    XBT_PUBLIC xbt_dict_t sg_host_get_properties(const_sg_host_t host); // deprecated v3.39
XBT_ATTRIB_DEPRECATED_v403("Please use sg_host_get_route_links instead: we want to kill xbt_dynar at some point")
    XBT_PUBLIC void sg_host_get_route(const_sg_host_t from, const_sg_host_t to, xbt_dynar_t links);
XBT_ATTRIB_DEPRECATED_v403("Please use sg_host_get_actors instead: we want to kill xbt_dynar at some point") XBT_PUBLIC
    void sg_host_get_actor_list(const_sg_host_t host, xbt_dynar_t whereto);
#endif

/** @brief Returns a NULL-terminated list of the existing properties' names.
 *
 * if @c size is not null, the properties count is also stored in it
 * Only free the vector after use, do not mess with the names stored in it as they are the original strings, not copies.
 */
XBT_PUBLIC const char** sg_host_get_property_names(const_sg_host_t host, int* size);

/** @ingroup m_host_management
 * @brief Returns the value of a given host property
 *
 * @param host a host
 * @param name a property name
 * @return value of a property (or nullptr if property not set)
 */
XBT_PUBLIC const char* sg_host_get_property_value(const_sg_host_t host, const char* name);

/** @brief Change the value of a given host property
 *
 * @param host a host
 * @param name a property name
 * @param value what to change the property to
 */
XBT_PUBLIC void sg_host_set_property_value(sg_host_t host, const char* name, const char* value);

/** @brief Returns a NULL-terminated list of links.
 *
 * if @c size is not null, the properties count is also stored in it
 *
 * Only free the vector after use, do not mess with the data stored in it as they are the original ones, not copies.
 */
XBT_PUBLIC const_sg_link_t* sg_host_get_route_links(const_sg_host_t from, const_sg_host_t to, int* size);
XBT_PUBLIC double sg_host_get_route_latency(const_sg_host_t from, const_sg_host_t to);
XBT_PUBLIC double sg_host_get_route_bandwidth(const_sg_host_t from, const_sg_host_t to);
XBT_PUBLIC void sg_host_sendto(sg_host_t from, sg_host_t to, double byte_amount);

/** @brief Returns a NULL-terminated list of actors.
 *
 * if @c size is not null, the properties count is also stored in it
 *
 * Only free the vector after use, do not mess with the data stored in it as they are the original ones, not copies.
 */
XBT_PUBLIC const_sg_actor_t* sg_host_get_actors(const_sg_host_t host, int* size);
SG_END_DECL

#endif /* SIMGRID_HOST_H_ */
