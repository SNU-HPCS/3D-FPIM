/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <assert.h>
#include <sstream>
#include <cmath>

#include "model.h"
#include "operations.h"
#include "tensors.h"

void OutputVector::operator=(Vector xparam) {
    VectorImpl* x = xparam.unwrap();
    OutputVectorImpl* y = impl_;
    y->checkCompatibility(x);
    for(unsigned int t = 0; t < x->nTiles(); ++t) {
        ProducerOperation* producer = x->getTile(t);
        OutputVectorTile* output = y->getTile(t);
        new PseudoOutputOperation(producer->getModel(), producer, output);
    }
}

void OutputImagePixelStream::operator=(ImagePixelStream xsparam) {
    ImagePixelStreamImpl* xs = xsparam.unwrap();
    OutputImagePixelStreamImpl* ys = impl_;
    ys->checkCompatibility(xs);
    for(unsigned int t = 0; t < xs->nTiles(); ++t) {
        ImagePixelStreamTile* xsTile = xs->getTile(t);
        OutputImagePixelStreamTile* ysTile = ys->getTile(t);
        // TODO: Convert the following into a single operation with codegened loops
        for(unsigned int h = 0; h < xs->imageHeight(); ++h) {
            for(unsigned int w = 0; w < xs->imageWidth(); ++w) {
                ProducerOperation* x = xsTile->get(h, w);
                OutputVectorTile* y = ysTile->get(h, w);
                new PseudoOutputOperation(x->getModel(), x, y);
            }
        }
    }
}

Vector::Vector(InputVector xparam) {
    InputVectorImpl* x = xparam.unwrap();
    VectorImpl* y = new VectorImpl(x->getModel(), x->length());
    y->checkCompatibility(x);
    for(unsigned int t = 0; t < x->nTiles(); ++t) {
        ProducerOperation* producer = 
            new PseudoInputOperation(x->getModel(), x->getTile(t));
        y->setTile(t, producer);
    }
    impl_ = y;
}

ImagePixelStream::ImagePixelStream(InputImagePixelStream xsparam) {
    InputImagePixelStreamImpl* xs = xsparam.unwrap();
    ImagePixelStreamImpl* ys = 
        new ImagePixelStreamImpl(xs->getModel(), 
            xs->imageWidth(), 
            xs->imageHeight(), 
            xs->kernelWidth(),
            xs->kernelHeight(),
            xs->nChannels());
    ys->checkCompatibility(xs);
    for(unsigned int t = 0; t < xs->nTiles(); ++t) {
        InputImagePixelStreamTile* xsTile = xs->getTile(t);
        ImagePixelStreamTile* ysTile = ys->getTile(t);
        // TODO: Convert the following into a single operation with codegened loops
        for(unsigned int h = 0; h < xs->imageHeight(); ++ h) {
            for(unsigned int w = 0; w < xs->imageWidth(); ++ w) {
                InputVectorTile* x = xsTile->get(h, w);
                ProducerOperation* y = new PseudoInputOperation(x->getModel(), x);
                ysTile->add(h, w, y);
            }
        }
    }
    impl_ = ys;
}

