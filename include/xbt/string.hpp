/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_STRING_HPP
#define SIMGRID_XBT_STRING_HPP

#include <simgrid/config.h>

#include <cstdarg>
#include <cstdlib>
#include <string>

#if SIMGRID_HAVE_MC

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <stdexcept>

#include <xbt/sysdep.h>

#endif

namespace simgrid {
namespace xbt {

#if SIMGRID_HAVE_MC

/** POD structure representation of a string
 */
struct string_data {
  char* data;
  std::size_t len;
};

/** A std::string-like with well-known representation
 *
 *  HACK, this is a (incomplete) replacement for `std::string`.
 *  It has a fixed POD representation (`simgrid::xbt::string_data`)
 *  which can be used to easily read the string content from another
 *  process.
 *
 *  The internal representation of a `std::string` is private.
 *  We could add some code to read this for a given implementation.
 *  However, even if we focus on GNU libstdc++ with Itanium ABI
 *  GNU libstdc++ currently has two different ABIs
 *
 *  * the pre-C++11 is a pointer to a ref-counted
 *    string-representation (with support for COW);
 *
 *  * the [C++11-conforming implementation](https://gcc.gnu.org/gcc-5/changes.html)
 *    does not use refcouting/COW but has a small string optimization.
 */
class XBT_PUBLIC string {
  static char NUL;
  string_data str;

public:
  // Types
  typedef std::size_t size_type;
  typedef char& reference;
  typedef const char& const_reference;
  typedef char* iterator;
  typedef const char* const_iterator;

  // Dtor
  ~string()
  {
    if (str.data != &NUL)
      delete[] str.data;
  }

  // Ctors
  string(const char* s, size_t size)
  {
    if (size == 0) {
      str.len  = 0;
      str.data = &NUL;
    } else {
      str.len  = size;
      str.data = new char[str.len + 1];
      std::copy_n(s, str.len, str.data);
      str.data[str.len] = '\0';
    }
  }
  string() : string(&NUL, 0) {}
  explicit string(const char* s) : string(s, strlen(s)) {}
  string(string const& s) : string(s.c_str(), s.size()) {}
  string(string&& s)
  {
    str        = std::move(s.str);
    s.str.len  = 0;
    s.str.data = &NUL;
  }
  explicit string(std::string const& s) : string(s.c_str(), s.size()) {}

  // Assign
  void assign(const char* s, size_t size)
  {
    if (str.data != &NUL) {
      delete[] str.data;
      str.data = nullptr;
      str.len  = 0;
    }
    if (size != 0) {
      str.len  = size;
      str.data = new char[str.len + 1];
      std::copy_n(s, str.len, str.data);
      str.data[str.len] = '\0';
    }
  }

  // Copy
  string& operator=(const char* s)
  {
    assign(s, std::strlen(s));
    return *this;
  }
  string& operator=(string const& s)
  {
    if (this != &s)
      assign(s.c_str(), s.size());
    return *this;
  }
  string& operator=(std::string const& s)
  {
    assign(s.c_str(), s.size());
    return *this;
  }

  // Capacity
  size_t size() const { return str.len; }
  size_t length() const { return str.len; }
  bool empty() const { return str.len != 0; }
  void shrink_to_fit() { /* Being there, but doing nothing */}

  // Element access
  char* data() { return str.data; }
  const char* data() const { return str.data; }
  char* c_str() { return str.data; }
  const char* c_str() const { return str.data; };
  reference at(size_type i)
  {
    if (i >= size())
      throw std::out_of_range("Out of range");
    return data()[i];
  }
  const_reference at(size_type i) const
  {
    if (i >= size())
      throw std::out_of_range("Out of range");
    return data()[i];
  }
  reference operator[](size_type i)
  {
    return data()[i];
  }
  const_reference operator[](size_type i) const
  {
    return data()[i];
  }
  // Conversion
  static string_data& to_string_data(string& s) { return s.str; }
  operator std::string() const { return std::string(this->c_str(), this->size()); }

  // Iterators
  iterator begin()               { return data(); }
  iterator end()                 { return data() + size(); }
  const_iterator begin() const   { return data(); }
  const_iterator end() const     { return data() + size(); }
  const_iterator cbegin() const  { return data(); }
  const_iterator cend() const    { return data() + size(); }
  // (Missing, reverse iterators)

  // Operations
  void clear()
  {
    str.len  = 0;
    str.data = &NUL;
  }

  bool equals(const char* data, std::size_t len) const
  {
    return this->size() == len
      && std::memcmp(this->c_str(), data, len) == 0;
  }

  bool operator==(string const& that) const
  {
    return this->equals(that.c_str(), that.size());
  }
  bool operator==(std::string const& that) const
  {
    return this->equals(that.c_str(), that.size());
  }
  bool operator==(const char* that) const
  {
    return this->equals(that, std::strlen(that));
  }

  template<class X>
  bool operator!=(X const& that) const
  {
    return not (*this == that);
  }

  // Compare:
  int compare(const char* data, std::size_t len) const
  {
    size_t n = std::min(this->size(), len);
    int res = memcmp(this->c_str(), data, n);
    if (res != 0)
      return res;
    else if (this->size() == len)
      return 0;
    else if (this->size() < len)
      return -1;
    else
      return 1;
  }
  int compare(string const& that) const
  {
    return this->compare(that.c_str(), that.size());
  }
  int compare(std::string const& that) const
  {
    return this->compare(that.c_str(), that.size());
  }
  int compare(const char* that) const
  {
    return this->compare(that, std::strlen(that));
  }

  // Define < <= >= > in term of compare():
  template<class X>
  bool operator<(X const& that) const
  {
    return this->compare(that) < 0;
  }
  template<class X>
  bool operator<=(X const& that) const
  {
    return this->compare(that) <= 0;
  }
  template<class X>
  bool operator>(X const& that) const
  {
    return this->compare(that) > 0;
  }
  template<class X>
  bool operator>=(X const& that) const
  {
    return this->compare(that) >= 0;
  }
};

inline
bool operator==(std::string const& a, string const& b)
{
  return b == a;
}
inline
bool operator!=(std::string const& a, string const& b)
{
  return b != a;
}
inline
bool operator<(std::string const& a, string const& b)
{
  return b > a;
}
inline
bool operator<=(std::string const& a, string const& b)
{
  return b >= a;
}
inline
bool operator>(std::string const& a, string const& b)
{
  return b < a;
}
inline
bool operator>=(std::string const& a, string const& b)
{
  return b <= a;
}

#else

typedef std::string string;

#endif

/** Create a C++ string from a C-style format
 *
 * @ingroup XBT_str
*/
XBT_PUBLIC std::string string_printf(const char* fmt, ...);

/** Create a C++ string from a C-style format
 *
 * @ingroup XBT_str
*/
XBT_PUBLIC std::string string_vprintf(const char* fmt, va_list ap);
}
}

#endif
