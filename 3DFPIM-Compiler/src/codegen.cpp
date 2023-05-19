/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <assert.h>
#include <fstream>
#include <sstream>

#include "3dfpim.h"

#include "coalescer.h"
#include "codegen.h"
#include "linearizer.h"
#include "memalloc.h"
#include "model.h"
#include "operations.h"
#include "placer.h"
#include "regalloc.h"

#include <iostream>

CodeGenerator::CodeGenerator(ModelImpl* model, Placer* placer, MemoryAllocator* memoryAllocator, Coalescer* coalescer, Linearizer* linearizer, RegisterAllocator* registerAllocator)
    : model_(model), placer_(placer), memoryAllocator_(memoryAllocator), coalescer_(coalescer), linearizer_(linearizer), registerAllocator_(registerAllocator)
{
    codegen();
}

void CodeGenerator::codegen() {

    // TODO: Define ABI for laying out the binary

    for(unsigned int pTile = 0; pTile < placer_->getNPTiles(); ++pTile) {

        // Generate code for the tile
        std::stringstream fileName;
        fileName << model_->getName() << "-tile" << pTile << ".3dfpim";
        std::ofstream tileCode;
        tileCode.open(fileName.str());
        std::list<TileOperation*>& tileOperationList = linearizer_->getTileOperationList(pTile);
        for(TileOperation* tileOp : tileOperationList) {
            if(SendOperation* send = dynamic_cast<SendOperation*>(tileOp)) {
                tileCode << codegen(send);
            } else if(ReceiveOperation* recv = dynamic_cast<ReceiveOperation*>(tileOp)) {
                tileCode << codegen(recv);
            } else if(WriteInputOperation* write = dynamic_cast<WriteInputOperation*>(tileOp)) {
                tileCode << codegen(write);
            } else if(ReadOutputOperation* read = dynamic_cast<ReadOutputOperation*>(tileOp)) {
                tileCode << codegen(read);
            } else {
                assert(0 && "Unsupported operation for code generation!");
            }
        }
        tileCode << "halt()" << std::endl;
        tileCode.close();

        // Generate code for each core in the tile
        for(unsigned int pCore = 0; pCore < N_CORES_PER_TILE; ++pCore) {
            std::stringstream fileName;
            fileName << model_->getName() << "-tile" << pTile << "-core" << pCore << ".3dfpim";
            std::ofstream coreCode;
            coreCode.open(fileName.str());
            std::list<CoreOperation*>& coreOperationList = 
                linearizer_->getCoreOperationList(pTile, pCore);
            for(CoreOperation* coreOp : coreOperationList) {
                if(MVMOperation* mvm = dynamic_cast<MVMOperation*>(coreOp)) {
                    coreCode << codegen(mvm);
                } else if(ALUVectorOperation* aluOp = dynamic_cast<ALUVectorOperation*>(coreOp)) {
                    coreCode << codegen(aluOp);
                } else if(SetImmediateOperation* seti = dynamic_cast<SetImmediateOperation*>(coreOp)) {
                    coreCode << codegen(seti);
                } else if(CopyOperation* copy = dynamic_cast<CopyOperation*>(coreOp)) {
                    coreCode << codegen(copy);
                } else if(LoadOperation* load = dynamic_cast<LoadOperation*>(coreOp)) {
                    coreCode << codegen(load);
                } else if(StoreOperation* store = dynamic_cast<StoreOperation*>(coreOp)) {
                    coreCode << codegen(store);
                } else if(MVMGuardOperation* guard = dynamic_cast<MVMGuardOperation*>(coreOp)) {
                    coreCode << codegen(guard);
                } else {
                    assert(0 && "Unsupported operation for code generation!");
                }
            }
            coreCode << "hlt()" << std::endl;
            coreCode.close();
        }

    }

}

std::string CodeGenerator::codegen(CoalescedMVMSet* coalescedMVMSet) {
    std::stringstream ss;
    ss << "mvm(['";

    int depth;
    bool isLast;
    int nStack;
    int precision;
    std::string layer_name = "";
    for(unsigned int i = 0; i < N_CONSTANT_MVMUS_PER_CORE; ++i) {
        if(coalescedMVMSet->usesPMVMU(i)) {
            MVMOperation* mvm = coalescedMVMSet->getPMVMU(i);
            depth = mvm->getDepth();
            isLast = mvm->isMVMLast();
            ss << 1;
            layer_name = mvm->printOperationType();
            nStack = mvm->getNStack();
            precision = mvm->getPrecision();
        } else {
            ss << 0;
        }
    }

    ss << "'], depth = " << depth;
    ss << ", precision = " << precision << "";
    ss << ", stacks = " << nStack;
    ss << ", name = '" << layer_name << "'";
    if(isLast)
        ss << ", isLast = True)\n";
    else
        ss << ", isLast = False)\n";
    return ss.str();
}

