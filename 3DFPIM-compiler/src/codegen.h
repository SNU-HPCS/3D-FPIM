/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include "common.h"

class CodeGenerator {

    private:

        ModelImpl* model_;
        Placer* placer_;
        MemoryAllocator* memoryAllocator_;
        Coalescer* coalescer_;
        Linearizer* linearizer_;
        RegisterAllocator* registerAllocator_;

        void codegen();
        std::string codegen(CoalescedMVMSet* coalescedMVMSet);
        std::string codegen(MergedMVMSet* mergedMVM);
        std::string codegen(MVMOperation* mvm);
        std::string codegen(ALUVectorOperation* aluOp);
        std::string codegen(SetImmediateOperation* seti);
        std::string codegen(CopyOperation* copy);
        std::string codegen(LoadOperation* load);
        std::string codegen(StoreOperation* store);
        std::string codegen(MVMGuardOperation* load);
        std::string codegen(SendOperation* send);
        std::string codegen(ReceiveOperation* recv);
        std::string codegen(WriteInputOperation* write);
        std::string codegen(ReadOutputOperation* read);

    public:

        CodeGenerator(ModelImpl* model, Placer* placer, MemoryAllocator* memoryAllocator, Coalescer* coalescer, Linearizer* linearizer, RegisterAllocator* registerAllocator);

};

