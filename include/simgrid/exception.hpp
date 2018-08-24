/* Copyright (c) 2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_EXCEPTIONS_HPP
#define SIMGRID_EXCEPTIONS_HPP

/** @file exception.hpp SimGrid-specific Exceptions
 *
 *  Defines all possible exception that could occur in a SimGrid library.
 */

#include <exception>

namespace simgrid {

/** Ancestor class of all SimGrid exception */
class exception : public std::runtime_error {
public:
  exception() : std::runtime_error("") {}
  exception(const char* message) : std::runtime_error(message) {}
};

/** Exception raised when a timeout elapsed */
class timeout_error : public simgrid::exception {
};

/** Exception raised when an host fails */
class host_failure : public simgrid::exception {
};

/** Exception raised when a communication fails because of the network */
class network_failure : public simgrid::exception {
};

/** Exception raised when something got canceled before completion */
class cancel_error : public simgrid::exception {
};

} // namespace simgrid

#endif