std::string CodeGenerator::codegen(MVMOperation* mvm) {
    CoalescedMVMSet* coalescedMVMSet = mvm->getCoalescedSet();
    if(coalescedMVMSet != NULL) {
        if(coalescedMVMSet->isSetLeader(mvm)) { 
        // Only one MVM in a coalesced set does code generation on behalf of the others
            return codegen(coalescedMVMSet);
        } else {
            return "";
        }
    } else {
        std::stringstream ss;
        ss << "mvm(xb_nma = ['";
        for(unsigned int i = 0; i < N_CONSTANT_MVMUS_PER_CORE; ++i) {
            if(i == placer_->getPMVMU(mvm)) {
                ss << 1;
            } else {
                ss << 0;
            }
        }
        ss << "']";
        ss << ", name = '" << mvm->printOperationType() << "'";
        ss << ", precision = " << mvm->getPrecision();
        ss << ", depth = " << mvm->getDepth();
        ss << ", stacks = " << mvm->getNStack();
        ss << ", slideId = " << mvm->getSlideId();
        if(mvm->isMVMLast())
            ss << ", isLast = True)\n";
        else
            ss << ", isLast = False)\n";
        return ss.str();
    }
}

std::string CodeGenerator::codegen(ALUVectorOperation* aluOp) {
    std::stringstream ss;
    ss << "alu";
    switch(aluOp->getOpCode()) {
        case ALUVectorOperation::MULI:
            ss << "i";
    }
    ss << "('";
    switch(aluOp->getOpCode()) {
        case ALUVectorOperation::RESIZE: ss << "resize"; break;
        case ALUVectorOperation::CONCAT: ss << "concat"; break;
        case ALUVectorOperation::ADD: ss << "add"; break;
        case ALUVectorOperation::SUB: ss << "sub"; break;
        case ALUVectorOperation::MUL:
        case ALUVectorOperation::MULI: ss << "mul"; break;
        case ALUVectorOperation::DIV: ss << "div"; break;
        case ALUVectorOperation::AND: ss << "and"; break;
        case ALUVectorOperation::OR: ss << "or"; break;
        case ALUVectorOperation::NOT: ss << "not"; break;
        case ALUVectorOperation::EQ: ss << "eq"; break;
        case ALUVectorOperation::NEQ: ss << "neq"; break;
        case ALUVectorOperation::LT: ss << "lt"; break;
        case ALUVectorOperation::LEQ: ss << "leq"; break;
        case ALUVectorOperation::GT: ss << "gt"; break;
        case ALUVectorOperation::GEQ: ss << "geq"; break;
        case ALUVectorOperation::MIN: ss << "min"; break;
        case ALUVectorOperation::MAX: ss << "max"; break;
        case ALUVectorOperation::MSE: ss << "mse"; break;
        case ALUVectorOperation::SIG: ss << "sig"; break;
        case ALUVectorOperation::NOACT: ss << "noact"; break;
        case ALUVectorOperation::TANH: ss << "tanh"; break;
        case ALUVectorOperation::EXP: ss << "exp"; break;
        case ALUVectorOperation::LOG: ss << "log"; break;
        case ALUVectorOperation::RELU: ss << "relu"; break;
        case ALUVectorOperation::RELUD: ss << "relud"; break;
        case ALUVectorOperation::LOG_SOFTMAX: ss << "log_softmax"; break;
        case ALUVectorOperation::LOG_SOFTMAXD: ss << "log_softmaxd"; break;
        case ALUVectorOperation::RNDCMP: ss << "rndcmp"; break;
    }
    ss << "', "
       << "d1=" << registerAllocator_->getRegister(aluOp) << ", "
       << "r1=" << registerAllocator_->getRegister(aluOp->getOperand(0)) << ", ";
    if(aluOp->numOperands() > 1) {
        ss << "r2=" << registerAllocator_->getRegister(aluOp->getOperand(1)) << ", ";
    }
    if(aluOp->isImmediate()) {
        ss << "imm=" << aluOp->getImmediate() << ", ";
    }
    ss << "intermediate=False, ";
    ss << "vec=" << aluOp->length()
       << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(SetImmediateOperation* seti) {
    std::stringstream ss;
    ss << "set("
       << "d1=" << registerAllocator_->getRegister(seti) << ", "
       << "imm=" << seti->getImmediate() << ", "
       << "vec=" << seti->length() << ", ";
    ss << "intermediate=False";
    ss << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(CopyOperation* copy) {
    std::stringstream ss;
    ss << "copy("
       << "d1=" << registerAllocator_->getRegister(copy) << ", "
       << "r1=" << registerAllocator_->getRegister(copy->getOperand(0)) << ", "
       << "vec=" << copy->length() << ", "
       << "src_type=" << 1 << ", ";
    ss << "intermediate=False";
    ss << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(LoadOperation* load) {
    std::stringstream ss;
    unsigned int loadWidth;
    for(loadWidth = MAX_LOAD_STORE_WIDTH; !(load->length()%loadWidth == 0); --loadWidth);
    ss << "load("
       << "d1=" << registerAllocator_->getRegister(load) << ", "
       << "r1=" << registerAllocator_->getRegister(load->getOperand(0)) << ", "
       << "load_width=" << loadWidth << ", "
       << "vec=" << load->length()/loadWidth << ", ";
    ss << "intermediate=False";
    ss << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(StoreOperation* store) {
    std::stringstream ss;
    unsigned int storeWidth;
    for(storeWidth = MAX_LOAD_STORE_WIDTH; !(store->length()%storeWidth == 0); --storeWidth);

    int counter = 0;
    for (auto u = store->user_begin(); u != store->user_end(); ++u) {
        if (MVMGuardOperation* guard = dynamic_cast<MVMGuardOperation*>(*u)) {
            continue;
        }
        else {
            counter += 1;
        }
    }

    ss << "store(d1=" << registerAllocator_->getRegister(store->getOperand(1)) << ", "
       << "r1=" << registerAllocator_->getRegister(store->getOperand(0)) << ", "
       << "counter=" << counter << ", "
       << "store_width=" << storeWidth << ", "
       << "vec=" << store->length()/storeWidth << ", ";
    ss << "dst=[";
    for (auto u = store->user_begin(); u != store->user_end(); ++u) {
        TileMemoryReadOperation* user = *u;
        if (LoadOperation* load = dynamic_cast<LoadOperation*>(user)) {
            ss << "(" << placer_->getPTile(load) << "," << placer_->getPCore(load) << "), ";
        }
        else if (MVMGuardOperation* guard = dynamic_cast<MVMGuardOperation*>(user)) {
            continue;
        }
        else if (SendOperation* send = dynamic_cast<SendOperation*>(user)) {
            ReceiveOperation* recv = send->getDst();
            for (auto ru = recv->user_begin(); ru != recv->user_end(); ++ru) {
                TileMemoryReadOperation* recv_user = *ru;
                if (LoadOperation* load = dynamic_cast<LoadOperation*>(recv_user)) {
                    ss << "(" << placer_->getPTile(load) << "," << placer_->getPCore(load) << "), ";
                }
                else if (MVMGuardOperation* guard = dynamic_cast<MVMGuardOperation*>(recv_user)) {
                    continue;
                }
                else if (SendOperation* sendrecv = dynamic_cast<SendOperation*>(recv_user)) {
                    assert(0 && "Send cannot consume receive");
                }
                else if (ReadOutputOperation* output = dynamic_cast<ReadOutputOperation*>(recv_user)) {
                    ss << "(1, 0), ";
                }
                else {
                    assert(0 && "Only load, send, output can consume receive");
                }
            }
        }
        else {
            assert(0 && "Only load and send can consume store");
        }
    }
    ss << "], ";
    ss << "intermediate=False";
    ss << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(MVMGuardOperation* guard) {
    std::stringstream ss;
    unsigned int guardWidth;
    for(guardWidth = MAX_LOAD_STORE_WIDTH; !(guard->length()%guardWidth == 0); --guardWidth);
    ss << "guard("
       << "r1=" << registerAllocator_->getRegister(guard->getOperand(0)) << ", "
       << "guard_width=" << guardWidth << ", "
       << "vec=" << guard->length()/guardWidth << ", ";
    ss << "intermediate=False";
    ss << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(SendOperation* send) {
    std::stringstream ss;
    unsigned int sendWidth;
    for(sendWidth = MAX_SEND_RECV_WIDTH; !(send->length()%sendWidth == 0); --sendWidth);
    ss << "send("
       << "mem_addr=" << memoryAllocator_->getTileMemoryAddress(send->getSrc(0)) << ", "
       << "vtile_id=" << placer_->getPTile(send) << ", " // FIXME: Assign sender IDs
       << "send_width=" << sendWidth << ", "
       << "target_addr=" << placer_->getPTile(send->getDst()) << ", "
       << "vec=" << send->length()/sendWidth << ", "
       << "op_id=" << send->id << ", ";
    ss << "intermediate=False";
    ss << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(ReceiveOperation* recv) {
    std::stringstream ss;
    unsigned int recvWidth;
    for(recvWidth = MAX_SEND_RECV_WIDTH; !(recv->length()%recvWidth == 0); --recvWidth);

    int counter = 0;
    for (auto u = recv->user_begin(); u != recv->user_end(); ++u) {
        if (MVMGuardOperation* guard = dynamic_cast<MVMGuardOperation*>(*u)) {
            continue;
        }
        else {
            counter += 1;
        }
    }

    ss << "receive(mem_addr=" << memoryAllocator_->getTileMemoryAddress(recv) << ", "
       << "vtile_id=" << placer_->getPTile(recv->getSrc()) << ", " // FIXME: Assign sender IDs
       << "receive_width=" << recvWidth << ", "
       << "counter=" << counter << ", "
       << "vec=" << recv->length()/recvWidth << ", "
       << "op_id=" << recv->id << ", ";
    ss << "intermediate=False";
    ss << ")\n";
    return ss.str();
}

std::string CodeGenerator::codegen(WriteInputOperation* write) {
    return "";
}

std::string CodeGenerator::codegen(ReadOutputOperation* read) {
    return "";
}

