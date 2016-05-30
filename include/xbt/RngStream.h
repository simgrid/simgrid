/* Copyright (c) 2012, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* RngStream.h for ANSI C */
#ifndef RNGSTREAM_H
#define RNGSTREAM_H

#include "misc.h"

typedef struct RngStream_InfoState * RngStream;

struct RngStream_InfoState {
   double Cg[6];
   double Bg[6];
   double Ig[6];
   int Anti;
   int IncPrec;
   char *name;
};

SG_BEGIN_DECL();

XBT_PUBLIC(int) RngStream_SetPackageSeed (unsigned long seed[6]);
XBT_PUBLIC(RngStream) RngStream_CreateStream (const char name[]);
XBT_PUBLIC(void) RngStream_DeleteStream (RngStream *pg);
XBT_PUBLIC(RngStream) RngStream_CopyStream (const RngStream src);
XBT_PUBLIC(void) RngStream_ResetStartStream (RngStream g);
XBT_PUBLIC(void) RngStream_ResetStartSubstream (RngStream g);
XBT_PUBLIC(void) RngStream_ResetNextSubstream (RngStream g);
XBT_PUBLIC(void) RngStream_SetAntithetic (RngStream g, int a);
XBT_PUBLIC(void) RngStream_IncreasedPrecis (RngStream g, int incp);
XBT_PUBLIC(int) RngStream_SetSeed (RngStream g, unsigned long seed[6]);
XBT_PUBLIC(void) RngStream_AdvanceState (RngStream g, long e, long c);
XBT_PUBLIC(void) RngStream_GetState (RngStream g, unsigned long seed[6]);
XBT_PUBLIC(void) RngStream_WriteState (RngStream g);
XBT_PUBLIC(void) RngStream_WriteStateFull (RngStream g);
XBT_PUBLIC(double) RngStream_RandU01 (RngStream g);
XBT_PUBLIC(int) RngStream_RandInt (RngStream g, int i, int j);

SG_END_DECL();

#endif


