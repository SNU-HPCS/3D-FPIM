/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <assert.h>
#include <sstream>

#include "model.h"
#include "common.h"
#include "operations.h"
#include "tensors.h"

Layer::Layer
    (Model model,
    unsigned int kernelWidth, unsigned int kernelHeight,
    unsigned int inImageWidth, unsigned int inImageHeight,
    unsigned int nInChannels, unsigned int outImageWidth,
    unsigned int outImageHeight, unsigned int nOutChannels,
    unsigned int DACScaling, bool isFC)
    : model_(model),
    kernelWidth_(kernelWidth),
    kernelHeight_(kernelHeight),
    inImageWidth_(inImageWidth),
    inImageHeight_(inImageHeight),
    nInChannels_(nInChannels),
    outImageWidth_(outImageWidth),
    outImageHeight_(outImageHeight),
    nOutChannels_(nOutChannels),
    isFC_(isFC)
{
    unsigned int nCompleteTile, nConcatTile;

    nCompleteTile = (nInChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
    if(nInChannels_ % MVMU_DIM != 0) {
        unsigned int maxConcat = MVMU_DIM / (nInChannels_ % MVMU_DIM);
        nConcatTile = (kernelWidth_ * kernelHeight_ - 1) / maxConcat + 1;
    } else {
        nConcatTile = 0;
    }

    nStack_ = std::min(nCompleteTile + nConcatTile, (unsigned int)MVMU_DPT);

    load_ = outImageWidth * outImageHeight
            * ((nStack_ - 1) * (STACK_SHIFT_LATENCY + PRECHARGE_LATENCY * DACScaling)
            + STACK_REUSE_LATENCY + PRECHARGE_LATENCY * DACScaling
            + ADC_LATENCY);

    model.unwrap()->addLayer(this);
};

unsigned int Layer::getNMVMU()
{
    unsigned int nInMVMUs = (nStack_ - 1) / MVMU_DPT + 1;
    unsigned int nOutMVMUs = (nOutChannels_ - 1) / MVMU_DIM + 1;

    return nInMVMUs * nOutMVMUs;
}

int Layer::setDuplicate
    (unsigned int duplicateWidth,
    unsigned int duplicateHeight)
{
    if (isFC_) {
        duplicateWidth_ = 1;
        duplicateHeight_ = 1;
    } else {
        duplicateWidth_ = duplicateWidth;
        duplicateHeight_ = duplicateHeight;
    }

    if(duplicateWidth_ > outImageWidth_
        || duplicateHeight_ > outImageHeight_) {
        return 0;
    }

    return 1;
}

InputVector InputVector::create
    (Model model, std::string name, unsigned int length) {

    InputVector vec;
    vec.impl_ = new InputVectorImpl(model.unwrap(), name, length);
    return vec;
}

InputImagePixelStream InputImagePixelStream::create
    (Model model, std::string name, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int nChannels) {

    InputImagePixelStream stream;
    stream.impl_ = new InputImagePixelStreamImpl
        (model.unwrap(), name, imageWidth, imageHeight, 1, 1, nChannels);
    return stream;
}

OutputVector OutputVector::create
    (Model model, std::string name, unsigned int length) {

    OutputVector vec;
    vec.impl_ = new OutputVectorImpl(model.unwrap(), name, length);
    return vec;
}

OutputImagePixelStream OutputImagePixelStream::create
    (Model model, std::string name, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int nChannels) {

    OutputImagePixelStream stream;
    stream.impl_ = new OutputImagePixelStreamImpl
        (model.unwrap(), name, imageWidth, imageHeight, 1, 1, nChannels);
    return stream;
}

ConstantMatrix ConstantMatrix::create
    (Model model, std::string name, 
    unsigned int width, unsigned int height) {

    ConstantMatrix m;
    m.impl_ = new ConstantMatrixImpl(model.unwrap(), name, width, height);
    return m;
}

ConvolutionalConstantMatrix ConvolutionalConstantMatrix::create
    (Model model, std::string name, 
    unsigned int kernelWidth, unsigned int kernelHeight, 
    unsigned int nInChannels, unsigned int nOutChannels,
    unsigned int stride, unsigned int outImageWidth, unsigned int outImageHeight,
    bool isPool) {

    ConvolutionalConstantMatrix m;
    m.impl_ = new ConvolutionalConstantMatrixImpl
        (model.unwrap(), name, 
        kernelWidth, kernelHeight, 
        nInChannels, nOutChannels, 
        stride, outImageWidth, outImageHeight,
        isPool);
    return m;
}

FCConstantMatrix FCConstantMatrix::create
    (Model model, std::string name, 
    unsigned int nInChannels, unsigned int nOutChannels,
    unsigned int kernelWidth, unsigned int kernelHeight) {

    FCConstantMatrix m;
    m.impl_ = new FCConstantMatrixImpl
        (model.unwrap(), name, 
        nInChannels, nOutChannels,
        kernelWidth, kernelHeight);
    return m;
}

Vector::Vector(VectorImpl* impl)
    : impl_(impl)
{ }

ImagePixelStream::ImagePixelStream(ImagePixelStreamImpl* impl)
    : impl_(impl)
{ }

OuterProduct::OuterProduct(Vector x1, Vector x2)
    : x1_(x1.unwrap()), x2_(x2.unwrap())
{ }

InputVectorImpl* InputVector::unwrap() {
    return impl_;
}

InputImagePixelStreamImpl* InputImagePixelStream::unwrap() {
    return impl_;
}

OutputVectorImpl* OutputVector::unwrap() {
    return impl_;
}

OutputImagePixelStreamImpl* OutputImagePixelStream::unwrap() {
    return impl_;
}

VectorImpl* Vector::unwrap() {
    return impl_;
}

ImagePixelStreamImpl* ImagePixelStream::unwrap() {
    return impl_;
}

ConstantMatrixImpl* ConstantMatrix::unwrap() {
    return impl_;
}

ConvolutionalConstantMatrixImpl* ConvolutionalConstantMatrix::unwrap() {
    return impl_;
}

FCConstantMatrixImpl* FCConstantMatrix::unwrap() {
    return impl_;
}

VectorImpl* OuterProduct::unwrap1() {
    return x1_;
}

VectorImpl* OuterProduct::unwrap2() {
    return x2_;
}

InputVectorImpl::InputVectorImpl
    (ModelImpl* model, std::string name, unsigned int length)
    : AbstractVector(model, name, length)
{
    tiles_.resize(nTiles());
    for(unsigned int i = 0; i < nTiles(); ++i) {
        unsigned int tileSize = MVMU_DIM;
        if(i == nTiles() - 1 && length%MVMU_DIM > 0) {
            tileSize = length%MVMU_DIM;
        }
        tiles_[i] = new InputVectorTile
            (model, name + "[" + std::to_string(i) + "]", tileSize);
    }
    model->addInputVectorImpl(this);
}

InputImagePixelStreamTile::InputImagePixelStreamTile
    (ModelImpl* model, std::string name, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int kernelWidth, unsigned int kernelHeight,
    unsigned int nChannels)
    : AbstractImagePixelStream
    (model, name, 
    imageWidth, imageHeight, 
    kernelWidth, kernelHeight, nChannels)
{
    stream_.resize(imageHeight);
    for(unsigned int h = 0; h < imageHeight; ++h) {
        stream_[h].resize(imageWidth);
        for(unsigned int w = 0; w < imageWidth; ++w) {
            stream_[h][w] = new InputVectorTile
                (model, name + "[" + std::to_string(h) + "][" + std::to_string(w) + "]",
                nChannels);
        }
    }
}

InputImagePixelStreamImpl::InputImagePixelStreamImpl
    (ModelImpl* model, std::string name, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int kernelWidth, unsigned int kernelHeight,
    unsigned int nChannels)
    : AbstractImagePixelStream
    (model, name, 
    imageWidth, imageHeight, 
    kernelWidth, kernelHeight, nChannels)
{
    tiles_.resize(nTiles());
    unsigned int nCompleteTile = (nChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
    for(unsigned int i = 0; i < nTiles(); ++i) {
        unsigned int tileSize;
        if(i < nCompleteTile) {
            tileSize = MVMU_DIM;
        } else {
            unsigned int maxConcat = MVMU_DIM / (nChannels_ % MVMU_DIM);
            tileSize = maxConcat * (nChannels_ % MVMU_DIM);
            if(i == nTiles() - 1 && (kernelWidth_ * kernelHeight_) % maxConcat > 0) {
                tileSize = ((kernelWidth_ * kernelHeight_) % maxConcat)
                            * (nChannels_ % MVMU_DIM);
            }
        }
        tiles_[i] = new InputImagePixelStreamTile
            (model, name + "[" + std::to_string(i) + "]", 
            imageWidth, imageHeight, 
            kernelWidth, kernelHeight,
            tileSize);
    }
    model->addInputImagePixelStreamImpl(this);
}

OutputVectorImpl::OutputVectorImpl
    (ModelImpl* model, std::string name, unsigned int length)
    : AbstractVector(model, name, length)
{
    tiles_.resize(nTiles());
    for(unsigned int i = 0; i < nTiles(); ++i) {
        unsigned int tileSize = MVMU_DIM;
        if(i == nTiles() - 1 && length%MVMU_DIM > 0) {
            tileSize = length%MVMU_DIM;
        }
        tiles_[i] = new OutputVectorTile
            (model, name + "[" + std::to_string(i) + "]", tileSize);
    }
    model->addOutputVectorImpl(this);
}

OutputImagePixelStreamTile::OutputImagePixelStreamTile
    (ModelImpl* model, std::string name, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int kernelWidth, unsigned int kernelHeight,
    unsigned int nChannels)
    : AbstractImagePixelStream
    (model, name, 
    imageWidth, imageHeight, 
    kernelWidth, kernelHeight, nChannels)
{
    stream_.resize(imageHeight);
    for(unsigned int h = 0; h < imageHeight; ++h) {
        stream_[h].resize(imageWidth);
        for(unsigned int w = 0; w < imageWidth; ++w) {
            stream_[h][w] = new OutputVectorTile
                (model, name + "[" + std::to_string(h) + "][" + std::to_string(w) + "]",
                nChannels);
        }
    }
}

OutputImagePixelStreamImpl::OutputImagePixelStreamImpl
    (ModelImpl* model, std::string name, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int kernelWidth, unsigned int kernelHeight, 
    unsigned int nChannels)
    : AbstractImagePixelStream
    (model, name, 
    imageWidth, imageHeight, 
    kernelWidth, kernelHeight, nChannels)
{
    tiles_.resize(nTiles());
    unsigned int nCompleteTile = (nChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
    for(unsigned int i = 0; i < nTiles(); ++i) {
        unsigned int tileSize;
        if(i < nCompleteTile) {
            tileSize = MVMU_DIM;
        } else {
            unsigned int maxConcat = MVMU_DIM / (nChannels_ % MVMU_DIM);
            tileSize = maxConcat * (nChannels_ % MVMU_DIM);
            if(i == nTiles() - 1 && (kernelWidth_ * kernelHeight_) % maxConcat > 0) {
                tileSize = ((kernelWidth_ * kernelHeight_) % maxConcat)
                            * (nChannels_ % MVMU_DIM);
            }
        }
        tiles_[i] = new OutputImagePixelStreamTile
            (model, name + "[" + std::to_string(i) + "]", 
            imageWidth, imageHeight, 
            kernelWidth, kernelHeight,
            tileSize);
    }
    model->addOutputImagePixelStreamImpl(this);
}

ConstantMatrixImpl::ConstantMatrixImpl
    (ModelImpl* model, std::string name, 
    unsigned int width, unsigned int height)
    : AbstractMatrix(model, name, width, height)
{
    tiles_.resize(nHeightTiles());
    for(unsigned int h = 0; h < nHeightTiles(); ++h) {
        unsigned int tileHeight = MVMU_DIM;
        if(h == nHeightTiles() - 1 && height%MVMU_DIM > 0) {
            tileHeight = height%MVMU_DIM;
        }
        tiles_[h].resize(nWidthTiles());
        for(unsigned int w = 0; w < nWidthTiles(); ++w) {
            unsigned int tileWidth = MVMU_DIM * MVMU_DPT;
            if(w == nWidthTiles() - 1 && width%(MVMU_DIM * MVMU_DPT) > 0) {
                tileWidth = width%(MVMU_DIM * MVMU_DPT);
            }
            tiles_[h][w] = 
                new ConstantMatrixTile
                (model, name + "[" + std::to_string(h) + "][" + std::to_string(w) + "]",
                tileWidth, tileHeight, nHeightTiles() * nWidthTiles());
        }
    }
    model->addConstantMatrixImpl(this);
}

ConvolutionalConstantMatrixImpl::ConvolutionalConstantMatrixImpl
    (ModelImpl* model, std::string name, 
    unsigned int kernelWidth, unsigned int kernelHeight, 
    unsigned int nInChannels, unsigned int nOutChannels,
    unsigned int stride, unsigned int outImageWidth, unsigned int outImageHeight,
    bool isPool)
    : AbstractTensor(model, name), 
    kernelWidth_(kernelWidth), kernelHeight_(kernelHeight), 
    nInChannels_(nInChannels), nOutChannels_(nOutChannels),
    stride_(stride), outImageWidth_(outImageWidth), outImageHeight_(outImageHeight),
    isPool_(isPool)
{
    auto it = model->layer_begin();
    Layer* layer = *it;

    assert(layer != NULL);
    duplicateWidth_ = layer->duplicateWidth_;
    duplicateHeight_ = layer->duplicateHeight_;
    model->layer_erase_head();

    tiles_.resize(getDuplicateHeight());
    for(unsigned int dh = 0; dh < getDuplicateHeight(); ++dh) {
        tiles_[dh].resize(getDuplicateWidth());
        for(unsigned int dw = 0; dw < getDuplicateWidth(); ++dw) {
            tiles_[dh][dw].resize(getNOutTiles());
            for(unsigned int h = 0; h < getNOutTiles(); h++){
                unsigned int tileHeight = MVMU_DIM;
                if(h == getNOutTiles() - 1 && nOutChannels%MVMU_DIM > 0) {
                    tileHeight = nOutChannels%MVMU_DIM;
                }

                tiles_[dh][dw][h].resize(getNInTiles());
                for(unsigned int w = 0; w < getNInTiles(); w++){
                    unsigned int tileWidth = nInChannels * kernelWidth * kernelHeight;
                    tiles_[dh][dw][h][w] = 
                        new ConstantMatrixTile
                        (model, name + "[" + 
                        std::to_string(dh) + "][" + 
                        std::to_string(dw) + "][" + 
                        std::to_string(h) + "][" + 
                        std::to_string(w) + "]", tileWidth, tileHeight,
                        getDuplicateHeight() * getDuplicateWidth() * getNOutTiles() * getNInTiles());
                }
            }
        }
    }

    model->addConvolutionalConstantMatrixImpl(this);
}

FCConstantMatrixImpl::FCConstantMatrixImpl
    (ModelImpl* model, std::string name, 
    unsigned int nInChannels, unsigned int nOutChannels,
    unsigned int kernelWidth, unsigned int kernelHeight)
    : AbstractTensor(model, name), 
    nInChannels_(nInChannels), nOutChannels_(nOutChannels),
    kernelWidth_(kernelWidth), kernelHeight_(kernelHeight)
{
    auto it = model->layer_begin();
    Layer* layer = *it;

    assert(layer != NULL);
    assert(layer->duplicateWidth_ == 1);
    assert(layer->duplicateHeight_ == 1);
    model->layer_erase_head();

    tiles_.resize(getNOutTiles());
    for(unsigned int h = 0; h < getNOutTiles(); h++){
        unsigned int tileHeight = MVMU_DIM;
        if(h == getNOutTiles() - 1 && nOutChannels%MVMU_DIM > 0) {
            tileHeight = nOutChannels%MVMU_DIM;
        }
        tiles_[h].resize(getNInTiles());
        for(unsigned int w = 0; w < getNInTiles(); w++){
            unsigned int tileWidth = nInChannels * kernelWidth * kernelHeight;
            tiles_[h][w] = 
                new ConstantMatrixTile
                (model, name + "[" + 
                std::to_string(h) + "][" + 
                std::to_string(w) + "]", tileWidth, tileHeight,
                getNOutTiles() * getNInTiles());
        }
    }

    model->addFCConstantMatrixImpl(this);
}

VectorImpl::VectorImpl(ModelImpl* model, unsigned int length)
    : AbstractVector(model, "", length), tiles_((length - 1)/MVMU_DIM + 1)
{
    model->addVectorImpl(this);
}

ImagePixelStreamTile::ImagePixelStreamTile
    (ModelImpl* model, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int kernelWidth, unsigned int kernelHeight, 
    unsigned int nChannels)
    : AbstractImagePixelStream
    (model, "",
    imageWidth, imageHeight, 
    kernelWidth, kernelHeight, nChannels)
{
    stream_.resize(imageHeight);
    for(unsigned int h = 0; h < imageHeight; ++h) {
        stream_[h].resize(imageWidth);
    }
}

ImagePixelStreamImpl::ImagePixelStreamImpl
    (ModelImpl* model, 
    unsigned int imageWidth, unsigned int imageHeight, 
    unsigned int kernelWidth, unsigned int kernelHeight, 
    unsigned int nChannels)
    : AbstractImagePixelStream
    (model, "", 
    imageWidth, imageHeight, 
    kernelWidth, kernelHeight, nChannels)
{
    tiles_.resize(nTiles());
    unsigned int nCompleteTile = (nChannels_ / MVMU_DIM) * kernelWidth_ * kernelHeight_;
    for(unsigned int i = 0; i < nTiles(); ++i) {
        unsigned int tileSize;
        if(i < nCompleteTile) {
            tileSize = MVMU_DIM;
        } else {
            unsigned int maxConcat = MVMU_DIM / (nChannels_ % MVMU_DIM);
            tileSize = maxConcat * (nChannels_ % MVMU_DIM);
            if(i == nTiles() - 1 && (kernelWidth_ * kernelHeight_) % maxConcat > 0) {
                tileSize = ((kernelWidth_ * kernelHeight_) % maxConcat)
                            * (nChannels_ % MVMU_DIM);
            }
        }
        tiles_[i] = new ImagePixelStreamTile
            (model, imageWidth, imageHeight, 
            kernelWidth, kernelHeight, 
            tileSize);
    }
    model->addImagePixelStreamImpl(this);
}

void AbstractVector::checkCompatibility(AbstractVector* v) {
    assert(model_ == v->model_);
    assert(length_ == v->length_);
}

void AbstractImagePixelStream::checkCompatibility(AbstractImagePixelStream* vs) {
    assert(model_ == vs->model_);
    assert(imageWidth_ == vs->imageWidth_);
    assert(imageHeight_ == vs->imageHeight_);
    assert(nChannels_ == vs->nChannels_);
}

void ConstantMatrixImpl::checkCompatibilityForMVM(AbstractVector* v) {
    assert(model_ == v->getModel());
    assert(width_ == v->length());
}

void ConvolutionalConstantMatrixImpl::checkCompatibility(AbstractImagePixelStream* vs) {
    assert(model_ == vs->getModel());
    assert(nInChannels_ == vs->nChannels());
}

void FCConstantMatrixImpl::checkCompatibility(AbstractImagePixelStream* vs) {
    assert(model_ == vs->getModel());
    assert(nInChannels_ == vs->nChannels());
}

InputVectorTile* InputVectorImpl::getTile(unsigned int t) {
    assert(tiles_[t] != NULL);
    return tiles_[t];
}

InputImagePixelStreamTile* InputImagePixelStreamImpl::getTile(unsigned int t) {
    assert(tiles_[t] != NULL);
    return tiles_[t];
}

ProducerOperation* VectorImpl::getTile(unsigned int t) {
    assert(tiles_[t] != NULL);
    return tiles_[t];
}

ImagePixelStreamTile* ImagePixelStreamImpl::getTile(unsigned int t) {
    assert(tiles_[t] != NULL);
    return tiles_[t];
}

OutputVectorTile* OutputVectorImpl::getTile(unsigned int t) {
    assert(tiles_[t] != NULL);
    return tiles_[t];
}

OutputImagePixelStreamTile* OutputImagePixelStreamImpl::getTile(unsigned int t) {
    assert(tiles_[t] != NULL);
    return tiles_[t];
}

ConstantMatrixTile* ConstantMatrixImpl::getTile(unsigned int h, unsigned int w) {
    assert(tiles_[h][w] != NULL);
    return tiles_[h][w];
}

ConstantMatrixTile* ConvolutionalConstantMatrixImpl::getTile
    (unsigned int dh, unsigned int dw, unsigned int h, unsigned int w) {

    assert(tiles_[dh][dw][h][w] != NULL);
    return tiles_[dh][dw][h][w];
}

ConstantMatrixTile* FCConstantMatrixImpl::getTile
    (unsigned int h, unsigned int w) {

    assert(tiles_[h][w] != NULL);
    return tiles_[h][w];
}

void VectorImpl::setTile(unsigned int t, ProducerOperation* producer) {
    assert(tiles_[t] == NULL && "Cannot reassign vector tile");
    tiles_[t] = producer;
}

void ImagePixelStreamTile::add(unsigned int h, unsigned int w, ProducerOperation* vec) {
    assert(vec->length() == nChannels());
    stream_[h][w] = vec;
}

InputVectorTile* InputImagePixelStreamTile::get(unsigned int h, unsigned int w) {
    return stream_[h][w];
}

ProducerOperation* ImagePixelStreamTile::get(unsigned int h, unsigned int w) {
    return stream_[h][w];
}

OutputVectorTile* OutputImagePixelStreamTile::get(unsigned int h, unsigned int w) {
    return stream_[h][w];
}

InputVectorImpl::~InputVectorImpl() {
    for(InputVectorTile* tile : tiles_) {
        delete tile;
    }
}

InputImagePixelStreamTile::~InputImagePixelStreamTile() {
    for(auto it : stream_) {
        for(InputVectorTile* tile : it) {
            delete tile;
        }
    }
}

InputImagePixelStreamImpl::~InputImagePixelStreamImpl() {
    for(InputImagePixelStreamTile* tile : tiles_) {
        delete tile;
    }
}

ImagePixelStreamImpl::~ImagePixelStreamImpl() {
    for(ImagePixelStreamTile* tile : tiles_) {
        delete tile;
    }
}

OutputVectorImpl::~OutputVectorImpl() {
    for(OutputVectorTile* tile : tiles_) {
        delete tile;
    }
}

OutputImagePixelStreamTile::~OutputImagePixelStreamTile() {
    for(auto it : stream_) {
        for(OutputVectorTile* tile : it) {
            delete tile;
        }
    }
}

OutputImagePixelStreamImpl::~OutputImagePixelStreamImpl() {
    for(OutputImagePixelStreamTile* tile : tiles_) {
        delete tile;
    }
}

ConstantMatrixImpl::~ConstantMatrixImpl() {
    for(auto tileRow : tiles_) {
        for(ConstantMatrixTile* tile : tileRow) {
            delete tile;
        }
    }
}

ConvolutionalConstantMatrixImpl::~ConvolutionalConstantMatrixImpl() {
    for(auto duplicateRow : tiles_) {
        for(auto duplicateColumn : duplicateRow) {
            for(auto kernelOut : duplicateColumn) {
                for(ConstantMatrixTile* tile : kernelOut) {
                    delete tile;
                }
            }
        }
    }
}

FCConstantMatrixImpl::~FCConstantMatrixImpl() {
    for(auto kernelOut : tiles_) {
        for(ConstantMatrixTile* tile : kernelOut) {
            delete tile;
        }
    }
}

std::string AbstractTensor::printNodeName() {
    std::stringstream ss;
    ss << '"' << printTensorType() << "\n" << name_ << '"';
    return ss.str();
}

std::string AbstractImagePixelStream::printNodeName() {
    std::stringstream ss;
    ss << '"' << printTensorType() << "\n" << name_ << '"';
    return ss.str();
}

std::string AbstractTensor::printNodeStyle() {
    return "";
}

std::string InputVectorTile::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#66CCFF\"]";
}

std::string InputVectorImpl::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#3399FF\"]";
}

std::string InputImagePixelStreamTile::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#3399FF\"]";
}

