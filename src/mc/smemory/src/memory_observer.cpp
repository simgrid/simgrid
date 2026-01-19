/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "memory_observer.h"

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/MemoryImpl.hpp"

void create_memory_write(void*);
void create_memory_read(void*);
void create_memory_access(MemOpType, void*);

void create_memory_access(MemOpType type, void* where)
{
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  if (issuer && issuer->get_memory_access())
    issuer->get_memory_access()->record_memory_access(type, where);
}

/* Write operations */

void create_memory_write(void* where)
{
  create_memory_access(MemOpType::WRITE, where);
}

void __mcsimgrid_write4v(int32_t a, int32_t* b)
{
  create_memory_write(b);
}

void __mcsimgrid_write8v(int64_t a, int64_t* b)
{
  create_memory_write(b);
}
void __mcsimgrid_writefv(float a, float* b)
{
  create_memory_write(b);
}
void __mcsimgrid_writedv(double a, double* b)
{
  create_memory_write(b);
}

void __mcsimgrid_write4(int32_t* a, int32_t* b)
{
  create_memory_write(b);
}
void __mcsimgrid_write8(int64_t* a, int64_t* b)
{
  create_memory_write(b);
}
void __mcsimgrid_writef(float* a, float* b)
{
  create_memory_write(b);
}
void __mcsimgrid_writed(double* a, double* b)
{
  create_memory_write(b);
}

/* Read operations */

void create_memory_read(void* where)
{
  create_memory_access(MemOpType::READ, where);
}

void __mcsimgrid_read4v(int32_t* a)
{
  create_memory_read(a);
}
void __mcsimgrid_read8v(int64_t* a)
{
  create_memory_read(a);
}
void __mcsimgrid_readfv(float* a)
{
  create_memory_read(a);
}
void __mcsimgrid_readdv(double* a)
{
  create_memory_read(a);
}

void __mcsimgrid_read4(int32_t** a)
{
  create_memory_read(a);
}
void __mcsimgrid_read8(int64_t** a)
{
  create_memory_read(a);
}
void __mcsimgrid_readf(float** a)
{
  create_memory_read(a);
}
void __mcsimgrid_readd(double** a)
{
  create_memory_read(a);
}
