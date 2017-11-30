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

/** @addtogroup msg_file
 * (#msg_file_t) and the functions for managing it.
 *
 *  \see #msg_file_t
 */

/** \ingroup msg_file
 * \brief Read a file (local or remote)
 *
 * \param size of the file to read
 * \param fd is a the file descriptor
 * \return the number of bytes successfully read or -1 if an error occurred
 */
sg_size_t MSG_file_read(msg_file_t fd, sg_size_t size)
{
  sg_size_t read_size;

  if (fd->size() == 0) /* Nothing to read, return */
    return 0;

  /* Find the host where the file is physically located and read it */
  msg_storage_t storage_src           = fd->localStorage;
  msg_host_t attached_host            = storage_src->getHost();
  read_size                           = fd->read(size);

  if (strcmp(attached_host->getCname(), MSG_host_self()->getCname())) {
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", attached_host->getCname(), read_size);
    msg_host_t m_host_list[] = {MSG_host_self(), attached_host};
    double flops_amount[]    = {0, 0};
    double bytes_amount[]    = {0, 0, static_cast<double>(read_size), 0};

    msg_task_t task = MSG_parallel_task_create("file transfer for read", 2, m_host_list, flops_amount, bytes_amount,
                      nullptr);
    msg_error_t transfer = MSG_parallel_task_execute(task);
    MSG_task_destroy(task);

    if(transfer != MSG_OK){
      if (transfer == MSG_HOST_FAILURE)
        XBT_WARN("Transfer error, %s remote host just turned off!", attached_host->getCname());
      if (transfer == MSG_TASK_CANCELED)
        XBT_WARN("Transfer error, task has been canceled!");

      return -1;
    }
  }
  return read_size;
}

/** \ingroup msg_file
 * \brief Write into a file (local or remote)
 *
 * \param size of the file to write
 * \param fd is a the file descriptor
 * \return the number of bytes successfully write or -1 if an error occurred
 */
sg_size_t MSG_file_write(msg_file_t fd, sg_size_t size)
{
  if (size == 0) /* Nothing to write, return */
    return 0;

  /* Find the host where the file is physically located (remote or local)*/
  msg_storage_t storage_src = fd->localStorage;
  msg_host_t attached_host  = storage_src->getHost();

  if (strcmp(attached_host->getCname(), MSG_host_self()->getCname())) {
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", attached_host->getCname(), size);
    msg_host_t m_host_list[] = {MSG_host_self(), attached_host};
    double flops_amount[]    = {0, 0};
    double bytes_amount[]    = {0, static_cast<double>(size), 0, 0};

    msg_task_t task = MSG_parallel_task_create("file transfer for write", 2, m_host_list, flops_amount, bytes_amount,
                                               nullptr);
    msg_error_t transfer = MSG_parallel_task_execute(task);
    MSG_task_destroy(task);

    if(transfer != MSG_OK){
      if (transfer == MSG_HOST_FAILURE)
        XBT_WARN("Transfer error, %s remote host just turned off!", attached_host->getCname());
      if (transfer == MSG_TASK_CANCELED)
        XBT_WARN("Transfer error, task has been canceled!");

      return -1;
    }
  }
  /* Write file on local or remote host */
  sg_size_t write_size = fd->write(size);

  return write_size;
}


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
  std::map<std::string, simgrid::s4u::Storage*>* storage_map = simgrid::s4u::allStorages();
  xbt_dynar_t res = xbt_dynar_new(sizeof(msg_storage_t),nullptr);
  for (auto const& s : *storage_map)
    xbt_dynar_push(res, &(s.second));
  delete storage_map;
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