std::string InputImagePixelStreamImpl::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#3399FF\"]";
}

std::string OutputVectorTile::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#66CCFF\"]";
}

std::string OutputVectorImpl::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#3399FF\"]";
}

std::string OutputImagePixelStreamTile::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#3399FF\"]";
}

std::string OutputImagePixelStreamImpl::printNodeStyle() {
    return "[shape=box,style=filled,fillcolor=\"#3399FF\"]";
}

std::string InputVectorTile::printTensorType() {
    return "InputVectorTile";
}

std::string InputVectorImpl::printTensorType() {
    return "InputVector";
}

std::string InputImagePixelStreamTile::printTensorType() {
    return "InputImagePixelStreamTile";
}

std::string InputImagePixelStreamImpl::printTensorType() {
    return "InputImagePixelStream";
}

std::string VectorImpl::printTensorType() {
    return "Vector";
}

std::string ImagePixelStreamTile::printTensorType() {
    return "ImagePixelStreamTile";
}

std::string ImagePixelStreamImpl::printTensorType() {
    return "ImagePixelStream";
}

std::string OutputVectorTile::printTensorType() {
    return "OutputVectorTile";
}

std::string OutputVectorImpl::printTensorType() {
    return "OutputVector";
}

std::string OutputImagePixelStreamTile::printTensorType() {
    return "OutputImagePixelStreamTile";
}

