/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/base.h"

#include "host_clm03.hpp"
#include "virtual_machine.hpp"

#ifndef SURF_VM_HPP_
#define SURF_VM_HPP_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE VMHL13Model;
class XBT_PRIVATE VMHL13;

/*********
 * Model *
 *********/
class VMHL13Model : public VMModel {
public:
  VMHL13Model();
  ~VMHL13Model(){};

  VirtualMachine *createVM(const char *name, sg_host_t host_PM) override;
  double shareResources(double now);
  void adjustWeightOfDummyCpuActions() override {};
  void updateActionsState(double /*now*/, double /*delta*/) override;
};

/************
 * Resource *
 ************/

class VMHL13 : public VirtualMachine {
public:
  VMHL13(VMModel *model, const char* name, xbt_dict_t props, sg_host_t host_PM);
  ~VMHL13() {}

  void suspend() override;
  void resume() override;

  void save() override;
  void restore() override;

  void migrate(sg_host_t ind_dst_pm) override;

  void setBound(double bound);
  void setAffinity(Cpu *cpu, unsigned long mask);
};

/**********
 * Action *
 **********/

}
}

#endif /* SURF_VM_HPP_ */
