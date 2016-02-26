/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_io, msg, "Logging specific to MSG (io)");

/** @addtogroup msg_file_management
 * \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Files" --> \endhtmlonly
 * (#msg_file_t) and the functions for managing it.
 *
 *  \see #msg_file_t
 */

/********************************* File **************************************/
void __MSG_file_get_info(msg_file_t fd){

  msg_file_priv_t priv = MSG_file_priv(fd);
  xbt_dynar_t info = simcall_file_get_info(priv->simdata->smx_file);
  sg_size_t *psize;

  priv->content_type = xbt_dynar_pop_as(info, char *);
  priv->storage_type = xbt_dynar_pop_as(info, char *);
  priv->storageId = xbt_dynar_pop_as(info, char *);
  priv->mount_point = xbt_dynar_pop_as(info, char *);
  psize = xbt_dynar_pop_as(info, sg_size_t*);
  priv->size = *psize;
  xbt_free(psize);
  xbt_dynar_free_container(&info);
}

/** \ingroup msg_file_management
 *
 * \brief Set the user data of a #msg_file_t.
 *
 * This functions attach \a data to \a file.
 */
msg_error_t MSG_file_set_data(msg_file_t fd, void *data)
{
  msg_file_priv_t priv = MSG_file_priv(fd);
  priv->data = data;
  return MSG_OK;
}

/** \ingroup msg_file_management
 *
 * \brief Return the user data of a #msg_file_t.
 *
 * This functions checks whether \a file is a valid pointer and return the user data associated to \a file if possible.
 */
void *MSG_file_get_data(msg_file_t fd)
{
  msg_file_priv_t priv = MSG_file_priv(fd);
  return priv->data;
}

/** \ingroup msg_file_management
 * \brief Display information related to a file descriptor
 *
 * \param fd is a the file descriptor
 */
void MSG_file_dump (msg_file_t fd){
  /* Update the cached information first */
  __MSG_file_get_info(fd);

  msg_file_priv_t priv = MSG_file_priv(fd);
  XBT_INFO("File Descriptor information:\n"
           "\t\tFull path: '%s'\n"
           "\t\tSize: %llu\n"
           "\t\tMount point: '%s'\n"
           "\t\tStorage Id: '%s'\n"
           "\t\tStorage Type: '%s'\n"
           "\t\tContent Type: '%s'\n"
           "\t\tFile Descriptor Id: %d",
           priv->fullpath, priv->size, priv->mount_point,
           priv->storageId, priv->storage_type,
           priv->content_type, priv->desc_id);
}

/** \ingroup msg_file_management
 * \brief Read a file (local or remote)
 *
 * \param size of the file to read
 * \param fd is a the file descriptor
 * \return the number of bytes successfully read or -1 if an error occurred
 */
sg_size_t MSG_file_read(msg_file_t fd, sg_size_t size)
{
  msg_file_priv_t file_priv = MSG_file_priv(fd);
  sg_size_t read_size;

  /* Find the host where the file is physically located and read it */
  msg_storage_t storage_src =(msg_storage_t) xbt_lib_get_elm_or_null(storage_lib, file_priv->storageId);
  msg_storage_priv_t storage_priv_src = MSG_storage_priv(storage_src);
  msg_host_t attached_host = MSG_host_by_name(storage_priv_src->hostname);
  read_size = simcall_file_read(file_priv->simdata->smx_file, size, attached_host);

  if(strcmp(storage_priv_src->hostname, MSG_host_get_name(MSG_host_self()))){
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", storage_priv_src->hostname, read_size);
    msg_host_t *m_host_list = NULL;
    m_host_list = (msg_host_t*) calloc(2, sizeof(msg_host_t));

    m_host_list[0] = MSG_host_self();
    m_host_list[1] = attached_host;
    double flops_amount[] = { 0, 0 };
    double bytes_amount[] = { 0, 0, (double)read_size, 0 };

    msg_task_t task = MSG_parallel_task_create("file transfer for read", 2, m_host_list, flops_amount, bytes_amount,
                      NULL);
    msg_error_t transfer = MSG_parallel_task_execute(task);
    MSG_task_destroy(task);
    free(m_host_list);
    if(transfer != MSG_OK){
      if (transfer == MSG_HOST_FAILURE)
        XBT_WARN("Transfer error, %s remote host just turned off!", MSG_host_get_name(attached_host));
      if (transfer == MSG_TASK_CANCELED)
        XBT_WARN("Transfer error, task has been canceled!");

      return -1;
    }
  }
  return read_size;
}

