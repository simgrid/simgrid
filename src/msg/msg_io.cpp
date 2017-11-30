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

static int MSG_host_get_file_descriptor_id(msg_host_t host)
{
  simgrid::MsgHostExt* priv = host->extension<simgrid::MsgHostExt>();
  if (priv->file_descriptor_table == nullptr) {
    priv->file_descriptor_table = new std::vector<int>(sg_storage_max_file_descriptors);
    std::iota(priv->file_descriptor_table->rbegin(), priv->file_descriptor_table->rend(), 0); // Fill with ..., 1, 0.
  }
  xbt_assert(not priv->file_descriptor_table->empty(), "Too much files are opened! Some have to be closed.");
  int desc = priv->file_descriptor_table->back();
  priv->file_descriptor_table->pop_back();
  return desc;
}

static void MSG_host_release_file_descriptor_id(msg_host_t host, int id)
{
  host->extension<simgrid::MsgHostExt>()->file_descriptor_table->push_back(id);
}

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

/** \ingroup msg_file
 * \brief Opens the file whose name is the string pointed to by path
 *
 * \param fullpath is the file location on the storage
 * \param data user data to attach to the file
 *
 * \return An #msg_file_t associated to the file
 */
msg_file_t MSG_file_open(const char* fullpath, void* data)
{
  msg_file_t fd         = new simgrid::s4u::File(fullpath, MSG_host_self());
  fd->desc_id           = MSG_host_get_file_descriptor_id(MSG_host_self());
  fd->setUserdata(data);
  return fd;
}

/** \ingroup msg_file
 * \brief Close the file
 *
 * \param fd is the file to close
 * \return 0 on success or 1 on error
 */
int MSG_file_close(msg_file_t fd)
{
  MSG_host_release_file_descriptor_id(MSG_host_self(), fd->desc_id);
  delete fd;

  return MSG_OK;
}

/**
 * \ingroup msg_file
 * \brief Copy a file to another location on a remote host.
 * \param file : the file to move
 * \param host : the remote host where the file has to be copied
 * \param fullpath : the complete path destination on the remote host
 * \return If successful, the function returns MSG_OK. Otherwise, it returns MSG_TASK_CANCELED.
 */
msg_error_t MSG_file_rcopy (msg_file_t file, msg_host_t host, const char* fullpath)
{
  /* Find the host where the file is physically located and read it */
  msg_storage_t storage_src = file->localStorage;
  msg_host_t src_host       = storage_src->getHost();
  MSG_file_seek(file, 0, SEEK_SET);
  sg_size_t read_size = file->read(file->size());

  /* Find the host that owns the storage where the file has to be copied */
  msg_storage_t storage_dest = nullptr;
  msg_host_t dst_host;
  size_t longest_prefix_length = 0;

  for (auto const& elm : host->getMountedStorages()) {
    std::string mount_point = std::string(fullpath).substr(0, elm.first.size());
    if (mount_point == elm.first && elm.first.length() > longest_prefix_length) {
      /* The current mount name is found in the full path and is bigger than the previous*/
      longest_prefix_length = elm.first.length();
      storage_dest          = elm.second;
    }
  }

  if (storage_dest != nullptr) {
    /* Mount point found, retrieve the host the storage is attached to */
    dst_host = storage_dest->getHost();
  }else{
    XBT_WARN("Can't find mount point for '%s' on destination host '%s'", fullpath, host->getCname());
    return MSG_TASK_CANCELED;
  }

  XBT_DEBUG("Initiate data transfer of %llu bytes between %s and %s.", read_size, src_host->getCname(),
            storage_dest->getHost()->getCname());
  msg_host_t m_host_list[] = {src_host, dst_host};
  double flops_amount[]    = {0, 0};
  double bytes_amount[]    = {0, static_cast<double>(read_size), 0, 0};

  msg_task_t task =
      MSG_parallel_task_create("file transfer for write", 2, m_host_list, flops_amount, bytes_amount, nullptr);
  msg_error_t err = MSG_parallel_task_execute(task);
  MSG_task_destroy(task);

  if (err != MSG_OK) {
    if (err == MSG_HOST_FAILURE)
      XBT_WARN("Transfer error, %s remote host just turned off!", storage_dest->getHost()->getCname());
    if (err == MSG_TASK_CANCELED)
      XBT_WARN("Transfer error, task has been canceled!");

    return err;
  }

  /* Create file on remote host, write it and close it */
  msg_file_t fd = new simgrid::s4u::File(fullpath, dst_host, nullptr);
  fd->write(read_size);
  delete fd;
  return MSG_OK;
}

/**
 * \ingroup msg_file
 * \brief Move a file to another location on a remote host.
 * \param file : the file to move
 * \param host : the remote host where the file has to be moved
 * \param fullpath : the complete path destination on the remote host
 * \return If successful, the function returns MSG_OK. Otherwise, it returns MSG_TASK_CANCELED.
 */
msg_error_t MSG_file_rmove (msg_file_t file, msg_host_t host, const char* fullpath)
{
  msg_error_t res = MSG_file_rcopy(file, host, fullpath);
  MSG_file_unlink(file);
  return res;
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
