/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <list>
#include <set>
#include <vector>

#include "common.h"

class Linearizer {

    private:

        ModelImpl* model_;
        Partitioner* partitioner_;
        Placer* placer_;

        std::vector<std::list<CoreOperation*>> coreOperationLists_;
        std::vector<std::list<TileOperation*>> tileOperationLists_;

        void linearize();
        void linearizeWithPredecessors(Operation* op, 
            std::set<Operation*>& isVisited, 
            std::set<Operation*>& wasAddedEarly, 
            bool addSelf=true, bool sliding=false);

        void addToList(Operation* op, std::set<Operation*>& isVisited);
        void addConsumersToList(ProducerOperation* producer, 
            std::set<Operation*>& isVisited, std::set<Operation*>& wasAddedEarly);
        bool addPredecessorsToList(Operation* op,
            std::set<Operation*>& isVisited, std::set<Operation*>& wasAddedEarly);
        LoadOperation* insertMVMGuard(MVMOperation* lastMVM);

    public:

        Linearizer(ModelImpl* model, Partitioner* partitioner, Placer* placer);

        std::list<CoreOperation*>& getCoreOperationList(unsigned int pTile, unsigned int pCore);
        std::list<TileOperation*>& getTileOperationList(unsigned int pTile);

};