/** \ingroup msg_file_management
 * \brief Write into a file (local or remote)
 *
 * \param size of the file to write
 * \param fd is a the file descriptor
 * \return the number of bytes successfully write or -1 if an error occurred
 */
sg_size_t MSG_file_write(msg_file_t fd, sg_size_t size)
{
  msg_file_priv_t file_priv = MSG_file_priv(fd);
  sg_size_t write_size, offset;

  /* Find the host where the file is physically located (remote or local)*/
  msg_storage_t storage_src =(msg_storage_t) xbt_lib_get_elm_or_null(storage_lib, file_priv->storageId);
  msg_storage_priv_t storage_priv_src = MSG_storage_priv(storage_src);
  msg_host_t attached_host = MSG_host_by_name(storage_priv_src->hostname);

  if(strcmp(storage_priv_src->hostname, MSG_host_get_name(MSG_host_self()))){
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", storage_priv_src->hostname, size);
    msg_host_t *m_host_list = NULL;
    m_host_list = (msg_host_t*) calloc(2, sizeof(msg_host_t));

    m_host_list[0] = MSG_host_self();
    m_host_list[1] = attached_host;
    double flops_amount[] = { 0, 0 };
    double bytes_amount[] = { 0, (double)size, 0, 0 };

    msg_task_t task = MSG_parallel_task_create("file transfer for write", 2, m_host_list, flops_amount, bytes_amount,
                                               NULL);
    msg_error_t transfer = MSG_parallel_task_execute(task);
    MSG_task_destroy(task);
    free(m_host_list);
    if(transfer != MSG_OK){
      if (transfer == MSG_HOST_FAILURE)
        XBT_WARN("Transfer error, %s remote host just turned off!", MSG_host_get_name(attached_host));
      if (transfer == MSG_TASK_CANCELED)
        XBT_WARN("Transfer error, task has been canceled!");

      return -1;
    }
  }
  /* Write file on local or remote host */
  offset = simcall_file_tell(file_priv->simdata->smx_file);
  write_size = simcall_file_write(file_priv->simdata->smx_file, size, attached_host);
  file_priv->size = offset+write_size;

  return write_size;
}

/** \ingroup msg_file_management
 * \brief Opens the file whose name is the string pointed to by path
 *
 * \param fullpath is the file location on the storage
 * \param data user data to attach to the file
 *
 * \return An #msg_file_t associated to the file
 */
msg_file_t MSG_file_open(const char* fullpath, void* data)
{
  char *name;
  msg_file_priv_t priv = xbt_new(s_msg_file_priv_t, 1);
  priv->data = data;
  priv->fullpath = xbt_strdup(fullpath);
  priv->simdata = xbt_new0(s_simdata_file_t,1);
  priv->simdata->smx_file = simcall_file_open(fullpath, MSG_host_self());
  priv->desc_id = __MSG_host_get_file_descriptor_id(MSG_host_self());

  name = bprintf("%s:%s:%d", priv->fullpath, MSG_host_get_name(MSG_host_self()), priv->desc_id);

  xbt_lib_set(file_lib, name, MSG_FILE_LEVEL, priv);
  msg_file_t fd = (msg_file_t) xbt_lib_get_elm_or_null(file_lib, name);
  __MSG_file_get_info(fd);
  xbt_free(name);

  return fd;
}

/** \ingroup msg_file_management
 * \brief Close the file
 *
 * \param fd is the file to close
 * \return 0 on success or 1 on error
 */
int MSG_file_close(msg_file_t fd)
{
  char *name;
  msg_file_priv_t priv = MSG_file_priv(fd);
  if (priv->data)
    xbt_free(priv->data);

  int res = simcall_file_close(priv->simdata->smx_file, MSG_host_self());
  name = bprintf("%s:%s:%d", priv->fullpath, MSG_host_get_name(MSG_host_self()), priv->desc_id);
  __MSG_host_release_file_descriptor_id(MSG_host_self(), priv->desc_id);
  xbt_lib_unset(file_lib, name, MSG_FILE_LEVEL, 1);
  xbt_free(name);
  return res;
}