std::string OutputImagePixelStreamImpl::printTensorType() {
    return "OutputStreamVector";
}

std::string ConstantMatrixTile::printTensorType() {
    return "ConstantMatrixTile";
}

std::string ConstantMatrixImpl::printTensorType() {
    return "ConstantMatrix";
}

std::string ConvolutionalConstantMatrixImpl::printTensorType() {
    return "ConvolutionalConstantMatrix";
}

std::string FCConstantMatrixImpl::printTensorType() {
    return "FCConstantMatrix";
}

void InputVectorImpl::printNodeAndEdges(std::ostream& fout) {
    fout << printNodeName() << " " << printNodeStyle() << ";" << std::endl;
    for(InputVectorTile* tile : tiles_) {
        fout << tile->printNodeName() << " " << tile->printNodeStyle() << ";" << std::endl;
        fout << printNodeName() << " -> " << tile->printNodeName() << " [style=dotted];" << std::endl;
        // NOTE: edges from tiles to their users are printed by the users in InputOperation::printNodeAndEdges
    }
}

void InputImagePixelStreamImpl::printNodeAndEdges(std::ostream& fout) {
    fout << printNodeName() << " " << printNodeStyle() << ";" << std::endl;
    for(InputImagePixelStreamTile* tile : tiles_) {
        fout << tile->printNodeName() << " " << tile->printNodeStyle() << ";" << std::endl;
        fout << printNodeName() << " -> " << tile->printNodeName() << " [style=dotted];" << std::endl;
        for(unsigned int h = 0; h < tile->imageHeight(); ++h) {
            for(unsigned int w = 0; w < tile->imageWidth(); ++w) {
                InputVectorTile* streamElement = tile->get(h, w);
                fout << streamElement->printNodeName() << " " << streamElement->printNodeStyle() << ";" << std::endl;
                fout << tile->printNodeName() << " -> " << streamElement->printNodeName() << " [style=dotted];" << std::endl;
                // NOTE: edges from stream elements to their users are printed by the users in InputOperation::printNodeAndEdges
            }
        }
    }
}

