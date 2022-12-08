/*
 *  Copyright (c) 2019 IMPACT Research Group, University of Illinois.
 *  All rights reserved.
 *
 *  This file is covered by the LICENSE.txt license file in the root directory.
 *
 */

#include <assert.h>

#include "3dfpim.h"

#include "coalescer.h"
#include "model.h"
#include "operations.h"
#include "placer.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include <omp.h>

Coalescer::Coalescer
    (ModelImpl* model, Placer* placer, 
    //std::vector<std::set<MergedMVMSet*>*>& coalesceableMVMSets)
    std::vector<std::vector<MergedMVMSet*>*>& coalesceableMVMVectors)
    //: model_(model), placer_(placer), coalesceableMVMSets_(coalesceableMVMSets)
    : model_(model), placer_(placer), coalesceableMVMVectors_(coalesceableMVMVectors)
{
    if(model_->getModelType() == ModelImpl::INFERENCE) {
        coalesceMVMOperations();
    }

    for(auto it = model_->op_begin(); it != model_->op_end(); ++it) {
        Operation* op = *it;
        if(MergedMVMSet* mergedMVM = dynamic_cast<MergedMVMSet*>(op)){
            assert(mergedMVM->getCoalescedSet() != NULL);
        }
    }
}

Coalescer::~Coalescer() {
    for(auto coreCoalescedSets : coalescedMVMSets_) {
        for(auto coalescedSet : coreCoalescedSets) {
            delete coalescedSet;
        }
    }
}

void Coalescer::coalesceMVMOperations() {

    // Strategy
    // 1) coalesce MVM operations for a single larger MVM!
    // 2) traverse in the reverse post order =>
    //      then find the eligible candidate w/ no dependency

    coalescedMVMSets_.resize(placer_->getNPCores());
    remainingCoalescedMVMSets_.resize(placer_->getNPCores());

    for(auto coalesceableMVMVector : coalesceableMVMVectors_) {
        // Extract coalesced set for each relevant core
        // indexed by [tile id, core id]
        std::map<unsigned int, std::map<unsigned int, CoalescedMergedMVMSet*>> 
            localCoalescedMergedMVMSets;

        for(auto mergedMVM : *coalesceableMVMVector) {
            unsigned int pMVMU = placer_->getPMVMU(*(mergedMVM->begin()));
            unsigned int pCore = placer_->getPCore(*(mergedMVM->begin()));
            unsigned int pTile = placer_->getPTile(*(mergedMVM->begin()));

            if(!localCoalescedMergedMVMSets[pTile].count(pCore)) {
                localCoalescedMergedMVMSets[pTile][pCore] = new CoalescedMergedMVMSet();
            }
            localCoalescedMergedMVMSets[pTile][pCore]->add(mergedMVM, pMVMU);
        }
        // Add extracted sets to full list
        for(auto it1 : localCoalescedMergedMVMSets) {
            unsigned int pTile = it1.first;
            for(auto it2 : it1.second) {
                unsigned int pCore = it2.first;
                CoalescedMergedMVMSet* coalescedSet = it2.second;

                // 1) if all the MVMs are mapped to a single core
                // => use it! (complete coalesce set)
                if(coalescedSet->isComplete()) {
                    // multiple nmvmus are mapped to a single coalescedMVMSet!
                    coalescedMVMSets_[pTile*N_CORES_PER_TILE + pCore].
                        push_back(coalescedSet);
                } 
                // 2) if all the MVMs are not mapped to a single core
                // => discard it! (not complete coalesce set)
                else {
                    // can still be coalesced together later
                    coalescedSet->removeAll();
                    delete coalescedSet;
                }
            }
        }
    }

    mergedCount = 0;
    int temp = 0;
    for(auto it = model_->op_begin(); it != model_->op_end(); ++it) {
        Operation* op = *it;
        if(MergedMVMSet* mergedMVM = dynamic_cast<MergedMVMSet*>(op)){
            CoalescedMergedMVMSet* coalescedSet = mergedMVM->getCoalescedSet();
            if(coalescedSet == NULL){
                mergedMVM->mergedId = mergedCount;
                mergedCount++;
            }
            else{
                mergedMVM->mergedId = -1;
            }
            temp++;
        }
    }

    if(mergedCount != 0){
        unsigned int** mergedMVMSetOfMergedMVMs;
        bool** preBool;
        bool** sucBool;

        unsigned int* preCount;
        unsigned int* sucCount;
        mergedMVMSetOfMergedMVMs = new unsigned int* [mergedCount];
        preBool = new bool* [mergedCount];
        sucBool = new bool* [mergedCount];

        preCount = new unsigned int [mergedCount];
        sucCount = new unsigned int [mergedCount];
        for(unsigned int i = 0; i < mergedCount; i++){
            mergedMVMSetOfMergedMVMs[i] = new unsigned int[mergedCount];
            preBool[i] = new bool[mergedCount];
            sucBool[i] = new bool[mergedCount];

            preCount[i] = 0;
            sucCount[i] = 0;
            for(unsigned int j = 0; j < mergedCount; j++){
                mergedMVMSetOfMergedMVMs[i][j] = 0;
                preBool[i][j] = false;
                sucBool[i][j] = false;
            }
        }

        // Analyze initial dependences between remaining MVM operations
        std::set<MergedMVMSet*> isMergedVisited;
        for(auto it = model_->op_begin(); it != model_->op_end(); ++it) {
            Operation* op = *it;
            if(dynamic_cast<ReadOutputOperation*>(op)) {
                std::set<Operation*> nearestVisited;
                std::set<MergedMVMSet*> nearestMerged = 
                    findNearestMergedMVMPredecessors(op, true, nearestVisited);
                for(auto it = nearestMerged.begin(); it != nearestMerged.end(); it++){
                    findMergedMVMPredecessors(*it, 
                        isMergedVisited,
                        mergedMVMSetOfMergedMVMs,
                        preCount,
                        sucCount,
                        preBool,
                        sucBool);
                }
            }
        }

        std::set<Operation*> isVisited;
        for(auto it = model_->op_begin(); it != model_->op_end(); ++it) {
            Operation* op = *it;
            // start from the bottom!!
            if(dynamic_cast<ReadOutputOperation*>(op)) {
                coalesceMVMPredecessors
                    (op, isVisited, 
                    mergedMVMSetOfMergedMVMs,
                    preCount,
                    sucCount,
                    preBool,
                    sucBool);
            }
        }

        delete [] preCount;
        delete [] sucCount;
        for(unsigned int i = 0; i < mergedCount; i++){
            delete [] mergedMVMSetOfMergedMVMs[i];
            delete [] preBool[i];
            delete [] sucBool[i];
        }
        delete [] mergedMVMSetOfMergedMVMs;
        delete [] preBool;
        delete [] sucBool;
    }
}

