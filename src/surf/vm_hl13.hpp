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
  Action *executeParallelTask(int host_nb,
                              sg_host_t *host_list,
							  double *flops_amount,
							  double *bytes_amount,
							  double rate) override;
  void updateActionsState(double /*now*/, double /*delta*/) override;
};

/************
 * Resource *
 ************/

class VMHL13 : public VirtualMachine {
public:
  VMHL13(VMModel *model, const char* name, xbt_dict_t props, sg_host_t host_PM);
  ~VMHL13();

  void suspend();
  void resume();

  void save();
  void restore();

  void migrate(sg_host_t ind_dst_pm);

  e_surf_resource_state_t getState();
  void setState(e_surf_resource_state_t state);

  sg_host_t getPm(); // will be vm_ws_get_pm()

  void setBound(double bound);
  void setAffinity(Cpu *cpu, unsigned long mask);

  //FIXME: remove
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  bool isUsed();

  Action *execute(double size);
  Action *sleep(double duration);
};

/**********
 * Action *
 **********/

}
}

#endif /* SURF_VM_HPP_ */
