/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <fstream>
#include <map>
#include <vector>
#include <string>

#include "common.h"

class Partitioner {


    private:

        ModelImpl* model_;
        CompilerOptions::GraphPartitioningScheme gp_;

        unsigned int nVMVMUs_;
        unsigned int nVCores_;
        unsigned int nVTiles_;

        std::vector<ConstantMatrixTile*> cmatTiles_;
        std::map<Operation*, unsigned int> op2vmvmu_;
        std::map<ConstantMatrixTile*, unsigned int> cmat2vmvmu_;
        std::vector<unsigned int> vmvmu2vcore_;
        std::vector<unsigned int> vcore2vtile_;

        bool isVMVMUAssigned(Operation* op);
        void assignVMVMU(Operation* op, unsigned int vMVMU);
        void reassignVMVMU(ALUVectorOperation* op, unsigned int vMVMU);
        void assignVMVMUsAndSpreadAffinity();
        void spreadVMVMUAffinityToOperands(ConsumerOperation* op, bool resize, unsigned int vMVMU);
        void spreadVMVMUAffinityToUsers(ProducerOperation* op);

        unsigned int numLoads_ = 0;
        unsigned int numStores_ = 0;
        unsigned int numSends_ = 0;
        unsigned int numReceives_ = 0;

        void assignVMVMUsInRowMajor();
        void assignVMVMUsInColMajor();
        void assignVMVMUsRandomly();
        void assignVCoresInVMVMUOrder();
        void assignVCoresWithKaHIP(); 
        void assignVTilesInVMVMUOrder();
        void assignVTilesWithKaHIP();

        void insertLoadsAndStores();
        void insertSendsAndRecives();
        void insertInputAndOutput();
        void insertCopies();

        void unlink(std::list<Operation*>::iterator it);

    public:

        Partitioner(ModelImpl* model, CompilerOptions::GraphPartitioningScheme gp);

        unsigned int getNVMVMUs() { return nVMVMUs_; }
        unsigned int getNVCores() { return nVCores_; }
        unsigned int getNVTiles() { return nVTiles_; }
        unsigned int getVMVMU(ConstantMatrixTile* tile);
        unsigned int getVCore(ConstantMatrixTile* tile);
        unsigned int getVTile(ConstantMatrixTile* tile);
        unsigned int getVMVMU(Operation* op);
        unsigned int getVCore(Operation* op);
        unsigned int getVTile(Operation* op);
        unsigned int getVCore(unsigned int vMVMU) { return vmvmu2vcore_[vMVMU]; }
        unsigned int getVTile(unsigned int vCore) { return vcore2vtile_[vCore]; }

        void cloneAssignment(Operation* cloneFrom, Operation* cloneTo);

        std::string printAssignment(Operation* op);
        void printReport(std::ofstream& report);

};

