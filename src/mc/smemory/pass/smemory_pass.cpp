/* Copyright (c) 2025-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* LLVM pass reporting the memory accesses of the application to the smemory runtime, for the data race detection.
 *
 * Before each memory access that may race, the pass inserts a call to __mcsimgrid_read(address, size) or
 * __mcsimgrid_write(address, size), where size is in bytes. It follows ThreadSanitizer:
 *  - The pass runs at the end of the optimization pipeline (also at -O0), so the local variables that the optimizations
 *    keep in registers are not instrumented.
 *  - The local variables whose address does not escape their function cannot be reached by another thread, so their
 *    accesses are not instrumented either (see instrument_all_locals to change that).
 *  - The loads from constant globals cannot race, since nobody can write there.
 *  - The memcpy, memmove and memset intrinsics are reported as ranges, which covers the copies and initializations of
 *    structures.
 *  - The atomic operations are not instrumented. The application is warned that the races they may hide or create are
 *    not detected.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "llvm/IR/IRBuilder.h"
#pragma clang diagnostic pop

#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"
#if LLVM_VERSION_MAJOR >= 22
#include "llvm/Plugins/PassPlugin.h"
#else
#include "llvm/Passes/PassPlugin.h"
#endif

using namespace llvm;

namespace {

// Set to true to also instrument the local variables whose address does not escape, e.g. to measure their cost
constexpr bool instrument_all_locals = false;

struct SharedMemPass : public PassInfoMixin<SharedMemPass> {
  // Whether the access to this address may race with another thread
  static bool may_race(Value* address, bool is_read)
  {
    if (address->getType()->getPointerAddressSpace() != 0) // Other address spaces are for GPUs and alike
      return false;

    const Value* object = getUnderlyingObject(address);
    if (auto const* global = dyn_cast<GlobalVariable>(object); is_read && global != nullptr && global->isConstant())
      return false;
    if (not instrument_all_locals && isa<AllocaInst>(object))
#if LLVM_VERSION_MAJOR >= 20
      return PointerMayBeCaptured(object, /*ReturnCaptures=*/true);
#else
      return PointerMayBeCaptured(object, /*ReturnCaptures=*/true, /*StoreCaptures=*/true);
#endif
    return true;
  }

  PreservedAnalyses run(Module& M, ModuleAnalysisManager&)
  {
    LLVMContext& context     = M.getContext();
    const DataLayout& layout = M.getDataLayout();
    Type* size_type          = Type::getInt64Ty(context);
    FunctionType* report_type =
        FunctionType::get(Type::getVoidTy(context), {PointerType::get(context, 0), size_type}, false);
    FunctionCallee report_read  = M.getOrInsertFunction("__mcsimgrid_read", report_type);
    FunctionCallee report_write = M.getOrInsertFunction("__mcsimgrid_write", report_type);

    unsigned read_count  = 0;
    unsigned write_count = 0;
    bool has_atomics     = false;

    for (Function& F : M) {
      if (F.isDeclaration())
        continue;
      // Collect the accesses first, as the instrumentation adds instructions
      std::vector<Instruction*> accesses;
      for (Instruction& I : instructions(F)) {
        if (I.isAtomic() || isa<FenceInst>(I)) // Atomic loads and stores, atomicrmw, cmpxchg and fences
          has_atomics = true;
        else if (isa<LoadInst, StoreInst, MemIntrinsic>(I))
          accesses.push_back(&I);
      }

      for (Instruction* I : accesses) {
        IRBuilder<> builder(I);
        auto report = [&](FunctionCallee callee, Value* address, Value* size, bool is_read) {
          if (may_race(address, is_read)) {
            builder.CreateCall(callee, {address, builder.CreateZExtOrTrunc(size, size_type)});
            (is_read ? read_count : write_count)++;
          }
        };

        if (auto* transfer = dyn_cast<MemTransferInst>(I)) { // memcpy and memmove
          report(report_read, transfer->getSource(), transfer->getLength(), true);
          report(report_write, transfer->getDest(), transfer->getLength(), false);
        } else if (auto* set = dyn_cast<MemSetInst>(I)) {
          report(report_write, set->getDest(), set->getLength(), false);
        } else if (isa<LoadInst, StoreInst>(I)) {
          TypeSize size = layout.getTypeStoreSize(getLoadStoreType(I));
          if (size.isScalable()) // The size of the vector is only known at run time, on some architectures
            continue;
          report(isa<LoadInst>(I) ? report_read : report_write, getLoadStorePointerOperand(I),
                 builder.getInt64(size.getFixedValue()), isa<LoadInst>(I));
        }
      }
    }

    if (has_atomics)
      errs() << "WARNING: your code contains atomic operations, which are not supported by Mc SimGrid yet. The races "
                "on the data that they protect may be reported wrongly, and the races on the atomic operations "
                "themselves are not detected.\n";
    outs() << "Successfully instrumented " << read_count << " read operations and " << write_count
           << " writes operations\n";

    return read_count + write_count > 0 ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

} // namespace

/* When a plugin is loaded by the driver, it calls this entry point to get the passes of the plugin */
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo()
{
  return {LLVM_PLUGIN_API_VERSION, "SharedMem", "0.0.2", [](PassBuilder& PB) {
            // Instrument after the optimizations, as ThreadSanitizer does
            PB.registerOptimizerLastEPCallback([](ModulePassManager& MPM, auto...) { MPM.addPass(SharedMemPass()); });
          }};
}
