/* xbt.h - Public interface to the xbt (SimGrid's toolbox)                  */

/* Copyright (c) 2004-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_BASE_H
#define XBT_BASE_H

/* Define _GNU_SOURCE for getline, isfinite, etc. */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#define XBT_ATTRIB_PRINTF(format_idx, arg_idx) __attribute__((__format__(__printf__, (format_idx), (arg_idx))))
#define XBT_ATTRIB_SCANF(format_idx, arg_idx) __attribute__((__format__(__scanf__, (format_idx), (arg_idx))))

#if defined(__cplusplus)
#if __cplusplus >= 201103L
#define XBT_ATTRIB_NORETURN [[noreturn]]
#endif
#if __cplusplus >= 201703L
#define XBT_ATTRIB_UNUSED [[maybe_unused]]
#endif
#if __cplusplus >= 201402L
#define XBT_ATTRIB_DEPRECATED(mesg) [[deprecated(mesg)]]
#endif
#elif defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 201112L
#define XBT_ATTRIB_NORETURN _Noreturn
#endif
#endif

#ifndef XBT_ATTRIB_NORETURN
#define XBT_ATTRIB_NORETURN __attribute__((noreturn))
#endif
#ifndef XBT_ATTRIB_UNUSED
#define XBT_ATTRIB_UNUSED  __attribute__((unused))
#endif
#ifndef XBT_ATTRIB_DEPRECATED
#define XBT_ATTRIB_DEPRECATED(mesg) __attribute__((deprecated(mesg)))
#endif

#define XBT_ATTRIB_DEPRECATED_v402(mesg)                                                                               \
  XBT_ATTRIB_DEPRECATED(mesg " (this compatibility wrapper will be dropped after v4.1)")
#define XBT_ATTRIB_DEPRECATED_v403(mesg)                                                                               \
  XBT_ATTRIB_DEPRECATED(mesg " (this compatibility wrapper will be dropped after v4.2)")
#define XBT_ATTRIB_DEPRECATED_v404(mesg)                                                                               \
  XBT_ATTRIB_DEPRECATED(mesg " (this compatibility wrapper will be dropped after v4.3)")

/* Work around https://github.com/microsoft/vscode-cpptools/issues/4503 */
#ifdef __INTELLISENSE__
#pragma diag_suppress 1094
#endif

#if !defined(__APPLE__)
#  define XBT_ATTRIB_CONSTRUCTOR(prio) __attribute__((__constructor__(prio)))
#  define XBT_ATTRIB_DESTRUCTOR(prio) __attribute__((__destructor__(prio)))
#else
#  define XBT_ATTRIB_CONSTRUCTOR(prio) __attribute__((__constructor__))
#  define XBT_ATTRIB_DESTRUCTOR(prio) __attribute__((__destructor__))
#endif

#define XBT_ATTRIB_NOINLINE __attribute__((noinline))

#if defined(__GNUC__)
#define XBT_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define XBT_ALWAYS_INLINE inline
#endif

/* Stringify argument. */
#define _XBT_STRINGIFY(a) #a

/* Concatenate arguments. _XBT_CONCAT2 adds a level of indirection over _XBT_CONCAT. */
#define _XBT_CONCAT(a, b) a##b
#define _XBT_CONCAT2(a, b) _XBT_CONCAT(a, b)
#define _XBT_CONCAT3(a, b, c) _XBT_CONCAT2(_XBT_CONCAT2(a, b), c)
#define _XBT_CONCAT4(a, b, c, d) _XBT_CONCAT2(_XBT_CONCAT3(a, b, c), d)

/*
 * Expands to `one' if there is only one argument for the variadic part.
 * Otherwise, expands to `more'.
 * Works with up to 63 arguments, which is the maximum mandated by the C99 standard.
 */
#define _XBT_IF_ONE_ARG(one, more, ...)                                 \
    _XBT_IF_ONE_ARG_(__VA_ARGS__,                                       \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, one)
#define _XBT_IF_ONE_ARG_(a64, a63, a62, a61, a60, a59, a58, a57,        \
                         a56, a55, a54, a53, a52, a51, a50, a49,        \
                         a48, a47, a46, a45, a44, a43, a42, a41,        \
                         a40, a39, a38, a37, a36, a35, a34, a33,        \
                         a32, a31, a30, a29, a28, a27, a26, a25,        \
                         a24, a23, a22, a21, a20, a19, a18, a17,        \
                         a16, a15, a14, a13, a12, a11, a10, a9,         \
                         a8, a7, a6, a5, a4, a3, a2, a1, N, ...) N

/* Expands to number of arguments. */
#define _XBT_COUNT_ARGS(...) _XBT_IF_ONE_ARG_(__VA_ARGS__,              \
                                              64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, \
                                              48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, \
                                              32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, \
                                              16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

/* Expands to list with each argument rendered as string. Add more cases if needed. */
#define _XBT_STRINGIFY_ARGS(...) _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_02_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_02_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_03_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_03_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_04_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_04_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_05_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_05_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_06_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_06_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_07_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_07_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_08_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_08_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_09_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_09_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_10_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_10_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_11_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_11_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_12_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_12_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_13_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_13_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_14_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_14_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_15_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_15_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_16_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_16_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_17_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_17_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_18_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_18_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_19_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_19_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_20_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_20_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_21_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_21_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_22_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_22_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_23_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_23_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_24_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_24_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_25_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_25_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_26_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_26_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_27_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_27_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_28_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_28_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_29_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_29_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_30_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_30_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_31_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_31_(a, ...) #a, _XBT_IF_ONE_ARG(_XBT_STRINGIFY, _XBT_STRINGIFY_A_32_, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_STRINGIFY_A_32_(...) error_maximum_size_of_XBT_STRINGIFY_ARGS_reached

/* Rationale of XBT_PUBLIC:
 *   * This is for library symbols visible from the application-land.
 *     Basically, any symbols defined in the include/directory must be like this (plus some other globals).
 *
 *     Just think of it as a special way to say "extern".
 */

#if defined(__ELF__)
#define XBT_PUBLIC __attribute__((visibility("default")))
#  define XBT_PUBLIC_DATA       extern __attribute__((visibility("default")))
#  define XBT_PRIVATE           __attribute__((visibility("hidden")))

#else
#define XBT_PUBLIC /* public */
#  define XBT_PUBLIC_DATA       extern
#  define XBT_PRIVATE           /** @private */

#endif

/* C++ users need love */
#ifndef SG_BEGIN_DECL
# ifdef __cplusplus
#  define SG_BEGIN_DECL extern "C" {
# else
#  define SG_BEGIN_DECL
# endif
#endif

#ifndef SG_END_DECL
# ifdef __cplusplus
#  define SG_END_DECL }
# else
#  define SG_END_DECL
# endif
#endif
/* End of cruft for C++ */

#endif
