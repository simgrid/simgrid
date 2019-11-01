/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_MAILBOX_H_
#define INCLUDE_SIMGRID_MAILBOX_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

/* C interface */
SG_BEGIN_DECL

XBT_PUBLIC void sg_mailbox_set_receiver(const char* alias);
XBT_PUBLIC int sg_mailbox_listen(const char* alias);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_MAILBOX_H_ */
