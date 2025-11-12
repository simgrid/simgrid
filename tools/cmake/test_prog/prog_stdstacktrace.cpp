/* Copyright (c) 2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stacktrace>

int main()
{
  (void)to_string(std::stacktrace::current());
}