/** \ingroup msg_file_management
 * \brief Unlink the file pointed by fd
 *
 * \param fd is the file descriptor (#msg_file_t)
 * \return 0 on success or 1 on error
 */
msg_error_t MSG_file_unlink(msg_file_t fd)
{
  msg_file_priv_t file_priv = MSG_file_priv(fd);
  /* Find the host where the file is physically located (remote or local)*/
  msg_storage_t storage_src =
      (msg_storage_t) xbt_lib_get_elm_or_null(storage_lib, file_priv->storageId);
  msg_storage_priv_t storage_priv_src = MSG_storage_priv(storage_src);
  msg_host_t attached_host = MSG_host_by_name(storage_priv_src->hostname);
  int res = simcall_file_unlink(file_priv->simdata->smx_file, attached_host);
  return (msg_error_t) res;
}

/** \ingroup msg_file_management
 * \brief Return the size of a file
 *
 * \param fd is the file descriptor (#msg_file_t)
 * \return the size of the file (as a #sg_size_t)
 */
sg_size_t MSG_file_get_size(msg_file_t fd){
  msg_file_priv_t priv = MSG_file_priv(fd);
  return simcall_file_get_size(priv->simdata->smx_file);
}

/**
 * \ingroup msg_file_management
 * \brief Set the file position indicator in the msg_file_t by adding offset bytes
 * to the position specified by origin (either SEEK_SET, SEEK_CUR, or SEEK_END).
 *
 * \param fd : file object that identifies the stream
 * \param offset : number of bytes to offset from origin
 * \param origin : Position used as reference for the offset. It is specified by one of the following constants defined
 *                 in \<stdio.h\> exclusively to be used as arguments for this function (SEEK_SET = beginning of file,
 *                 SEEK_CUR = current position of the file pointer, SEEK_END = end of file)
 * \return If successful, the function returns MSG_OK (=0). Otherwise, it returns MSG_TASK_CANCELED (=8).
 */
msg_error_t MSG_file_seek(msg_file_t fd, sg_offset_t offset, int origin)
{
  msg_file_priv_t priv = MSG_file_priv(fd);
  return (msg_error_t) simcall_file_seek(priv->simdata->smx_file, offset, origin);
}

/**
 * \ingroup msg_file_management
 * \brief Returns the current value of the position indicator of the file
 *
 * \param fd : file object that identifies the stream
 * \return On success, the current value of the position indicator is returned.
 *
 */
sg_size_t MSG_file_tell(msg_file_t fd)
{
  msg_file_priv_t priv = MSG_file_priv(fd);
  return simcall_file_tell(priv->simdata->smx_file);
}

const char *MSG_file_get_name(msg_file_t fd) {
  xbt_assert((fd != NULL), "Invalid parameters");
  msg_file_priv_t priv = MSG_file_priv(fd);
  return priv->fullpath;
}

/**
 * \ingroup msg_file_management
 * \brief Move a file to another location on the *same mount point*.
 *
 */
msg_error_t MSG_file_move (msg_file_t fd, const char* fullpath)
{
  msg_file_priv_t priv = MSG_file_priv(fd);
  return (msg_error_t) simcall_file_move(priv->simdata->smx_file, fullpath);
}

/**
 * \ingroup msg_file_management
 * \brief Copy a file to another location on a remote host.
 * \param file : the file to move
 * \param host : the remote host where the file has to be copied
 * \param fullpath : the complete path destination on the remote host
 * \return If successful, the function returns MSG_OK. Otherwise, it returns MSG_TASK_CANCELED.
 */
