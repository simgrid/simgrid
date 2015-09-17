/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#if !defined(SIMGRID_MC_LIBDW_HPP)
#define SIMGRID_MC_LIBDW_HPP

#include <stdint.h>

#include <dwarf.h>
#include <elfutils/libdw.h>

#include "mc/Frame.hpp"
#include "mc/ObjectInformation.hpp"
#include "mc/Variable.hpp"

/** \brief Computes the the element_count of a DW_TAG_enumeration_type DIE
 *
 * This is the number of elements in a given array dimension.
 *
 * A reference of the compilation unit (DW_TAG_compile_unit) is
 * needed because the default lower bound (when there is no DW_AT_lower_bound)
 * depends of the language of the compilation unit (DW_AT_language).
 *
 * \param die  DIE for the DW_TAG_enumeration_type or DW_TAG_subrange_type
 * \param unit DIE of the DW_TAG_compile_unit
 */
static std::uint64_t MC_dwarf_subrange_element_count(Dwarf_Die * die,
                                                Dwarf_Die * unit);

/** \brief Computes the number of elements of a given DW_TAG_array_type.
 *
 * \param die DIE for the DW_TAG_array_type
 */
static std::uint64_t MC_dwarf_array_element_count(Dwarf_Die * die, Dwarf_Die * unit);

/** \brief Process a DIE
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                const char *ns);

/** \brief Process a type DIE
 */
static void MC_dwarf_handle_type_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                     Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                     const char *ns);

/** \brief Calls MC_dwarf_handle_die on all childrend of the given die
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_children(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                     Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                     const char *ns);

/** \brief Handle a variable (DW_TAG_variable or other)
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_variable_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                         Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                         const char *ns);

/** \brief Get the DW_TAG_type of the DIE
 *
 *  \param die DIE
 *  \return DW_TAG_type attribute as a new string (NULL if none)
 */
static std::uint64_t MC_dwarf_at_type(Dwarf_Die * die);

#endif
