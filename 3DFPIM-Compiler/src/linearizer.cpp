/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <assert.h>

#include "3dfpim.h"

#include "linearizer.h"
#include "model.h"
#include "operations.h"
#include "partitioner.h"
#include "placer.h"

#include <iostream>
using namespace std;

Linearizer::Linearizer(ModelImpl* model, Partitioner* partitioner, Placer* placer)
    : model_(model), partitioner_(partitioner), placer_(placer), coreOperationLists_(placer_->getNPCores()), tileOperationLists_(placer_->getNPTiles())
{
    linearize();
}

void Linearizer::linearize() {
    // Begin traversal from operations that output final results, namely matrix update operations and output operations
    std::set<Operation*> isVisited;
    std::set<Operation*> wasAddedEarly;
    for(auto it = model_->op_begin(); it != model_->op_end(); ++it) {
        Operation* op = *it;
        if(dynamic_cast<ReadOutputOperation*>(op)) {
            linearizeWithPredecessors(op, isVisited, wasAddedEarly);
        }
    }
}

void Linearizer::linearizeWithPredecessors
    (Operation* op, std::set<Operation*>& isVisited, 
    std::set<Operation*>& wasAddedEarly, 
    bool addSelf, bool sliding) {

    if(!isVisited.count(op)) {
        if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(op)) { // MVMOp
            assert(addSelf); 

            if(sliding == false) {
                assert(mvm->isMVMLast());
                assert(mvm->getMergedSet());
                assert(mvm->getMergedSet()->getSlidingSet());
                // retrieve the sliding mvm operations
                SlidingMergedMVMSet* slidingSet = mvm->getMergedSet()->getSlidingSet();

                // linearize in sliding order
                for(MergedMVMSet* mergedM : *slidingSet) {
                    assert(mergedM != NULL && "sliding set cannot have null merged set");
                    
                    // start linearizing from the last mvm of the merged set
                    MVMOperation* lastMVM = mergedM->getLast();
                    assert(lastMVM->numOperands() != 0 && "MVM operation must have at least one operand");

                    // prioritize mvm (load => mvm => load => mvm)
                    for(int op = lastMVM->numOperands() - 1; op >= 0; --op) {
                        ProducerOperation* operand = lastMVM->getOperand(op);

                        if(!dynamic_cast<MVMOperation*>(operand)) {
                            linearizeWithPredecessors(
                                    operand, isVisited, wasAddedEarly, /*addSelf*/true, /*sliding*/false);
                        } else {
                            linearizeWithPredecessors(
                                    operand, isVisited, wasAddedEarly, /*addSelf*/true, /*sliding*/true);
                        }
                    }

                    // add self
                    addToList(lastMVM, isVisited);

                    // consume outputs immediately after they are produced
                    addConsumersToList(lastMVM, isVisited, wasAddedEarly);
                }
            } else {
                assert(!mvm->isMVMLast());
                assert(mvm->getMergedSet());
                assert(mvm->numOperands() != 0 && "MVM operation must have at least one operand");

                for(int op = mvm->numOperands() - 1; op >= 0; op--) {
                    ProducerOperation* operand = mvm->getOperand(op);

                    if(!dynamic_cast<MVMOperation*>(operand)) {
                        linearizeWithPredecessors(
                                operand, isVisited, wasAddedEarly, /*addSelf*/true, /*sliding*/false);
                    } else {
                        linearizeWithPredecessors(
                                operand, isVisited, wasAddedEarly, /*addSelf*/true, /*sliding*/true);
                    }
                }

                if(mvm->numOperands() == 1) {
                    MVMOperation* lastMVM = mvm->getMergedSet()->getLast();

                    // Add MVMGuardOperation
                    // Before starting merged MVM,
                    // check if the last required vector is loaded in the eDRAM
                    LoadOperation* load = insertMVMGuard(lastMVM);
                    if(load != NULL && !isVisited.count(load)) {
                        assert(load->numOperands() == 1);
                        SetImmediateOperation* loadset = dynamic_cast<SetImmediateOperation*>(load->getOperand(0));
                        assert(loadset != NULL);
                        unsigned int address = loadset->getImmediate();

                        SetImmediateOperation* seti = new SetImmediateOperation(model_, address);
                        partitioner_->cloneAssignment(load, seti);
                        addToList(seti, isVisited);

                        assert(load->numSrcs() == 1);
                        MVMGuardOperation* guard = new MVMGuardOperation(model_, lastMVM, load->getSrc(0));
                        partitioner_->cloneAssignment(load, guard);
                        guard->addTileMemoryAddressOperand(seti);
                        addToList(guard, isVisited);
                    }
                }

                // add self
                addToList(mvm, isVisited);
            }
        } else { // Non-MVMOp
            if(ConsumerOperation* consumer = 
                dynamic_cast<ConsumerOperation*>(op)) {
                
                // prioritize mvm / alu operation first
                for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                    ProducerOperation* operand = consumer->getOperand(o);
                    if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(operand)){
                        linearizeWithPredecessors
                        (mvm, 
                        isVisited, wasAddedEarly);
                    } else if(dynamic_cast<ALUVectorOperation*>(operand)){
                        linearizeWithPredecessors
                        (operand, 
                        isVisited, wasAddedEarly);
                        //(addSelf || (dynamic_cast<CopyOperation*>(op) != NULL)));
                    }
                }
                // then perform non mvm / alu operation
                for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                    ProducerOperation* operand = consumer->getOperand(o);
                    if(!dynamic_cast<MVMOperation*>(operand) && !dynamic_cast<ALUVectorOperation*>(operand)){
                        // if I'm the parent of MVM operation
                        // then, my predecessor should be added along with me!
                        linearizeWithPredecessors
                        (operand, 
                        isVisited, wasAddedEarly);
                        //(addSelf || (dynamic_cast<CopyOperation*>(op) != NULL)));
                    }
                }
                // this should be the copy operation
            }
            // starting point
            if(TileMemoryReadOperation* read = 
                dynamic_cast<TileMemoryReadOperation*>(op)) {
                
                for(unsigned int i = 0; i < read->numSrcs(); ++i) {
                    linearizeWithPredecessors
                        (read->getSrc(i), isVisited, wasAddedEarly);
                }
                assert(!wasAddedEarly.count(read));
            }
            if(ReceiveOperation* recv = dynamic_cast<ReceiveOperation*>(op)) {
                linearizeWithPredecessors(recv->getSrc(), 
                isVisited, wasAddedEarly);
                assert(!wasAddedEarly.count(recv));
            }
            if(addSelf) {
                ALUVectorOperation* alu = dynamic_cast<ALUVectorOperation*>(op);
                if(alu != NULL && alu->isResize()) {
                    assert(!wasAddedEarly.count(op));
                } else {
                    if(!wasAddedEarly.count(op)) { 
                    // Do not add a consumer operation 
                    // if it was added early by a predecesor matrix operation
                        addToList(op, isVisited);
                    }
                }
            }
        }
    }
}

