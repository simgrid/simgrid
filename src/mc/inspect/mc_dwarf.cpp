/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simgrid/util.hpp"
#include "xbt/log.h"
#include "xbt/string.hpp"
#include "xbt/sysdep.h"
#include <simgrid/config.h>

#include "src/mc/inspect/ObjectInformation.hpp"
#include "src/mc/inspect/Variable.hpp"
#include "src/mc/inspect/mc_dwarf.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/remote/RemoteSimulation.hpp"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <memory>
#include <unordered_map>
#include <utility>

#include <boost/range/algorithm.hpp>

#include <elfutils/libdw.h>
#include <elfutils/version.h>

#include <boost/algorithm/string/predicate.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dwarf, mc, "DWARF processing");

/** @brief The default DW_TAG_lower_bound for a given DW_AT_language.
 *
 *  The default for a given language is defined in the DWARF spec.
 *
 *  @param language constant as defined by the DWARf spec
 */
static uint64_t MC_dwarf_default_lower_bound(int lang);

/** @brief Computes the the element_count of a DW_TAG_enumeration_type DIE
 *
 * This is the number of elements in a given array dimension.
 *
 * A reference of the compilation unit (DW_TAG_compile_unit) is
 * needed because the default lower bound (when there is no DW_AT_lower_bound)
 * depends of the language of the compilation unit (DW_AT_language).
 *
 * @param die  DIE for the DW_TAG_enumeration_type or DW_TAG_subrange_type
 * @param unit DIE of the DW_TAG_compile_unit
 */
static uint64_t MC_dwarf_subrange_element_count(Dwarf_Die* die, Dwarf_Die* unit);

/** @brief Computes the number of elements of a given DW_TAG_array_type.
 *
 * @param die DIE for the DW_TAG_array_type
 */
static uint64_t MC_dwarf_array_element_count(Dwarf_Die* die, Dwarf_Die* unit);

/** @brief Process a DIE
 *
 *  @param info the resulting object for the library/binary file (output)
 *  @param die  the current DIE
 *  @param unit the DIE of the compile unit of the current DIE
 *  @param frame containing frame if any
 */
static void MC_dwarf_handle_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                simgrid::mc::Frame* frame, const char* ns);

/** @brief Process a type DIE
 */
static void MC_dwarf_handle_type_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                     simgrid::mc::Frame* frame, const char* ns);

/** @brief Calls MC_dwarf_handle_die on all children of the given die
 *
 *  @param info the resulting object for the library/binary file (output)
 *  @param die  the current DIE
 *  @param unit the DIE of the compile unit of the current DIE
 *  @param frame containing frame if any
 */
static void MC_dwarf_handle_children(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                     simgrid::mc::Frame* frame, const char* ns);

/** @brief Handle a variable (DW_TAG_variable or other)
 *
 *  @param info the resulting object for the library/binary file (output)
 *  @param die  the current DIE
 *  @param unit the DIE of the compile unit of the current DIE
 *  @param frame containing frame if any
 */
static void MC_dwarf_handle_variable_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, const Dwarf_Die* unit,
                                         simgrid::mc::Frame* frame, const char* ns);

/** @brief Get the DW_TAG_type of the DIE
 *
 *  @param die DIE
 *  @return DW_TAG_type attribute as a new string (nullptr if none)
 */
static std::uint64_t MC_dwarf_at_type(Dwarf_Die* die);

namespace simgrid {
namespace dwarf {

enum class TagClass { Unknown, Type, Subprogram, Variable, Scope, Namespace };

/*** Class of forms defined in the DWARF standard */
enum class FormClass {
  Unknown,
  Address, // Location in the program's address space
  Block,   // Arbitrary block of bytes
  Constant,
  String,
  Flag,      // Boolean value
  Reference, // Reference to another DIE
  ExprLoc,   // DWARF expression/location description
  LinePtr,
  LocListPtr,
  MacPtr,
  RangeListPtr
};

static TagClass classify_tag(int tag)
{
  static const std::unordered_map<int, TagClass> map = {
      {DW_TAG_array_type, TagClass::Type},            {DW_TAG_class_type, TagClass::Type},
      {DW_TAG_enumeration_type, TagClass::Type},      {DW_TAG_typedef, TagClass::Type},
      {DW_TAG_pointer_type, TagClass::Type},          {DW_TAG_reference_type, TagClass::Type},
      {DW_TAG_rvalue_reference_type, TagClass::Type}, {DW_TAG_string_type, TagClass::Type},
      {DW_TAG_structure_type, TagClass::Type},        {DW_TAG_subroutine_type, TagClass::Type},
      {DW_TAG_union_type, TagClass::Type},            {DW_TAG_ptr_to_member_type, TagClass::Type},
      {DW_TAG_set_type, TagClass::Type},              {DW_TAG_subrange_type, TagClass::Type},
      {DW_TAG_base_type, TagClass::Type},             {DW_TAG_const_type, TagClass::Type},
      {DW_TAG_file_type, TagClass::Type},             {DW_TAG_packed_type, TagClass::Type},
      {DW_TAG_volatile_type, TagClass::Type},         {DW_TAG_restrict_type, TagClass::Type},
      {DW_TAG_interface_type, TagClass::Type},        {DW_TAG_unspecified_type, TagClass::Type},
      {DW_TAG_shared_type, TagClass::Type},

      {DW_TAG_subprogram, TagClass::Subprogram},

      {DW_TAG_variable, TagClass::Variable},          {DW_TAG_formal_parameter, TagClass::Variable},

      {DW_TAG_lexical_block, TagClass::Scope},        {DW_TAG_try_block, TagClass::Scope},
      {DW_TAG_catch_block, TagClass::Scope},          {DW_TAG_inlined_subroutine, TagClass::Scope},
      {DW_TAG_with_stmt, TagClass::Scope},

      {DW_TAG_namespace, TagClass::Namespace}};

  auto res = map.find(tag);
  return res != map.end() ? res->second : TagClass::Unknown;
}

/** @brief Find the DWARF data class for a given DWARF data form
 *
 *  This mapping is defined in the DWARF spec.
 *
 *  @param form The form (values taken from the DWARF spec)
 *  @return An internal representation for the corresponding class
 * */
static FormClass classify_form(int form)
{
  static const std::unordered_map<int, FormClass> map = {
      {DW_FORM_addr, FormClass::Address},

      {DW_FORM_block2, FormClass::Block},           {DW_FORM_block4, FormClass::Block},
      {DW_FORM_block, FormClass::Block},            {DW_FORM_block1, FormClass::Block},

      {DW_FORM_data1, FormClass::Constant},         {DW_FORM_data2, FormClass::Constant},
      {DW_FORM_data4, FormClass::Constant},         {DW_FORM_data8, FormClass::Constant},
      {DW_FORM_udata, FormClass::Constant},         {DW_FORM_sdata, FormClass::Constant},
#if _ELFUTILS_PREREQ(0, 171)
      {DW_FORM_implicit_const, FormClass::Constant},
#endif

      {DW_FORM_string, FormClass::String},          {DW_FORM_strp, FormClass::String},

      {DW_FORM_ref_addr, FormClass::Reference},     {DW_FORM_ref1, FormClass::Reference},
      {DW_FORM_ref2, FormClass::Reference},         {DW_FORM_ref4, FormClass::Reference},
      {DW_FORM_ref8, FormClass::Reference},         {DW_FORM_ref_udata, FormClass::Reference},

      {DW_FORM_flag, FormClass::Flag},              {DW_FORM_flag_present, FormClass::Flag},

      {DW_FORM_exprloc, FormClass::ExprLoc}

      // TODO sec offset
      // TODO indirect
  };

  auto res = map.find(form);
  return res != map.end() ? res->second : FormClass::Unknown;
}

/** @brief Get the name of the tag of a given DIE
 *
 *  @param die DIE
 *  @return name of the tag of this DIE
 */
inline XBT_PRIVATE const char* tagname(Dwarf_Die* die)
{
  return tagname(dwarf_tag(die));
}

} // namespace dwarf
} // namespace simgrid

