/*
 *  Copyright (c) 2019 IMPACT Research Group, University of Illinois.
 *  All rights reserved.
 *
 *  This file is covered by the LICENSE.txt license file in the root directory.
 *
 */
/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <assert.h>
#include <bitset>
#include <sstream>
#include <algorithm>

#include "3dfpim.h"

#include "linearizer.h"
#include "memalloc.h"
#include "model.h"
#include "operations.h"
#include "partitioner.h"
#include "placer.h"
#include "regalloc.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include <chrono>

class CoreAllocator {

    private:

        std::bitset<REGISTERS_PER_CORE> memPool_;

    public:

        static const unsigned int OUT_OF_REGISTERS = REGISTERS_PER_CORE;

        unsigned int allocate(unsigned int size);
        void free(unsigned int pos, unsigned int size);

};

class SpillTracker {

    private:

        std::map<ProducerOperation*, StoreOperation*> producer2spill;
        std::map<ProducerOperation*, LoadOperation*> producer2reload;
        std::map<LoadOperation*, ProducerOperation*> reload2producer;

    public:

        bool isSpilled(ProducerOperation* producer) 
            { return producer2spill.count(producer); }
        bool hasLiveNowReload(ProducerOperation* producer) 
            { return producer2reload.count(producer); }
        bool isLiveNowReload(LoadOperation* load) 
            { return reload2producer.count(load); }

        StoreOperation* getSpillOperation
            (ProducerOperation* producer);
        LoadOperation* getLiveNowReload
            (ProducerOperation* producer);
        ProducerOperation* getOriginalProducer
            (LoadOperation* load);

        void setSpillOperation
            (ProducerOperation* producer, StoreOperation* store);
        void setLiveNowReload
            (ProducerOperation* producer, LoadOperation* load);
        void killLiveNowReload
            (LoadOperation* load);

        std::map<ProducerOperation*, LoadOperation*>::iterator 
            reloads_begin() { return producer2reload.begin(); }
        std::map<ProducerOperation*, LoadOperation*>::iterator 
            reloads_end() { return producer2reload.end(); }

};

unsigned int CoreAllocator::allocate(unsigned int size) {
    for(unsigned int i = 0; i <= REGISTER_FILE_SIZE - size; ++i) {
        unsigned int j;
        for(j = i; j < i + size; ++j) {
            if(memPool_[j]) {
                break;
            }
        }
        if(j == i + size) {
            for(unsigned int k = i; k < j; ++k) {
                memPool_.set(k);
            }
            return REGISTER_FILE_START_ADDRESS + i;
        } else {
            i = j;
        }
    }
    return OUT_OF_REGISTERS;
}

void CoreAllocator::free(unsigned int reg, unsigned int size) {
    unsigned int pos = reg - REGISTER_FILE_START_ADDRESS;
    for(unsigned int i = pos; i < pos + size; ++i) {
        assert(memPool_[i] && "Attempt to free unallocated registers!");
        memPool_.reset(i);
    }
}

StoreOperation* SpillTracker::getSpillOperation(ProducerOperation* producer) {
    assert(isSpilled(producer));
    return producer2spill[producer];
}

LoadOperation* SpillTracker::getLiveNowReload(ProducerOperation* producer) {
    assert(hasLiveNowReload(producer));
    return producer2reload[producer];
}

ProducerOperation* SpillTracker::getOriginalProducer(LoadOperation* load) {
    assert(isLiveNowReload(load));
    return reload2producer[load];
}

void SpillTracker::setSpillOperation
    (ProducerOperation* producer, StoreOperation* store) {
    
    assert(!producer2spill.count(producer) && "Register allocation error: spilling a register that has already been spilled!");
    producer2spill[producer] = store;
}

void SpillTracker::setLiveNowReload
    (ProducerOperation* producer, LoadOperation* load) {
    
    assert(!hasLiveNowReload(producer) && "Register allocation error: reloading a spilled register that has already been reloaded!");
    producer2reload[producer] = load;
    reload2producer[load] = producer;
}

void SpillTracker::killLiveNowReload(LoadOperation* load) {
    // check if a load to producer exist
    assert(isLiveNowReload(load));
    ProducerOperation* producer = reload2producer[load];
    producer2reload.erase(producer);
    reload2producer.erase(load);
}

