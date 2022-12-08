/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#ifndef _PUMA_TEST_FULLY_CONNECTED_LAYER_
#define _PUMA_TEST_FULLY_CONNECTED_LAYER_

#include "3dfpim.h"

static ImagePixelStream fully_connected_layer
    (Model model, std::string layerName, 
    unsigned int in_channels, unsigned int out_channels, 
    ImagePixelStream in_stream, unsigned int k_size_x = 1, unsigned int k_size_y = 1) {

    FCConstantMatrix mat = 
        FCConstantMatrix::create
        (model, layerName, in_channels, out_channels, k_size_x, k_size_y);

    return sig(mat*in_stream);
}

static Vector fully_connected_vector_layer(Model model, std::string layerName, unsigned int in_size, unsigned int out_size, Vector in) {

    ConstantMatrix mat = ConstantMatrix::create
        (model, layerName + "mat", in_size, out_size);

    return sig(mat*in);

}

#endif