// ***** Attributes

/** @brief Get an attribute of a given DIE as a string
 *
 *  @param die       the DIE
 *  @param attribute attribute
 *  @return value of the given attribute of the given DIE
 */
static const char* MC_dwarf_attr_integrate_string(Dwarf_Die* die, int attribute)
{
  Dwarf_Attribute attr;
  if (not dwarf_attr_integrate(die, attribute, &attr))
    return nullptr;
  else
    return dwarf_formstring(&attr);
}

static Dwarf_Off MC_dwarf_attr_dieoffset(Dwarf_Die* die, int attribute)
{
  Dwarf_Attribute attr;
  if (dwarf_hasattr_integrate(die, attribute) == 0)
    return 0;
  dwarf_attr_integrate(die, attribute, &attr);
  Dwarf_Die subtype_die;
  xbt_assert(dwarf_formref_die(&attr, &subtype_die) != nullptr, "Could not find DIE");
  return dwarf_dieoffset(&subtype_die);
}

static Dwarf_Off MC_dwarf_attr_integrate_dieoffset(Dwarf_Die* die, int attribute)
{
  Dwarf_Attribute attr;
  if (dwarf_hasattr_integrate(die, attribute) == 0)
    return 0;
  dwarf_attr_integrate(die, DW_AT_type, &attr);
  Dwarf_Die subtype_die;
  xbt_assert(dwarf_formref_die(&attr, &subtype_die) != nullptr, "Could not find DIE");
  return dwarf_dieoffset(&subtype_die);
}

/** @brief Find the type/subtype (DW_AT_type) for a DIE
 *
 *  @param die the DIE
 *  @return DW_AT_type reference as a global offset in hexadecimal (or nullptr)
 */
static std::uint64_t MC_dwarf_at_type(Dwarf_Die* die)
{
  return MC_dwarf_attr_integrate_dieoffset(die, DW_AT_type);
}

static uint64_t MC_dwarf_attr_integrate_addr(Dwarf_Die* die, int attribute)
{
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate(die, attribute, &attr) == nullptr)
    return 0;
  Dwarf_Addr value;
  if (dwarf_formaddr(&attr, &value) == 0)
    return (uint64_t)value;
  else
    return 0;
}

static uint64_t MC_dwarf_attr_integrate_uint(Dwarf_Die* die, int attribute, uint64_t default_value)
{
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate(die, attribute, &attr) == nullptr)
    return default_value;
  Dwarf_Word value;
  return dwarf_formudata(dwarf_attr_integrate(die, attribute, &attr), &value) == 0 ? (uint64_t)value : default_value;
}

static bool MC_dwarf_attr_flag(Dwarf_Die* die, int attribute, bool integrate)
{
  Dwarf_Attribute attr;
  if ((integrate ? dwarf_attr_integrate(die, attribute, &attr) : dwarf_attr(die, attribute, &attr)) == nullptr)
    return false;

  bool result;
  xbt_assert(not dwarf_formflag(&attr, &result), "Unexpected form for attribute %s",
             simgrid::dwarf::attrname(attribute));
  return result;
}

/** @brief Find the default lower bound for a given language
 *
 *  The default lower bound of an array (when DW_TAG_lower_bound
 *  is missing) depends on the language of the compilation unit.
 *
 *  @param lang Language of the compilation unit (values defined in the DWARF spec)
 *  @return     Default lower bound of an array in this compilation unit
 * */
static uint64_t MC_dwarf_default_lower_bound(int lang)
{
  const std::unordered_map<int, unsigned> map = {
      {DW_LANG_C, 0},           {DW_LANG_C89, 0},            {DW_LANG_C99, 0},            {DW_LANG_C11, 0},
      {DW_LANG_C_plus_plus, 0}, {DW_LANG_C_plus_plus_11, 0}, {DW_LANG_C_plus_plus_14, 0}, {DW_LANG_D, 0},
      {DW_LANG_Java, 0},        {DW_LANG_ObjC, 0},           {DW_LANG_ObjC_plus_plus, 0}, {DW_LANG_Python, 0},
      {DW_LANG_UPC, 0},

      {DW_LANG_Ada83, 1},       {DW_LANG_Ada95, 1},          {DW_LANG_Fortran77, 1},      {DW_LANG_Fortran90, 1},
      {DW_LANG_Fortran95, 1},   {DW_LANG_Fortran03, 1},      {DW_LANG_Fortran08, 1},      {DW_LANG_Modula2, 1},
      {DW_LANG_Pascal83, 1},    {DW_LANG_PL1, 1},            {DW_LANG_Cobol74, 1},        {DW_LANG_Cobol85, 1}};

  auto res = map.find(lang);
  xbt_assert(res != map.end(), "No default DW_TAG_lower_bound for language %i and none given", lang);
  return res->second;
}