RegisterAllocator::RegisterAllocator
    (ModelImpl* model, Partitioner* partitioner, 
    Placer* placer, MemoryAllocator* memoryAllocator, Linearizer* linearizer)
    : model_(model), partitioner_(partitioner), 
    placer_(placer), memoryAllocator_(memoryAllocator), linearizer_(linearizer)
{
    // times two => for optimistic allocation
    op2reg_size = model->op_count * 2;
    op2reg_ = new int [op2reg_size];
    for(int i = 0; i < op2reg_size; i++)
        op2reg_[i] = -1;
    registerAllocation();
}

RegisterAllocator::~RegisterAllocator()
{
    delete [] op2reg_;
}

bool RegisterAllocator::isRegisterAssigned(ProducerOperation* producer) {
    assert(producer->id < op2reg_size);
    return (op2reg_[producer->id] != -1);
}

void RegisterAllocator::assignRegister(ProducerOperation* producer, unsigned int reg) {
    assert(!isRegisterAssigned(producer) && "Cannot reassign register");
    op2reg_[producer->id] = reg;
}

// assign a reserved register!
void RegisterAllocator::assignReservedInputRegister(ProducerOperation* producer) {

    assert(!writesToReservedOutputRegister(producer) && 
        "Cannot assign reserved input registers to matrix operations that write to reserved output registers!");
    assert(producer->numUsers() == 1 && "Producer serving a matrix operation can only have one user");

    ConsumerOperation* consumer = *(producer->user_begin());
    unsigned int reg;
    if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(consumer)) {
        reg = INPUT_REGISTERS_START_ADDRESS + 
            placer_->getPMVMU(mvm)*MVMU_DIM;
    } else if(ALUVectorOperation* alu = dynamic_cast<ALUVectorOperation*>(consumer)) {
        if (alu->isResize()) {
            // first operand
            if (producer == alu->getOperand(0)) {
                reg = INPUT_REGISTERS_START_ADDRESS +
                    placer_->getPMVMU(alu)*MVMU_DIM;
            }
            // second operand
            else {
                reg = INPUT_REGISTERS_START_ADDRESS +
                    placer_->getPMVMU(alu)*MVMU_DIM +
                    alu->length() - producer->length();
            }
        }
    } else {
        assert(0 && "Cannot assign reserved input register to producer that doesn't feed a matrix operation");
    }
    assignRegister(producer, reg);
}

void RegisterAllocator::assignReservedOutputRegister
    (ProducerOperation* producer) {
    
    assert(writesToReservedOutputRegister(producer) && 
        "Cannot assign reserved output registers to non-matrix operations");
    unsigned int reg;
    if(MVMOperation* mvm = 
        dynamic_cast<MVMOperation*>(producer)) {
        
        reg = OUTPUT_REGISTERS_START_ADDRESS + 
            placer_->getPMVMU(mvm)*MVMU_DIM;
    } else {
        assert(0 && "Cannot assign reserved output register to producer that is not a matrix operation");
    }
    assignRegister(producer, reg);
}

bool RegisterAllocator::readsFromReservedInputRegister
    (ConsumerOperation* consumer) {

    return (dynamic_cast<MVMOperation*>(consumer) != NULL);
}

bool RegisterAllocator::writesToReservedOutputRegister
    (ProducerOperation* producer) {
    
    return (dynamic_cast<MVMOperation*>(producer) != NULL);
}

bool RegisterAllocator::producerDoesNotWriteToRegister(ProducerOperation* producer) {
    if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(producer)){
        if(!mvm->isMVMLast()) return true;
    }
    return false;
}

unsigned int RegisterAllocator::getRegister(ProducerOperation* producer) {
    assert(isRegisterAssigned(producer) && "Register has not been assigned!");
    return op2reg_[producer->id];
}

void RegisterAllocator::registerAllocation() {
    // Allocate registers
    for(unsigned int pTile = 0; pTile < placer_->getNPTiles(); ++pTile) {
        for(unsigned int pCore = 0; pCore < N_CORES_PER_TILE; ++pCore) {
            allocateReservedInputRegisters(pTile, pCore);
            allocateReservedOutputRegisters(pTile, pCore);
            allocateDataRegisters(pTile, pCore);
        }
    }

}

