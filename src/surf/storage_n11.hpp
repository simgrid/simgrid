/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

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
  StorageN11Model();
  StorageImpl* createStorage(std::string id, std::string type_id, std::string content_name,
                             std::string attach) override;
  double next_occuring_event(double now) override;
  void update_actions_state(double now, double delta) override;
};

/************
 * Resource *
 ************/

class StorageN11 : public StorageImpl {
public:
  StorageN11(StorageModel* model, std::string name, kernel::lmm::System* maxminSystem, double bread, double bwrite,
             std::string type_id, std::string content_name, sg_size_t size, std::string attach);
  virtual ~StorageN11() = default;
  StorageAction* io_start(sg_size_t size, s4u::Io::OpType type);
  StorageAction* read(sg_size_t size);
  StorageAction* write(sg_size_t size);
};

/**********
 * Action *
 **********/

class StorageN11Action : public StorageAction {
public:
  StorageN11Action(kernel::resource::Model* model, double cost, bool failed, StorageImpl* storage,
                   s4u::Io::OpType type);
  void suspend() override;
  void cancel() override;
  void resume() override;
  void set_max_duration(double duration) override;
  void set_priority(double priority) override;
  void update_remains_lazy(double now) override;
};

}
}

#endif /* STORAGE_N11_HPP_ */
