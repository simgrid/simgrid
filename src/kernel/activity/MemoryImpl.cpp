#include "MemoryImpl.hpp"

namespace simgrid::kernel::activity {

MemoryAccessImpl::MemoryAccessImpl(actor::ActorImpl* issuer) : issuer_(issuer) {}

void MemoryAccessImpl::record_memory_access(MemOpType type, void* where)
{

  // printf(">> Creating a memory observation of type %s at adress %p\n", to_c_str(type), where);
  // xbt_backtrace_display_current();
  fflush(stdout);
  memory_accesses_.emplace_back(type, where);
}

void MemoryAccessImpl::serialize(mc::Channel& channel)
{
  size_t size = memory_accesses_.size();
  channel.pack<unsigned long>(size);
  for (auto mem : memory_accesses_) {
    channel.pack<MemOpType>(mem.get_type());
    channel.pack<void*>(mem.get_location());
  }

  memory_accesses_.clear();
}
} // namespace simgrid::kernel::activity
