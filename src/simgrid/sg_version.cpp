/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "simgrid/config.h"
#include "simgrid/version.h"
#include "xbt/base.h"
#include "xbt/log.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sg_version, surf, "About the versioning of SimGrid");

void sg_version_check(int lib_version_major, int lib_version_minor, int lib_version_patch)
{
  if ((lib_version_major != SIMGRID_VERSION_MAJOR) || (lib_version_minor != SIMGRID_VERSION_MINOR)) {
    fprintf(stderr,
            "FATAL ERROR: Your program was compiled with SimGrid version %d.%d.%d, "
            "and then linked against SimGrid %d.%d.%d. Please fix this.\n",
            lib_version_major, lib_version_minor, lib_version_patch, SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR,
            SIMGRID_VERSION_PATCH);
    abort();
  }
  if (lib_version_patch != SIMGRID_VERSION_PATCH) {
    if (SIMGRID_VERSION_PATCH > 89 || lib_version_patch > 89) {
      fprintf(
          stderr,
          "FATAL ERROR: Your program was compiled with SimGrid version %d.%d.%d, "
          "and then linked against SimGrid %d.%d.%d. \n"
          "One of them is a development version, and should not be mixed with the stable release. Please fix this.\n",
          lib_version_major, lib_version_minor, lib_version_patch, SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR,
          SIMGRID_VERSION_PATCH);
      abort();
    }
    fprintf(stderr,
            "Warning: Your program was compiled with SimGrid version %d.%d.%d, "
            "and then linked against SimGrid %d.%d.%d. Proceeding anyway.\n",
            lib_version_major, lib_version_minor, lib_version_patch, SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR,
            SIMGRID_VERSION_PATCH);
  }
}

void sg_version_get(int* ver_major, int* ver_minor, int* ver_patch)
{
  *ver_major = SIMGRID_VERSION_MAJOR;
  *ver_minor = SIMGRID_VERSION_MINOR;
  *ver_patch = SIMGRID_VERSION_PATCH;
}

void sg_version()
{
  XBT_HELP("This program was linked against %s (git: %s), found in %s.", SIMGRID_VERSION_STRING, SIMGRID_GIT_VERSION,
           SIMGRID_INSTALL_PREFIX);

#if SIMGRID_HAVE_MC
  XBT_HELP("   Model-checking support compiled in.");
#else
  XBT_HELP("   Model-checking support disabled at compilation.");
#endif

#if SIMGRID_HAVE_NS3
  XBT_HELP("   ns-3 support compiled in.");
#else
  XBT_HELP("   ns-3 support disabled at compilation.");
#endif

#if SIMGRID_HAVE_LUA
  XBT_HELP("   Lua support compiled in.");
#else
  XBT_HELP("   Lua support disabled at compilation.");
#endif

#if SIMGRID_HAVE_MALLOCATOR
  XBT_HELP("   Mallocator support compiled in.");
#else
  XBT_HELP("   Mallocator support disabled at compilation.");
#endif

  XBT_HELP("\nTo cite SimGrid in a publication, please use:\n"
           "   Henri Casanova, Arnaud Giersch, Arnaud Legrand, Martin Quinson, Frédéric Suter. \n"
           "   Versatile, Scalable, and Accurate Simulation of Distributed Applications and Platforms. \n"
           "   Journal of Parallel and Distributed Computing, Elsevier, 2014, 74 (10), pp.2899-2917.\n"
           "The pdf file and a BibTeX entry for LaTeX users can be found at http://hal.inria.fr/hal-01017319");
}