void RegisterAllocator::allocateReservedInputRegisters
    (unsigned int pTile, unsigned int pCore) {
    
    // Assign reserved input registers and ensure no overlap in live ranges
    std::set<ProducerOperation*> liveNow;
    std::list<CoreOperation*>& coreOperationList = 
        linearizer_->getCoreOperationList(pTile, pCore);
    
    // return the list of operator
    // iterate over the core operation
    for(auto op = coreOperationList.rbegin(); 
        op != coreOperationList.rend(); ++op) {
        // if producer => remove from the liveness
        if(ProducerOperation* producer = dynamic_cast<ProducerOperation*>(*op)) {
            liveNow.erase(producer);
        }
        if(ConsumerOperation* consumer = dynamic_cast<ConsumerOperation*>(*op)) {
            // if the consumer is MVM op.
            if(readsFromReservedInputRegister(consumer)) {
                for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                    ProducerOperation* producer = consumer->getOperand(o);
                    if(!liveNow.count(producer) && !isResize(producer)) {

                        // do not affect live range if it is non-last MVM
                        if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(producer)){
                            if(!mvm->isMVMLast()) continue;
                        }

                        liveNow.insert(producer);
                        assignReservedInputRegister(producer);
                        for(ProducerOperation* p : liveNow) {
                            if(p != producer && getRegister(p) == 
                                getRegister(producer)) {
                                // NOTE: The linearizer ensures that there are no live range conflicts by placing matrix operation operands immediately before they are consumed
                                assert(0 && "Register allocation error: conflict detected in live ranges of operations using the same reserved input registers!");
                            }
                        }
                    }
                }
            }
        }

        // allocate input registers to mvmOp-feeding operations
        if (ProducerOperation* producer = dynamic_cast<ProducerOperation*>(*op)) {
            for (auto u = producer->user_begin(); u != producer->user_end(); ++u) {
                if (ALUVectorOperation* alu = dynamic_cast<ALUVectorOperation*>(*u)) {
                    if (alu->isResize()) {
                        assignReservedInputRegister(producer);
                    }
                }
            }
        }
    }
}

void RegisterAllocator::allocateReservedOutputRegisters
    (unsigned int pTile, unsigned int pCore) {
    
    // Assign reserved output registers and ensure no overlap in live ranges
    std::set<ProducerOperation*> liveNow;
    std::list<CoreOperation*>& coreOperationList = 
        linearizer_->getCoreOperationList(pTile, pCore);
    
    for(auto op = coreOperationList.rbegin(); 
        op != coreOperationList.rend(); ++op) {

        if(ProducerOperation* producer = dynamic_cast<ProducerOperation*>(*op)) {
            liveNow.erase(producer);
        }
        if(ConsumerOperation* consumer = dynamic_cast<ConsumerOperation*>(*op)) {
            for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                ProducerOperation* producer = consumer->getOperand(o);
                // if the producer is MVM
                if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(producer)){
                    if(!mvm->isMVMLast()) continue;
                }
                if(writesToReservedOutputRegister(producer)) {
                    if(!liveNow.count(producer)) {
                        liveNow.insert(producer);
                        assignReservedOutputRegister(producer);
                        for(ProducerOperation* p : liveNow) {
                            if(p != producer && getRegister(p) == getRegister(producer)) {
                                // NOTE: The linearizer ensures that there are no 
                                // live range conflicts by placing matrix operation 
                                // consumers immediately after they are produced
                                assert(0 && "Register allocation error: conflict detected in live ranges of \
                                        operations using the same reserved output registers!");
                            }
                        }
                    }
                }
            }
        }
    }
}

