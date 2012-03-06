/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.              */

#include "msg_private.h"

// FILE

size_t MSG_file_read(const char* storage, void* ptr, size_t size, size_t nmemb,  m_file_t stream)
{
  return simcall_file_read(storage, ptr, size, nmemb, (smx_file_t)stream);
}

size_t MSG_file_write(const char* storage, const void* ptr, size_t size, size_t nmemb, m_file_t stream)
{
  return simcall_file_write(storage, ptr, size, nmemb, (smx_file_t)stream);
}

m_file_t MSG_file_open(const char* storage, const char* path, const char* mode)
{
  return (m_file_t) simcall_file_open(storage, path, mode);
}

int MSG_file_close(const char* storage, m_file_t fp)
{
  return simcall_file_close(storage, (smx_file_t)fp);
}

int MSG_file_stat(const char* storage, int fd, void* buf)
{
  return simcall_file_stat(storage, fd, buf);
}