// search for the list of mvm predecessors
std::set<MergedMVMSet*> Coalescer::findNearestMergedMVMPredecessors
    (Operation* op, bool start, std::set<Operation*>& isVisited) {
    
    std::set<MergedMVMSet*> mergedPred;
    if(!isVisited.count(op)){
        isVisited.insert(op);
        if(MergedMVMSet* mergedMVM = dynamic_cast<MergedMVMSet*>(op)) {
            if(start || mergedMVM->mergedId == -1) {
                for(MVMOperation* mvm : *mergedMVM) {
                    for(unsigned int o = 0; o < mvm->numOperands(); ++o) {
                        ProducerOperation* predecessor = mvm->getOperand(o);
                        // if mvm op => insert the mergedMVM
                        if(!dynamic_cast<MVMOperation*>(predecessor)) {
                            std::set<MergedMVMSet*> pred = 
                                findNearestMergedMVMPredecessors(predecessor, false, isVisited);
                            mergedPred.insert(pred.begin(), pred.end());
                        }
                    }
                }
            }
        }
        else if(ConsumerOperation* consumer = dynamic_cast<ConsumerOperation*>(op)) {
            for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                ProducerOperation* predecessor = consumer->getOperand(o);
                // if mvm op => insert the mergedMVM
                if(MVMOperation* mvmPred = dynamic_cast<MVMOperation*>(predecessor)) {
                    MergedMVMSet* mergedM = mvmPred->getMergedSet();
                    if(mergedM->mergedId == -1){
                        std::set<MergedMVMSet*> pred = 
                            findNearestMergedMVMPredecessors(mergedM, false, isVisited);
                        mergedPred.insert(pred.begin(), pred.end());
                    }
                    else{
                        mergedPred.insert(mergedM);
                    }
                }
                // if not => insert the predecessor until it is mergedMVM
                else{
                    std::set<MergedMVMSet*> pred = 
                        findNearestMergedMVMPredecessors(predecessor, false, isVisited);
                    mergedPred.insert(pred.begin(), pred.end());
                }
            }
        }
        if(TileMemoryReadOperation* read = dynamic_cast<TileMemoryReadOperation*>(op)) {
            for(unsigned int i = 0; i < read->numSrcs(); ++i) {
                TileMemoryWriteOperation* predecessor = read->getSrc(i);
                std::set<MergedMVMSet*> pred = 
                    findNearestMergedMVMPredecessors(predecessor, false, isVisited);
                mergedPred.insert(pred.begin(), pred.end());
            }
        }
        if(ReceiveOperation* recv = dynamic_cast<ReceiveOperation*>(op)) {
            SendOperation* predecessor = recv->getSrc();
            std::set<MergedMVMSet*> pred = 
                findNearestMergedMVMPredecessors(predecessor, false, isVisited);
            mergedPred.insert(pred.begin(), pred.end());
        }
    }

    return mergedPred;
}

