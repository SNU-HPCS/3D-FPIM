/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <sys/time.h>

#include "common.h"

class ModelImpl {

    public:

        enum ModelType { UNSPECIALIZED, INFERENCE };

    private:

        std::string name_;
        ModelType modelType_;

        std::list<Layer*> layers_;
        std::vector<InputVectorImpl*> inputVectors_;
        std::vector<InputImagePixelStreamImpl*> inputImagePixelStreams_;
        std::vector<VectorImpl*> vectors_;
        std::vector<ImagePixelStreamImpl*> imagePixelStreams_;
        std::vector<OutputVectorImpl*> outputVectors_;
        std::vector<OutputImagePixelStreamImpl*> outputImagePixelStreams_;
        std::vector<ConstantMatrixImpl*> constantMatrices_;
        std::vector<ConvolutionalConstantMatrixImpl*> convolutionMatrices_;
        std::vector<FCConstantMatrixImpl*> fcMatrices_;
        std::list<Operation*> operations_;
        std::vector<std::vector<MergedMVMSet*>*> coalesceableMVMVectors_;

        Partitioner* partitioner_;
        Placer* placer_;
        MemoryAllocator* memoryAllocator_;
        Coalescer* coalescer_;
        Linearizer* linearizer_;
        RegisterAllocator* registerAllocator_;
        CodeGenerator* codeGenerator_;

        // Debug information
        void printGraph(std::string fileName);

    public:

        int op_count;
        ModelImpl(std::string name);
        ~ModelImpl();

        void addLayer(Layer* layer);
        void addInputVectorImpl(InputVectorImpl* vec);
        void addInputImagePixelStreamImpl(InputImagePixelStreamImpl* stream);
        void addVectorImpl(VectorImpl* vec);
        void addImagePixelStreamImpl(ImagePixelStreamImpl* stream);
        void addOutputVectorImpl(OutputVectorImpl* vec);
        void addOutputImagePixelStreamImpl(OutputImagePixelStreamImpl* stream);
        void addConstantMatrixImpl(ConstantMatrixImpl* mat);
        void addConvolutionalConstantMatrixImpl(ConvolutionalConstantMatrixImpl* mat);
        void addFCConstantMatrixImpl(FCConstantMatrixImpl* mat);
        int addOperation(Operation* op);
        void addCoalesceableMVMVector(std::vector<MergedMVMSet*>* coalesceableMVMVector);

        void unlink(std::list<Operation*>::iterator it);

        void loadBalance();
        void compile(CompilerOptions& options);

        std::string getName() { return name_; }
        ModelType getModelType() { return modelType_; }

        // Iterators
        std::list<Layer*>::iterator layer_begin() { return layers_.begin(); }
        std::list<Layer*>::iterator layer_end() { return layers_.end(); }
        std::list<Layer*>::reverse_iterator layer_rbegin() { return layers_.rbegin(); }
        std::list<Layer*>::reverse_iterator layer_rend() { return layers_.rend(); }
        std::list<Layer*>::iterator layer_erase_head() { return layers_.erase(layer_begin()); }

        std::vector<ConstantMatrixImpl*>::iterator const_mat_begin() { return constantMatrices_.begin(); }
        std::vector<ConstantMatrixImpl*>::iterator const_mat_end() { return constantMatrices_.end(); }
        std::vector<ConvolutionalConstantMatrixImpl*>::iterator conv_mat_begin() { return convolutionMatrices_.begin(); }
        std::vector<ConvolutionalConstantMatrixImpl*>::iterator conv_mat_end() { return convolutionMatrices_.end(); }
        std::vector<FCConstantMatrixImpl*>::iterator fc_mat_begin() { return fcMatrices_.begin(); }
        std::vector<FCConstantMatrixImpl*>::iterator fc_mat_end() { return fcMatrices_.end(); }
        std::list<Operation*>::iterator op_begin() { return operations_.begin(); }
        std::list<Operation*>::iterator op_end() { return operations_.end(); }

        // Debug information
        std::string printAssignment(Operation* op);

};

