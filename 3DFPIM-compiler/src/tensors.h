/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <iostream>
#include <string>
#include <vector>

#include "common.h"

class AbstractTensor {

    protected:

        ModelImpl* model_;
        std::string name_;

        AbstractTensor(ModelImpl* model, std::string name) : 
            model_(model), name_(name) {}

    public:

        ModelImpl* getModel() const { return model_; }
        std::string name() { return name_; }

        std::string printNodeName();
        virtual std::string printNodeStyle();
        virtual std::string printTensorType()=0;

};

class AbstractVector : public AbstractTensor {

    protected:

        unsigned int length_;

        AbstractVector
            (ModelImpl* model, std::string name, unsigned int length)
            : AbstractTensor(model, name), length_(length) {}

    public:

        void checkCompatibility(AbstractVector* v);

        unsigned int length() const { return length_; }
        unsigned int nTiles() { return (length_ - 1)/MVMU_DIM + 1; }

};

class AbstractMatrix : public AbstractTensor {

    protected:

        unsigned int width_;
        unsigned int height_;

        AbstractMatrix
            (ModelImpl* model, std::string name, 
            unsigned int width, unsigned int height)
            : AbstractTensor(model, name), width_(width), height_(height) {}

    public:

        unsigned int width() const { return width_; }
        unsigned int height() const { return height_; }

};

class AbstractImagePixelStream : public AbstractTensor {

    protected:

        unsigned int imageWidth_;
        unsigned int imageHeight_;
        unsigned int nChannels_;
        unsigned int kernelWidth_;
        unsigned int kernelHeight_;

    public:

        AbstractImagePixelStream
            (ModelImpl* model, std::string name, 
            unsigned int imageWidth, unsigned int imageHeight, 
            unsigned int kernelWidth, unsigned int kernelHeight,
            unsigned int nChannels)
            : AbstractTensor(model, name), 
            imageWidth_(imageWidth), imageHeight_(imageHeight), 
            kernelWidth_(kernelWidth), kernelHeight_(kernelHeight),
            nChannels_(nChannels) {}

        unsigned int imageWidth() { return imageWidth_; }
        unsigned int imageHeight() { return imageHeight_; }
        unsigned int nChannels() { return nChannels_; }
        unsigned int kernelWidth() { return kernelWidth_; }
        unsigned int kernelHeight() { return kernelHeight_; }
        unsigned int nTiles() {
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nChannels_ % MVMU_DIM != 0) { // Concatenate incomplete vectors
                unsigned int maxConcat = MVMU_DIM / (nChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else { // No incomplete vectors
                nConcatTile = 0;
            }

            return nCompleteTile + nConcatTile;
        }

        void checkCompatibility(AbstractImagePixelStream* vs);

        std::string printNodeName();
        virtual std::string printTensorType()=0;

};

class InputVectorTile : public AbstractVector {

    public:

        InputVectorTile
            (ModelImpl* model, std::string name, unsigned int length) 
            : AbstractVector(model, name, length) { }

        std::string printNodeStyle();
        std::string printTensorType();

};

class InputVectorImpl : public AbstractVector {

    protected:

        std::vector<InputVectorTile*> tiles_;

    public:

        InputVectorImpl
            (ModelImpl* model, std::string name, unsigned int length);
        ~InputVectorImpl();

        unsigned int nTiles() { return (length_ - 1)/MVMU_DIM + 1; }
        InputVectorTile* getTile(unsigned int t);

        std::string printNodeStyle();
        std::string printTensorType();
        void printNodeAndEdges(std::ostream& fout);

};

class InputImagePixelStreamTile : public AbstractImagePixelStream {

    protected:

        std::vector< std::vector<InputVectorTile*> > stream_;

    public:

        InputImagePixelStreamTile
            (ModelImpl* model, std::string name, 
            unsigned int imageWidth, unsigned int imageHeight, 
            unsigned int kernelWidth, unsigned int kernelHeight,
            unsigned int nChannels);
        ~InputImagePixelStreamTile();

        InputVectorTile* get(unsigned int h, unsigned int w);

