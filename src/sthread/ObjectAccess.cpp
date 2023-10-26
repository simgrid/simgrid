/* Copyright (c) 2002-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Object access checker, for use in sthread. */

#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/simix.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/sthread/sthread.h"
#include "xbt/string.hpp"

#include <unordered_map>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(sthread);

namespace simgrid::sthread {
XBT_DECLARE_ENUM_CLASS(AccessType, ENTER, EXIT, BOTH);

class ObjectAccessObserver final : public simgrid::kernel::actor::SimcallObserver {
  AccessType type_;
  void* objaddr_;
  const char* objname_;
  const char* file_;
  int line_;

public:
  ObjectAccessObserver(simgrid::kernel::actor::ActorImpl* actor, AccessType type, void* objaddr, const char* objname,
                       const char* file, int line)
      : SimcallObserver(actor), type_(type), objaddr_(objaddr), objname_(objname), file_(file), line_(line)
  {
  }
  void serialize(std::stringstream& stream) const override;
  std::string to_string() const override;
};
void ObjectAccessObserver::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::OBJECT_ACCESS << ' ';
  stream << (short)type_ << ' ' << objaddr_ << ' ' << objname_ << ' ' << file_ << ' ' << line_;
}
std::string ObjectAccessObserver::to_string() const
{
  return std::string("AccessObject(") + objname_ + ")";
}
}; // namespace simgrid::sthread

struct ObjectOwner {
  simgrid::kernel::actor::ActorImpl* owner = nullptr;
  const char* file                         = nullptr;
  int line                                 = -1;
  int recursive_depth                      = 0;
  explicit ObjectOwner(simgrid::kernel::actor::ActorImpl* o) : owner(o) {}
};

std::unordered_map<void*, ObjectOwner*> owners;
static void clean_owners()
{
  for (auto it = owners.begin(); it != owners.end();) {
    delete it->second;
    it = owners.erase(it);
  }
}
static ObjectOwner* get_owner(void* object)
{
  if (owners.empty())
    std::atexit(clean_owners);
  if (auto it = owners.find(object); it != owners.end())
    return it->second;
  auto* o = new ObjectOwner(nullptr);
  owners.emplace(object, o);
  return o;
}

int sthread_access_begin(void* objaddr, const char* objname, const char* file, int line, const char* func)
{
  sthread_disable();
  auto* self = simgrid::kernel::actor::ActorImpl::self();
  simgrid::sthread::ObjectAccessObserver observer(self, simgrid::sthread::AccessType::ENTER, objaddr, objname, file,
                                                  line);
  bool success = simgrid::kernel::actor::simcall_answered(
      [self, objaddr, objname, file, line]() -> bool {
        XBT_INFO("%s takes %s", self->get_cname(), objname);
        auto* ownership = get_owner(objaddr);
        if (ownership->owner == self) {
          ownership->recursive_depth++;
          return true;
        } else if (ownership->owner != nullptr) {
          auto msg = std::string("Unprotected concurent access to ") + objname + ": " + ownership->owner->get_name();
          if (not xbt_log_no_loc) {
            msg += simgrid::xbt::string_printf(" at %s:%d", ownership->file, ownership->line);
            if (ownership->recursive_depth > 1) {
              msg += simgrid::xbt::string_printf(" (and %d other locations)", ownership->recursive_depth - 1);
              if (ownership->recursive_depth != 2)
                msg += "s";
            }
          } else {
            msg += simgrid::xbt::string_printf(" from %d location", ownership->recursive_depth);
            if (ownership->recursive_depth != 1)
              msg += "s";
          }
          msg += " vs " + self->get_name();
          if (xbt_log_no_loc)
            msg += std::string(" (locations hidden because of --log=no_loc).");
          else
            msg += simgrid::xbt::string_printf(" at %s:%d.", file, line);
          XBT_CRITICAL("%s", msg.c_str());
          return false;
        }
        ownership->owner = self;
        ownership->file  = file;
        ownership->line  = line;
        ownership->recursive_depth = 1;
        return true;
      },
      &observer);
  MC_assert(success);
  xbt_assert(success, "Problem detected, bailing out");
  sthread_enable();
  return true;
}
void sthread_access_end(void* objaddr, const char* objname, const char* file, int line, const char* func)
{
  sthread_disable();
  auto* self = simgrid::kernel::actor::ActorImpl::self();
  simgrid::sthread::ObjectAccessObserver observer(self, simgrid::sthread::AccessType::EXIT, objaddr, objname, file,
                                                  line);
  simgrid::kernel::actor::simcall_answered(
      [self, objaddr, objname]() -> void {
        XBT_INFO("%s releases %s", self->get_cname(), objname);
        auto* ownership = get_owner(objaddr);
        xbt_assert(ownership->owner == self,
                   "safety check failed: %s is not owner of the object it's releasing. That object owned by %s.",
                   self->get_cname(), (ownership->owner == nullptr ? "nobody" : ownership->owner->get_cname()));
        ownership->recursive_depth--;
        if (ownership->recursive_depth == 0)
          ownership->owner = nullptr;
      },
      &observer);
  sthread_enable();
}