/** @brief Finds the number of elements in a DW_TAG_subrange_type or DW_TAG_enumeration_type DIE
 *
 *  @param die  the DIE
 *  @param unit DIE of the compilation unit
 *  @return     number of elements in the range
 * */
static uint64_t MC_dwarf_subrange_element_count(Dwarf_Die* die, Dwarf_Die* unit)
{
  xbt_assert(dwarf_tag(die) == DW_TAG_enumeration_type || dwarf_tag(die) == DW_TAG_subrange_type,
             "MC_dwarf_subrange_element_count called with DIE of type %s", simgrid::dwarf::tagname(die));

  // Use DW_TAG_count if present:
  if (dwarf_hasattr_integrate(die, DW_AT_count))
    return MC_dwarf_attr_integrate_uint(die, DW_AT_count, 0);
  // Otherwise compute DW_TAG_upper_bound-DW_TAG_lower_bound + 1:

  if (not dwarf_hasattr_integrate(die, DW_AT_upper_bound))
    // This is not really 0, but the code expects this (we do not know):
    return 0;

  uint64_t upper_bound = MC_dwarf_attr_integrate_uint(die, DW_AT_upper_bound, static_cast<uint64_t>(-1));

  uint64_t lower_bound = 0;
  if (dwarf_hasattr_integrate(die, DW_AT_lower_bound))
    lower_bound = MC_dwarf_attr_integrate_uint(die, DW_AT_lower_bound, static_cast<uint64_t>(-1));
  else
    lower_bound = MC_dwarf_default_lower_bound(dwarf_srclang(unit));
  return upper_bound - lower_bound + 1;
}

/** @brief Finds the number of elements in an array type (DW_TAG_array_type)
 *
 *  The compilation unit might be needed because the default lower
 *  bound depends on the language of the compilation unit.
 *
 *  @param die the DIE of the DW_TAG_array_type
 *  @param unit the DIE of the compilation unit
 *  @return number of elements in this array type
 * */
static uint64_t MC_dwarf_array_element_count(Dwarf_Die* die, Dwarf_Die* unit)
{
  xbt_assert(dwarf_tag(die) == DW_TAG_array_type, "MC_dwarf_array_element_count called with DIE of type %s",
             simgrid::dwarf::tagname(die));

  int result = 1;
  Dwarf_Die child;
  for (int res = dwarf_child(die, &child); res == 0; res = dwarf_siblingof(&child, &child)) {
    int child_tag = dwarf_tag(&child);
    if (child_tag == DW_TAG_subrange_type || child_tag == DW_TAG_enumeration_type)
      result *= MC_dwarf_subrange_element_count(&child, unit);
  }
  return result;
}

// ***** Variable

/** Sort the variable by name and address.
 *
 *  We could use boost::container::flat_set instead.
 */
static bool MC_compare_variable(simgrid::mc::Variable const& a, simgrid::mc::Variable const& b)
{
  int cmp = a.name.compare(b.name);
  if (cmp < 0)
    return true;
  else if (cmp > 0)
    return false;
  else
    return a.address < b.address;
}

// ***** simgrid::mc::Type*

/** @brief Initialize the location of a member of a type
 * (DW_AT_data_member_location of a DW_TAG_member).
 *
 *  @param  type   a type (struct, class)
 *  @param  member the member of the type
 *  @param  child  DIE of the member (DW_TAG_member)
 */
static void MC_dwarf_fill_member_location(const simgrid::mc::Type* type, simgrid::mc::Member* member, Dwarf_Die* child)
{
  xbt_assert(not dwarf_hasattr(child, DW_AT_data_bit_offset), "Can't groke DW_AT_data_bit_offset.");

  if (not dwarf_hasattr_integrate(child, DW_AT_data_member_location)) {
    if (type->type == DW_TAG_union_type)
      return;
    xbt_die("Missing DW_AT_data_member_location field in DW_TAG_member %s of type <%" PRIx64 ">%s",
            member->name.c_str(), (uint64_t)type->id, type->name.c_str());
  }

  Dwarf_Attribute attr;
  dwarf_attr_integrate(child, DW_AT_data_member_location, &attr);
  int form                             = dwarf_whatform(&attr);
  simgrid::dwarf::FormClass form_class = simgrid::dwarf::classify_form(form);
  switch (form_class) {
    case simgrid::dwarf::FormClass::ExprLoc:
    case simgrid::dwarf::FormClass::Block:
      // Location expression:
      {
        Dwarf_Op* expr;
        size_t len;
        xbt_assert(not dwarf_getlocation(&attr, &expr, &len),
                   "Could not read location expression DW_AT_data_member_location in DW_TAG_member %s of type <%" PRIx64
                   ">%s",
                   MC_dwarf_attr_integrate_string(child, DW_AT_name), (uint64_t)type->id, type->name.c_str());
        member->location_expression = simgrid::dwarf::DwarfExpression(expr, expr + len);
        break;
      }
    case simgrid::dwarf::FormClass::Constant:
      // Offset from the base address of the object:
      {
        Dwarf_Word offset;
        xbt_assert(not dwarf_formudata(&attr, &offset), "Cannot get %s location <%" PRIx64 ">%s",
                   MC_dwarf_attr_integrate_string(child, DW_AT_name), (uint64_t)type->id, type->name.c_str());
        member->offset(offset);
        break;
      }

    default:
      // includes FormClass::LocListPtr (reference to a location list: TODO) and FormClass::Reference (it's supposed to
      // be possible in DWARF2 but I couldn't find its semantic in the spec)
      xbt_die("Can't handle form class (%d) / form 0x%x as DW_AT_member_location", (int)form_class, (unsigned)form);
  }
}