        std::string printNodeStyle();
        std::string printTensorType();

};

class InputImagePixelStreamImpl : public AbstractImagePixelStream {

    protected:

        std::vector<InputImagePixelStreamTile*> tiles_;

    public:

        InputImagePixelStreamImpl
            (ModelImpl* model, std::string name, 
            unsigned int imageWidth, unsigned int imageHeight, 
            unsigned int kernelWidth, unsigned int kernelHeight,
            unsigned int nChannels);
        ~InputImagePixelStreamImpl();

        unsigned int nTiles() {
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nChannels_ % MVMU_DIM != 0) { // Concatenate incomplete vectors
                unsigned int maxConcat = MVMU_DIM / (nChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else { // No incomplete vectors
                nConcatTile = 0;
            }

            return nCompleteTile + nConcatTile;
        }
        InputImagePixelStreamTile* getTile(unsigned int t);

        std::string printNodeStyle();
        std::string printTensorType();
        void printNodeAndEdges(std::ostream& fout);

};

class VectorImpl : public AbstractVector {

    protected:

        std::vector<ProducerOperation*> tiles_;

    public:

        VectorImpl(ModelImpl* model, unsigned int length);

        unsigned int nTiles() { return (length_ - 1)/MVMU_DIM + 1; }
        ProducerOperation* getTile(unsigned int t);
        void setTile(unsigned int t, ProducerOperation* producer);

        std::string printTensorType();

};

class ImagePixelStreamTile : public AbstractImagePixelStream {

    protected:

        std::vector< std::vector<ProducerOperation*> > stream_;

    public:

        ImagePixelStreamTile
            (ModelImpl* model, 
            unsigned int imageWidth, unsigned int imageHeight, 
            unsigned int kernelWidth, unsigned int kernelHeight, 
            unsigned int nChannels);

        void add(unsigned int h, unsigned int w, ProducerOperation* vec);
        ProducerOperation* get(unsigned int h, unsigned int w);

        std::string printTensorType();

};

class ImagePixelStreamImpl : public AbstractImagePixelStream {

    protected:

        std::vector<ImagePixelStreamTile*> tiles_;

    public:

        ImagePixelStreamImpl
            (ModelImpl* model, 
            unsigned int imageWidth, unsigned int imageHeight, 
            unsigned int kernelWidth, unsigned int kernelHeight, 
            unsigned int nChannels);
        ~ImagePixelStreamImpl();

        unsigned int nTiles() {
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nChannels_ % MVMU_DIM != 0) {
                // Concatenate incomplete vectors in
                // (kernelWidth => kernelHeight => channel) order
                unsigned int maxConcat = MVMU_DIM / (nChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else { // No incomplete vectors
                nConcatTile = 0;
            }

            return nCompleteTile + nConcatTile;
        }
        ImagePixelStreamTile* getTile(unsigned int t);

        std::string printTensorType();

};

class OutputVectorTile : public AbstractVector {

    public:

        OutputVectorTile
            (ModelImpl* model, std::string name, unsigned int length)
            : AbstractVector(model, name, length) { }

        std::string printNodeStyle();
        std::string printTensorType();

};

class OutputVectorImpl : public AbstractVector {

    protected:

        std::vector<OutputVectorTile*> tiles_;

    public:

        OutputVectorImpl
            (ModelImpl* model, std::string name, unsigned int length);
        ~OutputVectorImpl();

        unsigned int nTiles() { return (length_ - 1)/MVMU_DIM + 1; }
        OutputVectorTile* getTile(unsigned int t);

        std::string printNodeStyle();
        std::string printTensorType();
        void printNodeAndEdges(std::ostream& fout);

};

class OutputImagePixelStreamTile : public AbstractImagePixelStream {

    protected:

        std::vector< std::vector<OutputVectorTile*> > stream_;

    public:

        OutputImagePixelStreamTile
            (ModelImpl* model, std::string name, 
            unsigned int imageWidth, unsigned int imageHeight, 
            unsigned int kernelWidth, unsigned int kernelHeight, 
            unsigned int nChannels);
        ~OutputImagePixelStreamTile();

