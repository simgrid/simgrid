/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/internal_config.h"
#include "mc_object_info.h"
#include "src/mc/mc_private.h"
#include "src/smpi/private.h"
#include "src/mc/mc_snapshot.h"
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_protocol.h"
#include "src/mc/mc_client.h"

#include "src/mc/Frame.hpp"
#include "src/mc/Variable.hpp"
#include "src/mc/ObjectInformation.hpp"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mcer_ignore, mc,
                                "Logging specific to MC ignore mechanism");

}