// Concatenate stream of vectors for merged MVM operations
ImagePixelStream* ConcatStream(ConvolutionalConstantMatrixImpl* M, ImagePixelStream xparam) {
    ImagePixelStreamImpl* xs = xparam.unwrap();

    ImagePixelStreamImpl** ys_arr;
    ys_arr = new ImagePixelStreamImpl*[M->getNOutTiles()];

    for(unsigned int i = 0; i < M->getNOutTiles(); i++){
        ImagePixelStreamImpl* ys = 
            new ImagePixelStreamImpl(xs->getModel(), 
                M->getOutWidth(), M->getOutHeight(),
                M->getKernelWidth(), M->getKernelHeight(),
                xs->nChannels());
        ys_arr[i] = ys;
    }

    assert(xs->kernelWidth() == 1 && xs->kernelHeight() == 1);

    for(unsigned int index = 0; index < M->getNOutTiles(); index++){
        ProducerOperation* accum[M->getOutHeight()][M->getOutWidth()][ys_arr[index]->nTiles()];

        std::vector<int> maxHeightVector(M->getDuplicateHeight());
        std::vector<int> offsetHeightVector(M->getDuplicateHeight());
        std::vector<int> maxWidthVector(M->getDuplicateWidth());
        std::vector<int> offsetWidthVector(M->getDuplicateWidth());
        for(int dh = 0; dh < M->getDuplicateHeight(); ++dh) {
            if(M->getIsPool()){
                maxHeightVector[dh] = 2 * (int(M->getOutHeight() / (M->getDuplicateHeight() * 2)) + int(dh * 2 < (M->getOutHeight() % (M->getDuplicateHeight() * 2))));
            } else{
                maxHeightVector[dh] = int(M->getOutHeight() / M->getDuplicateHeight()) + int(dh < (M->getOutHeight() % M->getDuplicateHeight()));
            }
            offsetHeightVector[dh] = 0;
            if(dh != 0){
                offsetHeightVector[dh] = (maxHeightVector[dh - 1] + offsetHeightVector[dh -1]);
            }
        }
        for(int dw = 0; dw < M->getDuplicateWidth(); ++dw) {
            if(M->getIsPool()){
                maxWidthVector[dw] = 2 * ((int(M->getOutWidth() / (M->getDuplicateWidth() * 2)) + int(dw * 2 < (M->getOutWidth() % (M->getDuplicateWidth() * 2)))));
            } else{
                maxWidthVector[dw] = int(M->getOutWidth() / M->getDuplicateWidth()) + int(dw < (M->getOutWidth() % M->getDuplicateWidth()));
            }
            offsetWidthVector[dw] = 0;
            if(dw != 0){
                offsetWidthVector[dw] = (maxWidthVector[dw - 1] + offsetWidthVector[dw -1]);
            }
        }

        int maxHeight = 2 * ((int(M->getOutHeight() / (M->getDuplicateHeight() * 2)) + int(0 * 2 < (M->getOutHeight() % (M->getDuplicateHeight() * 2)))));
        int maxWidth = 2 * ((int(M->getOutWidth() / (M->getDuplicateWidth() * 2)) + int(0 * 2 < (M->getOutWidth() % (M->getDuplicateWidth() * 2)))));
        
        for(int hm = 0; hm < maxHeight; ++hm) {
            for(int wm = 0; wm < maxWidth; ++wm) {
                for(int dh = 0; dh < M->getDuplicateHeight(); ++dh) {
                    for(int dw = 0; dw < M->getDuplicateWidth(); ++dw) {
                        int ho = offsetHeightVector[dh] + hm;
                        int wo = offsetWidthVector[dw] + wm;
                        bool outputInBounds = ho < M->getOutHeight() && 
                                                wo < M->getOutWidth() &&
                                                hm < maxHeightVector[dh] &&
                                                wm < maxWidthVector[dw];

                        if(outputInBounds){
                            for(unsigned int idx = 0; idx < ys_arr[index]->nTiles(); idx++)
                                accum[ho][wo][idx] = NULL;
                            for(unsigned int kh = 0; kh < ys_arr[index]->kernelHeight(); ++kh) {
                                for(unsigned int kw = 0; kw < ys_arr[index]->kernelWidth(); ++kw) {
                                    for(unsigned int t = 0; t < xs->nTiles(); t++) {
                                        ImagePixelStreamTile* xsTile = xs->getTile(t);
                                        unsigned int padding_h = (M->getStride() == 1) ? (xs->imageHeight() - M->getOutHeight())/2 : 0;
                                        unsigned int padding_w = (M->getStride() == 1) ? (xs->imageWidth() - M->getOutWidth())/2 : 0;
                                        unsigned int hi = (ho + padding_h) * M->getStride() - 
                                            ys_arr[index]->kernelHeight()/2 + kh;
                                        unsigned int wi = (wo + padding_w) * M->getStride() - 
                                            ys_arr[index]->kernelWidth()/2 + kw;
                                        bool inputInBounds = hi >= 0
                                                            && hi < xs->imageHeight() 
                                                            && wi >= 0
                                                            && wi < xs->imageWidth();
                                        
                                        unsigned int length;
                                        if(t == xs->nTiles() - 1) {
                                            length = (xs->nChannels() % MVMU_DIM == 0) ?
                                                MVMU_DIM : xs->nChannels() % MVMU_DIM;
                                        } else {
                                            length = MVMU_DIM;
                                        }
                                        
                                        unsigned int maxConcat = MVMU_DIM / length;
                                        unsigned int t_new = 
                                            t * ys_arr[index]->kernelWidth() * ys_arr[index]->kernelHeight() +
                                            (kh * ys_arr[index]->kernelWidth() + kw) / maxConcat;

                                        if(inputInBounds) {
                                            ProducerOperation* xTile = xsTile->get(hi, wi);
                                            if(accum[ho][wo][t_new] == NULL){
                                                accum[ho][wo][t_new] = xTile;
                                            } else{
                                                accum[ho][wo][t_new] = new ALUVectorOperation
                                                (accum[ho][wo][t_new]->getModel(),
                                                ALUVectorOperation::RESIZE,
                                                accum[ho][wo][t_new], xTile);
                                            }
                                        } else{
                                            ProducerOperation* setOp = 
                                                new SetImmediateOperation
                                                (M->getModel(), 0, length); 

                                            if(accum[ho][wo][t_new] == NULL){
                                                accum[ho][wo][t_new] = setOp;
                                            } else{
                                                accum[ho][wo][t_new] = new ALUVectorOperation
                                                (accum[ho][wo][t_new]->getModel(),
                                                ALUVectorOperation::RESIZE,
                                                accum[ho][wo][t_new], setOp);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        for(unsigned int t_new = 0; t_new < ys_arr[index]->nTiles(); t_new++) {
            ImagePixelStreamTile* ysTile = ys_arr[index]->getTile(t_new);
            for(unsigned int ho = 0; ho < M->getOutHeight(); ++ho) {
                for(unsigned int wo = 0; wo < M->getOutWidth(); ++wo) {
                    ysTile->add(ho, wo, accum[ho][wo][t_new]);
                }
            }
        }
    }

    ImagePixelStream* pixel_arr = new ImagePixelStream [M->getNOutTiles()];
    for(unsigned int i = 0; i < M->getNOutTiles(); i++){
        pixel_arr[i] = ImagePixelStream(ys_arr[i]);
    }

    return pixel_arr;
}

ImagePixelStream* ConcatStream(FCConstantMatrixImpl* M, ImagePixelStream xparam) {
    ImagePixelStreamImpl* xs = xparam.unwrap();

    ImagePixelStreamImpl** ys_arr;
    ys_arr = new ImagePixelStreamImpl*[M->getNOutTiles()];

    for(unsigned int i = 0; i < M->getNOutTiles(); i++){
        ImagePixelStreamImpl* ys = 
            new ImagePixelStreamImpl(xs->getModel(), 
                1, 1, M->getKernelWidth(), M->getKernelHeight(),
                xs->nChannels());
        ys_arr[i] = ys;
    }

    assert(xs->kernelWidth() == 1 && xs->kernelHeight() == 1);

    for(unsigned int index = 0; index < M->getNOutTiles(); index++){
        ProducerOperation* accum[ys_arr[index]->nTiles()];
        for(unsigned int idx = 0; idx < ys_arr[index]->nTiles(); idx++) accum[idx] = NULL;
        for(unsigned int t = 0; t < xs->nTiles(); t++) {
            for(unsigned int h = 0; h < xs->imageHeight(); ++h) {
                for(unsigned int w = 0; w < xs->imageWidth(); ++w) {
                    ImagePixelStreamTile* xsTile = xs->getTile(t);
                    ProducerOperation* xTile = xsTile->get(h, w);

                    unsigned int length;
                    if(t == xs->nTiles() - 1) {
                        length = (xs->nChannels() % MVMU_DIM == 0) ? MVMU_DIM : xs->nChannels() % MVMU_DIM;
                    } else {
                        length = MVMU_DIM;
                    }

                    unsigned int maxConcat = MVMU_DIM / length;
                    unsigned int t_new =
                        t * ys_arr[index]->kernelWidth() * ys_arr[index]->kernelHeight() +
                        (h * ys_arr[index]->kernelWidth() + w) / maxConcat;

                    if(accum[t_new] == NULL){
                        accum[t_new] = xTile;
                    } else{
                        accum[t_new] = new ALUVectorOperation
                        (accum[t_new]->getModel(),
                        ALUVectorOperation::RESIZE,
                        accum[t_new], xTile);
                    }
                }
            }
        }
        for(unsigned int t_new = 0; t_new < ys_arr[index]->nTiles(); t_new++) {
            ImagePixelStreamTile* ysTile = ys_arr[index]->getTile(t_new);
            ysTile->add(0, 0, accum[t_new]);
        }
    }

    ImagePixelStream* pixel_arr = new ImagePixelStream [M->getNOutTiles()];
    for(unsigned int i = 0; i < M->getNOutTiles(); i++){
        pixel_arr[i] = ImagePixelStream(ys_arr[i]);
    }

    return pixel_arr;
}

Vector unaryOp(Vector xparam, ALUVectorOperation::OpCode op) {
    VectorImpl* x = xparam.unwrap();
    VectorImpl* y = new VectorImpl(x->getModel(), x->length());
    y->checkCompatibility(x);
    for(unsigned int t = 0; t < x->nTiles(); ++t) {
        ProducerOperation* producer = new ALUVectorOperation(x->getModel(), op, x->getTile(t));
        y->setTile(t, producer);
    }
    return Vector(y);
}

Vector operator~(Vector x) {
    return unaryOp(x, ALUVectorOperation::NOT);
}

Vector sig(Vector x) {
    return unaryOp(x, ALUVectorOperation::SIG);
}

Vector noact(Vector x) {
    return unaryOp(x, ALUVectorOperation::NOACT);
}

Vector tanh(Vector x) {
    return unaryOp(x, ALUVectorOperation::TANH);
}

Vector exp(Vector x) {
    return unaryOp(x, ALUVectorOperation::EXP);
}

Vector log(Vector x) {
    return unaryOp(x, ALUVectorOperation::LOG);
}

Vector relu(Vector x) {
    return unaryOp(x, ALUVectorOperation::RELU);
}

Vector relud(Vector x) {
    return unaryOp(x, ALUVectorOperation::RELUD);
}

Vector log_softmax(Vector x) {
    return unaryOp(x, ALUVectorOperation::LOG_SOFTMAX);
}

Vector log_softmaxd(Vector x) {
    return unaryOp(x, ALUVectorOperation::LOG_SOFTMAXD);
}

Vector rndcmp(Vector x) {
    return unaryOp(x, ALUVectorOperation::RNDCMP);
}

Vector binaryOp(Vector x1param, Vector x2param, ALUVectorOperation::OpCode op) {
    VectorImpl* x1 = x1param.unwrap();
    VectorImpl* x2 = x2param.unwrap();
    VectorImpl* y = new VectorImpl(x1->getModel(), x1->length());
    y->checkCompatibility(x1);
    y->checkCompatibility(x2);
    for(unsigned int t = 0; t < x1->nTiles(); ++t) {
        ProducerOperation* producer = 
            new ALUVectorOperation
                (x1->getModel(), op, x1->getTile(t), x2->getTile(t));
        y->setTile(t, producer);
    }
    return Vector(y);
}

Vector operator+(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::ADD);
}

Vector operator-(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::SUB);
}

Vector operator*(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::MUL);
}

Vector operator/(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::DIV);
}

Vector operator&(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::AND);
}

Vector operator|(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::OR);
}

Vector operator==(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::EQ);
}

Vector operator!=(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::NEQ);
}

Vector operator<(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::LT);
}

Vector operator<=(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::LEQ);
}

