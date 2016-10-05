/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "network_cm02.hpp"

namespace simgrid {
  namespace surf {

    class XBT_PRIVATE NetworkSmpiModel : public NetworkCm02Model {
    public:
      NetworkSmpiModel();
      ~NetworkSmpiModel();

      using NetworkCm02Model::gapAppend; // Explicit about overloaded method (silence Woverloaded-virtual from clang)
      void gapAppend(double size, Link* link, NetworkAction *action);
      void gapRemove(Action *action);
      double latencyFactor(double size);
      double bandwidthFactor(double size);
      double bandwidthConstraint(double rate, double bound, double size);
      void communicateCallBack() {};
    };
  }
}
