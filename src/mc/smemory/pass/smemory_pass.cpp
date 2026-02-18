/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "llvm/IR/IRBuilder.h"
#pragma clang diagnostic pop

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <llvm/IR/Instructions.h>
#include <vector>

using namespace llvm;

namespace {

struct SharedMemPass : public PassInfoMixin<SharedMemPass> {

  // For now, Mc SimGrid won't support atomic operations. So we will warn the user if the code contains any
  bool contains_atomic_operation = false;

  /* Decide which instructions are goind to be modified.
     Fills two vectors, reads and writes, with pointers to the said instructions.
  */
  void exploreIR(Module& M, std::vector<Instruction*>& reads, std::vector<Instruction*>& writes)
  {
    for (Function& F : M) {
      for (BasicBlock& BB : F) {
        for (Instruction& instr : BB) {
          // https://llvm.org/doxygen/classllvm_1_1Instruction.html

          // outs() << "Instruction: " << instr.getOpcodeName() << "\n";

          if (isa<LoadInst>(instr)) {
            // outs() << " is a load\n";
            reads.push_back(&instr);
            // for( Use &u : instr.operands() ) { outs() << "  " << u << "\n"; }
            //  FIXME: we only have the 'from' fields about loads, so we need to find it along with the return
          }
          if (isa<StoreInst>(instr)) {
            // outs() << " is a store\n";
            writes.push_back(&instr);
            // for( Use &u : instr.operands() ) {outs() << "  " << u << "\n"; }
          }

          if (isa<AtomicRMWInst>(instr)) {
            contains_atomic_operation = true;
          }

          // mayReadFromMemory ()
          // mayReadOrWriteMemory ()
          // isAtomic ()
          // getAccessType ()
        }
      }
    }
  }