Vector operator>(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::GT);
}

Vector operator>=(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::GEQ);
}

Vector min(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::MIN);
}

Vector max(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::MAX);
}

Vector mse(Vector x1, Vector x2) {
    return binaryOp(x1, x2, ALUVectorOperation::MSE);
}

Vector immediateOp(Vector xparam, float imm, ALUVectorOperation::OpCode op) {
    VectorImpl* x = xparam.unwrap();
    VectorImpl* y = new VectorImpl(x->getModel(), x->length());
    y->checkCompatibility(x);
    for(unsigned int t = 0; t < x->nTiles(); ++t) {
        ProducerOperation* producer = new ALUVectorOperation(x->getModel(), op, x->getTile(t), imm);
        y->setTile(t, producer);
    }
    return Vector(y);
}

Vector operator*(float imm, Vector x) {
    return immediateOp(x, imm, ALUVectorOperation::MULI);
}

ImagePixelStream sig(ImagePixelStream xsparam) {
    ImagePixelStreamImpl* xs = xsparam.unwrap();
    ImagePixelStreamImpl* ys = 
        new ImagePixelStreamImpl
        (xs->getModel(), xs->imageWidth(), xs->imageHeight(),
        xs->kernelWidth(), xs->kernelHeight(),
        xs->nChannels());
    ys->checkCompatibility(xs);
    for(unsigned int t = 0; t < xs->nTiles(); ++t) {
        ImagePixelStreamTile* xsTile = xs->getTile(t);
        ImagePixelStreamTile* ysTile = ys->getTile(t);
        // TODO: Convert the following into a single operation with codegened loops
        for(unsigned int h = 0; h < xs->imageHeight(); ++h) {
            for(unsigned int w = 0; w < xs->imageWidth(); ++w) {
                ProducerOperation* x = xsTile->get(h, w);
                ProducerOperation* y = 
                    new ALUVectorOperation(x->getModel(), ALUVectorOperation::SIG, x);
                ysTile->add(h, w, y);
            }
        }
    }
    return ImagePixelStream(ys);
}

ImagePixelStream noact(ImagePixelStream xsparam) {
    ImagePixelStreamImpl* xs = xsparam.unwrap();
    ImagePixelStreamImpl* ys = 
        new ImagePixelStreamImpl
        (xs->getModel(), xs->imageWidth(), xs->imageHeight(),
        xs->kernelWidth(), xs->kernelHeight(),
        xs->nChannels());
    ys->checkCompatibility(xs);
    for(unsigned int t = 0; t < xs->nTiles(); ++t) {
        ImagePixelStreamTile* xsTile = xs->getTile(t);
        ImagePixelStreamTile* ysTile = ys->getTile(t);
        // TODO: Convert the following into a single operation with codegened loops
        for(unsigned int h = 0; h < xs->imageHeight(); ++h) {
            for(unsigned int w = 0; w < xs->imageWidth(); ++w) {
                ProducerOperation* x = xsTile->get(h, w);
                ProducerOperation* y = 
                    new ALUVectorOperation(x->getModel(), ALUVectorOperation::NOACT, x);
                ysTile->add(h, w, y);
            }
        }
    }
    return ImagePixelStream(ys);
}

ImagePixelStream maxpool(ImagePixelStream xsparam, unsigned int hspan, unsigned int wspan) {
    ImagePixelStreamImpl* xs = xsparam.unwrap();
    unsigned int ysWidth = (xs->imageWidth() - 1)/wspan + 1;
    unsigned int ysHeight = (xs->imageHeight() - 1)/hspan + 1;
    ImagePixelStreamImpl* ys = 
        new ImagePixelStreamImpl(xs->getModel(), 
        ysWidth, 
        ysHeight, 
        xs->kernelWidth(),
        xs->kernelHeight(),
        xs->nChannels());
    for(unsigned int t = 0; t < xs->nTiles(); ++t) {
        ImagePixelStreamTile* xsTile = xs->getTile(t);
        ImagePixelStreamTile* ysTile = ys->getTile(t);
        // TODO: Convert the following into a single operation with codegened loops
        ProducerOperation* accum[ysHeight][ysWidth][hspan*wspan];
        for(unsigned int hi = 0; hi < xs->imageHeight(); ++hi) {
            for(unsigned int wi = 0; wi < xs->imageWidth(); ++wi) {
                ProducerOperation* xTile = xsTile->get(hi, wi);
                unsigned int ho = hi/hspan;
                unsigned int hh = hi%hspan;
                unsigned int wo = wi/wspan;
                unsigned int ww = wi%wspan;
                unsigned int accumIdx = hh*wspan + ww;
                if(accumIdx == 0) {
                    accum[ho][wo][accumIdx] = xTile;
                } else {
                    accum[ho][wo][accumIdx] = 
                        new ALUVectorOperation
                        (accum[ho][wo][accumIdx - 1]->getModel(), 
                        ALUVectorOperation::MAX, 
                        accum[ho][wo][accumIdx - 1], xTile);
                }
                if((hh == hspan - 1 || hi == xs->imageHeight() - 1) && 
                    (ww == wspan - 1 || wi == xs->imageWidth() - 1)) {

                    ysTile->add(ho, wo, accum[ho][wo][accumIdx]);
                }
            }
        }
    }
    return ImagePixelStream(ys);
}

