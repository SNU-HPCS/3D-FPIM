/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "common.h"

class Operation {

    protected:

        ModelImpl* model_;
        unsigned int length_;

        Operation() { }

        Operation(ModelImpl* model, unsigned int length);

    public:

        int id;
        virtual ~Operation() { }

        ModelImpl* getModel() const { return model_; }
        unsigned int length() const { return length_; }

        std::string printNodeName();
        virtual std::string printNodeStyle();
        virtual std::string printOperationType()=0;
        virtual void printNodeAndEdges(std::ostream& fout);

};

class ProducerOperation : public virtual Operation {

    protected:

        std::set<ConsumerOperation*> users_;

        ProducerOperation() { }

    public:

        void addUser(ConsumerOperation* user) { users_.insert(user); }
        void removeUser(ConsumerOperation* user) { users_.erase(user); }

        typedef std::set<ConsumerOperation*>::iterator user_iterator;
        user_iterator user_begin() { return users_.begin(); }
        user_iterator user_end() { return users_.end(); }
        unsigned int numUsers() { return users_.size(); }

        void printNodeAndEdges(std::ostream& fout);

};

class ConsumerOperation : public virtual Operation {

    protected:

        std::vector<ProducerOperation*> operands_;

        ConsumerOperation(ProducerOperation* op1=NULL, ProducerOperation* op2=NULL);

    public:

        unsigned int numOperands() { return operands_.size(); }
        ProducerOperation* getOperand(unsigned int i) { return operands_[i]; }
        bool uses(ProducerOperation* op);
        void replaceOperand(ProducerOperation* op, ProducerOperation* replacement);

};

class TileMemoryWriteOperation : public virtual Operation {

    protected:

        std::set<TileMemoryReadOperation*> users_;

        TileMemoryWriteOperation() { }

    public:

        unsigned int numUsers() { return users_.size(); }
        void addUser(TileMemoryReadOperation* user) { users_.insert(user); }
        void removeUser(TileMemoryReadOperation* user) { users_.erase(user); }

        typedef std::set<TileMemoryReadOperation*>::iterator user_iterator;
        user_iterator user_begin() { return users_.begin(); }
        user_iterator user_end() { return users_.end(); }

        void printNodeAndEdges(std::ostream& fout);

};

class TileMemoryReadOperation : public virtual Operation {

    protected:

        std::vector<TileMemoryWriteOperation*> srcs_;

        TileMemoryReadOperation
            (TileMemoryWriteOperation* src1, TileMemoryWriteOperation* src2=NULL);

    public:

        unsigned int numSrcs() { return srcs_.size(); }
        TileMemoryWriteOperation* getSrc(unsigned int i) { return srcs_[i]; }
        void replaceSrc(TileMemoryWriteOperation* old, TileMemoryWriteOperation* replacement);

};

class InputOperation : public virtual Operation {

    protected:

        InputVectorTile* src_;

        InputOperation(InputVectorTile* src);

    public:

        InputVectorTile* getSrc() { return src_; }

        void printNodeAndEdges(std::ostream& fout);

};

class OutputOperation : public virtual Operation {

    protected:

        OutputVectorTile* dst_;

        OutputOperation(OutputVectorTile* dst);

    public:

        OutputVectorTile* getDst() { return dst_; }

        void printNodeAndEdges(std::ostream& fout);

};

class CoreOperation : public virtual Operation {

};

class TileOperation : public virtual Operation {

};

// An individual (MVMU_DIM x MVMU_DIM) MVM operation
class MVMOperation : 
    public ProducerOperation, 
    public ConsumerOperation, 
    public CoreOperation {

    protected:

        ConstantMatrixTile* mat_;
        MergedMVMSet* mergedSet_;
        CoalescedMVMSet* coalescedSet_;

        bool isLast_;
        int depth_;
        int nStack_;
        int precision_;
        int slideId_;

    public:

        MVMOperation(ModelImpl* model, ConstantMatrixTile* mat, bool isLast, int depth,
            int nStack,
            int precision,
            int slideId,
            ProducerOperation* src1 = NULL, ProducerOperation* src2 = NULL);
        bool isMVMLast() { return isLast_; } 
        int getDepth() { return depth_; }
        int getNStack() { return nStack_; }
        int getPrecision() { return precision_; }
        int getSlideId() { return slideId_; }

        void setMergedSet(MergedMVMSet* mergedSet);
        void setCoalescedSet(CoalescedMVMSet* coalescedSet);
        void resetMergedSet();
        MergedMVMSet* getMergedSet() { return mergedSet_; }
        CoalescedMVMSet* getCoalescedSet() { return coalescedSet_; }

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout) 
            { ProducerOperation::printNodeAndEdges(fout); }

};

