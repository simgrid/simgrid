/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>
#include <xbt/module.h>

#include "mc/mc.h"
#include "simgrid/sg_config.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/surf_interface.hpp"

#include <fstream>
#include <string>

/*********
 * Utils *
 *********/

std::vector<std::string> surf_path;