void RegisterAllocator::allocateDataRegisters(unsigned int pTile, unsigned int pCore) {

    // Live range analysis
    std::list<CoreOperation*>& coreOperationList = 
        linearizer_->getCoreOperationList(pTile, pCore);

    int listSize = coreOperationList.size() + 1;

    // set of pointers
    ProducerOperation*** liveIn = new ProducerOperation** [listSize];
    int* liveIn_count = new int [listSize];

    for(int i = 0; i < listSize; i++){
        liveIn_count[i] = 0;
    }
    
    int scale_init = 100000;
    int scale_factor = scale_init;

    int currOpId = coreOperationList.size() - 1;
    int nextOpId = coreOperationList.size();

    for(auto op = coreOperationList.rbegin(); op != coreOperationList.rend(); ++op) {

        while(listSize / scale_factor <= liveIn_count[nextOpId]) scale_factor /= 10;

        liveIn[currOpId] = new ProducerOperation* [listSize / scale_factor];

        auto array_end = std::copy(liveIn[nextOpId]
            , liveIn[nextOpId] 
            + liveIn_count[nextOpId]
            , liveIn[currOpId]);

        // remove a producer when necessary
        if(ProducerOperation* producer = dynamic_cast<ProducerOperation*>(*op)){
            array_end = std::remove(liveIn[currOpId]
                , liveIn[currOpId] + liveIn_count[nextOpId]
                , producer);
        }
        liveIn_count[currOpId] = std::distance(liveIn[currOpId], array_end);
        
        // Add operations consumed by the operation
        bool scale_en = false;
        if(ConsumerOperation* consumer = dynamic_cast<ConsumerOperation*>(*op)) {
            // if not mvm operation
            if(!readsFromReservedInputRegister(consumer)) {
                // then we iterate over the mvm operations
                for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                    ProducerOperation* producer = consumer->getOperand(o);
                    // not mvm operation
                    if(!writesToReservedOutputRegister(producer) && !isResize(producer)) {
                        // livePtr
                        bool exist = false;
                        for(int i = 0; i < liveIn_count[currOpId]; i++){
                            if(liveIn[currOpId][i] == producer){
                               exist = true;
                               break;
                            }
                        }
                        if(!exist){
                            liveIn[currOpId][liveIn_count[currOpId]++] = producer;
                        }
                        // if the size exceeds the threshold => scale the array
                        if(liveIn_count[currOpId] == listSize / scale_factor){
                            scale_en = true;
                            break;
                        }
                    }
                }
            }
        }

        if(scale_en) {
            scale_factor /= 10;
            delete [] liveIn[currOpId];
            liveIn_count[currOpId] = 0;
            --op;
            continue;
        }
        nextOpId = currOpId--;
        scale_factor = scale_init;
    }
        
    // Allocate data registers
    CoreAllocator allocator;
    SpillTracker spillTracker;
    std::set<ProducerOperation*> liveNow;
    unsigned int spillAddressReg = allocator.allocate(1);
    
    currOpId = 0;
    nextOpId = 1;
    for(auto op = coreOperationList.begin(); op != coreOperationList.end(); ++op) {
        // check if the size overflows
        nextOpId = currOpId + 1;

        ProducerOperation** liveOut = liveIn[nextOpId];
        int liveOut_count = liveIn_count[nextOpId];

        if(ConsumerOperation* consumer = dynamic_cast<ConsumerOperation*>(*op)) {
            // iterate over the consumers!
            // find only the consumer which does not read from reserved ones
            if(!readsFromReservedInputRegister(consumer)) {

                // Make sure all operands are available
                for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                    ProducerOperation* producer = consumer->getOperand(o);
                    if(!writesToReservedOutputRegister(producer) && !isResize(producer)) {
                        
                        // if the producer is live + has been reloaded
                        // do not require additional thing
                        // it is already live and nothing special
                        if(liveNow.count(producer) || 
                            spillTracker.isLiveNowReload
                            (dynamic_cast<LoadOperation*>(producer))) {
                            
                            numUnspilledRegAccesses_ += producer->length();

                        // it indicates that the producer is not live
                        // or is spilled ...
                        } else {
                            // Reload operands that have been spilled
                            assert(spillTracker.isSpilled(producer));

                            // producer2reload!
                            // i.e., the producer is not load & producer is not live
                            if(spillTracker.hasLiveNowReload(producer)) {
                                // If already reloaded, reuse reload
                                numUnspilledRegAccesses_ += producer->length();
                                LoadOperation* load = 
                                    spillTracker.getLiveNowReload(producer);
                                consumer->replaceOperand(producer, load);
                            // not to be reloaded
                            } else {
                                // Reload from spilled register
                                numSpilledRegAccesses_ += producer->length();

                                // retrieve spill operation for the module
                                StoreOperation* spillOp = 
                                    spillTracker.getSpillOperation(producer);
                                SetImmediateOperation* seti = 
                                    new SetImmediateOperation
                                    (model_, memoryAllocator_->
                                    getTileMemoryAddress(spillOp));
                                partitioner_->cloneAssignment(producer, seti);
                                assignRegister(seti, spillAddressReg);

                                // new load operation 
                                // for the spilled (store) operation
                                LoadOperation* load = new LoadOperation(model_, spillOp);


                                numLoadsFromSpilling_ += load->length();
                                load->addTileMemoryAddressOperand(seti);
                                partitioner_->cloneAssignment(producer, load);

                                // allocate register for the load operation
                                unsigned int reg = 
                                    allocateRegistersWithSpilling
                                    (load->length(), allocator, liveNow, 
                                    spillTracker, spillAddressReg, coreOperationList, op);
                                assignRegister(load, reg);

                                consumer->replaceOperand(producer, load);
                                coreOperationList.insert(op, seti);
                                coreOperationList.insert(op, load);
                                // check spilling
                                // set reload2producer and producer2reload
                                spillTracker.setLiveNowReload(producer, load);
                            }
                        }
                    }
                }

                // Free registers for operands that are no longer live
                for(unsigned int o = 0; o < consumer->numOperands(); ++o) {
                    ProducerOperation* producer = consumer->getOperand(o);
                    if(!writesToReservedOutputRegister(producer) && !isResize(producer)) {
                        if(liveNow.count(producer)) {
                            // iterate over the live In
                            bool exist = false;
                            for(int i = 0; i < liveOut_count; i++){
                                if(liveOut[i] == producer){
                                   exist = true;
                                   break;
                                }
                            }
                            if(!exist){
                                liveNow.erase(producer);
                                allocator.free(getRegister(producer), producer->length());
                            }
                        }
                        else if(LoadOperation* load = 
                            dynamic_cast<LoadOperation*>(producer)) {
                            
                            assert(spillTracker.isLiveNowReload(load));
                            ProducerOperation* originalProducer = 
                                spillTracker.getOriginalProducer(load);

                            // iterate over the live In
                            bool exist = false;
                            for(int i = 0; i < liveOut_count; i++){
                                if(liveOut[i] == producer){
                                   exist = true;
                                   break;
                                }
                            }
                            if(!exist){
                                spillTracker.killLiveNowReload(load);
                                allocator.free(getRegister(load), load->length());
                            }
                        } 
                        else {
                            assert(0 && "Operand must either be a live operation or a spilled register load!");
                        }
                    }
                }
            }
        }

        // Allocate register for new operation
        if(ProducerOperation* producer = 
            dynamic_cast<ProducerOperation*>(*op)) {

            // iterate over the live In
            bool exist = false;
            for(int i = 0; i < liveOut_count; i++){
                if(liveOut[i] == producer){
                   exist = true;
                   break;
                }
            }
            if(exist){
                unsigned int reg = 
                    allocateRegistersWithSpilling
                    (producer->length(), allocator, 
                    liveNow, spillTracker, 
                    spillAddressReg, coreOperationList, op);
                assignRegister(producer, reg);
                liveNow.insert(producer);
            } else {
                // Producer already assigned to a reserved input or output register
                assert(isRegisterAssigned(producer) || 
                    producerDoesNotWriteToRegister(producer));
            }
        }
        currOpId++;
    }

    int max = 0;
    for(int i = 0; i < listSize - 1; i++){
        if(max < liveIn_count[i])
            max = liveIn_count[i];
    }

    for(int i = 0; i < listSize - 1; i++){
        delete [] liveIn[i];
    }
    delete [] liveIn;
    delete [] liveIn_count;
}