// A 3D MVM operation
// Performs multiple individual MVM operations in an MVMU,
// and accumulate the result in the analog domain.
class MergedMVMSet : 
    public ProducerOperation,
    public ConsumerOperation,
    public CoreOperation {

    protected:

        std::set<MVMOperation*> mvms_;
        CoalescedMergedMVMSet* coalescedSet_;
        SlidingMergedMVMSet* slidingSet_;

    public:
        int mergedId;

        MergedMVMSet(ModelImpl* model, unsigned int length) : 
            Operation(model, length) { 

            coalescedSet_ = NULL; 
            slidingSet_ = NULL;
        }
        
        std::set<MVMOperation*>::iterator begin() { return mvms_.begin(); }
        std::set<MVMOperation*>::iterator end() { return mvms_.end(); }

        void add(MVMOperation* mvm);
        void remove(MVMOperation* mvm);
        bool exist(MVMOperation* mvm) { return mvms_.count(mvm); }
        unsigned int getSize() { return mvms_.size(); }
        MVMOperation* getLast(){
            for(auto mvm : mvms_){
                if(mvm->isMVMLast())
                    return mvm;
            }
        }

        void setCoalescedSet(CoalescedMergedMVMSet* coalescedSet);
        void setSlidingSet(SlidingMergedMVMSet* slidingSet);
        void resetCoalescedSet();
        CoalescedMergedMVMSet* getCoalescedSet() { return coalescedSet_; }
        SlidingMergedMVMSet* getSlidingSet() { return slidingSet_; }

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout);
        void printMVMList();

};

class CoalescedMVMSet {

    private:

        std::vector<MVMOperation*> mvms_;

    public:

        CoalescedMVMSet() : mvms_(N_CONSTANT_MVMUS_PER_CORE) { }

        std::vector<MVMOperation*>::iterator begin() { return mvms_.begin(); }
        std::vector<MVMOperation*>::iterator end() { return mvms_.end(); }

        bool usesPMVMU(unsigned int pMVMU) { return mvms_[pMVMU] != NULL; }
        MVMOperation* getPMVMU(unsigned int pMVMU) { return mvms_[pMVMU]; }

        void add(MVMOperation* mvm, unsigned int pMVMU);
        bool isSetLeader(MVMOperation* mvm);
        unsigned int getSize() {
            int size = 0;
            for(unsigned int i = 0; i < N_CONSTANT_MVMUS_PER_CORE; i++)
                if(mvms_[i] != NULL) size++;
            return size;
        }

};

class CoalescedMergedMVMSet {

    private:

        std::vector<MergedMVMSet*> mvms_;

    public:

        CoalescedMergedMVMSet() : mvms_(N_CONSTANT_MVMUS_PER_CORE) { }

        std::vector<MergedMVMSet*>::iterator begin() { return mvms_.begin(); }
        std::vector<MergedMVMSet*>::iterator end() { return mvms_.end(); }

        MergedMVMSet* getMVMU(unsigned int pMVMU) { return mvms_[pMVMU]; }
        bool usesPMVMU(unsigned int pMVMU) { return mvms_[pMVMU] != NULL; }
        void add(MergedMVMSet* mvm, unsigned int pMVMU);
        void removeAll();
        bool isSetLeader(MergedMVMSet* mvm);
        bool isComplete();

};

// A ordered set of MVM operations
// performed by a single MVMU
class SlidingMergedMVMSet {

    private:

        std::vector<MergedMVMSet*> mvms_;

    public:

        SlidingMergedMVMSet(int slide_size) : mvms_(slide_size) {};

        std::vector<MergedMVMSet*>::iterator begin() { return mvms_.begin(); }
        std::vector<MergedMVMSet*>::iterator end() { return mvms_.end(); }
        void add(MergedMVMSet* mvm, int slideId);

};

