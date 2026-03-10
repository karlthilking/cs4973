/*  emit-addr-pass.cpp 
 *  Traverses instructions in arbitrary program and prints addresses
 *  for all loads and stores that are performed 
 */
#ifndef LLVM_EMIT_ADDR_PASS_H
#define LLVM_EMIT_ADDR_PASS_H

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#include <cassert>
using namespace llvm;

namespace {
class EmitGlobalAddrPass : public PassInfoMixin<EmitGlobalAddrPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
    {
        // iterate through each function in the module
        for (Function &F : M) {
            LLVMContext &C = F.getContext();
            // create printf routine to insert before loads and stores
            // in order to display addresses
            FunctionCallee printf = M.getOrInsertFunction(
                "printf",
                FunctionType::get(
                    Type::getInt32Ty(C),
                    {PointerType::get(PointerType::getUnqual(C), 0)},
                    true
                )
            );
            // iterate through each basic block in the module
            for (BasicBlock &BB : F) {
                // iterate through each instruction in the basic block
                for (Instruction &I : BB) {
                    // if a load instruction
                    if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
                        // get the memory location of the load
                        Value *ptr = LI->getPointerOperand();
                        // only print address if this is a global var
                        if (!isa<GlobalVariable>(getUnderlyingObject(ptr)))
                            continue;
                        IRBuilder<> Builder(LI);
                        Value *p = Builder.CreatePointerCast(
                            ptr, Builder.getPtrTy()
                        );
                        Value *fmt = Builder.CreateGlobalString("r%p\n");
                        // call printf with address of load
                        Builder.CreateCall(printf, {fmt, ptr});
                    } else if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
                        Value *ptr = SI->getPointerOperand();
                        if (!isa<GlobalVariable>(getUnderlyingObject(ptr)))
                            continue;
                        IRBuilder<> Builder(SI);
                        Value *p = Builder.CreatePointerCast(
                            ptr, Builder.getPtrTy()
                        );
                        Value *fmt = Builder.CreateGlobalString("w%p\n");
                        // call printf with address of store
                        Builder.CreateCall(printf, {fmt, ptr});
                    }
                }
            }
        }
        return PreservedAnalyses::none();
    }

    static bool isRequired() { return true; }
};
} // anonymous namespace

PassPluginLibraryInfo
getEmitAddrPassPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "EmitGlobalAddrPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, ModulePassManager &MPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "emit-addr-pass") {
                            MPM.addPass(EmitGlobalAddrPass());
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
