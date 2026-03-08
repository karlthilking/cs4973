/*  emit-addr-pass.cpp 
 *  Traverses instructions in arbitrary program and prints addresses
 *  for all loads and stores that are performed 
 */
#ifndef LLVM_EMIT_ADDR_PASS_H
#define LLVM_EMIT_ADDR_PASS_H

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {
class EmitAddrPass : public PassInfoMixin<EmitAddrPass> {
public:
    PreservedAnalyses
    run(Function &F, FunctionAnalysisManager &AM)
    {
        for (auto &BB : F) {        // traverse each basic block
            for (auto &I : BB) {    // traverse each instruction
                // dyn_cast'ing Instruction instance to subclass succeeds if
                // the underlying type is the subclass casted to, otherwise
                // dyn_cast will fail (and return false)
                if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
                    // get address that was loaded
                    const Value *addr = MemoryLocation::get(LI).Ptr;
                    // r for read
                    errs() << 'r' << addr << '\n';
                } else if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
                    // get address that was stored to
                    const Value *addr = MemoryLocation::get(SI).Ptr;
                    // w for write
                    errs() << 'w' << addr << '\n';
                }
            }
        }
        return PreservedAnalyses::all();
    }

    static bool
    isRequired()
    {
        return true;
    }
};
} // anonymous namespace

PassPluginLibraryInfo
getEmitAddrPassPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "EmitAddrPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "emit-addr-pass") {
                            FPM.addPass(EmitAddrPass());
                            return true;
                        }
                        return false;
                    });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return getEmitAddrPassPluginInfo();
}

#endif // LLVM_EMIT_ADDR_PASS_H