class ALUVectorOperation : 
    public ProducerOperation, 
    public ConsumerOperation, 
    public CoreOperation {

    public:

        enum OpCode {
            ADD, SUB, MUL, DIV,                                                 /* Arithmetic */
            MULI,                                                               /* Arithmetic immediate */
            AND, OR, NOT,                                                       /* Logical */
            EQ, NEQ, LT, LEQ, GT, GEQ,                                          /* Comparison */
            MIN, MAX,                                                           /* Min/Max */
            MSE,                                                                /* Other binary */
            SIG, TANH, EXP, LOG, RELU, RELUD, LOG_SOFTMAX, LOG_SOFTMAXD, RNDCMP, /* Nonlinear */
            RESIZE,
            CONCAT,
            NOACT
        };

    protected:

        OpCode opCode_;
        float imm_;

    public:

        ALUVectorOperation
            (ModelImpl* model, OpCode opCode, 
            ProducerOperation* src1=NULL, 
            ProducerOperation* src2=NULL);

        ALUVectorOperation
            (ModelImpl* model, OpCode opCode, 
            ProducerOperation* src1, float imm);

        OpCode getOpCode() { return opCode_; }
        bool isImmediate() { return opCode_ == MULI; }
        bool isResize() { return opCode_ == RESIZE; }
        float getImmediate() { return imm_; }

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout) 
            { ProducerOperation::printNodeAndEdges(fout); }

};

class SetImmediateOperation : public ProducerOperation, public CoreOperation {

    protected:

        unsigned int imm_;

    public:

        SetImmediateOperation
            (ModelImpl* model, unsigned int imm, 
            unsigned int length=1);

        unsigned int getImmediate() { return imm_; }

        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout) 
            { ProducerOperation::printNodeAndEdges(fout); }

};

class CopyOperation : 
    public ProducerOperation, 
    public ConsumerOperation, 
    public CoreOperation {

    public:

        CopyOperation(ModelImpl* model, ProducerOperation* src);

        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout) 
            { ProducerOperation::printNodeAndEdges(fout); }

};

class LoadOperation : 
    public ProducerOperation, 
    public ConsumerOperation, 
    public TileMemoryReadOperation, 
    public CoreOperation {

    public:

        LoadOperation(ModelImpl* model, TileMemoryWriteOperation* src);

        void addTileMemoryAddressOperand(ProducerOperation* address);

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout) 
            { ProducerOperation::printNodeAndEdges(fout); }

};

class StoreOperation : 
    public ConsumerOperation, 
    public TileMemoryWriteOperation, 
    public CoreOperation {

    public:

        StoreOperation(ModelImpl* model, ProducerOperation* src);

        void addTileMemoryAddressOperand(ProducerOperation* address);

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout) 
            { TileMemoryWriteOperation::printNodeAndEdges(fout); }

};

class MVMGuardOperation :
    public ConsumerOperation,
    public TileMemoryReadOperation,
    public CoreOperation {

    protected:

        MVMOperation* mvm_;

    public:

        MVMGuardOperation(ModelImpl* model, MVMOperation* mvm, TileMemoryWriteOperation* src);

        void addTileMemoryAddressOperand(ProducerOperation* address);

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout);

};

class SendOperation : public TileMemoryReadOperation, public TileOperation {

    protected:

        ReceiveOperation* dst_;

    public:

        SendOperation(ModelImpl* model, TileMemoryWriteOperation* src);

        ReceiveOperation* getDst() { return dst_; }
        void setDst(ReceiveOperation* dst);

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout);

};

class ReceiveOperation : 
    public TileMemoryWriteOperation, 
    public TileOperation {

    protected:

        SendOperation* src_;

    public:

        ReceiveOperation(ModelImpl* model, SendOperation* src);

        SendOperation* getSrc() { return src_; }

        std::string printNodeStyle();
        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout) 
            { TileMemoryWriteOperation::printNodeAndEdges(fout); }

};

class WriteInputOperation : 
    public InputOperation, 
    public TileMemoryWriteOperation, 
    public TileOperation {

    public:

        WriteInputOperation(ModelImpl* model, InputVectorTile* src);

        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout);

};

class ReadOutputOperation : 
    public OutputOperation, 
    public TileMemoryReadOperation, 
    public TileOperation {

    public:

        ReadOutputOperation
            (ModelImpl* model, TileMemoryWriteOperation* src, 
            OutputVectorTile* dst);

        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout);

};

/* Psudeo-operations: Not real operations. Will be replaced before code generation. */

class PseudoInputOperation : public InputOperation, public ProducerOperation {

    public:

        PseudoInputOperation(ModelImpl* model, InputVectorTile* src);

        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout);

};

class PseudoOutputOperation : public OutputOperation, public ConsumerOperation {

    public:

        PseudoOutputOperation(ModelImpl* model, ProducerOperation* src, OutputVectorTile* dst);

        std::string printOperationType();
        void printNodeAndEdges(std::ostream& fout);

};