Vector operator*(ConstantMatrix Mparam, Vector xparam) {
    ConstantMatrixImpl* M = Mparam.unwrap();
    ModelImpl* model = M->getModel();

    VectorImpl** x_arr;
    x_arr = new VectorImpl*[M->nHeightTiles()];
    for(unsigned int i = 0; i < M->nHeightTiles(); i++){
        x_arr[i] = xparam.unwrap();
        M->checkCompatibilityForMVM(x_arr[i]);
    }

    VectorImpl* y = new VectorImpl(model, M->height());

    SlidingMergedMVMSet*** slidingSets = new SlidingMergedMVMSet**[M->nHeightTiles()];
    for(int h = 0; h < M->nHeightTiles(); ++h) {
        slidingSets[h] = new SlidingMergedMVMSet*[M->nWidthTiles()];
        for(int w_mat = 0; w_mat < M->nWidthTiles(); ++w_mat) {
            slidingSets[h][w_mat] = NULL;
        }
    }

    std::vector<MergedMVMSet*>* coalesceableMVMVector = new std::vector<MergedMVMSet*>();
    
    for(unsigned int h = 0; h < M->nHeightTiles(); ++h) {
        ProducerOperation* accum[M->nWidthTiles()]; 
        // TODO: The following implements a sequential reduction; 
        // a tree reduction would be more efficient
        // then iterate over the matrix input
        for(unsigned int w_mat = 0; w_mat < M->nWidthTiles(); ++w_mat){
            unsigned int w_size = (w_mat == M->nWidthTiles() - 1) ? M->nWidthRest() : M->nWidthDPT();
            ProducerOperation* concat[w_size]; 

            MergedMVMSet* merge = new MergedMVMSet(model, M->getTile(h, w_mat)->height());

            // for each iteration, concatenate the input tile
            for(unsigned int w_vec = 0; w_vec < w_size; ++w_vec) {
                unsigned int tile_index = w_vec + w_mat * M->nWidthDPT();
                ProducerOperation* concatOp = (w_vec != 0) ? concat[w_vec - 1] : NULL;
                int precision = int(std::ceil(log2(std::ceil(float(M->nWidthTiles()))))) + 4;

                ProducerOperation* producer = concat[w_vec] = 
                        new MVMOperation(model, M->getTile(h, w_mat), (w_vec == w_size - 1),
                        w_vec, w_size, precision, 0, x_arr[h]->getTile(tile_index), concatOp);

                merge->add(dynamic_cast<MVMOperation*>(concat[w_vec]));
            }

            if(slidingSets[h][w_mat] == NULL) {
                slidingSets[h][w_mat] = new SlidingMergedMVMSet(1);
            }
            slidingSets[h][w_mat]->add(merge, 0);

            coalesceableMVMVector->push_back(merge);

            if(w_mat == 0) {
                accum[w_mat] = concat[w_size - 1];
            } else {
                accum[w_mat] = 
                    new ALUVectorOperation
                    (model, ALUVectorOperation::ADD, 
                    concat[w_size - 1], accum[w_mat - 1]);
            }
        }
        y->setTile(h, accum[M->nWidthTiles() - 1]);
    }

    model->addCoalesceableMVMVector(coalesceableMVMVector);
    return Vector(y);
}

ImagePixelStream operator+
    (ImagePixelStream xsparam1,
    ImagePixelStream xsparam2) {

    ImagePixelStreamImpl* xs1 = xsparam1.unwrap();
    ImagePixelStreamImpl* xs2 = xsparam2.unwrap();

    ImagePixelStreamImpl* ys =
        new ImagePixelStreamImpl
        (xs1->getModel(), xs1->imageWidth(), xs1->imageHeight(), 
        xs1->kernelWidth(), xs1->kernelHeight(),
        xs1->nChannels());

    ys->checkCompatibility(xs1);
    ys->checkCompatibility(xs2);

    for(unsigned int t = 0; t < xs1->nTiles(); ++t) {
        ImagePixelStreamTile* xs1Tile = xs1->getTile(t);
        ImagePixelStreamTile* xs2Tile = xs2->getTile(t);
        ImagePixelStreamTile* ysTile = ys->getTile(t);

        for(unsigned int h = 0; h < xs1->imageHeight(); ++h) {
            for(unsigned int w = 0; w < xs1->imageWidth(); ++w) {
                ProducerOperation* x1 = xs1Tile->get(h, w);
                ProducerOperation* x2 = xs2Tile->get(h, w);

                ProducerOperation* y = new ALUVectorOperation
                    (x1->getModel(), ALUVectorOperation::ADD, x1, x2);

                ysTile->add(h, w, y);
            }
        }
    }

    return ImagePixelStream(ys);
}

