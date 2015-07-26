/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_CHANNEL_HPP
#define SIMGRID_S4U_CHANNEL_HPP

#include <boost/unordered_map.hpp>

#include "simgrid/s4u/process.hpp"

namespace simgrid {
namespace s4u {

/** @brief Channel
 *
 * Rendez-vous point for network communications, similar to URLs on which you could post and retrieve data.
 * They are not network locations (you can post and retrieve on a given mailbox from anywhere on the network).
 */
class Channel {
	friend Process;

private:
	Channel(const char*name, smx_rdv_t inferior);
public:
	~Channel();
	
protected:
	smx_rdv_t getInferior() { return p_inferior; }

public:
	/** Retrieve the channel associated to the given string */
	static Channel *byName(const char *name);

private:
	std::string p_name;
	smx_rdv_t p_inferior;
	static boost::unordered_map<std::string, Channel *> *channels;
};
}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_CHANNEL_HPP */