/** @brief Populate the list of members of a type
 *
 *  @param info ELF object containing the type DIE
 *  @param die  DIE of the type
 *  @param unit DIE of the compilation unit containing the type DIE
 *  @param type the type
 */
static void MC_dwarf_add_members(const simgrid::mc::ObjectInformation* /*info*/, Dwarf_Die* die,
                                 const Dwarf_Die* /*unit*/, simgrid::mc::Type* type)
{
  Dwarf_Die child;
  xbt_assert(type->members.empty());
  for (int res = dwarf_child(die, &child); res == 0; res = dwarf_siblingof(&child, &child)) {
    int tag = dwarf_tag(&child);
    if (tag == DW_TAG_member || tag == DW_TAG_inheritance) {
      // Skip declarations:
      if (MC_dwarf_attr_flag(&child, DW_AT_declaration, false))
        continue;

      // Skip compile time constants:
      if (dwarf_hasattr(&child, DW_AT_const_value))
        continue;

      // TODO, we should use another type (because is is not a type but a member)
      simgrid::mc::Member member;
      if (tag == DW_TAG_inheritance)
        member.flags |= simgrid::mc::Member::INHERITANCE_FLAG;

      const char* name = MC_dwarf_attr_integrate_string(&child, DW_AT_name);
      if (name)
        member.name = name;
      // Those base names are used by GCC and clang for virtual table pointers
      // respectively ("__vptr$ClassName", "__vptr.ClassName"):
      if (boost::algorithm::starts_with(member.name, "__vptr$") ||
          boost::algorithm::starts_with(member.name, "__vptr."))
        member.flags |= simgrid::mc::Member::VIRTUAL_POINTER_FLAG;
      // A cleaner solution would be to check against the type:
      // ---
      // tag: DW_TAG_member
      // name: "_vptr$Foo"
      // type:
      //   # Type for a pointer to a vtable
      //   tag: DW_TAG_pointer_type
      //   type:
      //     # Type for a vtable:
      //     tag: DW_TAG_pointer_type
      //     name: "__vtbl_ptr_type"
      //     type:
      //       tag: DW_TAG_subroutine_type
      //       type:
      //         tag: DW_TAG_base_type
      //         name: "int"
      // ---

      member.byte_size = MC_dwarf_attr_integrate_uint(&child, DW_AT_byte_size, 0);
      member.type_id   = MC_dwarf_at_type(&child);

      xbt_assert(not dwarf_hasattr(&child, DW_AT_data_bit_offset), "Can't groke DW_AT_data_bit_offset.");

      MC_dwarf_fill_member_location(type, &member, &child);

      xbt_assert(member.type_id, "Missing type for member %s of <%" PRIx64 ">%s", member.name.c_str(),
                 (uint64_t)type->id, type->name.c_str());

      type->members.push_back(std::move(member));
    }
  }
}

/** @brief Create a MC type object from a DIE
 *
 *  @param info current object info object
 *  @param die DIE (for a given type)
 *  @param unit compilation unit of the current DIE
 *  @return MC representation of the type
 */
static simgrid::mc::Type MC_dwarf_die_to_type(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                              simgrid::mc::Frame* frame, const char* ns)
{
  simgrid::mc::Type type;
  type.type          = dwarf_tag(die);
  type.name          = std::string();
  type.element_count = -1;

  // Global Offset
  type.id = dwarf_dieoffset(die);

  const char* prefix = "";
  switch (type.type) {
    case DW_TAG_structure_type:
      prefix = "struct ";
      break;
    case DW_TAG_union_type:
      prefix = "union ";
      break;
    case DW_TAG_class_type:
      prefix = "class ";
      break;
    default:
      prefix = "";
  }

  const char* name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
  if (name != nullptr) {
    if (ns)
      type.name = simgrid::xbt::string_printf("%s%s::%s", prefix, ns, name);
    else
      type.name = simgrid::xbt::string_printf("%s%s", prefix, name);
  }

  type.type_id = MC_dwarf_at_type(die);

  // Some compilers do not emit DW_AT_byte_size for pointer_type,
  // so we fill this. We currently assume that the model-checked process is in
  // the same architecture..
  if (type.type == DW_TAG_pointer_type)
    type.byte_size = sizeof(void*);

  // Computation of the byte_size
  if (dwarf_hasattr_integrate(die, DW_AT_byte_size))
    type.byte_size = MC_dwarf_attr_integrate_uint(die, DW_AT_byte_size, 0);
  else if (type.type == DW_TAG_array_type || type.type == DW_TAG_structure_type || type.type == DW_TAG_class_type) {
    Dwarf_Word size;
    if (dwarf_aggregate_size(die, &size) == 0)
      type.byte_size = size;
  }

  switch (type.type) {
    case DW_TAG_array_type:
      type.element_count = MC_dwarf_array_element_count(die, unit);
      // TODO, handle DW_byte_stride and (not) DW_bit_stride
      break;

    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
      break;

    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_class_type:
      MC_dwarf_add_members(info, die, unit, &type);
      MC_dwarf_handle_children(info, die, unit, frame,
                               ns ? simgrid::xbt::string_printf("%s::%s", ns, name).c_str() : type.name.c_str());
      break;

    default:
      XBT_DEBUG("Unhandled type: %d (%s)", type.type, simgrid::dwarf::tagname(type.type));
      break;
  }

  return type;
}

static void MC_dwarf_handle_type_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                     simgrid::mc::Frame* frame, const char* ns)
{
  simgrid::mc::Type type = MC_dwarf_die_to_type(info, die, unit, frame, ns);
  auto& t                = (info->types[type.id] = std::move(type));
  if (not t.name.empty() && type.byte_size != 0)
    info->full_types_by_name[t.name] = &t;
}

