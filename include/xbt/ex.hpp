/* Copyright (c) 2005-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/*  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>       */
/*  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>         */
/*  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>           */
/*  All rights reserved.                                                    */

/* This code is inspirated from the OSSP version (as retrieved back in 2004)*/
/* It was heavily modified to fit the SimGrid framework.                    */

/* The OSSP version has the following copyright notice:
**  OSSP ex - Exception Handling
**  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>
**
**  This file is part of OSSP ex, an exception handling library
**  which can be found at http://www.ossp.org/pkg/lib/ex/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED `AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
 */

/* The extensions made for the SimGrid project can either be distributed    */
/* under the same license, or under the LGPL v2.1                           */

#ifndef SIMGRID_XBT_EX_HPP
#define SIMGRID_XBT_EX_HPP

#include <stdexcept>
#include <xbt/exception.hpp>

#include <xbt/ex.h>

/** A legacy exception
 *
 *  It is defined by a category and a value within that category (as well as
 *  an optional error message).
 *
 *  This used to be a structure for C exceptions but it has been retrofitted
 *  as a C++ exception and some of its data has been moved in the
 *  @ref WithContextException base class. We should deprecate it and replace it
 *  with either C++ different exceptions or `std::system_error` which already
 *  provides this (category + error code) logic.
 *
 *  @ingroup XBT_ex_c
 */
struct XBT_PUBLIC() xbt_ex :
  public std::runtime_error,
  public simgrid::xbt::WithContextException {
public:

  xbt_ex() :
    std::runtime_error("")
  {}

  /**
   *
   * @param throwpoint Throw point (use XBT_THROW_POINT)
   * @param message    Exception message
   */
  xbt_ex(simgrid::xbt::ThrowPoint throwpoint, const char* message) :
    std::runtime_error(message),
    simgrid::xbt::WithContextException(throwpoint, simgrid::xbt::backtrace())
  {}

  ~xbt_ex() override;

  /** Category (what went wrong) */
  xbt_errcat_t category;

  /** Why did it went wrong */
  int value;

};

#endif
