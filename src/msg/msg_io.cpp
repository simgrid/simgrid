/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "src/msg/msg_private.hpp"
#include "src/plugins/file_system/FileSystem.hpp"
#include <numeric>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_io, msg, "Logging specific to MSG (io)");

extern "C" {

/********************************* Storage **************************************/
/** @addtogroup msg_storage_management
 * (#msg_storage_t) and the functions for managing it.
 */

/** \ingroup msg_storage_management
 *
 * \brief Returns the name of the #msg_storage_t.
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char* MSG_storage_get_name(msg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->getCname();
}

const char* MSG_storage_get_host(msg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->getHost()->getCname();
}

/** \ingroup msg_storage_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this storage
 * \param storage a storage
 * \return a dict containing the properties
 */
xbt_dict_t MSG_storage_get_properties(msg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters (storage is nullptr)");
  xbt_dict_t as_dict = xbt_dict_new_homogeneous(xbt_free_f);
  std::map<std::string, std::string>* props = storage->getProperties();
  if (props == nullptr)
    return nullptr;
  for (auto const& elm : *props) {
    xbt_dict_set(as_dict, elm.first.c_str(), xbt_strdup(elm.second.c_str()), nullptr);
  }
  return as_dict;
}

/** \ingroup msg_storage_management
 * \brief Change the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \param value what to change the property to
 */
void MSG_storage_set_property_value(msg_storage_t storage, const char* name, char* value)
{
  storage->setProperty(name, value);
}

/** \ingroup m_storage_management
 * \brief Returns the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \return value of a property (or nullptr if property not set)
 */
const char *MSG_storage_get_property_value(msg_storage_t storage, const char *name)
{
  return storage->getProperty(name);
}

/** \ingroup msg_storage_management
 * \brief Finds a msg_storage_t using its name.
 * \param name the name of a storage
 * \return the corresponding storage
 */
msg_storage_t MSG_storage_get_by_name(const char *name)
{
  return simgrid::s4u::Storage::byName(name);
}

/** \ingroup msg_storage_management
 * \brief Returns a dynar containing all the storage elements declared at a given point of time
 */
xbt_dynar_t MSG_storages_as_dynar()
{
  std::map<std::string, simgrid::s4u::Storage*>* storage_list = new std::map<std::string, simgrid::s4u::Storage*>;
  simgrid::s4u::getStorageList(storage_list);
  xbt_dynar_t res = xbt_dynar_new(sizeof(msg_storage_t),nullptr);
  for (auto const& s : *storage_list)
    xbt_dynar_push(res, &(s.second));
  delete storage_list;
  return res;
}

void* MSG_storage_get_data(msg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->getUserdata();
}

msg_error_t MSG_storage_set_data(msg_storage_t storage, void *data)
{
  storage->setUserdata(data);
  return MSG_OK;
}

sg_size_t MSG_storage_read(msg_storage_t storage, sg_size_t size)
{
  return storage->read(size);
}

sg_size_t MSG_storage_write(msg_storage_t storage, sg_size_t size)
{
  return storage->write(size);
}

}