unsigned int RegisterAllocator::allocateRegistersWithSpilling
    (unsigned int length, 
    CoreAllocator& allocator, 
    std::set<ProducerOperation*>& liveNow, 
    SpillTracker& spillTracker, 
    unsigned int spillAddressReg, 
    std::list<CoreOperation*>& coreOperationList, 
    std::list<CoreOperation*>::iterator& op) {


    // TODO: Better heuristic for which is the best register to free (e.g., the one which will be used the latest into the future)
    ConsumerOperation* consumer = dynamic_cast<ConsumerOperation*>(*op);
    unsigned int reg = allocator.allocate(length);
    if(reg != CoreAllocator::OUT_OF_REGISTERS) {
        return reg;
    } else {
        // First try to free registers by killing live reloads 
        // that are not used by this operation
        std::set<LoadOperation*> removeLoad;
        for(auto killCandidate = spillTracker.reloads_begin(); 
            killCandidate != spillTracker.reloads_end(); ++killCandidate) {

            ProducerOperation* producerToKill = killCandidate->first;
            LoadOperation* reloadToKill = killCandidate->second;
            if(consumer == NULL || !consumer->uses(producerToKill) && 
                    !consumer->uses(reloadToKill)) {
                removeLoad.insert(reloadToKill);
                allocator.free(getRegister(reloadToKill), reloadToKill->length());
                reg = allocator.allocate(length);
                if(reg != CoreAllocator::OUT_OF_REGISTERS) {
                    break;
                }
            }
        }
        for(LoadOperation* remove : removeLoad)
            spillTracker.killLiveNowReload(remove);

        // if resolved using spill tracker -> return
        if(reg != CoreAllocator::OUT_OF_REGISTERS)
            return reg;

        // If unable to kill enough reloads, 
        // then spill live operations that are not used by this operation
        std::set<ProducerOperation*> removeList;
        for(ProducerOperation* spillCandidate : liveNow) {
            if(consumer == NULL || !consumer->uses(spillCandidate)) {
                assert(spillCandidate != NULL);
                unsigned int vTileIndex = partitioner_->getVTile(spillCandidate);
                unsigned int spillLength = spillCandidate->length();
                unsigned int address = memoryAllocator_->memalloc(vTileIndex, spillLength);
                SetImmediateOperation* setiStore = 
                    new SetImmediateOperation(model_, address);
                partitioner_->cloneAssignment(spillCandidate, setiStore);
                assignRegister(setiStore, spillAddressReg);
                StoreOperation* store = new StoreOperation(model_, spillCandidate);
                numStoresFromSpilling_ += store->length();
                partitioner_->cloneAssignment(spillCandidate, store);
                memoryAllocator_->assignTileMemoryAddress(store, address);
                store->addTileMemoryAddressOperand(setiStore);
                coreOperationList.insert(op, setiStore);
                coreOperationList.insert(op, store);
                removeList.insert(spillCandidate);

                spillTracker.setSpillOperation(spillCandidate, store);
                allocator.free(getRegister(spillCandidate), spillCandidate->length());
                reg = allocator.allocate(length);
                if(reg != CoreAllocator::OUT_OF_REGISTERS) {
                    break;
                }
            }
        }

        // remove late
        for(ProducerOperation* remove : removeList)
            liveNow.erase(remove);

        // then check functionality
        if(reg != CoreAllocator::OUT_OF_REGISTERS)
            return reg;

        // If unable to spill enough live operations, then fail
        assert(0 && "Register allocation error: cannot find enough registers to spill!");
    }

}

