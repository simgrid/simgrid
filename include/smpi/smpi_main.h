/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define main smpi_windows_main__(int argc, char **argv);\
int main(int argc, char **argv){\
smpi_main(&smpi_windows_main__,argc,argv);\
return 0;\
}\
int smpi_windows_main__
