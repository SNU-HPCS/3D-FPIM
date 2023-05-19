/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <map>
#include <set>
#include <vector>
#include <sys/time.h>

#include "common.h"

class Coalescer {

    private:
        ModelImpl* model_;
        Placer* placer_;

        std::vector<std::vector<MergedMVMSet*>*>& coalesceableMVMVectors_;
        std::vector<std::vector<CoalescedMergedMVMSet*>> coalescedMVMSets_;
        std::vector<std::vector<CoalescedMergedMVMSet*>> remainingCoalescedMVMSets_;
        
        // for each 
        std::vector<CoalescedMergedMVMSet*>** perMVMAvailCoalescedMVMSets_;

        void coalesceMVMOperations();
        std::set<int> findMergedMVMPredecessors(MergedMVMSet* mergedMVM, 
            std::set<MergedMVMSet*>& isVisted,
            unsigned int** mergedMVMSetOfMergedMVMs,
            unsigned int* preCount,
            unsigned int* sucCount,
            bool** preBool, 
            bool** sucBool);

        std::set<MergedMVMSet*> findNearestMergedMVMPredecessors(Operation* op, bool start
            , std::set<Operation*>& isVisited);
        void coalesceMVMPredecessors(Operation* op, 
            std::set<Operation*>& isVisited, 
            unsigned int** mergedMVMSetOfMergedMVMs,
            unsigned int* preCount,
            unsigned int* sucCount,
            bool** preBool,
            bool** sucBool);

    public:

        int mergedCount;
        Coalescer(ModelImpl* model, 
            Placer* placer, 
            std::vector<std::vector<MergedMVMSet*>*>& coalesceableMVMVectors);
        ~Coalescer();

};

