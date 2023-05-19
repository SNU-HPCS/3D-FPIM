/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>

#include "3dfpim.h"

#include "coalescer.h"
#include "codegen.h"
#include "linearizer.h"
#include "memalloc.h"
#include "model.h"
#include "operations.h"
#include "partitioner.h"
#include "placer.h"
#include "regalloc.h"
#include "tensors.h"

Model Model::create(std::string name) {
    Model model;
    model.impl_ = new ModelImpl(name);
    return model;
}

void Model::destroy() {
    delete impl_;
}

ModelImpl* Model::unwrap() {
    return impl_;
}

void Model::loadBalance() {
    impl_->loadBalance();
}

void Model::compile(CompilerOptions options) {
    impl_->compile(options);
}

ModelImpl::ModelImpl(std::string name)
    : name_(name), modelType_(UNSPECIALIZED), 
    partitioner_(NULL), placer_(NULL), 
    memoryAllocator_(NULL), coalescer_(NULL), 
    linearizer_(NULL), registerAllocator_(NULL), 
    codeGenerator_(NULL), op_count(0)
{
}

ModelImpl::~ModelImpl() {
    if(partitioner_ != NULL) {
        delete partitioner_;
    }
    if(placer_ != NULL) {
        delete placer_;
    }
    if(memoryAllocator_ != NULL) {
        delete memoryAllocator_;
    }
    if(coalescer_ != NULL) {
        delete coalescer_;
    }
    if(linearizer_ != NULL) {
        delete linearizer_;
    }
    if(registerAllocator_ != NULL) {
        delete registerAllocator_;
    }
    if(codeGenerator_ != NULL) {
        delete codeGenerator_;
    }
    for(InputVectorImpl* vec : inputVectors_) {
        delete vec;
    }
    for(InputImagePixelStreamImpl* stream : inputImagePixelStreams_) {
        delete stream;
    }
    for(VectorImpl* vec : vectors_) {
        delete vec;
    }
    for(ImagePixelStreamImpl* stream : imagePixelStreams_) {
        delete stream;
    }
    for(OutputVectorImpl* vec : outputVectors_) {
        delete vec;
    }
    for(OutputImagePixelStreamImpl* stream : outputImagePixelStreams_) {
        delete stream;
    }
    for(ConstantMatrixImpl* matrix : constantMatrices_) {
        delete matrix;
    }
    for(ConvolutionalConstantMatrixImpl* matrix : convolutionMatrices_) {
        delete matrix;
    }
    for(FCConstantMatrixImpl* matrix : fcMatrices_) {
        delete matrix;
    }
    for(Operation* op : operations_) {
        delete op;
    }
    for(auto coalesceableMVMVector : coalesceableMVMVectors_) {
        delete coalesceableMVMVector;
    }
}

void ModelImpl::addLayer(Layer* layer) {
    layers_.push_back(layer);
}

void ModelImpl::addInputVectorImpl(InputVectorImpl* vec) {
    inputVectors_.push_back(vec);
}

void ModelImpl::addInputImagePixelStreamImpl(InputImagePixelStreamImpl* stream) {
    inputImagePixelStreams_.push_back(stream);
}

void ModelImpl::addVectorImpl(VectorImpl* vec) {
    vectors_.push_back(vec);
}

void ModelImpl::addImagePixelStreamImpl(ImagePixelStreamImpl* stream) {
    imagePixelStreams_.push_back(stream);
}

void ModelImpl::addOutputVectorImpl(OutputVectorImpl* vec) {
    outputVectors_.push_back(vec);
}

void ModelImpl::addOutputImagePixelStreamImpl(OutputImagePixelStreamImpl* stream) {
    outputImagePixelStreams_.push_back(stream);
}

void ModelImpl::addConstantMatrixImpl(ConstantMatrixImpl* mat) {
    if(modelType_ == UNSPECIALIZED) {
        modelType_ = INFERENCE;
    } else {
        assert(modelType_ == INFERENCE && "Cannot mix inference and training matrices in the same model");
    }
    constantMatrices_.push_back(mat);
}

void ModelImpl::addConvolutionalConstantMatrixImpl(ConvolutionalConstantMatrixImpl* mat) {
    if(modelType_ == UNSPECIALIZED) {
        modelType_ = INFERENCE;
    } else {
        assert(modelType_ == INFERENCE && "Cannot mix inference and training matrices in the same model");
    }
    convolutionMatrices_.push_back(mat);
}

void ModelImpl::addFCConstantMatrixImpl(FCConstantMatrixImpl* mat) {
    if(modelType_ == UNSPECIALIZED) {
        modelType_ = INFERENCE;
    } else {
        assert(modelType_ == INFERENCE && "Cannot mix inference and training matrices in the same model");
    }
    fcMatrices_.push_back(mat);
}

int ModelImpl::addOperation(Operation* op) {
    operations_.push_back(op);
    return op_count++;
}

void ModelImpl::addCoalesceableMVMVector(std::vector<MergedMVMSet*>* coalesceableMVMVector) {
    coalesceableMVMVectors_.push_back(coalesceableMVMVector);
}

void ModelImpl::unlink(std::list<Operation*>::iterator it) {
    delete *it;
    operations_.erase(it);
}

