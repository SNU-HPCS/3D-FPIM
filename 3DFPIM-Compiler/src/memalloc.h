/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <map>

#include "common.h"

class MemoryAllocator {

    private:

        ModelImpl* model_;
        Partitioner* partitioner_;

        std::map<TileMemoryWriteOperation*, unsigned int> op2mem_;
        std::vector<unsigned int> vTileAvailableMemory_;

        bool isTileMemoryAddressAssigned(TileMemoryWriteOperation* op);
        void memoryAllocation();

    public:

        MemoryAllocator(ModelImpl* model, Partitioner* partitioner);

        void assignTileMemoryAddress(TileMemoryWriteOperation* op, unsigned int address);
        unsigned int getTileMemoryAddress(TileMemoryWriteOperation* op);
        unsigned int memalloc(unsigned int vTile, unsigned int size);

        std::string printAssignment(Operation* op);

};