std::set<int> Coalescer::findMergedMVMPredecessors
    (MergedMVMSet* mergedMVM,
    std::set<MergedMVMSet*>& isVisited,
    unsigned int** mergedMVMSetOfMergedMVMs,
    unsigned int* preCount,
    unsigned int* sucCount,
    bool** preBool, 
    bool** sucBool) {

    // if the mvm predessors 
    // has not counted the predecessor
    std::set<int> mergedPred;

    // Visit nodes in reverse postorder 
    // (find MVM predecessors of all predecessors of the operation 
    // to determine predecessors of self)
    if(!isVisited.count(mergedMVM)) {
        isVisited.insert(mergedMVM);
        CoalescedMergedMVMSet* coalescedSet = mergedMVM->getCoalescedSet();
        if(coalescedSet != NULL) {
            assert(coalescedSet->isComplete() && mergedMVM != NULL); 
            // All previously coalesced sets should be complete
            // If MVM is coalesced, include predecessors of 
            // all in the coalesced set
            // iterate over mvm first
            // get the list of predecessors
            for(MergedMVMSet* mergedM : *coalescedSet){
                assert(mergedM != NULL);

                std::set<Operation*> nearestVisited;
                std::set<MergedMVMSet*> nearestMerged = 
                    findNearestMergedMVMPredecessors(mergedM, true, nearestVisited);

                // search for the nearest
                for(auto it = nearestMerged.begin(); it != nearestMerged.end(); it++){
                    CoalescedMergedMVMSet* coalSet = (*it)->getCoalescedSet();

                    if(isVisited.count(*it)){
                        int preTemp = preCount[(*it)->mergedId];
                        for(unsigned int precount = 0; 
                            precount < preTemp; precount++){
                            unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                                [(*it)->mergedId][precount];
                            mergedPred.insert(mergedMVMPreId);
                        }
                    }
                    else{
                        std::set<int> pred = findMergedMVMPredecessors(*it, 
                            isVisited,
                            mergedMVMSetOfMergedMVMs,
                            preCount,
                            sucCount,
                            preBool,
                            sucBool);
                        mergedPred.insert(pred.begin(), pred.end());
                    }

                    if(coalSet == NULL){
                        mergedPred.insert((*it)->mergedId);
                    }
                    else{
                        assert(coalSet->isComplete()); 
                    }
                }
            }
        } 
        // if there is no coalesced
        else {
            std::set<Operation*> nearestVisited;
            std::set<MergedMVMSet*> nearestMerged = 
                findNearestMergedMVMPredecessors(mergedMVM, true, nearestVisited);

            for(auto it = nearestMerged.begin(); it != nearestMerged.end(); it++){

                if(isVisited.count(*it)){
                    int preTemp = preCount[(*it)->mergedId];
                    for(unsigned int precount = 0; 
                        precount < preTemp; precount++){
                        unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                            [(*it)->mergedId][precount];
                        mergedPred.insert(mergedMVMPreId);
                    }
                }
                else{
                    std::set<int> pred = findMergedMVMPredecessors(*it, 
                        isVisited,
                        mergedMVMSetOfMergedMVMs,
                        preCount,
                        sucCount,
                        preBool,
                        sucBool);
                    mergedPred.insert(pred.begin(), pred.end());
                }

                CoalescedMergedMVMSet* coalSet = (*it)->getCoalescedSet();
                if(coalSet == NULL){
                    mergedPred.insert((*it)->mergedId);
                }
                else{
                    assert(coalSet->isComplete()); 
                }
            }
        }
        // if I'm merged mvm => then add it
        for(auto it = mergedPred.begin(); it != mergedPred.end(); it++){
            int mergedMId = *it;
            // add the mergedM as the predecessor of the mergedMVM
            if(!preBool[mergedMVM->mergedId][mergedMId]){
                mergedMVMSetOfMergedMVMs[mergedMVM->mergedId]
                    [preCount[mergedMVM->mergedId]++] = mergedMId;
                preBool[mergedMVM->mergedId][mergedMId] = true;
            }

            if(!sucBool[mergedMId][mergedMVM->mergedId]){
                mergedMVMSetOfMergedMVMs[mergedMId]
                    [mergedCount - 1 - sucCount[mergedMId]++] = mergedMVM->mergedId;
                sucBool[mergedMId][mergedMVM->mergedId] = true;
            }
        }

    } 
    // if visited => fast return
    else {
        // mergedPred 
        int preTemp = preCount[mergedMVM->mergedId];
        for(unsigned int precount = 0; 
            precount < preTemp; precount++){

            unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                [mergedMVM->mergedId][precount];
            mergedPred.insert(mergedMVMPreId);
        }
    }

    // iterate over the predecessors
    return mergedPred;
}


