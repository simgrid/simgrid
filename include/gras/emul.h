/* $Id$                     */

/* gras/emul.h - public interface to emulation support                      */
/*                (specific parts for SG or RL)                             */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_COND_H
#define GRAS_COND_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */

SG_BEGIN_DECL()
/** @addtogroup GRAS_emul
 *  @brief Code execution "emulation" and "virtualization".
 * 
 *  Emulation and virtualization words have a lot of different meanings in
 *  computer science. Here is what we mean, and what this module allows you
 *  to do (if it does not match your personal belives, I'm sorry):
 * 
 *  - Virtualization: Having some specific code for the simulation or for the reality
 *  - Emulation: Report within the simulator the execution time of your code
 * 
 *  \section GRAS_emul_virtualization Virtualization 
 * 
 *  The whole idea of GRAS is to share the same code between the simulator
 *  and the real implementation. But it is sometimes impossible, such as
 *  when you want to deal with the OS. As an example, you may want to add
 *  some extra delay before initiating a communication in RL to ensure that
 *  the receiver is listening. This is usually useless in SG since you have
 *  a much better control on process launch time.
 * 
 *  This would be done with the following snipet:
 *  \verbatim if (gras_if_RL()) 
   gras_os_sleep(1);\endverbatim
 * 
 *  Please note that those are real functions and not pre-processor
 *  defines. This is to ensure that the same object code can be linked
 *  against the SG library or the RL one without recompilation.
 * 
 *  @{
 */
/** \brief Returns true only if the program runs on real life */
XBT_PUBLIC(int) gras_if_RL(void);

/** \brief Returns true only if the program runs within the simulator */
XBT_PUBLIC(int) gras_if_SG(void);

/** @} */

XBT_PUBLIC(int) gras_bench_always_begin(const char *location, int line);
XBT_PUBLIC(int) gras_bench_always_end(void);
XBT_PUBLIC(int) gras_bench_once_begin(const char *location, int line);
XBT_PUBLIC(int) gras_bench_once_end(void);

/** @addtogroup GRAS_emul
 *  \section GRAS_emul_timing Emulation
 *  
 *  For simulation accuracy, it is mandatory to report the execution time
 *  of your code into the simulator. For example, if your application is a
 *  parallel matrix multiplication, you naturally have to slow down the
 *  simulated hosts actually doing the computation.
 *  
 *  If you know beforehands how long each task will last, simply add a call
 *  to the gras_bench_fixed function described below. If not, you can have
 *  GRAS benchmarking your code automatically. Simply enclose the code to
 *  time between a macro GRAS_BENCH_*_BEGIN and GRAS_BENCH_*_END, and
 *  you're done. There is three pair of such macros, whose characteristics
 *  are summarized in the following table. 
 * 
 *  <table>
 *   <tr>
 *    <td><b>Name</b></td> 
 *    <td><b>Run on host machine?</b></td>
 *    <td><b>Benchmarked?</b></td>
 *    <td><b>Corresponding time reported to simulation?</b></td>
 *   </tr> 
 *   <tr>
 *    <td>GRAS_BENCH_ALWAYS_BEGIN()<br> 
 *        GRAS_BENCH_ALWAYS_END()</td> 
 *    <td>Each time</td>
 *    <td>Each time</td>
 *    <td>Each time</td>
 *   </tr>
 *   <tr>
 *    <td>GRAS_BENCH_ONCE_RUN_ONCE_BEGIN()<br> 
 *        GRAS_BENCH_ONCE_RUN_ONCE_END()</td>
 *    <td>Only first time</td>
 *    <td>Only first time</td>
 *    <td>Each time (with stored value)</td>
 *   </tr>
 *   <tr>
 *    <td>GRAS_BENCH_ONCE_RUN_ALWAYS_BEGIN()<br> 
 *        GRAS_BENCH_ONCE_RUN_ALWAYS_END()</td>
 *    <td>Each time</td>
 *    <td>Only first time</td>
 *    <td>Each time (with stored value)</td>
 *   </tr>
 *  </table>
 *  
 *  As you can see, whatever macro pair you use, the corresponding value is
 *  repported to the simulator. After all, that's what those macro are
 *  about ;)
 * 
 *  The GRAS_BENCH_ALWAYS_* macros are the simplest ones. Each time the
 *  corresponding block is encountered, the corresponding code is executed
 *  and timed. Then, the simulated host is given the corresponding amount
 *  of work.
 * 
 *  The GRAS_BENCH_ONCE_RUN_ONCE_* macros are good for cases where you know
 *  that your execution time is constant and where you don't care about the
 *  result in simulation mode. In our example, each sub-block
 *  multiplication takes exactly the same amount of work (time depends only
 *  on size, not on content), and the operation result can safely be
 *  ignored for algorithm result. Doing so allows you to considerably
 *  reduce the amount of computation needed when running on simulator.
 * 
 *  The GRAS_BENCH_ONCE_RUN_ALWAYS_* macros are good for cases where you
 *  know that each block will induce the same amount of work (you thus
 *  don't want to bench it each time), but you actually need the result (so
 *  you have to run it each time). You may ask why you don't use
 *  GRAS_BENCH_ONCE_RUN_ONCE_* macros in this case (why you save the
 *  benchmarking time).  The timing operation is not very intrusive by
 *  itself, but it has to be done in an exclusive way between the several
 *  GRAS threads (protected by mutex). So, the day where there will be
 *  threads in GRAS, this will do a big difference. Ok, I agree. For now,
 *  it makes no difference.
 * 
 *  <b>Caveats</b>
 * 
 *   - Blocks are automatically differenciated using the filename and line
 *     position at which the *_BEGIN part was called. Don't put two of them
 *     on the same line.
 * 
 *   - You cannot nest blocks. It would make no sense, either.
 * 
 *   - By the way, GRAS is not exactly designed for parallel algorithm such
 *     as parallel matrix multiplication but for distributed ones, you weirdo.
 *     But it's just an example ;)
 *  
 * @{
 */
/** \brief Start benchmarking this code block
    \hideinitializer */
#define GRAS_BENCH_ALWAYS_BEGIN()           gras_bench_always_begin(__FILE__, __LINE__)
/** \brief Stop benchmarking this code block
    \hideinitializer */
#define GRAS_BENCH_ALWAYS_END()             gras_bench_always_end()

/** \brief Start benchmarking this code block if it has never been benchmarked, run it in any case
 *  \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ALWAYS_BEGIN()  gras_bench_once_begin(__FILE__, __LINE__)
/** \brief Stop benchmarking this part of the code
    \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ALWAYS_END()    gras_bench_once_end()

/** \brief Start benchmarking this code block if it has never been benchmarked, ignore it if it was
    \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ONCE_BEGIN()    if (gras_bench_once_begin(__FILE__, __LINE__)) {
/** \brief Stop benchmarking this part of the code
    \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ONCE_END()      } gras_bench_once_end()

XBT_PUBLIC(void) gras_cpu_burn(double flops);
/** @} */

SG_END_DECL()
#endif /* GRAS_COND_H */
