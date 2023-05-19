/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#ifndef _PUMA_TEST_CONV_LAYER_
#define _PUMA_TEST_CONV_LAYER_

#include "3dfpim.h"
#include <assert.h>

static ImagePixelStream conv_layer
    (Model model, std::string layerName, 
    unsigned int k_size_x, unsigned int k_size_y, 
    unsigned int in_size_x, unsigned int in_size_y, 
    unsigned int in_channels, unsigned int out_channels, 
    unsigned int stride, unsigned int out_size_x, unsigned int out_size_y,
    ImagePixelStream in_stream, bool is_pool = false) {

    ConvolutionalConstantMatrix mat = 
        ConvolutionalConstantMatrix::create
        (model, layerName, k_size_x, k_size_y, in_channels, out_channels, stride, out_size_x, out_size_y, is_pool);

    return sig(mat*in_stream);

}

static ImagePixelStream convnoact_layer
    (Model model, std::string layerName, 
    unsigned int k_size_x, unsigned int k_size_y, 
    unsigned int in_size_x, unsigned int in_size_y, 
    unsigned int in_channels, unsigned int out_channels, 
    unsigned int stride, unsigned int out_size_x, unsigned int out_size_y,
    ImagePixelStream in_stream, bool is_pool = false) {

    ConvolutionalConstantMatrix mat = 
        ConvolutionalConstantMatrix::create
        (model, layerName, k_size_x, k_size_y, in_channels, out_channels, stride, out_size_x, out_size_y, is_pool);

    return mat*in_stream;

}

static ImagePixelStream convmax_layer
    (Model model, std::string layerName, 
    unsigned int k_size_x, unsigned int k_size_y, 
    unsigned int in_size_x, unsigned int in_size_y, 
    unsigned int in_channels, unsigned int out_channels, 
    unsigned int stride, unsigned int out_size_x, unsigned int out_size_y,
    unsigned int max_pool_size_x, unsigned int max_pool_size_y, 
    ImagePixelStream in_stream, bool is_pool = true) {

    ConvolutionalConstantMatrix mat = 
        ConvolutionalConstantMatrix::create
        (model, layerName, k_size_x, k_size_y, in_channels, out_channels, stride, out_size_x, out_size_y, is_pool);

    return maxpool(sig(mat*in_stream), max_pool_size_y, max_pool_size_x);

}

#endif

