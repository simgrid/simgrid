/* Copyright (c) 2017-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check availability of AddressSanitizer */

#if defined(__has_feature)
#define HAS_FEATURE(x) __has_feature(x)
#else
#define HAS_FEATURE(x) 0
#endif

#if not HAS_FEATURE(address_sanitizer) && not defined(__SANITIZE_ADDRESS__)
#error "ASan feature not found."
#endif

#include <sanitizer/asan_interface.h>

#if defined(CHECK_FIBER_SUPPORT)
// Verify the existence of the fiber annotation interface, with the expected signature
void (*start_fiber)(void**, const void*, size_t)   = __sanitizer_start_switch_fiber;
void (*finish_fiber)(void*, const void**, size_t*) = __sanitizer_finish_switch_fiber;
#endif

int main(void)
{
}