        OutputVectorTile* get(unsigned int h, unsigned int w);

        std::string printNodeStyle();
        std::string printTensorType();

};

class OutputImagePixelStreamImpl : public AbstractImagePixelStream {

    protected:

        std::vector<OutputImagePixelStreamTile*> tiles_;

    public:

        OutputImagePixelStreamImpl
            (ModelImpl* model, std::string name, 
            unsigned int imageWidth, unsigned int imageHeight, 
            unsigned int kernelWidth, unsigned int kernelHeight, 
            unsigned int nChannels);
        ~OutputImagePixelStreamImpl();

        unsigned int nTiles() {
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nChannels_ % MVMU_DIM != 0) {
                // Concatenate incomplete vectors in
                // (kernelWidth => kernelHeight => channel) order
                unsigned int maxConcat = MVMU_DIM / (nChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else { // No incomplete vectors
                nConcatTile = 0;
            }

            return nCompleteTile + nConcatTile;
        }
        OutputImagePixelStreamTile* getTile(unsigned int t);

        std::string printNodeStyle();
        std::string printTensorType();
        void printNodeAndEdges(std::ostream& fout);

};

class ConstantMatrixTile : public AbstractMatrix {

    protected:

        std::vector<MVMOperation*> users_;
        int numTotalTiles_;

    public:

        std::set<MVMOperation*> mvm_sets;
        ConstantMatrixTile
            (ModelImpl* model, std::string name, unsigned int width, 
            unsigned int height, int numTotalTiles)
            : AbstractMatrix(model, name, width, height),
            numTotalTiles_(numTotalTiles) { }

        void addUser(MVMOperation* user) { users_.push_back(user); }
        unsigned int numUsers() { return users_.size(); }
        int getNTiles() { return numTotalTiles_; }
        MVMOperation* getUser(unsigned int i) { return users_[i]; }

        std::string printTensorType();

};

// MVMU for fully connected layer
class ConstantMatrixImpl : public AbstractMatrix {

    protected:

        std::vector< std::vector<ConstantMatrixTile*> > tiles_;

    public:

        ConstantMatrixImpl
            (ModelImpl* model, std::string name, 
            unsigned int width, unsigned int height);
        ~ConstantMatrixImpl();

        unsigned int nHeightTiles() { return (height_ - 1)/(MVMU_DIM) + 1; }
        unsigned int nWidthTiles() { return ((width_ - 1)/MVMU_DIM) / MVMU_DPT + 1; }
        unsigned int nWidthRest() { 
            unsigned int rest = ((width_ - 1)/(MVMU_DIM) + 1) % MVMU_DPT; 
            return rest != 0 ? rest : MVMU_DPT;
        }
        unsigned int nWidthDPT() { return MVMU_DPT; }
        ConstantMatrixTile* getTile(unsigned int h, unsigned int w);

        std::string printTensorType();

        void checkCompatibilityForMVM(AbstractVector* v);

};

// MVMU for convolution opertaions
class ConvolutionalConstantMatrixImpl : public AbstractTensor {

    protected:

        unsigned int kernelWidth_;
        unsigned int kernelHeight_;
        unsigned int nInChannels_;
        unsigned int nOutChannels_;
        unsigned int stride_;
        unsigned int outImageWidth_;
        unsigned int outImageHeight_;
        unsigned int duplicateWidth_;
        unsigned int duplicateHeight_;
        bool isPool_;
        std::vector<
            std::vector<
            std::vector<
            std::vector<ConstantMatrixTile*> > > >tiles_;

    public:

        ConvolutionalConstantMatrixImpl
            (ModelImpl* model, std::string name, 
            unsigned int kernelWidth, unsigned int kernelHeight, 
            unsigned int nInChannels, unsigned int nOutChannels,
            unsigned int stride, unsigned int outImageWidth, unsigned int outImageHeight,
            bool isPool);
        ~ConvolutionalConstantMatrixImpl();

