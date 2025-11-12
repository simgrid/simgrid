/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

/* Write operations = store */

// Values: write from a to b, a is a value and b is a pointer
extern "C" void __mcsimgrid_write4v(int32_t a, int32_t* b);
extern "C" void __mcsimgrid_write8v(int64_t a, int64_t* b);
extern "C" void __mcsimgrid_writefv(float a, float* b);
extern "C" void __mcsimgrid_writedv(double a, double* b);

// Pointers: write from a to b, a and b are pointers
extern "C" void __mcsimgrid_write4(int32_t* a, int32_t* b);
extern "C" void __mcsimgrid_write8(int64_t* a, int64_t* b);
extern "C" void __mcsimgrid_writef(float* a, float* b);
extern "C" void __mcsimgrid_writed(double* a, double* b);

/* Read operations = load */

// Pointers
extern "C" void __mcsimgrid_read4v(int32_t* a);
extern "C" void __mcsimgrid_read8v(int64_t* a);
extern "C" void __mcsimgrid_readfv(float* a);
extern "C" void __mcsimgrid_readdv(double* a);

// Pointers to pointers
extern "C" void __mcsimgrid_read4(int32_t** a);
extern "C" void __mcsimgrid_read8(int64_t** a);
extern "C" void __mcsimgrid_readf(float** a);
extern "C" void __mcsimgrid_readd(double** a);
