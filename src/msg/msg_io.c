/* Copyright (c) 2004 - 2013. The SimGrid Team.
 * All rights reserved.                                                       */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.   */

#include "msg_private.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_io, msg,
                                "Logging specific to MSG (io)");

/** @addtogroup msg_file_management
 *     \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Files" --> \endhtmlonly
 * (#msg_file_t) and the functions for managing it.
 *
 *  \see #msg_file_t
 */

/********************************* File **************************************/
void __MSG_file_get_info(msg_file_t fd){
  xbt_dynar_t info = simcall_file_get_info(fd->simdata->smx_file);
  fd->info->content_type = xbt_dynar_pop_as(info, char *);
  fd->info->storage_type = xbt_dynar_pop_as(info, char *);
  fd->info->storageId = xbt_dynar_pop_as(info, char *);
  fd->info->mount_point = xbt_dynar_pop_as(info, char *);
  fd->info->size = xbt_dynar_pop_as(info, size_t);

  xbt_dynar_free_container(&info);
}

/** \ingroup msg_file_management
 *
 * \brief Set the user data of a #msg_file_t.
 *
 * This functions checks whether some data has already been associated to \a file
   or not and attach \a data to \a file if it is possible.
 */
msg_error_t MSG_file_set_data(msg_file_t fd, void *data)
{
  SIMIX_file_set_data(fd->simdata->smx_file,data);

  return MSG_OK;
}

/** \ingroup msg_file_management
 *
 * \brief Return the user data of a #msg_file_t.
 *
 * This functions checks whether \a file is a valid pointer or not and return
   the user data associated to \a file if it is possible.
 */