LoadOperation* Linearizer::insertMVMGuard(MVMOperation* lastMVM) {
    MVMOperation* mvm = lastMVM;

    while(1) {
        // if the mvmOp is the first mvm of mergedSet
        if(mvm->numOperands() == 1) {
            return NULL;
        }

        for(int op = 0; op < mvm->numOperands(); ++op) {
            ProducerOperation* operand = mvm->getOperand(op);

            if(!dynamic_cast<MVMOperation*>(operand)) { // the operand is not an MVMOp
                while(1) {
                    if(ALUVectorOperation* resize = dynamic_cast<ALUVectorOperation*>(operand)) {
                        ProducerOperation* rProducer = resize->getOperand(1);

                        if(LoadOperation* load = dynamic_cast<LoadOperation*>(rProducer)) {
                            return load;
                        } else if(SetImmediateOperation* set = dynamic_cast<SetImmediateOperation*>(rProducer)) {
                            operand = resize->getOperand(0);
                            continue;
                        } else if(CopyOperation* copy = dynamic_cast<CopyOperation*>(rProducer)) {
                            LoadOperation* load = dynamic_cast<LoadOperation*>(copy->getOperand(0));
                            assert(load != NULL);
                            return load;
                        } else {
                            assert(0 && "Only Load, Set, Copy can feed Resize");
                        }
                    } else if(LoadOperation* load = dynamic_cast<LoadOperation*>(operand)) {
                        return load;
                    } else if(SetImmediateOperation* set = dynamic_cast<SetImmediateOperation*>(operand)) {
                        break;
                    } else if(CopyOperation* copy = dynamic_cast<CopyOperation*>(operand)) {
                        LoadOperation* load = dynamic_cast<LoadOperation*>(copy->getOperand(0));
                        assert(load != NULL);
                        return load;
                    } else {
                        assert(0 && "Only Resize, Load, Set, Copy can feed MVMOp");
                    }
                }
            } else { 
                mvm = dynamic_cast<MVMOperation*>(operand);
                break;
            }
        }
    }

    assert(0);
}

void Linearizer::addToList (Operation* op, std::set<Operation*>& isVisited) {
    assert(!isVisited.count(op));
    if(CoreOperation* coreOp = dynamic_cast<CoreOperation*>(op)) {
        if(MergedMVMSet* mergedMVM = dynamic_cast<MergedMVMSet*>(coreOp)){
            getCoreOperationList
                (placer_->getPTile(mergedMVM), placer_->getPCore(mergedMVM)).push_back(coreOp);
        } else{
            getCoreOperationList
                (placer_->getPTile(coreOp), placer_->getPCore(coreOp)).push_back(coreOp);
        }
        
    }
    if(TileOperation* tileOp = dynamic_cast<TileOperation*>(op)) {
        getTileOperationList(placer_->getPTile(tileOp)).push_back(tileOp);
    }
    isVisited.insert(op);
}