void Coalescer::coalesceMVMPredecessors(Operation* op, 
    std::set<Operation*>& isVisited, 
    unsigned int** mergedMVMSetOfMergedMVMs,
    unsigned int* preCount,
    unsigned int* sucCount,
    bool** preBool,
    bool** sucBool) {

    if(!isVisited.count(op)) {
        // Visit nodes in reverse postorder 
        // (not necessary, but visiting in same order as 
        // linearization helps reduce register pressure)
        if(ConsumerOperation* consumer = 
            dynamic_cast<ConsumerOperation*>(op)) {
            // reverse post order!
            // we iterate over the consumers

            for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                ProducerOperation* predecessor = consumer->getOperand(o);
                coalesceMVMPredecessors
                    (predecessor, isVisited, 
                    mergedMVMSetOfMergedMVMs,
                    preCount,
                    sucCount,
                    preBool,
                    sucBool);
            }

            // if the consumer (current op) is MVM operation
            // (coalesce only the MVM operations)
            if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(consumer)) {
                // if the current mvm is not currently coalesced
                MergedMVMSet* mergedMVM = mvm->getMergedSet();
                if(mergedMVM->getCoalescedSet() == NULL) {
                    
                    // retrieve the coalesced set for the given (tile / core)
                    std::vector<CoalescedMergedMVMSet*> &coreCoalescedSets = 
                        coalescedMVMSets_[placer_->getPTile(mvm)*N_CORES_PER_TILE + 
                        placer_->getPCore(mvm)];
                    std::vector<CoalescedMergedMVMSet*> &remainingCoreCoalescedSets = 
                        remainingCoalescedMVMSets_[placer_->getPTile(mvm)*N_CORES_PER_TILE + 
                        placer_->getPCore(mvm)];

                    unsigned int pMVMU = placer_->getPMVMU(mvm);
                    CoalescedMergedMVMSet* coalescedSet = NULL;

                    // iterate over the coalesced set for the given (tile, core)
                    // iterate over the existing partial sets
                    int remainingSetIdx = -1;
                    for(unsigned int coalescedSetIdx = 0; 
                        coalescedSetIdx < remainingCoreCoalescedSets.size();
                        ++coalescedSetIdx) {

                        // set the remainingSetIdx!
                        remainingSetIdx = coalescedSetIdx;

                        coalescedSet = remainingCoreCoalescedSets[coalescedSetIdx];

                        // should not be full
                        assert(coalescedSet->isComplete() == false);
                        // if the pMVMU is already allocated in the set
                        // => do not use it (of course before the iteration
                        // there would not exist any coalescedSet candidate
                        // as only a complete coalescedset would exist

                        // then check the dependency
                        // => i.e.,
                        // a dependent operation should not exist
                        // in the same set
                        if(!coalescedSet->usesPMVMU(pMVMU)) {
                            bool hasDataHazard = false;
        
                            for(MergedMVMSet* mergedM : *coalescedSet) {
                                // check if there is a dependecy
                                // (either predecessor / successor)
                                if(mergedM != NULL){
                                    hasDataHazard = preBool[mergedMVM->mergedId][mergedM->mergedId]
                                        || sucBool[mergedMVM->mergedId][mergedM->mergedId];
                                }
                                if(hasDataHazard) break;
                            }
                            if(!hasDataHazard) break;
                        }
                        coalescedSet = NULL; // Candidate doesn't work
                    }
                    if(coalescedSet == NULL) {
                        // Create new coalesced set if none found
                        coalescedSet = new CoalescedMergedMVMSet();
                        coreCoalescedSets.push_back(coalescedSet);
                        remainingCoreCoalescedSets.push_back(coalescedSet);
                    }

                    // Add to coalesced set and update dependence information
                    for(MergedMVMSet* mergedM : *coalescedSet) {
                        if(mergedM != NULL) {
                            // Make all predecessors of mvm 
                            // predecessors of m and successors of m
                            // i.e., synchronize the dependency of the
                            // coalescedSet!!!
                            // iterate over the predecessors
                            int preTemp1 = preCount[mergedMVM->mergedId];
                            int sucTemp1 = sucCount[mergedM->mergedId];

                            for(unsigned int precount = 0; 
                                precount < preTemp1; precount++){
                                // the mergedMVMPreId => returns the id of the pred
                                unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                                    [mergedMVM->mergedId][precount];

                                if(!preBool[mergedM->mergedId][mergedMVMPreId]){
                                    //// the mvm's predecessor becomes m's predecessor
                                    mergedMVMSetOfMergedMVMs[mergedM->mergedId]
                                        [preCount[mergedM->mergedId]++] = mergedMVMPreId;
                                    preBool[mergedM->mergedId][mergedMVMPreId] = true;
                                }
                                //// the m becomes mem's predeceddors's successor

                                if(!sucBool[mergedMVMPreId][mergedM->mergedId]){
                                    mergedMVMSetOfMergedMVMs[mergedMVMPreId]
                                        [mergedCount - 1 - sucCount[mergedMVMPreId]++] = mergedM->mergedId;
                                    sucBool[mergedMVMPreId][mergedM->mergedId] = true;
                                }
                            }

                            if(preTemp1 != 0 && sucTemp1 != 0){

                                #pragma omp parallel for if(sucTemp1 > 1000 || preTemp1 > 10000)
                                for(unsigned int succount = 0; 
                                    succount < sucTemp1; succount++){
                                    unsigned int mergedMVMSucId = 
                                        mergedMVMSetOfMergedMVMs
                                        [mergedM->mergedId][mergedCount - 1 - succount];

                                    for(unsigned int precount = 0; 
                                        precount < preTemp1; precount++){
                                        unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                                            [mergedMVM->mergedId][precount];

                                        if(!preBool[mergedMVMSucId][mergedMVMPreId]){
                                            mergedMVMSetOfMergedMVMs[mergedMVMSucId]
                                                [preCount[mergedMVMSucId]++] = mergedMVMPreId;
                                            preBool[mergedMVMSucId][mergedMVMPreId] = true;
                                        }
                                    }
                                }

                                #pragma omp parallel for if(preTemp1 > 1000 || sucTemp1 > 10000)
                                for(unsigned int precount = 0; 
                                    precount < preTemp1; precount++){
                                    unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                                        [mergedMVM->mergedId][precount];
                                    for(unsigned int succount = 0; 
                                        succount < sucTemp1; succount++){

                                        // the mergedMVMSucId => returns the id of the succ
                                        // the merged suc id => 
                                        unsigned int mergedMVMSucId = 
                                            mergedMVMSetOfMergedMVMs
                                            [mergedM->mergedId][mergedCount - 1 - succount];

                                        if(!sucBool[mergedMVMPreId][mergedMVMSucId]){
                                            mergedMVMSetOfMergedMVMs[mergedMVMPreId]
                                                [mergedCount - 1 - sucCount[mergedMVMPreId]++] 
                                                    = mergedMVMSucId;
                                            sucBool[mergedMVMPreId][mergedMVMSucId] = true;
                                        }
                                    }
                                }
                            }

                            // m's predecessor becomes the predecessor of mvm
                            // mvm's successor becomes the successor 
                            // of the m's predecessors
                            int preTemp2 = preCount[mergedM->mergedId];
                            int sucTemp2 = sucCount[mergedMVM->mergedId];
                            
                            for(unsigned int precount = 0; 
                                precount < preTemp2; precount++){
                                // the mergedMVMPreId => returns the id of the pred
                                unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                                    [mergedM->mergedId][precount];

                                //// the mvm's predecessor becomes m's predecessor
                                if(!preBool[mergedMVM->mergedId][mergedMVMPreId]){
                                    mergedMVMSetOfMergedMVMs[mergedMVM->mergedId]
                                        [preCount[mergedMVM->mergedId]++] = mergedMVMPreId;
                                    preBool[mergedMVM->mergedId][mergedMVMPreId] = true;
                                }

                                if(!sucBool[mergedMVMPreId][mergedMVM->mergedId]){
                                    // the m becomes mem's predeceddors's successor
                                    mergedMVMSetOfMergedMVMs[mergedMVMPreId]
                                        [mergedCount - 1 - sucCount[mergedMVMPreId]] = mergedMVM->mergedId;
                                    sucCount[mergedMVMPreId] += 1;
                                    sucBool[mergedMVMPreId][mergedMVM->mergedId] = true;
                                }
                            }
                            if(preTemp2 != 0 && sucTemp2 != 0){
                                #pragma omp parallel for if(sucTemp2 > 1000 || preTemp2 > 10000)
                                for(unsigned int succount = 0; 
                                    succount < sucTemp2; succount++){

                                    // the mergedMVMSucId => returns the id of the succ
                                    unsigned int mergedMVMSucId = 
                                        mergedMVMSetOfMergedMVMs
                                        [mergedMVM->mergedId][mergedCount - 1- succount];
                                    for(unsigned int precount = 0; 
                                        precount < preTemp2; precount++){

                                        unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                                            [mergedM->mergedId][precount];
                                        
                                        if(!preBool[mergedMVMSucId][mergedMVMPreId]){
                                            mergedMVMSetOfMergedMVMs[mergedMVMSucId]
                                                [preCount[mergedMVMSucId]++] = mergedMVMPreId;
                                            preBool[mergedMVMSucId][mergedMVMPreId] = true;
                                        }
                                    }
                                }

                                #pragma omp parallel for if(preTemp2 > 1000 || sucTemp2 > 10000)
                                for(unsigned int precount = 0; 
                                    precount < preTemp2; precount++){
                                    unsigned int mergedMVMPreId = mergedMVMSetOfMergedMVMs
                                        [mergedM->mergedId][precount];
                                    for(unsigned int succount = 0; 
                                        succount < sucTemp2; succount++){

                                        // the mergedMVMSucId => returns the id of the succ
                                        unsigned int mergedMVMSucId = 
                                            mergedMVMSetOfMergedMVMs
                                            [mergedMVM->mergedId][mergedCount - 1- succount];

                                        if(!sucBool[mergedMVMPreId][mergedMVMSucId]){
                                            mergedMVMSetOfMergedMVMs[mergedMVMPreId]
                                                [mergedCount - 1 - sucCount[mergedMVMPreId]++] = mergedMVMSucId;
                                            sucBool[mergedMVMPreId][mergedMVMSucId] = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    coalescedSet->add(mergedMVM, pMVMU);
                    if(coalescedSet->isComplete() && remainingSetIdx != -1) {
                        // remove from the vector when complete
                        remainingCoreCoalescedSets.erase(
                            remainingCoreCoalescedSets.begin()+remainingSetIdx);
                    }
                }
            }
        }
        if(TileMemoryReadOperation* read = 
            dynamic_cast<TileMemoryReadOperation*>(op)) {

            for(unsigned int i = 0; i < read->numSrcs(); ++i) {
                TileMemoryWriteOperation* predecessor = read->getSrc(i);

                coalesceMVMPredecessors
                    (predecessor, isVisited, 
                    mergedMVMSetOfMergedMVMs,
                    preCount,
                    sucCount,
                    preBool,
                    sucBool);
            }
        }
        if(ReceiveOperation* recv = dynamic_cast<ReceiveOperation*>(op)) {
            SendOperation* predecessor = recv->getSrc();
            coalesceMVMPredecessors
                (predecessor, isVisited, 
                 mergedMVMSetOfMergedMVMs,
                 preCount,
                 sucCount,
                 preBool,
                 sucBool);
        }
        isVisited.insert(op);
    }
}

