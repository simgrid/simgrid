/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.              */

#include "msg_private.h"

// FILE

size_t MSG_file_read(void* ptr, size_t size, size_t nmemb,  m_file_t stream)
{
  return simcall_file_read(ptr, size, nmemb, (smx_file_t)stream);
}

size_t MSG_file_write(const void* ptr, size_t size, size_t nmemb, m_file_t stream)
{
  return simcall_file_write(ptr, size, nmemb, (smx_file_t)stream);
}

m_file_t MSG_file_open(const char* path, const char* mode)
{
  return (m_file_t) simcall_file_open(path, mode);
}

int MSG_file_close(m_file_t fp)
{
  return simcall_file_close((smx_file_t)fp);
}

int MSG_file_stat(int fd, void* buf)
{
  return simcall_file_stat(fd, buf);
}
