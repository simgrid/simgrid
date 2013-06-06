/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.              */

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

/** \ingroup msg_file_management
 * \brief Read elements of a file
 *
 * \param ptr buffer to where the data is copied
 * \param size of each element
 * \param nmemb is the number of elements of data to read
 * \param stream to read
 * \return the number of items successfully read
 */
size_t MSG_file_read(void* ptr, size_t size, size_t nmemb,  msg_file_t stream)
{
  return simcall_file_read(ptr, size, nmemb, stream->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Write elements into a file
 *
 * \param ptr buffer from where the data is copied
 * \param size of each element
 * \param nmemb is the number of elements of data to write
 * \param stream to write
 * \return the number of items successfully write
 */
size_t MSG_file_write(const void* ptr, size_t size, size_t nmemb, msg_file_t stream)
{
  return simcall_file_write(ptr, size, nmemb, stream->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Opens the file whose name is the string pointed to by path
 *
 * \param mount is the mount point where find the file is located
 * \param path is the file location on the storage
 * \param mode points to a string beginning with one of the following sequences (Additional characters may follow these sequences.):
 *      r      Open text file for reading.  The stream is positioned at the beginning of the file.
 *      r+     Open for reading and writing.  The stream is positioned at the beginning of the file.
 *      w      Truncate file to zero length or create text file for writing.  The stream is positioned at the beginning of the file.
 *      w+     Open for reading and writing.  The file is created if it does not exist, otherwise it is truncated.  The stream is positioned at the
 *             beginning of the file.
 *      a      Open for appending (writing at end of file).  The file is created if it does not exist.  The stream is positioned at the end of the file.
 *      a+     Open for reading and appending (writing at end of file).  The file is created if it does not exist.  The initial file position for  reading
 *             is at the beginning of the file, but output is always appended to the end of the file.
 *
 * \return An #msg_file_t associated to the file
 */
msg_file_t MSG_file_open(const char* mount, const char* path, const char* mode)
{
  msg_file_t file = xbt_new(s_msg_file_t,1);
  file->name = strdup(path);
  file->simdata = xbt_new0(s_simdata_file_t,1);
  file->simdata->smx_file = simcall_file_open(mount, path, mode);
  return file;
}

/** \ingroup msg_file_management
 * \brief Close the file
 *
 * \param fp is the file to close
 * \return 0 on success or 1 on error
 */
int MSG_file_close(msg_file_t fp)
{
  int res = simcall_file_close(fp->simdata->smx_file);
  free(fp->name);
  xbt_free(fp->simdata);
  xbt_free(fp);
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
  int res = simcall_file_unlink(fd->simdata->smx_file);
  free(fd->name);
  xbt_free(fd->simdata);
  xbt_free(fd);
  return res;
}

/** \ingroup msg_file_management
 * \brief Return the size of a file
 *
 * \param fd is the file descriptor (#msg_file_t)
 * \return the size of the file (as a size_t)
 */

size_t MSG_file_get_size(msg_file_t fd){
  size_t size = simcall_file_get_size(fd->simdata->smx_file);
  return size;
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
