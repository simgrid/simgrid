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
//__mcsimgrid_read4(void*whereto, void*wherefrom)
//__mcsimgrid_write4(void*whereto, void*wherefrom)

#define MCWRITE4V "__mcsimgrid_write4v"
#define MCWRITE8V "__mcsimgrid_write8v"
#define MCWRITEFV "__mcsimgrid_writefv"
#define MCWRITEDV "__mcsimgrid_writedv"

#define MCWRITE4 "__mcsimgrid_write4"
#define MCWRITE8 "__mcsimgrid_write8"
#define MCWRITEF "__mcsimgrid_writef"
#define MCWRITED "__mcsimgrid_writed"

#define MCREAD4V "__mcsimgrid_read4v"
#define MCREAD8V "__mcsimgrid_read8v"
#define MCREADFV "__mcsimgrid_readfv"
#define MCREADDV "__mcsimgrid_readdv"

#define MCREAD4 "__mcsimgrid_read4"
#define MCREAD8 "__mcsimgrid_read8"
#define MCREADF "__mcsimgrid_readf"
#define MCREADD "__mcsimgrid_readd"

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

      FunctionCallee MCwriteFunc;
      SmallVector<Value*, 2> addresses;
      //  There are two arguments to the store instruction: a value to store and an address
      //

      /* Build a function that will return a void */

      Type* retTy = Type::getVoidTy(context);
      SmallVector<Type*, 2> argsTy;

      for (Use& u : rr->operands()) {
        /* Use the operand's type */
        Type* t = u->getType();
        argsTy.push_back(t);

        // outs() << "operand " << u->getName() << " type " << *(u->getType()) << "\n";

        /* And push its value */
        addresses.push_back(u);
      }

      // Insert the new call before the actual store
      FunctionType* funcTy = FunctionType::get(retTy, argsTy, false);

      /* the first operand is the "from" field
         it is either a value or a pointer
         the second operand is the "to" field
         it is a pointer
         therefore, the type of the first operand determines the function to insert */

      if (rr->getNumOperands() != 2) {
        errs() << "Wrong number of operands -- skipping\n";
        continue;
      }

      Type* t = rr->getOperand(0)->getType();
      // Value types

      // outs() << "type   " << *t << "\n";

      if (t->isIntegerTy()) {
        // outs() << "   is integer\n";
        if (t->isIntegerTy(32)) {
          MCwriteFunc = module->getOrInsertFunction(MCWRITE4V, funcTy);
        }
        if (t->isIntegerTy(64)) {
          MCwriteFunc = module->getOrInsertFunction(MCWRITE8V, funcTy);
        }
        if (not(t->isIntegerTy(32) || t->isIntegerTy(64))) {
          errs() << "Unknown integer type: i" << t->getIntegerBitWidth() << " -- skipping\n";
          continue;
        }
      }

      if (t->isFloatTy()) {
        MCwriteFunc = module->getOrInsertFunction(MCWRITEFV, funcTy);
      }
      if (t->isDoubleTy()) {
        MCwriteFunc = module->getOrInsertFunction(MCWRITEDV, funcTy);
      }

      // Pointer types

      if (t->isPointerTy()) {
        //	  outs() << "oh ca va bien là les pointeurs\n";
        MCwriteFunc = module->getOrInsertFunction(MCWRITE4, funcTy);

        // errs() << "   well, a pointer -- skipping\n";
        // continue;
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

      if (MCwriteFunc)
        builder.CreateCall(MCwriteFunc, addresses);
    }

    /* Reads */

    for (auto& rr : reads) {
      IRBuilder<> builder(rr);
      auto& context = rr->getContext();
      auto module   = rr->getModule();

      FunctionCallee MCreadFunc;
      SmallVector<Value*, 1> addresses;

      /* Build a function that will return a void */
      Type* retTy = Type::getVoidTy(context);
      SmallVector<Type*, 1> argsTy;

      // outs() << "Instrument " << rr->getOpcodeName() << "\n";
      // outs() << "      Source code: " << *rr << "\n";

      for (Use& u : rr->operands()) {
        /* Use the operand's type */
        argsTy.push_back(u->getType());

        /* And push its value */
        addresses.push_back(u);
      }

      // Insert the new call before the actual store
      FunctionType* funcTy = FunctionType::get(retTy, argsTy, false);

      /* Only one type is interesting
       */
      if (rr->getNumOperands() != 1) {
        errs() << "Wrong number of operands -- skipping\n";
        continue;
      }

      // outs() << "Nb of operands: " << rr->getNumOperands() << "\n";

      Type* t = rr->getType();

      // Value types
      // Load always takes a pointer

      if (t->isIntegerTy()) {
        // outs() << "   is integer\n";
        if (t->isIntegerTy(32)) {
          MCreadFunc = module->getOrInsertFunction(MCREAD4V, funcTy);
        }
        if (t->isIntegerTy(64)) {
          MCreadFunc = module->getOrInsertFunction(MCREAD8V, funcTy);
        }
        if (not(t->isIntegerTy(32) || t->isIntegerTy(64))) {
          errs() << "Unknown integer type: i" << t->getIntegerBitWidth() << " -- skipping\n";
          continue;
        }
      }

      if (t->isFloatTy()) {
        MCreadFunc = module->getOrInsertFunction(MCREADFV, funcTy);
      }
      if (t->isDoubleTy()) {
        MCreadFunc = module->getOrInsertFunction(MCREADDV, funcTy);
      }

      // Pointer types

      if (t->isPointerTy()) {
        //	  outs() << "oh ca va bien là les pointeurs\n";
        MCreadFunc = module->getOrInsertFunction(MCREAD4, funcTy);

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
      if (MCreadFunc)
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