  /* Replace load instructions with a call to mcread and write instructions with a call
     to mcwrite. Returns true if the code has been modified.
  */
  bool instrument(std::vector<Instruction*>& reads, std::vector<Instruction*>& writes)
  {

    if (reads.size() == 0 && writes.size() == 0)
      return false;

    /* Writes */

    for (auto& rr : writes) {
      IRBuilder<> builder(rr);
      auto& context = rr->getContext();
      auto module   = rr->getModule();

      // outs() << "Instrument " << rr->getOpcodeName() << "\n";
      // outs() << "      Source code: " << *rr << "\n";

      // Write instruction have two arguments: a value to store and an address
      //   The first, value, is the "from" field it is either a value or a pointer
      //   The second, pointer, is the "to" field
      // We keep only the address, and add the size (in bits)
      // TODO: do we need to mark the first one as a read when it's a pointer?
      if (rr->getNumOperands() != 2) {
        errs() << "Wrong number of operands (expected 2 got " << rr->getNumOperands() << ") -- skipping\n";
        continue;
      }

      // Build the call we need to insert before the actual store
      Type* retTy = Type::getVoidTy(context);
      SmallVector<Type*, 2> argsTy{rr->getOperand(1)->getType(), Type::getInt8Ty(context)};
      FunctionType* funcTy       = FunctionType::get(retTy, argsTy, false);
      FunctionCallee MCwriteFunc = module->getOrInsertFunction("__mcsimgrid_write", funcTy);
      ;

      // Compute the operands of the inserted call
      SmallVector<Value*, 2> addresses; // The rewritten operands
      addresses.push_back(rr->getOperand(1));
      unsigned char touchedSize = 0; // wanna be second operand, once we know its value

      Type* t = rr->getOperand(0)->getType();

      // outs() << "type   " << *t << "\n";

      // Value types
      if (t->isIntegerTy()) {
        touchedSize = t->getIntegerBitWidth();
      }
      if (t->isFloatTy()) {
        touchedSize = sizeof(float) * 8;
      }
      if (t->isDoubleTy()) {
        touchedSize = sizeof(double) * 8;
      }

      // Pointer types

      if (t->isPointerTy()) {
        //	  outs() << "oh ca va bien là les pointeurs\n";
        touchedSize = sizeof(void*) * 8;

        // Type* pt = llvm::dyn_cast<llvm::PointerType>(t)->getContainedType(0);
        // outs() << "Pointed type   " << *pt << "\n";

        //   if( pt->isIntegerTy() ) {
        //       //                            outs() << "   is integer ptr\n";
        //       if( pt->isIntegerTy( 32 ) ) {
        //           outs() << "   is integer 32 ptr \n";
        //           MCwriteFunc = module->getOrInsertFunction( MCWRITE4, funcTy );
        //       }
        //       if( pt->isIntegerTy( 64 ) ) {
        //           outs() << "   is integer 64 ptr \n";
        //           MCwriteFunc = module->getOrInsertFunction( MCWRITE8, funcTy );
        //       }
        //       if( not ( pt->isIntegerTy( 32 ) || pt->isIntegerTy( 64 ) ) ) {
        //           errs() << "Unknown integer ptr type: p" << pt->getIntegerBitWidth() <<  " -- skipping\n";
        //       }
        //   }

        //   if( pt->isFloatTy() ){
        //       outs() << "   is float ptr\n";
        //       MCwriteFunc = module->getOrInsertFunction( MCWRITEF, funcTy );
        //   }
        //   if( pt->isDoubleTy() ){
        //       outs() << "   is double ptr\n";
        //       MCwriteFunc = module->getOrInsertFunction( MCWRITED, funcTy );
        //   }
        //   if( pt->isPointerTy() ){
        //       outs() << "oh ca va bien là les pointeurs\n";
        //       MCwriteFunc = module->getOrInsertFunction( MCWRITE4, funcTy );
        //   }
      }

      addresses.push_back(ConstantInt::get(Type::getInt8Ty(context), touchedSize));
      if (addresses.size() != 2) {
        fprintf(stderr, "Got %lu params instead of 2\n", addresses.size());
        abort();
      }
      if (touchedSize > 0)
        builder.CreateCall(MCwriteFunc, addresses);
    }
    // writes.front()->getModule()->dump();

    /* Reads */

    for (auto& rr : reads) {
      IRBuilder<> builder(rr);
      auto& context = rr->getContext();
      auto module   = rr->getModule();

      // outs() << "Instrument " << rr->getOpcodeName() << "\n";
      // outs() << "      Source code: " << *rr << "\n";

      // Load instruction have only one argument: an address from where the read is made
      if (rr->getNumOperands() != 1) {
        errs() << "Wrong number of operands -- skipping\n";
        continue;
      }

      // Build the call we need to insert before the actual read
      Type* retTy = Type::getVoidTy(context);
      SmallVector<Type*, 2> argsTy{rr->getOperand(0)->getType(), Type::getInt8Ty(context)};
      FunctionType* funcTy      = FunctionType::get(retTy, argsTy, false);
      FunctionCallee MCreadFunc = module->getOrInsertFunction("__mcsimgrid_read", funcTy);

      // Compute the operands of the inserted call
      SmallVector<Value*, 2> addresses; // The rewritten operands
      addresses.push_back(rr->getOperand(0));
      unsigned char touchedSize = 0; // wanna be second operand, once we know its value

      Type* t = rr->getType();

      // Value types
      // Load always takes a pointer
      if (t->isIntegerTy()) {
        touchedSize = t->getIntegerBitWidth();
      }
      if (t->isFloatTy()) {
        touchedSize = sizeof(float) * 8;
      }
      if (t->isDoubleTy()) {
        touchedSize = sizeof(double) * 8;
      }

      // Pointer types

      if (t->isPointerTy()) {
        //	  outs() << "oh ca va bien là les pointeurs\n";
        touchedSize = sizeof(void*) * 8;

        // Type* pt = llvm::dyn_cast<llvm::PointerType>(t)->getContainedType(0);
        //   outs() << "Pointed type   " << *pt << "\n";

        //   if( pt->isIntegerTy() ) {
        //       //                            outs() << "   is integer ptr\n";
        //       if( pt->isIntegerTy( 32 ) ) {
        //           outs() << "   is integer 32 ptr \n";
        //           MCreadFunc = module->getOrInsertFunction( MCREAD4, funcTy );
        //       }
        //       if( pt->isIntegerTy( 64 ) ) {
        //           outs() << "   is integer 64 ptr \n";
        //           MCreadFunc = module->getOrInsertFunction( MCREAD8, funcTy );
        //       }
        //       if( not ( pt->isIntegerTy( 32 ) || pt->isIntegerTy( 64 ) ) ) {
        //           errs() << "Unknown integer ptr type: " << pt->getIntegerBitWidth() <<  " -- skipping\n";
        //       }
        //   }

        //   if( pt->isFloatTy() ){
        //       outs() << "   is float ptr\n";
        //       MCreadFunc = module->getOrInsertFunction( MCREADF, funcTy );
        //   }
        //   if( pt->isDoubleTy() ){
        //       outs() << "   is double ptr\n";
        //       MCreadFunc = module->getOrInsertFunction( MCREADD, funcTy );
        //   }
        //   if( pt->isPointerTy() ){
        //       outs() << "oh ca va bien là les pointeurs\n";
        //       MCreadFunc = module->getOrInsertFunction( MCREAD4, funcTy );
        //   }
      }
      // outs() << "***Creating a new read call***\n";
      addresses.push_back(ConstantInt::get(Type::getInt8Ty(context), touchedSize));
      if (addresses.size() != 2) {
        fprintf(stderr, "Got %lu params instead of 2\n", addresses.size());
        abort();
      }
      if (touchedSize > 0)
        builder.CreateCall(MCreadFunc, addresses);
    }
    // reads.front()->getModule()->dump();
    return true;
  }

  /* Run the pass */
  PreservedAnalyses run(Module& M, ModuleAnalysisManager& MPM)
  {

    std::vector<Instruction*> reads;
    std::vector<Instruction*> writes;
    bool modified;

    exploreIR(M, reads, writes);
    modified = instrument(reads, writes);

    if (contains_atomic_operation)
      errs() << "WARNING: your code contains atomic operations which are not yet supported by Mc SimGrid."
             << " The model checking phase may miss some bugs.\n";

    outs() << "Successfully instrumented " << reads.size() << " read operations and " << writes.size()
           << " writes operations\n";

    if (modified) {
      return PreservedAnalyses::none();
    } else {
      return PreservedAnalyses::all();
    }
  }
};

} // namespace

PassPluginLibraryInfo getPassPluginInfo()
{
  const auto callback = [](PassBuilder& PB) {
    PB.registerPipelineStartEPCallback([&](ModulePassManager& MPM, auto) {
      MPM.addPass(SharedMemPass());
      return true;
    });
  };

  return {LLVM_PLUGIN_API_VERSION, "SharedMem", "0.0.1", callback};
};

/* When a plugin is loaded by the driver, it will call this entry point to
   obtain information about this plugin and about how to register its passes.
*/
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo()
{
  return getPassPluginInfo();
}

// https://sh4dy.com/2024/06/29/learning_llvm_01/

/*
  clang -shared -o sharedmem.so sharedmem.cpp -fPIC
  clang -O1 -fpass-plugin=./sharedmem.so test.c -o test

*/