        unsigned int getKernelWidth() { return kernelWidth_; }
        unsigned int getKernelHeight() { return kernelHeight_; }
        unsigned int getNInChannels() { return nInChannels_; }
        unsigned int getNOutChannels() { return nOutChannels_; }
        unsigned int getStride() { return stride_; }
        unsigned int getOutWidth() { return outImageWidth_; }
        unsigned int getOutHeight() { return outImageHeight_; }
        unsigned int getDuplicateWidth() { return duplicateWidth_; }
        unsigned int getDuplicateHeight() { return duplicateHeight_; }
        bool getIsPool() { return isPool_; }

        // # of input side 3D MVMU
        unsigned int getNInTiles() {
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nInChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nInChannels_ % MVMU_DIM != 0) {
                // Concatenate incomplete vectors in
                // (kernelWidth => kernelHeight => channel) order
                unsigned int maxConcat = MVMU_DIM / (nInChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else {
                nConcatTile = 0;
            }

            return (nCompleteTile + nConcatTile - 1) / MVMU_DPT + 1;
        }
        // # of stack used in MVMUs
        unsigned int getNInDPT() { return MVMU_DPT; }
        // # of stack used in the last MVMU
        unsigned int getNInRestDPT() { 
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nInChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nInChannels_ % MVMU_DIM != 0) {
                unsigned int maxConcat = MVMU_DIM / (nInChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else {
                nConcatTile = 0;
            }

            unsigned int nInRestDPT = (nCompleteTile + nConcatTile) % MVMU_DPT;
            return (nInRestDPT == 0) ? MVMU_DPT : nInRestDPT;
        }
        // # of output side 3D MVMU
        unsigned int getNOutTiles() { return (nOutChannels_ - 1)/MVMU_DIM + 1; }

        ConstantMatrixTile* getTile
            (unsigned int dh, unsigned int dw,
            unsigned int h, unsigned int w);
        void checkCompatibility(AbstractImagePixelStream* vs);

        std::string printTensorType();

};

// MVMU for fully connected operations after convolution layers
class FCConstantMatrixImpl : public AbstractTensor {

    protected:

        unsigned int nInChannels_;
        unsigned int nOutChannels_;
        unsigned int kernelWidth_;
        unsigned int kernelHeight_;
        std::vector<
            std::vector<ConstantMatrixTile*> >tiles_;

    public:

        FCConstantMatrixImpl
            (ModelImpl* model, std::string name, 
            unsigned int nInChannels, unsigned int nOutChannels,
            unsigned int kernelWidth_ = 1, unsigned int kernelHeight_ = 1);
        ~FCConstantMatrixImpl();

        unsigned int getKernelWidth() { return kernelWidth_; }
        unsigned int getKernelHeight() { return kernelHeight_; }
        unsigned int getNInChannels() { return nInChannels_; }
        unsigned int getNOutChannels() { return nOutChannels_; }

        unsigned int getNInTiles() {
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nInChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nInChannels_ % MVMU_DIM != 0) {
                unsigned int maxConcat = MVMU_DIM / (nInChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else {
                nConcatTile = 0;
            }

            return (nCompleteTile + nConcatTile - 1) / MVMU_DPT + 1;
        }
        unsigned int getNInDPT() { return MVMU_DPT; }
        unsigned int getNInRestDPT() { 
            unsigned int nCompleteTile, nConcatTile;

            nCompleteTile = (nInChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
            if(nInChannels_ % MVMU_DIM != 0) {
                unsigned int maxConcat = MVMU_DIM / (nInChannels_ % MVMU_DIM);
                nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
            } else {
                nConcatTile = 0;
            }

            unsigned int nInRestDPT = (nCompleteTile + nConcatTile) % MVMU_DPT;
            return (nInRestDPT == 0) ? MVMU_DPT : nInRestDPT;
        }
        unsigned int getNOutTiles() { return (nOutChannels_ - 1)/MVMU_DIM + 1; }

        ConstantMatrixTile* getTile(unsigned int h, unsigned int w);
        void checkCompatibility(AbstractImagePixelStream* vs);

        std::string printTensorType();

};

