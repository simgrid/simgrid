/* amok modinter - interface to AMOK modules initialization and such        */

/* Copyright (c) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_BASE_H
#define AMOK_BASE_H

void amok_init(void);
void amok_exit(void);

/* module creation functions */
void amok_pm_modulecreate(void);

#endif /* AMOK_BASE_H */
