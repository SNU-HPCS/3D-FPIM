/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#include <assert.h>
#include <string>
#include <vector>
#include <cmath>

#include "3dfpim.h"
#include "load-balancer.h"
#include "conv-layer.h"
#include "fully-connected-layer.h"

int main() {

    Model model = Model::create("vgg19");

    // Input
    unsigned int in_size_x = 224;
    unsigned int in_size_y = 224;
    unsigned int in_channels = 3;
    auto in_stream = InputImagePixelStream::create
        (model, "in_stream", in_size_x, in_size_y, in_channels);

    // Layer 1 (convolution) configurations
    unsigned int k_size_x1 = 3;
    unsigned int k_size_y1 = 3;
    unsigned int in_size_x1 = 224;
    unsigned int in_size_y1 = 224;
    unsigned int in_channels1 = 3;
    unsigned int out_channels1 = 64;
    unsigned int stride1 = 1;
    unsigned int out_size_x1 = in_size_x1;
    unsigned int out_size_y1 = in_size_y1;

    // Layer 2 (convolution with max pool) configurations
    unsigned int k_size_x2 = 3;
    unsigned int k_size_y2 = 3;
    unsigned int in_size_x2 = 224;
    unsigned int in_size_y2 = 224;
    unsigned int in_channels2 = 64;
    unsigned int out_channels2 = 64;
    unsigned int stride2 = 1;
    unsigned int out_size_x2 = 224;
    unsigned int out_size_y2 = 224;
    unsigned int max_pool_size_x2 = 2;
    unsigned int max_pool_size_y2 = 2;

    // Layer 3 (convolution) configurations
    unsigned int k_size_x3 = 3;
    unsigned int k_size_y3 = 3;
    unsigned int in_size_x3 = 112;
    unsigned int in_size_y3 = 112;
    unsigned int in_channels3 = 64;
    unsigned int out_channels3 = 128;
	unsigned int stride3 = 1;
	unsigned int out_size_x3 = 112;
	unsigned int out_size_y3 = 112;

    // Layer 4 (convolution with max pool) configurations
    unsigned int k_size_x4 = 3;
    unsigned int k_size_y4 = 3;
    unsigned int in_size_x4 = 112;
    unsigned int in_size_y4 = 112;
    unsigned int in_channels4 = 128;
    unsigned int out_channels4 = 128;
	unsigned int stride4 = 1;
	unsigned int out_size_x4 = 112;
	unsigned int out_size_y4 = 112;
    unsigned int max_pool_size_x4 = 2;
    unsigned int max_pool_size_y4 = 2;

    // Layer 5 (convolution) configurations
    unsigned int k_size_x5 = 3;
    unsigned int k_size_y5 = 3;
    unsigned int in_size_x5 = 56;
    unsigned int in_size_y5 = 56;
    unsigned int in_channels5 = 128;
    unsigned int out_channels5 = 256;
	unsigned int stride5 = 1;
	unsigned int out_size_x5 = 56;
	unsigned int out_size_y5 = 56;

    // Layer 6 (convolution) configurations
    unsigned int k_size_x6 = 3;
    unsigned int k_size_y6 = 3;
    unsigned int in_size_x6 = 56;
    unsigned int in_size_y6 = 56;
    unsigned int in_channels6 = 256;
    unsigned int out_channels6 = 256;
	unsigned int stride6 = 1;
	unsigned int out_size_x6 = 56;
	unsigned int out_size_y6 = 56;

    // Layer 6x (convolution) configurations
    unsigned int k_size_x6x = 3;
    unsigned int k_size_y6x = 3;
    unsigned int in_size_x6x = 56;
    unsigned int in_size_y6x = 56;
    unsigned int in_channels6x = 256;
    unsigned int out_channels6x = 256;
	unsigned int stride6x = 1;
	unsigned int out_size_x6x = 56;
	unsigned int out_size_y6x = 56;

    // Layer 7 (convolution with max pool) configurations
    unsigned int k_size_x7 = 3;
    unsigned int k_size_y7 = 3;
    unsigned int in_size_x7 = 56;
    unsigned int in_size_y7 = 56;
    unsigned int in_channels7 = 256;
    unsigned int out_channels7 = 256;
	unsigned int stride7 = 1;
	unsigned int out_size_x7 = 56;
	unsigned int out_size_y7 = 56;
    unsigned int max_pool_size_x7 = 2;
    unsigned int max_pool_size_y7 = 2;

    // Layer 8 (convolution) configurations
    unsigned int k_size_x8 = 3;
    unsigned int k_size_y8 = 3;
    unsigned int in_size_x8 = 28;
    unsigned int in_size_y8 = 28;
    unsigned int in_channels8 = 256;
    unsigned int out_channels8 = 512;
	unsigned int stride8 = 1;
	unsigned int out_size_x8 = 28;
	unsigned int out_size_y8 = 28;

    // Layer 9 (convolution) configurations
    unsigned int k_size_x9 = 3;
    unsigned int k_size_y9 = 3;
    unsigned int in_size_x9 = 28;
    unsigned int in_size_y9 = 28;
    unsigned int in_channels9 = 512;
    unsigned int out_channels9 = 512;
	unsigned int stride9 = 1;
	unsigned int out_size_x9 = 28;
	unsigned int out_size_y9 = 28;

    // Layer 9x (convolution) configurations
    unsigned int k_size_x9x = 3;
    unsigned int k_size_y9x = 3;
    unsigned int in_size_x9x = 28;
    unsigned int in_size_y9x = 28;
    unsigned int in_channels9x = 512;
    unsigned int out_channels9x = 512;
	unsigned int stride9x = 1;
	unsigned int out_size_x9x = 28;
	unsigned int out_size_y9x = 28;

    // Layer 10 (convolution with max pool) configurations
    unsigned int k_size_x10 = 3;
    unsigned int k_size_y10 = 3;
    unsigned int in_size_x10 = 28;
    unsigned int in_size_y10 = 28;
    unsigned int in_channels10 = 512;
    unsigned int out_channels10 = 512;
	unsigned int stride10 = 1;
	unsigned int out_size_x10 = 28;
	unsigned int out_size_y10 = 28;
    unsigned int max_pool_size_x10 = 2;
    unsigned int max_pool_size_y10 = 2;

    // Layer 11 (convolution) configurations
    unsigned int k_size_x11 = 3;
    unsigned int k_size_y11 = 3;
    unsigned int in_size_x11 = 14;
    unsigned int in_size_y11 = 14;
    unsigned int in_channels11 = 512;
    unsigned int out_channels11 = 512;
	unsigned int stride11 = 1;
	unsigned int out_size_x11 = 14;
	unsigned int out_size_y11 = 14;

    // Layer 12 (convolution) configurations
    unsigned int k_size_x12 = 3;
    unsigned int k_size_y12 = 3;
    unsigned int in_size_x12 = 14;
    unsigned int in_size_y12 = 14;
    unsigned int in_channels12 = 512;
    unsigned int out_channels12 = 512;
	unsigned int stride12 = 1;
	unsigned int out_size_x12 = 14;
	unsigned int out_size_y12 = 14;

    // Layer 12x (convolution) configurations
    unsigned int k_size_x12x = 3;
    unsigned int k_size_y12x = 3;
    unsigned int in_size_x12x = 14;
    unsigned int in_size_y12x = 14;
    unsigned int in_channels12x = 512;
    unsigned int out_channels12x = 512;
	unsigned int stride12x = 1;
	unsigned int out_size_x12x = 14;
	unsigned int out_size_y12x = 14;

    // Layer 13 (convolution with max pool) configurations
    unsigned int k_size_x13 = 3;
    unsigned int k_size_y13 = 3;
    unsigned int in_size_x13 = 14;
    unsigned int in_size_y13 = 14;
    unsigned int in_channels13 = 512;
    unsigned int out_channels13 = 512;
	unsigned int stride13 = 1;
	unsigned int out_size_x13 = 14;
	unsigned int out_size_y13 = 14;
    unsigned int max_pool_size_x13 = 2;
    unsigned int max_pool_size_y13 = 2;

    // Layer 14 (fully-connected) configurations
    unsigned int k_size_x14 = 7;
    unsigned int k_size_y14 = 7;
    unsigned int in_size_x14 = 7;
    unsigned int in_size_y14 = 7;
    unsigned int in_channels14 = 512;
    unsigned int out_channels14 = 4096;
	unsigned int stride14 = 1;
	unsigned int out_size_x14 = 1;
	unsigned int out_size_y14 = 1;

    // Layer 15 (fully-connected) configurations
    unsigned int k_size_x15 = 1;
    unsigned int k_size_y15 = 1;
    unsigned int in_size_x15 = 1;
    unsigned int in_size_y15 = 1;
    unsigned int in_channels15 = 4096;
    unsigned int out_channels15 = 4096;
	unsigned int stride15 = 1;
	unsigned int out_size_x15 = 1;
	unsigned int out_size_y15 = 1;

    // Layer 16 (fully-connected) configurations
    unsigned int k_size_x16 = 1;
    unsigned int k_size_y16 = 1;
    unsigned int in_size_x16 = 1;
    unsigned int in_size_y16 = 1;
    unsigned int in_channels16 = 4096;
    unsigned int out_channels16 = 1000;
	unsigned int stride16 = 1;
	unsigned int out_size_x16 = 1;
	unsigned int out_size_y16 = 1;

    // Output
    unsigned int out_size_x = 1;
    unsigned int out_size_y = 1;
    unsigned int out_channels = 1000;
    auto out_stream = 
        OutputImagePixelStream::create
        (model, "out_stream", out_size_x, out_size_y, out_channels);

    // Load balance
    balance_conv(model, k_size_x1, k_size_y1, in_size_x1, in_size_y1, in_channels1,
            out_size_x1, out_size_y1, out_channels1);
    balance_conv(model, k_size_x2, k_size_y2, in_size_x2, in_size_y2, in_channels2,
            out_size_x2, out_size_y2, out_channels2);
    balance_conv(model, k_size_x3, k_size_y3, in_size_x3, in_size_y3, in_channels3,
            out_size_x3, out_size_y3, out_channels3);
    balance_conv(model, k_size_x4, k_size_y4, in_size_x4, in_size_y4, in_channels4,
            out_size_x4, out_size_y4, out_channels4);
    balance_conv(model, k_size_x5, k_size_y5, in_size_x5, in_size_y5, in_channels5,
            out_size_x5, out_size_y5, out_channels5);
    balance_conv(model, k_size_x6, k_size_y6, in_size_x6, in_size_y6, in_channels6,
            out_size_x6, out_size_y6, out_channels6);
    balance_conv(model, k_size_x6x, k_size_y6x, in_size_x6x, in_size_y6x, in_channels6x,
            out_size_x6x, out_size_y6x, out_channels6x);
    balance_conv(model, k_size_x7, k_size_y7, in_size_x7, in_size_y7, in_channels7,
            out_size_x7, out_size_y7, out_channels7);
    balance_conv(model, k_size_x8, k_size_y8, in_size_x8, in_size_y8, in_channels8,
            out_size_x8, out_size_y8, out_channels8);
    balance_conv(model, k_size_x9, k_size_y9, in_size_x9, in_size_y9, in_channels9,
            out_size_x9, out_size_y9, out_channels9);
    balance_conv(model, k_size_x9x, k_size_y9x, in_size_x9x, in_size_y9x, in_channels9x,
            out_size_x9x, out_size_y9x, out_channels9x);
    balance_conv(model, k_size_x10, k_size_y10, in_size_x10, in_size_y10, in_channels10,
            out_size_x10, out_size_y10, out_channels10);
    balance_conv(model, k_size_x11, k_size_y11, in_size_x11, in_size_y11, in_channels11,
            out_size_x11, out_size_y11, out_channels11);
    balance_conv(model, k_size_x12, k_size_y12, in_size_x12, in_size_y12, in_channels12,
            out_size_x12, out_size_y12, out_channels12);
    balance_conv(model, k_size_x12x, k_size_y12x, in_size_x12x, in_size_y12x, in_channels12x,
            out_size_x12x, out_size_y12x, out_channels12x);
    balance_conv(model, k_size_x13, k_size_y13, in_size_x13, in_size_y13, in_channels13,
            out_size_x13, out_size_y13, out_channels13);
    balance_fc(model, k_size_x14, k_size_y14, in_size_x14, in_size_y14, in_channels14,
            out_size_x14, out_size_y14, out_channels14);
    balance_fc(model, k_size_x15, k_size_y15, in_size_x15, in_size_y15, in_channels15,
            out_size_x15, out_size_y15, out_channels15);
    balance_fc(model, k_size_x16, k_size_y16, in_size_x16, in_size_y16, in_channels16,
            out_size_x16, out_size_y16, out_channels16);

    model.loadBalance();

    // Define network
    auto out1 = conv_layer(model, "conv01", k_size_x1, k_size_y1, in_size_x1, in_size_y1, in_channels1, out_channels1, stride1, out_size_x1, out_size_y1, in_stream);
    auto out2 = convmax_layer(model, "conv02", k_size_x2, k_size_y2, in_size_x2, in_size_y2, in_channels2, out_channels2, stride2, out_size_x2, out_size_y2, max_pool_size_x2, max_pool_size_y2, out1, /*is_pool*/true);
    auto out3 = conv_layer(model, "conv03", k_size_x3, k_size_y3, in_size_x3, in_size_y3, in_channels3, out_channels3, stride3, out_size_x3, out_size_y3, out2);
    auto out4 = convmax_layer(model, "conv04", k_size_x4, k_size_y4, in_size_x4, in_size_y4, in_channels4, out_channels4, stride4, out_size_x4, out_size_y4, max_pool_size_x4, max_pool_size_y4, out3, /*is_pool*/true);
    auto out5 = conv_layer(model, "conv05", k_size_x5, k_size_y5, in_size_x5, in_size_y5, in_channels5, out_channels5, stride5, out_size_x5, out_size_y5, out4);
    auto out6 = conv_layer(model, "conv06", k_size_x6, k_size_y6, in_size_x6, in_size_y6, in_channels6, out_channels6, stride6, out_size_x6, out_size_y6, out5);
    auto out6x = conv_layer(model, "conv06x", k_size_x6x, k_size_y6x, in_size_x6x, in_size_y6x, in_channels6x, out_channels6x, stride6x, out_size_x6x, out_size_y6x, out6);
    auto out7 = convmax_layer(model, "conv07", k_size_x7, k_size_y7, in_size_x7, in_size_y7, in_channels7, out_channels7, stride7, out_size_x7, out_size_y7, max_pool_size_x7, max_pool_size_y7, out6x, /*is_pool*/true);
    auto out8 = conv_layer(model, "conv08", k_size_x8, k_size_y8, in_size_x8, in_size_y8, in_channels8, out_channels8, stride8, out_size_x8, out_size_y8, out7);
    auto out9 = conv_layer(model, "conv09", k_size_x9, k_size_y9, in_size_x9, in_size_y9, in_channels9, out_channels9, stride9, out_size_x9, out_size_y9, out8);
    auto out9x = conv_layer(model, "conv09x", k_size_x9x, k_size_y9x, in_size_x9x, in_size_y9x, in_channels9x, out_channels9x, stride9x, out_size_x9x, out_size_y9x, out9);
    auto out10 = convmax_layer(model, "conv10", k_size_x10, k_size_y10, in_size_x10, in_size_y10, in_channels10, out_channels10, stride10, out_size_x10, out_size_y10, max_pool_size_x10, max_pool_size_y10, out9x, /*is_pool*/true);
    auto out11 = conv_layer(model, "conv11", k_size_x11, k_size_y11, in_size_x11, in_size_y11, in_channels11, out_channels11, stride11, out_size_x11, out_size_y11, out10);
    auto out12 = conv_layer(model, "conv12", k_size_x12, k_size_y12, in_size_x12, in_size_y12, in_channels12, out_channels12, stride12, out_size_x12, out_size_y12, out11);
    auto out12x = conv_layer(model, "conv12x", k_size_x12x, k_size_y12x, in_size_x12x, in_size_y12x, in_channels12x, out_channels12x, stride12x, out_size_x12x, out_size_y12x, out12);
    auto out13 = convmax_layer(model, "conv13", k_size_x13, k_size_y13, in_size_x13, in_size_y13, in_channels13, out_channels13, stride13, out_size_x13, out_size_y13, max_pool_size_x13, max_pool_size_y13, out12x, /*is_pool*/true);
    auto out14 = fully_connected_layer(model, "fc1", in_channels14, out_channels14, out13, k_size_x14, k_size_y14);
    auto out15 = fully_connected_layer(model, "fc2", in_channels15, out_channels15, out14, k_size_x15, k_size_y15);
    auto out16 = fully_connected_layer(model, "fc3", in_channels16, out_channels16, out15, k_size_x16, k_size_y16);

    out_stream = out16;

    // Compile
    model.compile();

    // Destroy model
    model.destroy();

    return 0;

}