msg_error_t MSG_file_rcopy (msg_file_t file, msg_host_t host, const char* fullpath)
{
  msg_file_priv_t file_priv = MSG_file_priv(file);
  sg_size_t read_size;

  /* Find the host where the file is physically located and read it */
  msg_storage_t storage_src =(msg_storage_t) xbt_lib_get_elm_or_null(storage_lib, file_priv->storageId);
  msg_storage_priv_t storage_priv_src = MSG_storage_priv(storage_src);
  msg_host_t attached_host = MSG_host_by_name(storage_priv_src->hostname);
  MSG_file_seek(file, 0, SEEK_SET);
  read_size = simcall_file_read(file_priv->simdata->smx_file, file_priv->size, attached_host);

  /* Find the real host destination where the file will be physically stored */
  xbt_dict_cursor_t cursor = NULL;
  char *mount_name, *storage_name, *file_mount_name, *host_name_dest;
  msg_storage_t storage_dest = NULL;
  msg_host_t host_dest;
  size_t longest_prefix_length = 0;

  xbt_dict_t storage_list = host->mountedStoragesAsDict();
  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name){
    file_mount_name = (char *) xbt_malloc ((strlen(mount_name)+1));
    strncpy(file_mount_name,fullpath,strlen(mount_name)+1);
    file_mount_name[strlen(mount_name)] = '\0';

    if(!strcmp(file_mount_name,mount_name) && strlen(mount_name)>longest_prefix_length){
      /* The current mount name is found in the full path and is bigger than the previous*/
      longest_prefix_length = strlen(mount_name);
      storage_dest = (msg_storage_t) xbt_lib_get_elm_or_null(storage_lib, storage_name);
    }
    free(file_mount_name);
  }
  xbt_dict_free(&storage_list);

  if(longest_prefix_length>0){
    /* Mount point found, retrieve the host the storage is attached to */
    msg_storage_priv_t storage_dest_priv = MSG_storage_priv(storage_dest);
    host_name_dest = (char*)storage_dest_priv->hostname;
    host_dest = MSG_host_by_name(host_name_dest);

  }else{
    XBT_WARN("Can't find mount point for '%s' on destination host '%s'", fullpath, sg_host_get_name(host));
    return MSG_TASK_CANCELED;
  }

  XBT_DEBUG("Initiate data transfer of %llu bytes between %s and %s.", read_size, storage_priv_src->hostname,
            host_name_dest);
  msg_host_t *m_host_list = NULL;
  m_host_list = (msg_host_t*) calloc(2, sizeof(msg_host_t));

  m_host_list[0] = attached_host;
  m_host_list[1] = host_dest;
  double flops_amount[] = { 0, 0 };
  double bytes_amount[] = { 0, (double)read_size, 0, 0 };

  msg_task_t task =
      MSG_parallel_task_create("file transfer for write", 2, m_host_list, flops_amount, bytes_amount, NULL);
  msg_error_t transfer = MSG_parallel_task_execute(task);
  MSG_task_destroy(task);
  free(m_host_list);
  if(transfer != MSG_OK){
    if (transfer == MSG_HOST_FAILURE)
      XBT_WARN("Transfer error, %s remote host just turned off!", host_name_dest);
    if (transfer == MSG_TASK_CANCELED)
      XBT_WARN("Transfer error, task has been canceled!");

    return (msg_error_t) -1;
  }

  /* Create file on remote host, write it and close it */
  smx_file_t smx_file = simcall_file_open(fullpath, host_dest);
  simcall_file_write(smx_file, read_size, host_dest);
  simcall_file_close(smx_file, host_dest);
  return MSG_OK;
}

/**
 * \ingroup msg_file_management
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

/**
 * \brief Destroys a file (internal call only)
 */
void __MSG_file_destroy(msg_file_priv_t file) {
  xbt_free(file->fullpath);
  xbt_free(file->simdata);
  xbt_free(file);
}

/********************************* Storage **************************************/
/** @addtogroup msg_storage_management
 * \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Storages" --> \endhtmlonly
 * (#msg_storage_t) and the functions for managing it.
 */

msg_storage_t __MSG_storage_create(smx_storage_t storage)
{
  const char *name = SIMIX_storage_get_name(storage);
  const char *host = SIMIX_storage_get_host(storage);
  msg_storage_priv_t storage_private = xbt_new0(s_msg_storage_priv_t, 1);
  storage_private->hostname = host;
  xbt_lib_set(storage_lib,name,MSG_STORAGE_LEVEL,storage_private);
  return xbt_lib_get_elm_or_null(storage_lib, name);
}

/**
 * \brief Destroys a storage (internal call only)
 */
void __MSG_storage_destroy(msg_storage_priv_t storage) {
  free(storage);
}

/** \ingroup msg_storage_management
 *
 * \brief Returns the name of the #msg_storage_t.
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char *MSG_storage_get_name(msg_storage_t storage) {
  xbt_assert((storage != NULL), "Invalid parameters");
  return SIMIX_storage_get_name(storage);
}

/** \ingroup msg_storage_management
 * \brief Returns the free space size of a storage element
 * \param storage a storage
 * \return the free space size of the storage element (as a #sg_size_t)
 */