void ModelImpl::printGraph(std::string fileName) {
    std::ofstream fout;
    fout.open(fileName);
    fout << "digraph model {" << std::endl;
    for(InputVectorImpl* vec : inputVectors_) {
        vec->printNodeAndEdges(fout);
    }
    for(InputImagePixelStreamImpl* stream : inputImagePixelStreams_) {
        stream->printNodeAndEdges(fout);
    }
    for(OutputVectorImpl* vec : outputVectors_) {
        vec->printNodeAndEdges(fout);
    }
    for(OutputImagePixelStreamImpl* stream : outputImagePixelStreams_) {
        stream->printNodeAndEdges(fout);
    }
    for(Operation* op : operations_) {
        op->printNodeAndEdges(fout);
    }
    fout << "}" << std::endl;
    fout.close();
}

std::string ModelImpl::printAssignment(Operation* op) {
    std::stringstream ss;
    if(partitioner_ != NULL) {
        ss << partitioner_->printAssignment(op);
    }
    if(placer_ != NULL) {
        ss << placer_->printAssignment(op);
    }
    if(memoryAllocator_ != NULL) {
        ss << memoryAllocator_->printAssignment(op);
    }
    if(registerAllocator_ != NULL) {
        ss << registerAllocator_->printAssignment(op);
    }
    return ss.str();
}

void ModelImpl::loadBalance() 
{
    bool load_balanced = false;

    for (auto r_it = layer_rbegin(); r_it != layer_rend(); ++r_it) {
        Layer* r_layer = *r_it;

        unsigned int scale_load = r_layer->load_;
        unsigned int total_mvmu_num = 0;
        bool all_layer_scaled = true;

        for (auto it = layer_begin(); it != layer_end(); ++it) {
            Layer* layer = *it;

            unsigned int duplicateWidth;
            unsigned int duplicateHeight;

            if (layer->isFC_) {
                duplicateWidth = 1;
                duplicateHeight = 1;
            }
            else {
                duplicateWidth = ceil(sqrt((double)layer->load_ / scale_load));
                duplicateHeight = ceil(sqrt((double)layer->load_ / scale_load));
            }

            if (!layer->setDuplicate(duplicateWidth, duplicateHeight)) {
                all_layer_scaled = false;
                break;
            }

            total_mvmu_num += duplicateWidth * duplicateHeight * layer->getNMVMU();
        }

        unsigned int total_tile_num = (total_mvmu_num - 1)
                                        / N_CONSTANT_MVMUS_PER_CORE
                                        / N_CORES_PER_TILE;

        if (all_layer_scaled && total_tile_num <= N_MAX_TILE) {
            load_balanced = true;
            break;
        }
    }

    if (!load_balanced) {
        assert(false && "Load balancing failed");
    }
}

void ModelImpl::compile(CompilerOptions& options) {

    if(options.printDebugInfo_) {
        printGraph(name_ + "-graph0.dot");
    }

    // Model partitioning
    std::cout << "Partitioning graph... " << std::flush;
    partitioner_ = new Partitioner(this, options.gp_);
    std::cout << "done." << std::endl;
    if(options.printDebugInfo_) {
        printGraph(name_ + "-graph1-partitioned.dot");
    }

    // Physical layout
    std::cout << "Physical layout... " << std::flush;
    placer_ = new Placer(this, partitioner_);
    std::cout << "done." << std::endl;
    if(options.printDebugInfo_) {
        printGraph(name_ + "-graph2-virtual-to-physical.dot");
    }

    // Memory allocation
    std::cout << "Memory allocation... " << std::flush;
    memoryAllocator_ = new MemoryAllocator(this, partitioner_);
    std::cout << "done." << std::endl;
    if(options.printDebugInfo_) {
        printGraph(name_ + "-graph3-memory-allocation.dot");
    }

    // Coalescing
    if(options.coalesceMVMOperations_) {
        std::cout << "MVM coalescing... " << std::flush;
        coalescer_ = new Coalescer(this, placer_, coalesceableMVMVectors_);
        std::cout << "done." << std::endl;
    }

    // Linearization
    std::cout << "Linearizing graph... " << std::flush;
    linearizer_ = new Linearizer(this, partitioner_, placer_);
    std::cout << "done." << std::endl;
    if(options.printDebugInfo_) {
        printGraph(name_ + "-graph4-linearization.dot");
    }

    // Register allocation
    std::cout << "Register allocation... " << std::flush;
    registerAllocator_ = new RegisterAllocator
        (this, partitioner_, placer_, memoryAllocator_, linearizer_);
    std::cout << "done." << std::endl;
    if(options.printDebugInfo_) {
        printGraph(name_ + "-graph5-register-allocation.dot");
    }

    // Code generation
    std::cout << "Code generation... " << std::flush;
    codeGenerator_ = new CodeGenerator
        (this, placer_, memoryAllocator_, coalescer_, 
        linearizer_, registerAllocator_);
    std::cout << "done." << std::endl;

    // Report
    std::ofstream report(name_ + "-report.out");
    partitioner_->printReport(report);
    registerAllocator_->printReport(report);
    report.close();

}