void OutputVectorImpl::printNodeAndEdges(std::ostream& fout) {
    fout << printNodeName() << " " << printNodeStyle() << ";" << std::endl;
    for(OutputVectorTile* tile : tiles_) {
        fout << tile->printNodeName() << " " << tile->printNodeStyle() << ";" << std::endl;
        fout << tile->printNodeName() << " -> " << printNodeName() << " [style=dotted];" << std::endl;
        // NOTE: edges to tiles from their sources are printed by the sources in OutputOperation::printNodeAndEdges
    }
}

void OutputImagePixelStreamImpl::printNodeAndEdges(std::ostream& fout) {
    fout << printNodeName() << " " << printNodeStyle() << ";" << std::endl;
    for(OutputImagePixelStreamTile* tile : tiles_) {
        fout << tile->printNodeName() << " " << tile->printNodeStyle() << ";" << std::endl;
        fout << tile->printNodeName() << " -> " << printNodeName() << " [style=dotted];" << std::endl;
        for(unsigned int h = 0; h < tile->imageHeight(); ++h) {
            for(unsigned int w = 0; w < tile->imageWidth(); ++w) {
                OutputVectorTile* streamElement = tile->get(h, w);
                fout << streamElement->printNodeName() << " " << streamElement->printNodeStyle() << ";" << std::endl;
                fout << streamElement->printNodeName() << " -> " << tile->printNodeName() << " [style=dotted];" << std::endl;
                // NOTE: edges to stream elements from their sources are printed by the users in OutputOperation::printNodeAndEdges
            }
        }
    }
}