ImagePixelStream operator* (ConvolutionalConstantMatrix Mparam, ImagePixelStream xsparam) {
    ConvolutionalConstantMatrixImpl* M = Mparam.unwrap();
    ModelImpl* model = M->getModel();

    int kernelWidth = M->getKernelWidth();
    int kernelHeight = M->getKernelHeight();
    int stride = M->getStride();
    int nInTiles = M->getNInTiles();
    int nInDPT = M->getNInDPT();
    int nInRestDPT = M->getNInRestDPT();
    int outImageWidth = M->getOutWidth();
    int outImageHeight = M->getOutHeight();
    int duplicateWidth = M->getDuplicateWidth();
    int duplicateHeight = M->getDuplicateHeight();

    ImagePixelStream* xparam_new = ConcatStream(M, xsparam);
    ImagePixelStreamImpl** xs_arr;
    xs_arr = new ImagePixelStreamImpl*[M->getNOutTiles()];
    for(unsigned int i = 0; i < M->getNOutTiles(); i++){
        xs_arr[i] = xparam_new[i].unwrap();
        M->checkCompatibility(xs_arr[i]);
    }

    ImagePixelStreamImpl* ys[nInTiles];
    for(int accumIdx = 0; accumIdx < nInTiles; accumIdx++){
        ys[accumIdx] = new ImagePixelStreamImpl
            (model, outImageWidth, outImageHeight,
            1, 1, M->getNOutChannels());
    }

    std::vector<int> maxHeightVector(duplicateHeight);
    std::vector<int> offsetHeightVector(duplicateHeight);
    std::vector<int> maxWidthVector(duplicateWidth);
    std::vector<int> offsetWidthVector(duplicateWidth);
    for(int dh = 0; dh < duplicateHeight; ++dh) {
        if(M->getIsPool()){
            maxHeightVector[dh] = 2 * (int(outImageHeight / (duplicateHeight * 2)) + int(dh * 2 < (outImageHeight % (duplicateHeight * 2))));
        } else{
            maxHeightVector[dh] = int(outImageHeight / duplicateHeight) + int(dh < (outImageHeight % duplicateHeight));
        }
        offsetHeightVector[dh] = 0;
        if(dh != 0){
            offsetHeightVector[dh] = (maxHeightVector[dh - 1] + offsetHeightVector[dh -1]);
        }
    }
    for(int dw = 0; dw < duplicateWidth; ++dw) {
        if(M->getIsPool()){
            maxWidthVector[dw] = 2 * ((int(outImageWidth / (duplicateWidth * 2)) + int(dw * 2 < (outImageWidth % (duplicateWidth * 2)))));
        } else{
            maxWidthVector[dw] = int(outImageWidth / duplicateWidth) + int(dw < (outImageWidth % duplicateWidth));
        }
        offsetWidthVector[dw] = 0;
        if(dw != 0){
            offsetWidthVector[dw] = (maxWidthVector[dw - 1] + offsetWidthVector[dw -1]);
        }
    }

    int maxHeight = 2 * (int(outImageHeight / (duplicateHeight * 2)) + int(0 * 2 < (outImageHeight % (duplicateHeight * 2))));
    int maxWidth = 2 * (int(outImageWidth / (duplicateWidth * 2)) + int(0 * 2 < (outImageWidth % (duplicateWidth * 2))));

    SlidingMergedMVMSet***** slidingSets = new SlidingMergedMVMSet****[duplicateHeight];
    for(int dh = 0; dh < duplicateHeight; ++dh) {
        slidingSets[dh] = new SlidingMergedMVMSet***[duplicateWidth];
        for(int dw = 0; dw < duplicateWidth; ++dw) {
            slidingSets[dh][dw] = new SlidingMergedMVMSet**[M->getNOutTiles()];
            for(int h = 0; h < M->getNOutTiles(); ++h) {
                slidingSets[dh][dw][h] = new SlidingMergedMVMSet*[nInTiles];
                for(int w_mat = 0; w_mat < nInTiles; ++w_mat) {
                    slidingSets[dh][dw][h][w_mat] = NULL;
                }
            }
        }
    }

    for(int hm = 0; hm < maxHeight; ++hm) {
        for(int wm = 0; wm < maxWidth; ++wm) {
            std::vector<MergedMVMSet*>* coalesceableMVMVector = 
                new std::vector<MergedMVMSet*>();
            for(int dh = 0; dh < duplicateHeight; ++dh) {
                for(int dw = 0; dw < duplicateWidth; ++dw) {
                    int ho = offsetHeightVector[dh] + hm;
                    int wo = offsetWidthVector[dw] + wm;
                    bool outputInBounds = ho < outImageHeight && 
                                            wo < outImageWidth &&
                                            hm < maxHeightVector[dh] &&
                                            wm < maxWidthVector[dw];

                    if(outputInBounds){
                        for(int h = 0; h < M->getNOutTiles(); ++h) {
                            for(int w_mat = 0; w_mat < nInTiles; ++w_mat) {
                                ConstantMatrixTile* mat = M->getTile(dh, dw, h, w_mat);
                                ImagePixelStreamTile* accumStreamIn = 
                                    (w_mat == 0) ? NULL : ys[w_mat - 1]->getTile(h); 
                                ImagePixelStreamTile* accumStreamOut = 
                                    ys[w_mat]->getTile(h); 

                                unsigned int w_size = (w_mat == nInTiles - 1) ? nInRestDPT : nInDPT;
                                ProducerOperation* concat[w_size]; 

                                MergedMVMSet* merge = new MergedMVMSet(model, mat->height());
                                int sliding_id = hm * maxWidthVector[dw] + wm;

                                if(sliding_id % 2 == 0) {
                                    for(int w_vec = 0; w_vec < w_size; ++w_vec) {
                                        unsigned int tileIdx = w_vec + w_mat * nInDPT;
                                        ImagePixelStreamTile* imageStream = xs_arr[h]->getTile(tileIdx);
                                        ProducerOperation* pixel = imageStream->get(ho, wo);
                                        ProducerOperation* concatOp = (w_vec != 0) ?  concat[w_vec - 1] : NULL;
                                        int precision = int(std::ceil(log2(std::ceil(float(nInTiles))))) + 4;

                                        ProducerOperation* producer = concat[w_vec] =
                                                new MVMOperation(model, mat, (w_vec == w_size - 1),
                                                w_vec, w_size, precision, sliding_id, pixel, concatOp);

                                        merge->add(dynamic_cast<MVMOperation*>(producer));
                                    }
                                } else {
                                    for(int w_vec = w_size - 1; w_vec >= 0; --w_vec) {
                                        unsigned int tileIdx = w_vec + w_mat * nInDPT;
                                        ImagePixelStreamTile* imageStream = xs_arr[h]->getTile(tileIdx);
                                        ProducerOperation* pixel = imageStream->get(ho, wo);
                                        ProducerOperation* concatOp = (w_vec != w_size - 1) ?  concat[w_vec + 1] : NULL;
                                        int precision = int(std::ceil(log2(std::ceil(float(nInTiles))))) + 4;

                                        ProducerOperation* producer = concat[w_vec] =
                                                new MVMOperation(model, mat, (w_vec == 0),
                                                w_vec, w_size, precision, sliding_id, pixel, concatOp);

                                        merge->add(dynamic_cast<MVMOperation*>(producer));
                                    }
                                }

                                if(slidingSets[dh][dw][h][w_mat] == NULL) {
                                    slidingSets[dh][dw][h][w_mat] = new SlidingMergedMVMSet(maxHeightVector[dh] * maxWidthVector[dw]);
                                }
                                slidingSets[dh][dw][h][w_mat]->add(merge, sliding_id);

                                if(sliding_id % 2 == 0) {
                                    if(w_mat == 0) {
                                        accumStreamOut->add(ho, wo, concat[w_size-1]);
                                    } else {
                                        accumStreamOut->add
                                            (ho, wo, new ALUVectorOperation
                                            (model, ALUVectorOperation::ADD, 
                                            concat[w_size-1], accumStreamIn->get(ho, wo)));
                                    }
                                } else {
                                    if(w_mat == 0) {
                                        accumStreamOut->add(ho, wo, concat[0]);
                                    } else {
                                        accumStreamOut->add
                                            (ho, wo, new ALUVectorOperation
                                            (model, ALUVectorOperation::ADD, 
                                            concat[0], accumStreamIn->get(ho, wo)));
                                    }
                                }
                                coalesceableMVMVector->push_back(merge);
                            }
                        }
                    }
                }
            }
            model->addCoalesceableMVMVector(coalesceableMVMVector);
        }
    }
    return ImagePixelStream(ys[nInTiles - 1]);
}