static std::unique_ptr<simgrid::mc::Variable> MC_die_to_variable(simgrid::mc::ObjectInformation* info, Dwarf_Die* die,
                                                                 const Dwarf_Die* /*unit*/,
                                                                 const simgrid::mc::Frame* frame, const char* ns)
{
  // Skip declarations:
  if (MC_dwarf_attr_flag(die, DW_AT_declaration, false))
    return nullptr;

  // Skip compile time constants:
  if (dwarf_hasattr(die, DW_AT_const_value))
    return nullptr;

  Dwarf_Attribute attr_location;
  if (dwarf_attr(die, DW_AT_location, &attr_location) == nullptr)
    // No location: do not add it ?
    return nullptr;

  auto variable         = std::make_unique<simgrid::mc::Variable>();
  variable->id          = dwarf_dieoffset(die);
  variable->global      = frame == nullptr; // Can be override base on DW_AT_location
  variable->object_info = info;

  const char* name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
  if (name)
    variable->name = name;
  variable->type_id = MC_dwarf_at_type(die);

  int form = dwarf_whatform(&attr_location);
  simgrid::dwarf::FormClass form_class;
  if (form == DW_FORM_sec_offset)
    form_class = simgrid::dwarf::FormClass::Constant;
  else
    form_class = simgrid::dwarf::classify_form(form);
  switch (form_class) {
    case simgrid::dwarf::FormClass::ExprLoc:
    case simgrid::dwarf::FormClass::Block:
      // Location expression:
      {
        Dwarf_Op* expr;
        size_t len;
        xbt_assert(not dwarf_getlocation(&attr_location, &expr, &len),
                   "Could not read location expression in DW_AT_location "
                   "of variable <%" PRIx64 ">%s",
                   (uint64_t)variable->id, variable->name.c_str());

        if (len == 1 && expr[0].atom == DW_OP_addr) {
          variable->global  = true;
          auto offset       = static_cast<uintptr_t>(expr[0].number);
          auto base         = reinterpret_cast<uintptr_t>(info->base_address());
          variable->address = reinterpret_cast<void*>(base + offset);
        } else
          variable->location_list = {
              simgrid::dwarf::LocationListEntry(simgrid::dwarf::DwarfExpression(expr, expr + len))};

        break;
      }

    case simgrid::dwarf::FormClass::LocListPtr:
    case simgrid::dwarf::FormClass::Constant:
      // Reference to location list:
      variable->location_list = simgrid::dwarf::location_list(*info, attr_location);
      break;

    default:
      xbt_die("Unexpected form 0x%x (%i), class 0x%x (%i) list for location in <%" PRIx64 ">%s", (unsigned)form, form,
              (unsigned)form_class, (int)form_class, (uint64_t)variable->id, variable->name.c_str());
  }

  // Handle start_scope:
  if (dwarf_hasattr(die, DW_AT_start_scope)) {
    Dwarf_Attribute attr;
    dwarf_attr(die, DW_AT_start_scope, &attr);
    form       = dwarf_whatform(&attr);
    form_class = simgrid::dwarf::classify_form(form);
    if (form_class == simgrid::dwarf::FormClass::Constant) {
      Dwarf_Word value;
      variable->start_scope = dwarf_formudata(&attr, &value) == 0 ? (size_t)value : 0;
    } else {
      // TODO: FormClass::RangeListPtr
      xbt_die("Unhandled form 0x%x, class 0x%X for DW_AT_start_scope of variable %s", (unsigned)form,
              (unsigned)form_class, name == nullptr ? "?" : name);
    }
  }

  if (ns && variable->global)
    variable->name = std::string(ns) + "::" + variable->name;

  // The current code needs a variable name,
  // generate a fake one:
  static int mc_anonymous_variable_index = 0;
  if (variable->name.empty()) {
    variable->name = "@anonymous#" + std::to_string(mc_anonymous_variable_index);
    mc_anonymous_variable_index++;
  }
  return variable;
}

static void MC_dwarf_handle_variable_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, const Dwarf_Die* unit,
                                         simgrid::mc::Frame* frame, const char* ns)
{
  std::unique_ptr<simgrid::mc::Variable> variable = MC_die_to_variable(info, die, unit, frame, ns);
  if (not variable)
    return;
  // Those arrays are sorted later:
  if (variable->global)
    info->global_variables.push_back(std::move(*variable));
  else if (frame != nullptr)
    frame->variables.push_back(std::move(*variable));
  else
    xbt_die("No frame for this local variable");
}

static void MC_dwarf_handle_scope_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                      simgrid::mc::Frame* parent_frame, const char* ns)
{
  // TODO, handle DW_TAG_type/DW_TAG_location for DW_TAG_with_stmt
  int tag                        = dwarf_tag(die);
  simgrid::dwarf::TagClass klass = simgrid::dwarf::classify_tag(tag);

  // (Template) Subprogram declaration:
  if (klass == simgrid::dwarf::TagClass::Subprogram && MC_dwarf_attr_flag(die, DW_AT_declaration, false))
    return;

  if (klass == simgrid::dwarf::TagClass::Scope)
    xbt_assert(parent_frame, "No parent scope for this scope");

  simgrid::mc::Frame frame;
  frame.tag         = tag;
  frame.id          = dwarf_dieoffset(die);
  frame.object_info = info;

  if (klass == simgrid::dwarf::TagClass::Subprogram) {
    const char* name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
    if (name && ns)
      frame.name = std::string(ns) + "::" + name;
    else if (name)
      frame.name = name;
  }

  frame.abstract_origin_id = MC_dwarf_attr_dieoffset(die, DW_AT_abstract_origin);

  // This is the base address for DWARF addresses.
  // Relocated addresses are offset from this base address.
  // See DWARF4 spec 7.5
  auto base = reinterpret_cast<std::uint64_t>(info->base_address());

  // TODO, support DW_AT_ranges
  uint64_t low_pc     = MC_dwarf_attr_integrate_addr(die, DW_AT_low_pc);
  frame.range.begin() = low_pc ? base + low_pc : 0;
  if (low_pc) {
    // DW_AT_high_pc:
    Dwarf_Attribute attr;
    xbt_assert(dwarf_attr_integrate(die, DW_AT_high_pc, &attr), "Missing DW_AT_high_pc matching with DW_AT_low_pc");

    Dwarf_Sword offset;
    Dwarf_Addr high_pc;

    switch (simgrid::dwarf::classify_form(dwarf_whatform(&attr))) {
      // DW_AT_high_pc if an offset from the low_pc:
      case simgrid::dwarf::FormClass::Constant:

        xbt_assert(dwarf_formsdata(&attr, &offset) == 0, "Could not read constant");
        frame.range.end() = frame.range.begin() + offset;
        break;

        // DW_AT_high_pc is a relocatable address:
      case simgrid::dwarf::FormClass::Address:
        xbt_assert(dwarf_formaddr(&attr, &high_pc) == 0, "Could not read address");
        frame.range.end() = base + high_pc;
        break;

      default:
        xbt_die("Unexpected class for DW_AT_high_pc");
    }
  }

  if (klass == simgrid::dwarf::TagClass::Subprogram) {
    Dwarf_Attribute attr_frame_base;
    if (dwarf_attr_integrate(die, DW_AT_frame_base, &attr_frame_base))
      frame.frame_base_location = simgrid::dwarf::location_list(*info, attr_frame_base);
  }

  // Handle children:
  MC_dwarf_handle_children(info, die, unit, &frame, ns);

  // We sort them in order to have an (somewhat) efficient by name
  // lookup:
  boost::range::sort(frame.variables, MC_compare_variable);

  // Register it:
  if (klass == simgrid::dwarf::TagClass::Subprogram)
    info->subprograms[frame.id] = std::move(frame);
  else if (klass == simgrid::dwarf::TagClass::Scope)
    parent_frame->scopes.push_back(std::move(frame));
}