sg_size_t MSG_storage_get_free_size(msg_storage_t storage){
  return simcall_storage_get_free_size(storage);
}

/** \ingroup msg_storage_management
 * \brief Returns the used space size of a storage element
 * \param storage a storage
 * \return the used space size of the storage element (as a #sg_size_t)
 */
sg_size_t MSG_storage_get_used_size(msg_storage_t storage){
  return simcall_storage_get_used_size(storage);
}

/** \ingroup msg_storage_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this storage
 * \param storage a storage
 * \return a dict containing the properties
 */
xbt_dict_t MSG_storage_get_properties(msg_storage_t storage)
{
  xbt_assert((storage != NULL), "Invalid parameters (storage is NULL)");
  return (simcall_storage_get_properties(storage));
}

/** \ingroup msg_storage_management
 * \brief Change the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \param value what to change the property to
 * \param free_ctn the freeing function to use to kill the value on need
 */
void MSG_storage_set_property_value(msg_storage_t storage, const char *name, char *value,void_f_pvoid_t free_ctn) {
  xbt_dict_set(MSG_storage_get_properties(storage), name, value,free_ctn);
}

/** \ingroup m_storage_management
 * \brief Returns the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \return value of a property (or NULL if property not set)
 */
const char *MSG_storage_get_property_value(msg_storage_t storage, const char *name)
{
  return (char*) xbt_dict_get_or_null(MSG_storage_get_properties(storage), name);
}

/** \ingroup msg_storage_management
 * \brief Finds a msg_storage_t using its name.
 * \param name the name of a storage
 * \return the corresponding storage
 */
msg_storage_t MSG_storage_get_by_name(const char *name)
{
  return (msg_storage_t) xbt_lib_get_elm_or_null(storage_lib,name);
}

/** \ingroup msg_storage_management
 * \brief Returns a dynar containing all the storage elements declared at a given point of time
 */
xbt_dynar_t MSG_storages_as_dynar(void) {
  xbt_lib_cursor_t cursor;
  char *key;
  void **data;
  xbt_dynar_t res = xbt_dynar_new(sizeof(msg_storage_t),NULL);

  xbt_lib_foreach(storage_lib, cursor, key, data) {
    if(xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), MSG_STORAGE_LEVEL) != NULL) {
      xbt_dictelm_t elm = xbt_dict_cursor_get_elm(cursor);
      xbt_dynar_push(res, &elm);
    }
  }
  return res;
}

/** \ingroup msg_storage_management
 *
 * \brief Set the user data of a #msg_storage_t.
 * This functions attach \a data to \a storage if possible.
 */
msg_error_t MSG_storage_set_data(msg_storage_t storage, void *data)
{
  msg_storage_priv_t priv = MSG_storage_priv(storage);
  priv->data = data;
  return MSG_OK;
}

/** \ingroup m_host_management
 *
 * \brief Returns the user data of a #msg_storage_t.
 *
 * This functions checks whether \a storage is a valid pointer and returns its associate user data if possible.
 */
void *MSG_storage_get_data(msg_storage_t storage)
{
  xbt_assert((storage != NULL), "Invalid parameters");
  msg_storage_priv_t priv = MSG_storage_priv(storage);
  return priv->data;
}

/** \ingroup msg_storage_management
 *
 * \brief Returns the content (file list) of a #msg_storage_t.
 * \param storage a storage
 * \return The content of this storage element as a dict (full path file => size)
 */
xbt_dict_t MSG_storage_get_content(msg_storage_t storage)
{
  return SIMIX_storage_get_content(storage);
}

/** \ingroup msg_storage_management
 *
 * \brief Returns the size of a #msg_storage_t.
 * \param storage a storage
 * \return The size of the storage
 */
sg_size_t MSG_storage_get_size(msg_storage_t storage)
{
  return SIMIX_storage_get_size(storage);
}

/** \ingroup msg_storage_management
 *
 * \brief Returns the host name the storage is attached to
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char *MSG_storage_get_host(msg_storage_t storage) {
  xbt_assert((storage != NULL), "Invalid parameters");
  msg_storage_priv_t priv = MSG_storage_priv(storage);
  return priv->hostname;
}
