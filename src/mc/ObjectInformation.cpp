#include "src/mc/Frame.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Variable.hpp"

namespace simgrid {
namespace mc {

ObjectInformation::ObjectInformation()
{
  this->flags = 0;
  this->start = nullptr;
  this->end = nullptr;
  this->start_exec = nullptr;
  this->end_exec = nullptr;
  this->start_rw = nullptr;
  this->end_rw = nullptr;
  this->start_ro = nullptr;
  this->end_ro = nullptr;
}

/** Find the DWARF offset for this ELF object
 *
 *  An offset is applied to address found in DWARF:
 *
 *  * for an executable obejct, addresses are virtual address
 *    (there is no offset) i.e.
 *    \f$\text{virtual address} = \{dwarf address}\f$;
 *
 *  * for a shared object, the addreses are offset from the begining
 *    of the shared object (the base address of the mapped shared
 *    object must be used as offset
 *    i.e. \f$\text{virtual address} = \text{shared object base address}
 *             + \text{dwarf address}\f$.
 */
void *ObjectInformation::base_address() const
{
  if (this->executable())
    return nullptr;

  void *result = this->start_exec;
  if (this->start_rw != NULL && result > (void *) this->start_rw)
    result = this->start_rw;
  if (this->start_ro != NULL && result > (void *) this->start_ro)
    result = this->start_ro;
  return result;
}

/* Find a function by instruction pointer */
simgrid::mc::Frame* ObjectInformation::find_function(const void *ip) const
{
  /* This is implemented by binary search on a sorted array.
   *
   * We do quite a lot ot those so we want this to be cache efficient.
   * We pack the only information we need in the index entries in order
   * to successfully do the binary search. We do not need the high_pc
   * during the binary search (only at the end) so it is not included
   * in the index entry. We could use parallel arrays as well.
   *
   * We cannot really use the std:: alogrithm for this.
   * We could use std::binary_search by including the high_pc inside
   * the FunctionIndexEntry.
   */
  const simgrid::mc::FunctionIndexEntry* base =
    this->functions_index.data();
  int i = 0;
  int j = this->functions_index.size() - 1;
  while (j >= i) {
    int k = i + ((j - i) / 2);

    /* In most of the search, we do not dereference the base[k].function.
     * This way the memory accesses are located in the base[k] array. */
    if (ip < base[k].low_pc)
      j = k - 1;
    else if (k < j && ip >= base[k + 1].low_pc)
      i = k + 1;

    /* At this point, the search is over.
     * Either we have found the correct function or we do not know
     * any function corresponding to this instruction address.
     * Only at the point do we derefernce the function pointer. */
    else if (ip < base[k].function->high_pc)
      return base[k].function;
    else
      return nullptr;
  }
  return nullptr;
}

simgrid::mc::Variable* ObjectInformation::find_variable(const char* name) const
{
  for (simgrid::mc::Variable& variable : this->global_variables)
    if(variable.name == name)
      return &variable;
  return nullptr;
}

}
}