static void mc_dwarf_handle_namespace_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                          simgrid::mc::Frame* frame, const char* ns)
{
  const char* name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
  xbt_assert(not frame, "Unexpected namespace in a subprogram");
  char* new_ns = ns == nullptr ? xbt_strdup(name) : bprintf("%s::%s", ns, name);
  MC_dwarf_handle_children(info, die, unit, frame, new_ns);
  xbt_free(new_ns);
}

static void MC_dwarf_handle_children(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                     simgrid::mc::Frame* frame, const char* ns)
{
  // For each child DIE:
  Dwarf_Die child;
  for (int res = dwarf_child(die, &child); res == 0; res = dwarf_siblingof(&child, &child))
    MC_dwarf_handle_die(info, &child, unit, frame, ns);
}

static void MC_dwarf_handle_die(simgrid::mc::ObjectInformation* info, Dwarf_Die* die, Dwarf_Die* unit,
                                simgrid::mc::Frame* frame, const char* ns)
{
  int tag                        = dwarf_tag(die);
  simgrid::dwarf::TagClass klass = simgrid::dwarf::classify_tag(tag);
  switch (klass) {
    // Type:
    case simgrid::dwarf::TagClass::Type:
      MC_dwarf_handle_type_die(info, die, unit, frame, ns);
      break;

      // Subprogram or scope:
    case simgrid::dwarf::TagClass::Subprogram:
    case simgrid::dwarf::TagClass::Scope:
      MC_dwarf_handle_scope_die(info, die, unit, frame, ns);
      return;

      // Variable:
    case simgrid::dwarf::TagClass::Variable:
      MC_dwarf_handle_variable_die(info, die, unit, frame, ns);
      break;

    case simgrid::dwarf::TagClass::Namespace:
      mc_dwarf_handle_namespace_die(info, die, unit, frame, ns);
      break;

    default:
      break;
  }
}

static Elf64_Half get_type(Elf* elf)
{
  const Elf64_Ehdr* ehdr64 = elf64_getehdr(elf);
  if (ehdr64)
    return ehdr64->e_type;
  const Elf32_Ehdr* ehdr32 = elf32_getehdr(elf);
  if (ehdr32)
    return ehdr32->e_type;
  xbt_die("Could not get ELF heeader");
}

static void read_dwarf_info(simgrid::mc::ObjectInformation* info, Dwarf* dwarf)
{
  // For each compilation unit:
  Dwarf_Off offset      = 0;
  Dwarf_Off next_offset = 0;
  size_t length;

  while (dwarf_nextcu(dwarf, offset, &next_offset, &length, nullptr, nullptr, nullptr) == 0) {
    Dwarf_Die unit_die;
    if (dwarf_offdie(dwarf, offset + length, &unit_die) != nullptr)
      MC_dwarf_handle_children(info, &unit_die, &unit_die, nullptr, nullptr);
    offset = next_offset;
  }
}

/** Get the build-id (NT_GNU_BUILD_ID) from the ELF file
 *
 *  This build-id may is used to locate an external debug (DWARF) file
 *  for this ELF file.
 *
 *  @param  elf libelf handle for an ELF file
 *  @return build-id for this ELF file (or an empty vector if none is found)
 */
static std::vector<char> get_build_id(Elf* elf)
{
#ifdef __linux
  // Summary: the GNU build ID is stored in a ("GNU, NT_GNU_BUILD_ID) note
  // found in a PT_NOTE entry in the program header table.

  size_t phnum;
  xbt_assert(elf_getphdrnum(elf, &phnum) == 0, "Could not read program headers");

  // Iterate over the program headers and find the PT_NOTE ones:
  for (size_t i = 0; i < phnum; ++i) {
    GElf_Phdr phdr_temp;
    const GElf_Phdr* phdr = gelf_getphdr(elf, i, &phdr_temp);
    if (phdr->p_type != PT_NOTE)
      continue;

    Elf_Data* data = elf_getdata_rawchunk(elf, phdr->p_offset, phdr->p_filesz, ELF_T_NHDR);

    // Iterate over the notes and find the NT_GNU_BUILD_ID one:
    size_t pos = 0;
    while (pos < data->d_size) {
      GElf_Nhdr nhdr;
      // Location of the name within Elf_Data:
      size_t name_pos;
      size_t desc_pos;
      pos = gelf_getnote(data, pos, &nhdr, &name_pos, &desc_pos);
      // A build ID note is identified by the pair ("GNU", NT_GNU_BUILD_ID)
      // (a namespace and a type within this namespace):
      if (nhdr.n_type == NT_GNU_BUILD_ID && nhdr.n_namesz == sizeof("GNU") &&
          memcmp((char*)data->d_buf + name_pos, "GNU", sizeof("GNU")) == 0) {
        XBT_DEBUG("Found GNU/NT_GNU_BUILD_ID note");
        char* start = (char*)data->d_buf + desc_pos;
        char* end   = start + nhdr.n_descsz;
        return std::vector<char>(start, end);
      }
    }
  }
#endif
  return std::vector<char>();
}