ImagePixelStream operator* (FCConstantMatrix Mparam, ImagePixelStream xsparam) {
    FCConstantMatrixImpl* M = Mparam.unwrap();
    ModelImpl* model = M->getModel();

    int nInTiles = M->getNInTiles();
    int nInDPT = M->getNInDPT();
    int nInRestDPT = M->getNInRestDPT();

    ImagePixelStream* xparam_new = ConcatStream(M, xsparam);
    ImagePixelStreamImpl** xs_arr;
    xs_arr = new ImagePixelStreamImpl*[M->getNOutTiles()];
    for(unsigned int i = 0; i < M->getNOutTiles(); i++){
        xs_arr[i] = xparam_new[i].unwrap();
        M->checkCompatibility(xs_arr[i]);
    }

    ImagePixelStreamImpl* ys[nInTiles];
    for(int accumIdx = 0; accumIdx < nInTiles; accumIdx++){
        ys[accumIdx] = new ImagePixelStreamImpl
            (model, 1, 1, 1, 1, M->getNOutChannels());
    }

    SlidingMergedMVMSet*** slidingSets = new SlidingMergedMVMSet**[M->getNOutTiles()];
    for(int h = 0; h < M->getNOutTiles(); ++h) {
        slidingSets[h] = new SlidingMergedMVMSet*[nInTiles];
        for(int w_mat = 0; w_mat < nInTiles; ++w_mat) {
            slidingSets[h][w_mat] = NULL;
        }
    }

    std::vector<MergedMVMSet*>* coalesceableMVMVector = 
        new std::vector<MergedMVMSet*>();
    
    for(int h = 0; h < M->getNOutTiles(); ++h) {
        for(int w_mat = 0; w_mat < nInTiles; ++w_mat) {
            ConstantMatrixTile* mat = M->getTile(h, w_mat);
            ImagePixelStreamTile* accumStreamIn = 
                (w_mat == 0) ? NULL : ys[w_mat - 1]->getTile(h); 
            ImagePixelStreamTile* accumStreamOut = 
                ys[w_mat]->getTile(h); 

            unsigned int w_size = (w_mat == nInTiles - 1) ? nInRestDPT : nInDPT;
            ProducerOperation* concat[w_size]; 

            MergedMVMSet* merge = new MergedMVMSet(model, mat->height());

            for(int w_vec = 0; w_vec < w_size; ++w_vec) {
                unsigned int tileIdx = w_vec + w_mat * nInDPT;
                ImagePixelStreamTile* imageStream = xs_arr[h]->getTile(tileIdx);
                ProducerOperation* pixel = imageStream->get(0, 0);
                ProducerOperation* concatOp = (w_vec != 0) ? concat[w_vec - 1] : NULL;
                int precision = int(std::ceil(log2(std::ceil(float(nInTiles))))) + 4;

                ProducerOperation* producer = concat[w_vec] = 
                        new MVMOperation(model, mat, (w_vec == w_size - 1),
                        w_vec, w_size, precision, 0, pixel, concatOp);

                merge->add(dynamic_cast<MVMOperation*>(producer));
            }

            if(slidingSets[h][w_mat] == NULL) {
                slidingSets[h][w_mat] = new SlidingMergedMVMSet(1);
            }
            slidingSets[h][w_mat]->add(merge, 0);

            if(w_mat == 0) {
                accumStreamOut->add(0, 0, concat[w_size-1]);
            } else {
                accumStreamOut->add
                    (0, 0, new ALUVectorOperation
                    (model, ALUVectorOperation::ADD, 
                    concat[w_size-1], 
                    accumStreamIn->get(0, 0)));
            }
            coalesceableMVMVector->push_back(merge);
        }
    }
    model->addCoalesceableMVMVector(coalesceableMVMVector);

    return ImagePixelStream(ys[nInTiles - 1]);
}

Operation::Operation(ModelImpl* model, unsigned int length) : 
    model_(model), length_(length) {
    assert(model != NULL);
    id = model->addOperation(this);
}

ConsumerOperation::ConsumerOperation(ProducerOperation* op1, ProducerOperation* op2) {
    if(op1 != NULL) {
        operands_.push_back(op1);
        op1->addUser(this);
        if(op2 != NULL) {
            operands_.push_back(op2);
            op2->addUser(this);
        }
    } else {
        assert(op2 == NULL);
    }
}

TileMemoryReadOperation::TileMemoryReadOperation
    (TileMemoryWriteOperation* src1, TileMemoryWriteOperation* src2) {

    assert(src1 != NULL);
    srcs_.push_back(src1);
    src1->addUser(this);
    if(src2 != NULL) {
        srcs_.push_back(src2);
        src2->addUser(this);
    }
}

InputOperation::InputOperation(InputVectorTile* src) : src_(src) {
    assert(src != NULL);
}

OutputOperation::OutputOperation(OutputVectorTile* dst) : dst_(dst) {
    assert(dst != NULL);
}

MVMOperation::MVMOperation
    (ModelImpl* model, 
    ConstantMatrixTile* mat, 
    bool isLast,
    int depth,
    int nStack,
    int precision,
    int slideId,
    ProducerOperation* src1,
    ProducerOperation* src2) : 
    Operation(model, mat->height()), 
    ConsumerOperation(src1, src2), 
    mat_(mat), mergedSet_(NULL), isLast_(isLast), 
    coalescedSet_(NULL), depth_(depth), nStack_(nStack), precision_(precision), slideId_(slideId){

    assert(mat != NULL && src1 != NULL);
    assert(mat->width() >= src1->length());
    if(src2 != NULL){
        assert(mat->height() >= src2->length());
    }
    mat->addUser(this);
}

ALUVectorOperation::ALUVectorOperation
    (ModelImpl* model, 
    OpCode opCode, 
    ProducerOperation* src1, 
    ProducerOperation* src2) : 
    Operation(model, src1->length()), 
    ConsumerOperation(src1, src2), opCode_(opCode), imm_(0.0f) {

    assert(!isImmediate());
    assert(src1 != NULL);
    switch(opCode_) {
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case AND:
        case OR:
        case EQ:
        case NEQ:
        case LT:
        case LEQ:
        case GT:
        case GEQ:
        case MIN:
        case MAX:
        case MSE:
            assert(src2 != NULL && src1->length() == src2->length());
            break;
        case RESIZE:
            length_ = src1->length() + src2->length();
            assert(length_ <= MVMU_DIM && "Concatenated vector length exceeds the MVMU size");
    }
}

