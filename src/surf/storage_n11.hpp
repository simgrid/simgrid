/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "StorageImpl.hpp"

#ifndef STORAGE_N11_HPP_
#define STORAGE_N11_HPP_

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE StorageN11Model;
class XBT_PRIVATE StorageN11;
class XBT_PRIVATE StorageN11Action;

/*********
 * Model *
 *********/

class StorageN11Model : public StorageModel {
public:
  StorageImpl* createStorage(std::string id, std::string type_id, std::string content_name,
                             std::string attach) override;
  double nextOccuringEvent(double now) override;
  void updateActionsState(double now, double delta) override;
};

/************
 * Resource *
 ************/

class StorageN11 : public StorageImpl {
public:
  StorageN11(StorageModel* model, std::string name, lmm_system_t maxminSystem, double bread, double bwrite,
             std::string type_id, std::string content_name, sg_size_t size, std::string attach);
  virtual ~StorageN11() = default;
  StorageAction* read(sg_size_t size);
  StorageAction* write(sg_size_t size);
};

/**********
 * Action *
 **********/

class StorageN11Action : public StorageAction {
public:
  StorageN11Action(Model* model, double cost, bool failed, StorageImpl* storage, e_surf_action_storage_type_t type);
  void suspend();
  int unref();
  void cancel();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setSharingWeight(double priority);
};

}
}

#endif /* STORAGE_N11_HPP_ */