bool RegisterAllocator::isResize(Operation* op) {
    bool isResize = false;

    if (ALUVectorOperation* alu = dynamic_cast<ALUVectorOperation*>(op)) {
        if (alu->isResize()) {
            isResize = true;
        }
    }

    return isResize;
}

void RegisterAllocator::printReport(std::ofstream& report) {
    report << "# load bytes from spilling = " << numLoadsFromSpilling_ << std::endl;
    report << "# store bytes from spilling = " << numStoresFromSpilling_ << std::endl;
    report << "# load + store bytes from spilling = " << numLoadsFromSpilling_ + numStoresFromSpilling_ << std::endl;
    report << "# unspilled register accesses = " << numUnspilledRegAccesses_ << std::endl;
    report << "# spilled register accesses = " << numSpilledRegAccesses_ << std::endl;
    report << "% spilled register accesses = " << 100.0*numSpilledRegAccesses_/(numSpilledRegAccesses_ + numUnspilledRegAccesses_) << "%" << std::endl;
}

std::string RegisterAllocator::printAssignment(Operation* op) {
    std::stringstream ss;
    if(ProducerOperation* producer = dynamic_cast<ProducerOperation*>(op)) {
        if(isRegisterAssigned(producer)) {
            ss << "\nregister = " << getRegister(producer);
        }
    }
    return ss.str();
}