ALUVectorOperation::ALUVectorOperation
    (ModelImpl* model, 
    OpCode opCode, 
    ProducerOperation* src1, 
    float imm) : 
    Operation(model, src1->length()), 
    ConsumerOperation(src1), 
    opCode_(opCode), imm_(imm) {

    assert(isImmediate());
    assert(src1 != NULL);
}

SetImmediateOperation::SetImmediateOperation
    (ModelImpl* model, unsigned int imm, unsigned int length) : 
    Operation(model, length), imm_(imm) {

}

CopyOperation::CopyOperation(ModelImpl* model, ProducerOperation* src) : Operation(model, src->length()), ConsumerOperation(src) {
    assert(src != NULL);
}

LoadOperation::LoadOperation(ModelImpl* model, TileMemoryWriteOperation* src) : Operation(model, src->length()), TileMemoryReadOperation(src) {
}

MVMGuardOperation::MVMGuardOperation(ModelImpl* model, MVMOperation* mvm, TileMemoryWriteOperation* src)
    : Operation(model, src->length()), mvm_(mvm), TileMemoryReadOperation(src) {
}

StoreOperation::StoreOperation(ModelImpl* model, ProducerOperation* src) : Operation(model, src->length()), ConsumerOperation(src) {
    assert(src != NULL);
}

SendOperation::SendOperation(ModelImpl* model, TileMemoryWriteOperation* src) : Operation(model, src->length()), TileMemoryReadOperation(src), dst_(NULL) {
}

ReceiveOperation::ReceiveOperation(ModelImpl* model, SendOperation* src) : Operation(model, src->length()), src_(src) {
    src->setDst(this);
}

WriteInputOperation::WriteInputOperation(ModelImpl* model, InputVectorTile* src) : Operation(model, src->length()), InputOperation(src) {
}

ReadOutputOperation::ReadOutputOperation
    (ModelImpl* model, TileMemoryWriteOperation* src, 
    OutputVectorTile* dst) : 
    Operation(model, src->length()), 
    TileMemoryReadOperation(src), 
    OutputOperation(dst) {

    assert(src->length() == dst->length());
}

PseudoInputOperation::PseudoInputOperation
    (ModelImpl* model, InputVectorTile* src) : 
    Operation(model, src->length()), 
    InputOperation(src) {

}

PseudoOutputOperation::PseudoOutputOperation
    (ModelImpl* model, ProducerOperation* op, OutputVectorTile* dst) : 
    Operation(model, op->length()), ConsumerOperation(op), OutputOperation(dst) {

    assert(op != NULL && op->length() == dst->length());
}

void LoadOperation::addTileMemoryAddressOperand(ProducerOperation* address) {
    assert(operands_.size() == 0 && "Cannot set tile memory address operand!");
    assert(address->length() == 1 && "Address must be of length 1!");
    operands_.push_back(address);
    address->addUser(this);
}

void StoreOperation::addTileMemoryAddressOperand(ProducerOperation* address) {
    assert(operands_.size() == 1 && "Cannot set tile memory address operand!");
    assert(address->length() == 1 && "Address must be of length 1!");
    operands_.push_back(address);
    address->addUser(this);
}

void MVMGuardOperation::addTileMemoryAddressOperand(ProducerOperation* address) {
    assert(operands_.size() == 0 && "Cannot set tile memory address operand!");
    assert(address->length() == 1 && "Address must be of length 1!");
    operands_.push_back(address);
    address->addUser(this);
}

void SendOperation::setDst(ReceiveOperation* dst) {
    assert(dst_ == NULL && "Cannot reset destination of send operation");
    dst_ = dst;
}

bool ConsumerOperation::uses(ProducerOperation* op) {
    for(unsigned int i = 0; i < operands_.size(); ++i) {
        if(operands_[i] == op) {
            return true;
        }
    }
    return false;
}

void ConsumerOperation::replaceOperand(ProducerOperation* op, ProducerOperation* replacement) {
    for(unsigned int i = 0; i < operands_.size(); ++i) {
        if(operands_[i] == op) {
            operands_[i] = replacement;
            op->removeUser(this);
            replacement->addUser(this);
        }
    }
}

void TileMemoryReadOperation::replaceSrc(TileMemoryWriteOperation* old, TileMemoryWriteOperation* replacement) {
    for(unsigned int i = 0; i < srcs_.size(); ++i) {
        if(srcs_[i] == old) {
            srcs_[i] = replacement;
            old->removeUser(this);
            replacement->addUser(this);
            return;
        }
    }
    assert(0 && "Source to be replaced not found");
}

void MVMOperation::setMergedSet(MergedMVMSet* mergedSet) {
    assert(mergedSet_ == NULL && "Cannot reassign coalesced set");
    mergedSet_ = mergedSet;
}

void MVMOperation::resetMergedSet() {
    mergedSet_ = NULL;
}

void MergedMVMSet::add(MVMOperation* mvm){
    mvms_.insert(mvms_.begin(), mvm);
    mvm->setMergedSet(this);
}

void MVMOperation::setCoalescedSet(CoalescedMVMSet* coalescedSet) {
    assert(coalescedSet_ == NULL && "Cannot reassign coalesced set");
    coalescedSet_ = coalescedSet;
}

void MergedMVMSet::setCoalescedSet(CoalescedMergedMVMSet* coalescedSet) {
    assert(coalescedSet_ == NULL && "Cannot reassign coalesced set");
    coalescedSet_ = coalescedSet;
}

void MergedMVMSet::setSlidingSet(SlidingMergedMVMSet* slidingSet) {
    assert(slidingSet_ == NULL && "Cannot reassign sliding set");
    slidingSet_ = slidingSet;
}

void MergedMVMSet::resetCoalescedSet() {
    coalescedSet_ = NULL;
}

void MergedMVMSet::remove(MVMOperation* mvm){
    mvms_.erase(mvm);
    mvm->resetMergedSet();
}

void CoalescedMergedMVMSet::add(MergedMVMSet* mergedMVM, unsigned int pMVMU) {
    assert(mvms_[pMVMU] == NULL);
    mvms_[pMVMU] = mergedMVM;
    mergedMVM->setCoalescedSet(this);
}

void CoalescedMVMSet::add(MVMOperation* mvm, unsigned int pMVMU) {
    assert(mvms_[pMVMU] == NULL);
    mvms_[pMVMU] = mvm;
    mvm->setCoalescedSet(this);
}

void CoalescedMergedMVMSet::removeAll() {
    for(unsigned int i = 0; i < mvms_.size(); ++i) {
        MergedMVMSet* mergedMVM = mvms_[i];
        if(mergedMVM != NULL) {
            mvms_[i] = NULL;
            mergedMVM->resetCoalescedSet();
        }
    }
}

bool CoalescedMVMSet::isSetLeader(MVMOperation* mvm) {
    for(unsigned int i = 0; i < mvms_.size(); ++i) {
        MVMOperation* m = mvms_[i];
        if(m != NULL) {
            return (m == mvm); // Leader is first non-NULL MVM in the set
        }
    }
    assert(0 && "Unreachable: cannot have caolesced set with all NULL mvms!");
}