/** Binary data to hexadecimal */
static inline std::array<char, 2> to_hex(std::uint8_t byte)
{
  constexpr std::array<char, 16> hexdigits{
      {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'}};
  // Horrid double braces!
  // Apparently, this is needed in C++11 (not in C++14).
  return {{hexdigits[byte >> 4], hexdigits[byte & 0xF]}};
}

/** Binary data to hexadecimal */
static std::string to_hex(const char* data, std::size_t count)
{
  std::string res;
  res.resize(2 * count);
  for (std::size_t i = 0; i < count; i++)
    std::copy_n(cbegin(to_hex(data[i])), 2, &res[2 * i]);
  return res;
}

/** Binary data to hexadecimal */
static std::string to_hex(std::vector<char> const& data)
{
  return to_hex(data.data(), data.size());
}

/** Base directories for external debug files */
static constexpr auto debug_paths = {
    "/usr/lib/debug/",
    "/usr/local/lib/debug/",
};

/** Locate an external debug file from the NT_GNU_BUILD_ID
 *
 *  This is one of the mechanisms used for
 *  [separate debug files](https://sourceware.org/gdb/onlinedocs/gdb/Separate-Debug-Files.html).
 */
// Example:
// /usr/lib/debug/.build-id/0b/dc77f1c29aea2b14ff5acd9a19ab3175ffdeae.debug
static std::string find_by_build_id(std::vector<char> id)
{
  std::string filename;
  std::string hex = to_hex(id);
  for (const char* const& debug_path : debug_paths) {
    // Example:
    filename = std::string(debug_path) + ".build-id/" + to_hex(id.data(), 1) + '/' +
               to_hex(id.data() + 1, id.size() - 1) + ".debug";
    XBT_DEBUG("Checking debug file: %s", filename.c_str());
    if (access(filename.c_str(), F_OK) == 0) {
      XBT_DEBUG("Found debug file: %s\n", hex.c_str());
      return filename;
    }
  }
  XBT_DEBUG("No debug info found for build ID %s\n", hex.data());
  return std::string();
}

/** @brief Populate the debugging information of the given ELF object
 *
 *  Read the DWARf information of the EFFL object and populate the
 *  lists of types, variables, functions.
 */
static void MC_load_dwarf(simgrid::mc::ObjectInformation* info)
{
  xbt_assert(elf_version(EV_CURRENT) != EV_NONE, "libelf initialization error");

  // Open the ELF file:
  int fd = open(info->file_name.c_str(), O_RDONLY);
  xbt_assert(fd >= 0, "Could not open file %s", info->file_name.c_str());
  Elf* elf = elf_begin(fd, ELF_C_READ, nullptr);
  xbt_assert(elf != nullptr, "Not an ELF file");
  Elf_Kind kind = elf_kind(elf);
  xbt_assert(kind == ELF_K_ELF, "Not an ELF file");

  // Remember if this is a `ET_EXEC` (fixed location) or `ET_DYN`:
  Elf64_Half type = get_type(elf);
  if (type == ET_EXEC)
    info->flags |= simgrid::mc::ObjectInformation::Executable;

  // Read DWARF debug information in the file:
  Dwarf* dwarf = dwarf_begin_elf(elf, DWARF_C_READ, nullptr);
  if (dwarf != nullptr) {
    read_dwarf_info(info, dwarf);
    dwarf_end(dwarf);
    elf_end(elf);
    close(fd);
    return;
  }
  dwarf_end(dwarf);

  // If there was no DWARF in the file, try to find it in a separate file.
  // Different methods might be used to store the DWARF information:
  //  * GNU NT_GNU_BUILD_ID
  //  * .gnu_debuglink
  // See https://sourceware.org/gdb/onlinedocs/gdb/Separate-Debug-Files.html
  // for reference of what we are doing.

  // Try with NT_GNU_BUILD_ID: we find the build ID in the ELF file and then
  // use this ID to find the file in some known locations in the filesystem.
  std::vector<char> build_id = get_build_id(elf);
  if (not build_id.empty()) {
    elf_end(elf);
    close(fd);

    // Find the debug file using the build id:
    std::string debug_file = find_by_build_id(build_id);
    xbt_assert(not debug_file.empty(),
               "Missing debug info for %s with build-id %s\n"
               "You might want to install the suitable debugging package.\n",
               info->file_name.c_str(), to_hex(build_id).c_str());

    // Load the DWARF info from this file:
    XBT_DEBUG("Load DWARF for %s from %s", info->file_name.c_str(), debug_file.c_str());
    fd = open(debug_file.c_str(), O_RDONLY);
    xbt_assert(fd >= 0, "Could not open file %s", debug_file.c_str());
    dwarf = dwarf_begin(fd, DWARF_C_READ);
    xbt_assert(dwarf != nullptr, "No DWARF info in %s for %s", debug_file.c_str(), info->file_name.c_str());
    read_dwarf_info(info, dwarf);
    dwarf_end(dwarf);
    close(fd);
    return;
  }

  // TODO, try to find DWARF info using .gnu_debuglink.

  elf_end(elf);
  close(fd);
  xbt_die("Debugging information not found for %s\n"
          "Try recompiling with -g\n",
          info->file_name.c_str());
}

// ***** Functions index

static void MC_make_functions_index(simgrid::mc::ObjectInformation* info)
{
  info->functions_index.clear();

  for (auto& e : info->subprograms) {
    if (e.second.range.begin() == 0)
      continue;
    simgrid::mc::FunctionIndexEntry entry;
    entry.low_pc   = (void*)e.second.range.begin();
    entry.function = &e.second;
    info->functions_index.push_back(entry);
  }

  info->functions_index.shrink_to_fit();

  // Sort the array by low_pc:
  boost::range::sort(info->functions_index,
                     [](simgrid::mc::FunctionIndexEntry const& a, simgrid::mc::FunctionIndexEntry const& b) {
                       return a.low_pc < b.low_pc;
                     });
}

static void MC_post_process_variables(simgrid::mc::ObjectInformation* info)
{
  // Someone needs this to be sorted but who?
  boost::range::sort(info->global_variables, MC_compare_variable);

  for (simgrid::mc::Variable& variable : info->global_variables)
    if (variable.type_id)
      variable.type = simgrid::util::find_map_ptr(info->types, variable.type_id);
}

static void mc_post_process_scope(simgrid::mc::ObjectInformation* info, simgrid::mc::Frame* scope)
{
  if (scope->tag == DW_TAG_inlined_subroutine) {
    // Attach correct namespaced name in inlined subroutine:
    auto i = info->subprograms.find(scope->abstract_origin_id);
    xbt_assert(i != info->subprograms.end(), "Could not lookup abstract origin %" PRIx64,
               (std::uint64_t)scope->abstract_origin_id);
    scope->name = i->second.name;
  }

  // Direct:
  for (simgrid::mc::Variable& variable : scope->variables)
    if (variable.type_id)
      variable.type = simgrid::util::find_map_ptr(info->types, variable.type_id);

  // Recursive post-processing of nested-scopes:
  for (simgrid::mc::Frame& nested_scope : scope->scopes)
    mc_post_process_scope(info, &nested_scope);
}

static simgrid::mc::Type* MC_resolve_type(simgrid::mc::ObjectInformation* info, unsigned type_id)
{
  if (not type_id)
    return nullptr;
  simgrid::mc::Type* type = simgrid::util::find_map_ptr(info->types, type_id);
  if (type == nullptr)
    return nullptr;

  // We already have the information on the type:
  if (type->byte_size != 0)
    return type;

  // Don't have a name, we can't find a more complete version:
  if (type->name.empty())
    return type;

  // Try to find a more complete description of the type:
  // We need to fix in order to support C++.
  simgrid::mc::Type** subtype = simgrid::util::find_map_ptr(info->full_types_by_name, type->name);
  if (subtype)
    type = *subtype;
  return type;
}

static void MC_post_process_types(simgrid::mc::ObjectInformation* info)
{
  // Lookup "subtype" field:
  for (auto& i : info->types) {
    i.second.subtype = MC_resolve_type(info, i.second.type_id);
    for (simgrid::mc::Member& member : i.second.members)
      member.type = MC_resolve_type(info, member.type_id);
  }
}

namespace simgrid {
namespace mc {

/** @brief Finds information about a given shared object/executable */
std::shared_ptr<ObjectInformation> createObjectInformation(std::vector<xbt::VmMap> const& maps, const char* name)
{
  auto result       = std::make_shared<ObjectInformation>();
  result->file_name = name;
  simgrid::mc::find_object_address(maps, result.get());
  MC_load_dwarf(result.get());
  MC_post_process_variables(result.get());
  MC_post_process_types(result.get());
  for (auto& entry : result.get()->subprograms)
    mc_post_process_scope(result.get(), &entry.second);
  MC_make_functions_index(result.get());
  return result;
}

/*************************************************************************/

void postProcessObjectInformation(const RemoteSimulation* process, ObjectInformation* info)
{
  for (auto& t : info->types) {
    Type* type    = &(t.second);
    Type* subtype = type;
    while (subtype->type == DW_TAG_typedef || subtype->type == DW_TAG_volatile_type ||
           subtype->type == DW_TAG_const_type)
      if (subtype->subtype)
        subtype = subtype->subtype;
      else
        break;

    // Resolve full_type:
    if (not subtype->name.empty() && subtype->byte_size == 0)
      for (auto const& object_info : process->object_infos) {
        auto i = object_info->full_types_by_name.find(subtype->name);
        if (i != object_info->full_types_by_name.end() && not i->second->name.empty() && i->second->byte_size) {
          type->full_type = i->second;
          break;
        }
      }
    else
      type->full_type = subtype;
  }
}

} // namespace mc
} // namespace simgrid

namespace simgrid {
namespace dwarf {

/** Convert a DWARF register into a libunwind register
 *
 *  DWARF and libunwind does not use the same convention for numbering the
 *  registers on some architectures. The function makes the necessary
 *  conversion.
 */
int dwarf_register_to_libunwind(int dwarf_register)
{
#if defined(__x86_64__)
  // It seems for this arch, DWARF and libunwind agree in the numbering:
  return dwarf_register;
#elif defined(__i386__)
  // Couldn't find the authoritative source of information for this.
  // This is inspired from http://source.winehq.org/source/dlls/dbghelp/cpu_i386.c#L517.
  constexpr std::array<int, 24> regs{
      {/*  0 */ UNW_X86_EAX, /*  1 */ UNW_X86_ECX,    /*  2 */ UNW_X86_EDX, /*  3 */ UNW_X86_EBX,
       /*  4 */ UNW_X86_ESP, /*  5 */ UNW_X86_EBP,    /*  6 */ UNW_X86_ESI, /*  7 */ UNW_X86_EDI,
       /*  8 */ UNW_X86_EIP, /*  9 */ UNW_X86_EFLAGS, /* 10 */ UNW_X86_CS,  /* 11 */ UNW_X86_SS,
       /* 12 */ UNW_X86_DS,  /* 13 */ UNW_X86_ES,     /* 14 */ UNW_X86_FS,  /* 15 */ UNW_X86_GS,
       /* 16 */ UNW_X86_ST0, /* 17 */ UNW_X86_ST1,    /* 18 */ UNW_X86_ST2, /* 19 */ UNW_X86_ST3,
       /* 20 */ UNW_X86_ST4, /* 21 */ UNW_X86_ST5,    /* 22 */ UNW_X86_ST6, /* 23 */ UNW_X86_ST7}};
  return regs.at(dwarf_register);
#else
#error This architecture is not supported yet for DWARF expression evaluation.
#endif
}

} // namespace dwarf
} // namespace simgrid
