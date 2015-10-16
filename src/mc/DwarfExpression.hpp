#ifndef SIMGRID_MC_DWARF_EXPRESSION_HPP
#define SIMGRID_MC_DWARF_EXPRESSION_HPP

#include <cstdint>

#include <stdexcept>

#include <mc/AddressSpace.hpp>

namespace simgrid {
namespace dwarf {

class evaluation_error : std::runtime_error {
public:
  evaluation_error(const char* what) : std::runtime_error(what) {}
  ~evaluation_error();
};

struct ExpressionContext {
  ExpressionContext() :
    cursor(nullptr), frame_base(nullptr), address_space(nullptr),
    object_info(nullptr), process_index(simgrid::mc::ProcessIndexMissing) {}
    
  unw_cursor_t* cursor;
  void* frame_base;
  simgrid::mc::AddressSpace* address_space;
  simgrid::mc::ObjectInformation* object_info;
  int process_index;
};

typedef std::vector<Dwarf_Op> DwarfExpression;

class ExpressionStack {
public:
  typedef std::uintptr_t value_type;
  static const std::size_t max_size = 64;
private:
  uintptr_t stack_[max_size];
  size_t size_;
public:
  ExpressionStack() : size_(0) {}

  // Access:
  std::size_t size() const { return size_; }
  bool empty()       const { return size_ == 0; }
  void clear()             { size_ = 0; }
  uintptr_t&       operator[](int i)       { return stack_[i]; }
  uintptr_t const& operator[](int i) const { return stack_[i]; }
  value_type& top()
  {
    if (size_ == 0)
      throw evaluation_error("Empty stack");
    return stack_[size_ - 1];
  }
  value_type& top(unsigned i)
  {
    if (size_ < i)
      throw evaluation_error("Invalid element");
    return stack_[size_ - 1 - i];
  }

  // Push/pop:
  void push(value_type value)
  {
    if (size_ == max_size)
      throw evaluation_error("Dwarf stack overflow");
    stack_[size_++] = value;
  }
  value_type pop()
  {
    if (size_ == 0)
      throw evaluation_error("Stack underflow");
    return stack_[--size_];
  }

  // Other operations:
  void dup() { push(top()); }
};

void execute(const Dwarf_Op* ops, std::size_t n,
  ExpressionContext const& context, ExpressionStack& stack);

inline
void execute(simgrid::dwarf::DwarfExpression const& expression,
  ExpressionContext const& context, ExpressionStack& stack)
{
  execute(expression.data(), expression.size(), context, stack);
}

}
}

#endif