bool CoalescedMergedMVMSet::isSetLeader(MergedMVMSet* mvm) {
    for(unsigned int i = 0; i < mvms_.size(); ++i) {
        MergedMVMSet* m = mvms_[i];
        if(m != NULL) {
            return (m == mvm); // Leader is first non-NULL MVM in the set
        }
    }
    assert(0 && "Unreachable: cannot have caolesced set with all NULL mvms!");
}

bool CoalescedMergedMVMSet::isComplete() {
    for(auto mvm : mvms_) {
        if(mvm == NULL) {
            return false;
        }
    }
    return true;
}

void SlidingMergedMVMSet::add(MergedMVMSet* mvm, int slideId) {
    assert(mvms_[slideId] == NULL);
    mvms_[slideId] = mvm;
    mvm->setSlidingSet(this);
}

std::string Operation::printNodeName() {
    std::stringstream ss;
    ss << '"' << "ID: " << id << "\n" << 
        printOperationType() << "\n" << 
        this << model_->printAssignment(this) << "\nlength = " << this->length_ << '"';
    return ss.str();
}

std::string Operation::printNodeStyle() {
    return "";
}

std::string MergedMVMSet::printNodeStyle() {
    return "[style=filled,fillcolor=\"#009933\"]";
}

std::string MVMOperation::printNodeStyle() {
    return "[style=filled,fillcolor=\"#009933\"]";
}

std::string ALUVectorOperation::printNodeStyle() {
    return "[style=filled,fillcolor=\"#66FF66\"]";
}

std::string LoadOperation::printNodeStyle() {
    return "[style=filled,fillcolor=\"#FFB366\"]";
}

std::string StoreOperation::printNodeStyle() {
    return "[style=filled,fillcolor=\"#FFB366\"]";
}

std::string MVMGuardOperation::printNodeStyle() {
    return "[style=filled,fillcolor=\"#FFB366\"]";
}

std::string SendOperation::printNodeStyle() {
    return "[style=filled,fillcolor=\"#FFFF66\"]";
}

std::string ReceiveOperation::printNodeStyle() {
    return "[style=filled,fillcolor=\"#FFFF66\"]";
}

std::string MergedMVMSet::printOperationType() {
    std::stringstream ss;
    ss << "MergedMVM: ";
    for(MVMOperation* temp : mvms_)
        ss << temp->id << " ";
    return ss.str();
}

std::string MVMOperation::printOperationType() {
    std::stringstream ss;
    ss << "MVM: " << mat_->name();
    return ss.str();
}

std::string ALUVectorOperation::printOperationType() {
    switch(opCode_) {
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case MULI: return "MULI";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case EQ: return "EQ";
        case NEQ: return "NEQ";
        case LT: return "LT";
        case LEQ: return "LEQ";
        case GT: return "GT";
        case GEQ: return "GEQ";
        case MIN: return "MIN";
        case MAX: return "MAX";
        case MSE: return "MSE";
        case SIG: return "SIG";
        case NOACT: return "NOACT";
        case TANH: return "TANH";
        case EXP: return "EXP";
        case LOG: return "LOG";
        case RELU: return "RELU";
        case RELUD: return "RELUD";
        case LOG_SOFTMAX: return "LOG_SOFTMAX";
        case LOG_SOFTMAXD: return "LOG_SOFTMAXD";
        case RNDCMP: return "RNDCMP";
        case RESIZE: return "RESIZE";
    }
}

std::string SetImmediateOperation::printOperationType() {
    std::stringstream ss;
    ss << "Set " << imm_;
    return ss.str();
}

std::string CopyOperation::printOperationType() {
    return "Copy";
}

std::string StoreOperation::printOperationType() {
    return "Store";
}

std::string LoadOperation::printOperationType() {
    return "Load";
}

std::string SendOperation::printOperationType() {
    return "Send";
}

std::string MVMGuardOperation::printOperationType() {
    return "Guard";
}

std::string ReceiveOperation::printOperationType() {
    return "Receive";
}

std::string WriteInputOperation::printOperationType() {
    return "WriteInput";
}

std::string ReadOutputOperation::printOperationType() {
    return "ReadOutput";
}

std::string PseudoInputOperation::printOperationType() {
    return "PseudoInput";
}

std::string PseudoOutputOperation::printOperationType() {
    return "PseudoOutput";
}

void MergedMVMSet::printNodeAndEdges(std::ostream& fout){
    fout << "INVALID" << std::endl;
}

void MergedMVMSet::printMVMList(){
    for(MVMOperation* temp : mvms_)
        std::cout << temp->id << " ";
    std::cout << std::endl;
}

void Operation::printNodeAndEdges(std::ostream& fout) {
    fout << printNodeName() << " " << printNodeStyle() << ";" << std::endl;
}

void ProducerOperation::printNodeAndEdges(std::ostream& fout) {
    Operation::printNodeAndEdges(fout);
    for(ConsumerOperation* user : users_) {
        fout << printNodeName() << " -> " << user->printNodeName() << ";" << std::endl;
    }
}

void TileMemoryWriteOperation::printNodeAndEdges(std::ostream& fout) {
    Operation::printNodeAndEdges(fout);
    for(TileMemoryReadOperation* user : users_) {
        fout << printNodeName() << " -> " << user->printNodeName() << ";" << std::endl;
    }
}

void SendOperation::printNodeAndEdges(std::ostream& fout) {
    Operation::printNodeAndEdges(fout);
    fout << printNodeName() << " -> " << dst_->printNodeName() << ";" << std::endl;
}

void MVMGuardOperation::printNodeAndEdges(std::ostream& fout) {
    Operation::printNodeAndEdges(fout);
    fout << printNodeName() << "->" << mvm_->printNodeName() << ";" << std::endl;
}

void InputOperation::printNodeAndEdges(std::ostream& fout) {
    fout << src_->printNodeName() << " -> " << printNodeName() << ";" << std::endl;
}

void OutputOperation::printNodeAndEdges(std::ostream& fout) {
    Operation::printNodeAndEdges(fout);
    fout << printNodeName() << " -> " << dst_->printNodeName() << ";" << std::endl;
}

void WriteInputOperation::printNodeAndEdges(std::ostream& fout) {
    TileMemoryWriteOperation::printNodeAndEdges(fout);
    InputOperation::printNodeAndEdges(fout);
}

void ReadOutputOperation::printNodeAndEdges(std::ostream& fout) {
    OutputOperation::printNodeAndEdges(fout);
}


void PseudoInputOperation::printNodeAndEdges(std::ostream& fout) {
    ProducerOperation::printNodeAndEdges(fout);
    InputOperation::printNodeAndEdges(fout);
}

void PseudoOutputOperation::printNodeAndEdges(std::ostream& fout) {
    OutputOperation::printNodeAndEdges(fout);
}