void *MSG_file_get_data(msg_file_t fd)
{
  return SIMIX_file_get_data(fd->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Display information related to a file descriptor
 *
 * \param fd is a the file descriptor
 */

void MSG_file_dump (msg_file_t fd){
//   THROW_UNIMPLEMENTED;
  /* Update the cached information first */
  __MSG_file_get_info(fd);
  XBT_INFO("File Descriptor information:\n\t\tFull name: '%s'"
      "\n\t\tSize: %zu\n\t\tMount point: '%s'\n\t\tStorage Id: '%s'"
      "\n\t\tStorage Type: '%s'\n\t\tContent Type: '%s'",
      fd->fullname, fd->info->size, fd->info->mount_point, fd->info->storageId,
      fd->info->storage_type, fd->info->content_type);
}

/** \ingroup msg_file_management
 * \brief Read a file
 *
 * \param size of the file to read
 * \param fd is a the file descriptor
 * \return the number of bytes successfully read
 */
size_t MSG_file_read(size_t size, msg_file_t fd)
{
  return simcall_file_read(size, fd->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Write into a file
 *
 * \param size of the file to write
 * \param fd is a the file descriptor
 * \return the number of bytes successfully write
 */
size_t MSG_file_write(size_t size, msg_file_t fd)
{
  return simcall_file_write(size, fd->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Opens the file whose name is the string pointed to by path
 *
 * \param mount is the mount point where find the file is located
 * \param fullname is the file location on the storage
 * \param data user data to attach to the file
 *
 * \return An #msg_file_t associated to the file
 */
msg_file_t MSG_file_open(const char* mount, const char* fullname, void* data)
{
  msg_file_t file = xbt_new(s_msg_file_t,1);
  file->fullname = xbt_strdup(fullname);
  file->simdata = xbt_new0(s_simdata_file_t,1);
  file->info = xbt_new0(s_file_info_t,1);
  file->simdata->smx_file = simcall_file_open(mount, fullname);
  SIMIX_file_set_data(file->simdata->smx_file, data);
  return file;
}

/** \ingroup msg_file_management
 * \brief Close the file
 *
 * \param fd is the file to close
 * \return 0 on success or 1 on error
 */
int MSG_file_close(msg_file_t fd)
{
  int res = simcall_file_close(fd->simdata->smx_file);
  free(fd->fullname);
  xbt_free(fd->simdata);
  xbt_free(fd->info);
  xbt_free(fd);
  return res;
}

/** \ingroup msg_file_management
 * \brief Unlink the file pointed by fd
 *
 * \param fd is the file descriptor (#msg_file_t)
 * \return 0 on success or 1 on error
 */
int MSG_file_unlink(msg_file_t fd)
{
  return simcall_file_unlink(fd->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Return the size of a file
 *
 * \param fd is the file descriptor (#msg_file_t)
 * \return the size of the file (as a size_t)
 */

size_t MSG_file_get_size(msg_file_t fd){
  return simcall_file_get_size(fd->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Search for file
 *
 * \param mount is the mount point where find the file is located
 * \param path the file regex to find
 * \return a xbt_dict_t of file where key is the name of file and the
 * value the msg_stat_t corresponding to the key
 */
xbt_dict_t MSG_file_ls(const char *mount, const char *path)
{
  xbt_assert(path,"You must set path");
  int size = strlen(path);
  if(size && path[size-1] != '/')
  {
    char *new_path = bprintf("%s/",path);
    XBT_DEBUG("Change '%s' for '%s'",path,new_path);
    xbt_dict_t dict = simcall_file_ls(mount, new_path);
    xbt_free(new_path);
    return dict;
  }

  return simcall_file_ls(mount, path);
}

/********************************* Storage **************************************/

/** @addtogroup msg_storage_management
 *     \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Storages" --> \endhtmlonly
 * (#msg_storage_t) and the functions for managing it.
 *
 */

/** \ingroup msg_storage_management
 * \brief Returns the free space size of a storage element
 * \param the storage name (#char*)
 * \return the free space size of the storage element (as a size_t)
 */
size_t MSG_storage_get_free_size(const char* name){
  return simcall_storage_get_free_size(name);
}

/** \ingroup msg_storage_management
 * \brief Returns the used space size of a storage element
 * \param the storage name (#char*)
 * \return the used space size of the storage element (as a size_t)
 */
size_t MSG_storage_get_used_size(const char* name){
  return simcall_storage_get_used_size(name);
}

/** \ingroup msg_storage_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this storage
 * \param storage a storage
 * \return a dict containing the properties
 */
xbt_dict_t MSG_storage_get_properties(msg_storage_t storage)
{
  xbt_assert((storage != NULL), "Invalid parameters (storage is NULL)");
  return (simcall_storage_get_properties(storage->simdata->smx_storage));
}

/** \ingroup msg_storage_management
 * \brief Finds a msg_storage_t using its name.
 * \param name the name of a storage.
 * \return the corresponding storage
 */
msg_storage_t MSG_storage_get_by_name(const char *name)
{
  return (msg_storage_t) xbt_lib_get_elm_or_null(storage_lib,name);
}

/** \ingroup msg_storage_management
 * \brief Returns a dynar containing all the storage elements declared at a given point of time
 *
 *
 *
 * @TODO implement a new msg_storage_t structure that hides members (use xbt_dict)
 */
xbt_dynar_t MSG_storages_as_dynar(void) {

  xbt_lib_cursor_t cursor;
  char *key;
  void **data;
  xbt_dynar_t res = xbt_dynar_new(sizeof(msg_storage_t),NULL);
  msg_storage_t storage;
  xbt_lib_foreach(storage_lib, cursor, key, data) {
    if(routing_get_network_element_type(key) == ROUTING_STORAGE_LEVEL) {
      xbt_dictelm_t elm = xbt_dict_cursor_get_elm(cursor);
      storage = xbt_new(s_msg_storage_t, 1);
      storage->name = elm->key;
      storage->simdata = xbt_new0(s_simdata_storage_t,1);
      smx_storage_t simix_storage = xbt_lib_get_or_null(storage_lib, elm->key, SURF_STORAGE_LEVEL);
      storage->simdata->smx_storage = simix_storage;
      storage->data = NULL;
      xbt_dynar_push(res, &storage);
    }
  }

  return res;
}
