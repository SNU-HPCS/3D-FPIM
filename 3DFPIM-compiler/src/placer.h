/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include "common.h"

class Placer {

    private:

        ModelImpl* model_;
        Partitioner* partitioner_;

        unsigned int nPTiles_;
        unsigned int nPCores_;
        unsigned int nPMVMUs_;

        std::vector<unsigned int> vtile2ptile_;
        std::vector<unsigned int> vcore2pcore_;
        std::vector<unsigned int> vmvmu2pmvmu_;

        void assignPTiles();
        void assignPCores();
        void assignPMVMUs();

    public:

        Placer(ModelImpl* model, Partitioner* partitioner);

        unsigned int getNPMVMUs() { return nPMVMUs_; }
        unsigned int getNPCores() { return nPCores_; }
        unsigned int getNPTiles() { return nPTiles_; }
        unsigned int getPMVMU(ConstantMatrixTile* tile);
        unsigned int getPTile(ConstantMatrixTile* tile);
        unsigned int getPCore(ConstantMatrixTile* tile);
        unsigned int getPMVMU(Operation* op);
        unsigned int getPTile(Operation* op);
        unsigned int getPCore(Operation* op);

        std::string printAssignment(Operation* op);

};

