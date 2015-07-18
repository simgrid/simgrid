/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_cm02.hpp"

/***********
 * Classes *
 ***********/

class NetworkSmpiModel;
typedef NetworkSmpiModel *NetworkSmpiModelPtr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/

class NetworkSmpiModel : public NetworkCm02Model {
public:
  NetworkSmpiModel();
  ~NetworkSmpiModel();

  void gapAppend(double size, Link* link, NetworkActionPtr action);
  void gapRemove(ActionPtr action);
  double latencyFactor(double size);
  double bandwidthFactor(double size);
  double bandwidthConstraint(double rate, double bound, double size);
  void communicateCallBack() {};
};


/************
 * Resource *
 ************/


/**********
 * Action *
 **********/



