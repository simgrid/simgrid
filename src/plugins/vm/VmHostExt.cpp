/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/plugins/vm/VmHostExt.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_vm);

namespace simgrid {
namespace vm {
simgrid::xbt::Extension<s4u::Host, VmHostExt> VmHostExt::EXTENSION_ID;

void VmHostExt::ensureVmExtInstalled()
{
  if (not EXTENSION_ID.valid())
    EXTENSION_ID = simgrid::s4u::Host::extension_create<VmHostExt>();
}
}
}
