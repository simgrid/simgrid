/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/types.h>
#include <stddef.h> /* offsetof() */
#include <stdio.h>

int main (void)
{

struct s0 {char c0; double d0;};
struct s1 {double d1; int i1; char c1;};
struct s2 {double d2; int i2; char c2[6];};
struct s3 {double d3; int a3; int b3;}; 

int gras_struct_packed;
int gras_struct_compact;
int gras_array_straddle_struct;
int gras_compact_struct;

	if (sizeof(struct s0) == sizeof(double)+sizeof(char))
	{
		gras_struct_packed=1;
	}
	else
	{
		gras_struct_packed=0;
	}
	if (offsetof(struct s1,c1) == sizeof(double)+sizeof(int))
	{
		gras_struct_compact=1;
	}
	else
	{
		gras_struct_compact=0;
	}
	if (offsetof(struct s2,c2) == sizeof(double)+sizeof(int))
	{
		gras_array_straddle_struct=1;
	}
	else
	{
		gras_array_straddle_struct=0;
	}
	if (offsetof(struct s3,b3) == sizeof(double)+sizeof(int))
	{
		gras_compact_struct=1;
	}
	else
	{
		gras_compact_struct=0;
	}


	if(gras_struct_packed == 0 && gras_struct_compact == 1) printf("GRAS_STRUCT_COMPACT ");

	if(gras_array_straddle_struct == 1) printf("GRAS_ARRAY_STRADDLE_STRUCT ");

	if(gras_compact_struct == 1) printf("GRAS_COMPACT_STRUCT ");

return 1;

}
