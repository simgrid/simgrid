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
 * \param storage is the name where find the stream
 * \param ptr buffer to where the data is copied
 * \param size of each element
 * \param nmemb is the number of elements of data to read
 * \param stream to read
 * \return the number of items successfully read
 */
size_t MSG_file_read(const char* storage, void* ptr, size_t size, size_t nmemb,  msg_file_t stream)
{
  return simcall_file_read(storage, ptr, size, nmemb, stream->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Write elements into a file
 *
 * \param storage is the name where find the stream
 * \param ptr buffer from where the data is copied
 * \param size of each element
 * \param nmemb is the number of elements of data to write
 * \param stream to write
 * \return the number of items successfully write
 */
size_t MSG_file_write(const char* storage, const void* ptr, size_t size, size_t nmemb, msg_file_t stream)
{
  return simcall_file_write(storage, ptr, size, nmemb, stream->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Opens the file whose name is the string pointed to by path
 *
 * \param storage is the name where find the file to open
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
msg_file_t MSG_file_open(const char* storage, const char* path, const char* mode)
{
  msg_file_t file = xbt_new(s_msg_file_t,1);
  file->name = strdup(path);
  file->simdata = xbt_new0(s_simdata_file_t,1);
  file->simdata->smx_file = simcall_file_open(storage, path, mode);
  return file;
}

/** \ingroup msg_file_management
 * \brief Close the file
 *
 * \param storage is the name where find the stream
 * \param fp is the file to close
 * \return 0 on success or 1 on error
 */
int MSG_file_close(const char* storage, msg_file_t fp)
{
  return simcall_file_close(storage, fp->simdata->smx_file);
}

/** \ingroup msg_file_management
 * \brief Stats the file pointed by fd
 *
 * \param storage is the name where find the stream
 * \param fd is the file descriptor (#msg_file_t)
 * \param buf is the return structure with informations
 * \return 0 on success or 1 on error
 */
int MSG_file_stat(const char* storage, msg_file_t fd, s_msg_stat_t *buf)
{
  int res;
  res = simcall_file_stat(storage, fd->simdata->smx_file, buf);
  return res;
}