void Linearizer::addConsumersToList
    (ProducerOperation* producer, 
    std::set<Operation*>& isVisited, 
    std::set<Operation*>& wasAddedEarly) {

    bool allConsumersCanBeAdded = true;

    for(auto u = producer->user_begin(); u != producer->user_end(); ++u) {
        // check user list for the mvm
        // this will be either mvm or sigmoid
        ConsumerOperation* consumer = *u;
        assert(consumer->numOperands() <= 2 && "number of operands cannot exceed 2");

        bool consumerCanBeAdded = true;
        for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
            if(!isVisited.count(consumer->getOperand(o))) {
                ProducerOperation* operand = consumer->getOperand(o);
                assert(operand != producer);

                bool predecessorAdded = addPredecessorsToList(operand, isVisited, wasAddedEarly);

                if(!predecessorAdded) {
                    consumerCanBeAdded = false;
                    break;
                }
            }
        }

        if(consumerCanBeAdded) {
            if(!wasAddedEarly.count(consumer)) {
                addToList(consumer, isVisited);
                wasAddedEarly.insert(consumer);
            }

            if(ProducerOperation* consumerProducer = dynamic_cast<ProducerOperation*>(consumer)) {
                addConsumersToList(consumerProducer, isVisited, wasAddedEarly);
            } else {
                StoreOperation* consumerStore = dynamic_cast<StoreOperation*>(consumer);
                assert(consumerStore != NULL);
            }
        } else {
            allConsumersCanBeAdded = false;
        }
    }

    if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(producer)) {
        if(!allConsumersCanBeAdded) {
            CopyOperation* copy = new CopyOperation(model_, producer);
            partitioner_->cloneAssignment(producer, copy);
            addToList(copy, isVisited);
            for(auto u = producer->user_begin(); u != producer->user_end(); ) {
                ConsumerOperation* consumer = *u;
                ++u; // replaceOperand may remove consumer from producer's users
                if(consumer != copy && !isVisited.count(consumer)) {
                    // replace producer with the copy
                    consumer->replaceOperand(producer, copy);
                }
            }
        }
    }
}

bool Linearizer::addPredecessorsToList
    (Operation* op,
    std::set<Operation*>& isVisited,
    std::set<Operation*>& wasAddedEarly) {

    assert(!isVisited.count(op));

    if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(op)) {
        return false;
    } else if(SendOperation* send = dynamic_cast<SendOperation*>(op)) {
        for(unsigned int i = 0; i < send->numSrcs(); ++i) {
            linearizeWithPredecessors(send->getSrc(i), isVisited, wasAddedEarly, /*addSelf*/true, /*sliding*/false);
        }
        addToList(op, isVisited);
        return true;
    } else if(ReceiveOperation* recv = dynamic_cast<ReceiveOperation*>(op)) {
        if(!isVisited.count(recv->getSrc())) {
            bool predecessorAdded = addPredecessorsToList(recv->getSrc(), isVisited, wasAddedEarly);
            assert(predecessorAdded);
        }
        addToList(op, isVisited);
        return true;
    } else if(StoreOperation* store = dynamic_cast<StoreOperation*>(op)) {
        for(unsigned int o = 0; o < store->numOperands(); ++o) {
            linearizeWithPredecessors(store->getOperand(o), isVisited, wasAddedEarly, /*addSelf*/true, /*sliding*/false);
        }
        if(!wasAddedEarly.count(op)) {
            addToList(op, isVisited);
        }
        return true;
    } else {
        if(TileMemoryReadOperation* read = dynamic_cast<TileMemoryReadOperation*>(op)) {
            for(unsigned int i = 0; i < read->numSrcs(); ++i) {
                if(!isVisited.count(read->getSrc(i))) {
                    bool predecessorAdded = addPredecessorsToList(read->getSrc(i), isVisited, wasAddedEarly);
                    assert(predecessorAdded);
                }
            }
        }

        if(ConsumerOperation* consumer = dynamic_cast<ConsumerOperation*>(op)) {
            bool consumerCanBeAdded = true;

            for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                ProducerOperation* operand = consumer->getOperand(o);

                if(!isVisited.count(operand)) {
                    bool predecessorAdded = addPredecessorsToList(operand, isVisited, wasAddedEarly);
                    if(!predecessorAdded) {
                        consumerCanBeAdded = false;
                        break;
                    }
                }
            }

            if(consumerCanBeAdded) {
                addToList(consumer, isVisited);
                return true;
            } else {
                return false;
            }
        } else {
            addToList(op, isVisited);
            return true;
        }
    }
}

std::list<CoreOperation*>& Linearizer::getCoreOperationList
    (unsigned int pTile, unsigned int pCore) {

    return coreOperationLists_[pTile*N_CORES_PER_TILE + pCore];
}

std::list<TileOperation*>& Linearizer::getTileOperationList(unsigned int pTile) {
    return tileOperationLists_[pTile];
}

