/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#ifndef _PUMA_H_
#define _PUMA_H_

#include <string>
#include <vector>

struct CompilerOptions {

        enum GraphPartitioningScheme { GP_ROW_MAJOR, GP_COL_MAJOR, GP_KAHIP, GP_RANDOM };

        GraphPartitioningScheme gp_ = GP_ROW_MAJOR;
        bool coalesceMVMOperations_ = false;
        bool printDebugInfo_ = false;

};

class ModelImpl;
class Model {

    private:

        ModelImpl* impl_;

    public:

        static Model create(std::string name);
        void destroy();

        void loadBalance();
        void compile(CompilerOptions options=CompilerOptions());

        ModelImpl* unwrap();

};

class Layer {

    public:

        Model model_;
        unsigned int kernelWidth_;
        unsigned int kernelHeight_;
        unsigned int inImageWidth_;
        unsigned int inImageHeight_;
        unsigned int nInChannels_;
        unsigned int outImageWidth_;
        unsigned int outImageHeight_;
        unsigned int nOutChannels_;
        bool isFC_;

        unsigned int duplicateWidth_;
        unsigned int duplicateHeight_;
        unsigned int nStack_;
        unsigned int load_;

    public:

        Layer
            (Model model,
            unsigned int kernelWidth, unsigned int kernelHeight,
            unsigned int inImageWidth, unsigned int inImageHeight,
            unsigned int nInChannels, unsigned int outImageWidth,
            unsigned int outImageHeight, unsigned int nOutChannels,
            unsigned int DACScaling, bool isFC);
        ~Layer();

        unsigned int getNMVMU();
        int setDuplicate(unsigned int duplicateWidth,
                unsigned int duplicateHeight);

};

class InputVectorImpl;
class InputVector {

    private:

        InputVectorImpl* impl_;

    public:

        static InputVector create(Model model, std::string name, unsigned int length);

        InputVectorImpl* unwrap();

};

class InputImagePixelStreamImpl;
class InputImagePixelStream {

    private:

        InputImagePixelStreamImpl* impl_;

    public:

        static InputImagePixelStream create(Model model, std::string name, unsigned int imageWidth, unsigned int imageHeight, unsigned int length);

        InputImagePixelStreamImpl* unwrap();

};

class VectorImpl;
class Vector {

    private:

        VectorImpl* impl_;

    public:

        Vector(VectorImpl* impl=NULL);
        Vector(InputVector x);

        VectorImpl* unwrap();

};

class ImagePixelStreamImpl;
class ImagePixelStream {

    private:

        ImagePixelStreamImpl* impl_;

    public:

        ImagePixelStream(ImagePixelStreamImpl* impl=NULL);
        ImagePixelStream(InputImagePixelStream stream);

        ImagePixelStreamImpl* unwrap();

};

class OutputVectorImpl;
class OutputVector {

    private:

        OutputVectorImpl* impl_;

    public:

        static OutputVector create(Model model, std::string name, unsigned int length);

        void operator=(Vector x);

        OutputVectorImpl* unwrap();

};

class OutputImagePixelStreamImpl;
class OutputImagePixelStream {

    private:

        OutputImagePixelStreamImpl* impl_;

    public:

        static OutputImagePixelStream create(Model model, std::string name, unsigned int imageWidth, unsigned int imageHeight, unsigned int length);

        void operator=(ImagePixelStream x);

        OutputImagePixelStreamImpl* unwrap();

};

class ConstantMatrixImpl;
class ConstantMatrix {

    private:

        ConstantMatrixImpl* impl_;

    public:

        static ConstantMatrix create(Model model, std::string name, unsigned int width, unsigned int height);

        ConstantMatrixImpl* unwrap();

};

class ConvolutionalConstantMatrixImpl;
class ConvolutionalConstantMatrix {

    private:

        ConvolutionalConstantMatrixImpl* impl_;

    public:

        static ConvolutionalConstantMatrix create(Model model, 
            std::string name, unsigned int kernelWidth, unsigned int kernelHeight, 
            unsigned int nInChannels, unsigned int nOutChannels, 
            unsigned int stride, unsigned int outImageWidth, unsigned int outImageHeight,
            bool isPool = false);

        ConvolutionalConstantMatrixImpl* unwrap();

};

class FCConstantMatrixImpl;
class FCConstantMatrix {

    private:

        FCConstantMatrixImpl* impl_;

    public:

        static FCConstantMatrix create(Model model, 
            std::string name, unsigned int nInChannels, unsigned int nOutChannels,
            unsigned int kernelWidth = 1, unsigned int kernelHeight = 1);

        FCConstantMatrixImpl* unwrap();

};

class OuterProduct {

    private:

        VectorImpl* x1_;
        VectorImpl* x2_;

    public:

        OuterProduct(Vector x1, Vector x2);

        VectorImpl* unwrap1();
        VectorImpl* unwrap2();

};

class ModelInstanceImpl;
class ModelInstance {

    private:

        ModelInstanceImpl* impl_;

    public:

        static ModelInstance create(Model model);

        void bind(std::string tensorName, float* data);
        void generateData();

        ModelInstanceImpl* unwrap();

};

// Vector element-wise unary operations
Vector operator~(Vector x);
Vector sig(Vector x);
Vector noact(Vector x);
Vector tanh(Vector x);
Vector exp(Vector x);
Vector log(Vector x);
Vector relu(Vector x);
Vector relud(Vector x);
Vector log_softmax(Vector x);
Vector log_softmaxd(Vector x);
Vector rndcmp(Vector x);

// Vector element-wise binary operations
Vector operator+(Vector x1, Vector x2);
Vector operator-(Vector x1, Vector x2);
Vector operator*(Vector x1, Vector x2);
Vector operator/(Vector x1, Vector x2);
Vector operator&(Vector x1, Vector x2);
Vector operator|(Vector x1, Vector x2);
Vector operator==(Vector x1, Vector x2);
Vector operator!=(Vector x1, Vector x2);
Vector operator<(Vector x1, Vector x2);
Vector operator<=(Vector x1, Vector x2);
Vector operator>(Vector x1, Vector x2);
Vector operator>=(Vector x1, Vector x2);
Vector min(Vector x1, Vector x2);
Vector max(Vector x1, Vector x2);
Vector mse(Vector x1, Vector x2);
Vector concat(Vector x1, Vector x2);

// Scalar-vector operations
Vector operator*(float imm, Vector x);

// Vector stream operations
ImagePixelStream concat(ImagePixelStream xs1, ImagePixelStream xs2);
ImagePixelStream sig(ImagePixelStream xs);
ImagePixelStream noact(ImagePixelStream xs);
ImagePixelStream maxpool(ImagePixelStream xs, unsigned int hspan, unsigned int wspan);

// Constant matrix operations
Vector operator*(ConstantMatrix M, Vector x);
ImagePixelStream operator*(ConvolutionalConstantMatrix M, ImagePixelStream x);
ImagePixelStream operator*(FCConstantMatrix M, ImagePixelStream x);
ImagePixelStream operator+(ImagePixelStream xsparam1, ImagePixelStream xsparam2);
// TODO: Implement built-in flatten operation from pixel stream to vector

#endif

