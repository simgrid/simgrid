/* Copyright (c) 2020-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check availability of ThreadSanitizer */

#if defined(__has_feature)
#define HAS_FEATURE(x) __has_feature(x)
#else
#define HAS_FEATURE(x) 0
#endif

#if not HAS_FEATURE(thread_sanitizer) && not defined(__SANITIZE_THREAD__)
#error "TSan feature not found."
#endif

#if defined(CHECK_FIBER_SUPPORT)
#include <sanitizer/tsan_interface.h>
// Verify the existence of the fiber annotation interface, with the expected signature
void* (*create_fiber)(unsigned)       = __tsan_create_fiber;
void (*destroy_fiber)(void*)          = __tsan_destroy_fiber;
void (*switch_fiber)(void*, unsigned) = __tsan_switch_to_fiber;
#endif

int main(void) {}
