/* Copyright (c) 2004-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

#ifndef VM_LIVE_MIGRATION_HPP_
#define VM_LIVE_MIGRATION_HPP_

namespace simgrid {
namespace vm {
class VmMigrationExt {
public:
  s4u::ActorPtr issuer_ = nullptr;
  s4u::ActorPtr tx_     = nullptr;
  s4u::ActorPtr rx_     = nullptr;
  static simgrid::xbt::Extension<simgrid::s4u::Host, VmMigrationExt> EXTENSION_ID;
  virtual ~VmMigrationExt() = default;
  explicit VmMigrationExt(s4u::ActorPtr issuer, s4u::ActorPtr rx, s4u::ActorPtr tx) : issuer_(issuer), tx_(tx), rx_(rx)
  {
  }
  static void ensureVmMigrationExtInstalled();
};

class MigrationRx {
  /* The miration_rx process uses mbox_ctl to let the caller of do_migration()  know the completion of the migration. */
  s4u::MailboxPtr mbox_ctl;
  /* The migration_rx and migration_tx processes use mbox to transfer migration data. */
  s4u::MailboxPtr mbox;
  s4u::VirtualMachine* vm_;
  s4u::Host* src_pm_ = nullptr;
  s4u::Host* dst_pm_ = nullptr;

public:
  explicit MigrationRx(s4u::VirtualMachine* vm, s4u::Host* dst_pm) : vm_(vm), dst_pm_(dst_pm)
  {
    src_pm_ = vm_->getPm();

    mbox_ctl = s4u::Mailbox::byName(std::string("__mbox_mig_ctl:") + vm_->getCname() + "(" + src_pm_->getCname() + "-" +
                                    dst_pm_->getCname() + ")");
    mbox = s4u::Mailbox::byName(std::string("__mbox_mig_src_dst:") + vm_->getCname() + "(" + src_pm_->getCname() + "-" +
                                dst_pm_->getCname() + ")");
  }
  void operator()();
};

class MigrationTx {
  /* The migration_rx and migration_tx processes use mbox to transfer migration data. */
  s4u::MailboxPtr mbox;
  s4u::VirtualMachine* vm_;
  s4u::Host* src_pm_ = nullptr;
  s4u::Host* dst_pm_ = nullptr;

public:
  explicit MigrationTx(s4u::VirtualMachine* vm, s4u::Host* dst_pm) : vm_(vm), dst_pm_(dst_pm)
  {
    src_pm_ = vm_->getPm();
    mbox = s4u::Mailbox::byName(std::string("__mbox_mig_src_dst:") + vm_->getCname() + "(" + src_pm_->getCname() + "-" +
                                dst_pm_->getCname() + ")");
  }
  void operator()();
  sg_size_t sendMigrationData(sg_size_t size, int stage, int stage2_round, double mig_speed, double timeout);
};
}
}
#endif
