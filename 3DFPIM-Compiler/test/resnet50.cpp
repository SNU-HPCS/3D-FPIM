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
#include "residual-block.h"
#include "fully-connected-layer.h"

int main() {

    Model model = Model::create("resnet50");

    // Input
    unsigned int in_size_x = 224;
    unsigned int in_size_y = 224;
    unsigned int in_channels = 3;
    auto in_stream = InputImagePixelStream::create
        (model, "in_stream", in_size_x, in_size_y, in_channels);

    // Layer 1 (convolution) configurations
    unsigned int k_size_x1 = 7;
    unsigned int k_size_y1 = 7;
    unsigned int in_size_x1 = 224;
    unsigned int in_size_y1 = 224;
    unsigned int in_channels1 = 3;
    unsigned int out_channels1 = 64;
    unsigned int stride1 = 2;
    unsigned int out_size_x1 = 112;
    unsigned int out_size_y1 = 112;
    unsigned int max_pool_size_x1 = 2;
    unsigned int max_pool_size_y1 = 2;

    // Layer 2 (residual block (BottleNeck)) configurations
    unsigned int in_size_x2 = 56;
    unsigned int in_size_y2 = 56;
    unsigned int in_channels2 = 64;
    unsigned int out_size_x2 = 56;
    unsigned int out_size_y2 = 56;
    unsigned int out_channels2 = 64;
    unsigned int stride2 = 1;
    unsigned int block_num2 = 3;

    // Layer 3 (residual block (BottleNeck)) configurations
    unsigned int in_size_x3 = 56;
    unsigned int in_size_y3 = 56;
    unsigned int in_channels3 = 256;
    unsigned int out_size_x3 = 28;
    unsigned int out_size_y3 = 28;
    unsigned int out_channels3 = 128;
    unsigned int stride3 = 2;
    unsigned int block_num3 = 4;

    // Layer 4 (residual block (BottleNeck)) configurations
    unsigned int in_size_x4 = 28;
    unsigned int in_size_y4 = 28;
    unsigned int in_channels4 = 512;
    unsigned int out_size_x4 = 14;
    unsigned int out_size_y4 = 14;
    unsigned int out_channels4 = 256;
    unsigned int stride4 = 2;
    unsigned int block_num4 = 6;

    // Layer 5 (residual block (BottleNeck)) configurations
    unsigned int in_size_x5 = 14;
    unsigned int in_size_y5 = 14;
    unsigned int in_channels5 = 1024;
    unsigned int out_size_x5 = 7;
    unsigned int out_size_y5 = 7;
    unsigned int out_channels5 = 512;
    unsigned int stride5 = 2;
    unsigned int block_num5 = 3;

    // Layer 6 (FC) configurations
    unsigned int max_pool_size_x6 = 7;
    unsigned int max_pool_size_y6 = 7;
    unsigned int k_size_x6 = out_size_x5 / max_pool_size_x6;
    unsigned int k_size_y6 = out_size_y5 / max_pool_size_y6;
    unsigned int in_size_x6 = 1;
    unsigned int in_size_y6 = 1;
    unsigned int in_channels6 = 2048;
    unsigned int out_channels6 = 1000;
    unsigned int out_size_x6 = 1;
    unsigned int out_size_y6 = 1;

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
    balance_bottleneck_block(model, in_size_x2, in_size_y2, in_channels2,
        out_size_x2, out_size_y2, out_channels2, stride2, block_num2);
    balance_bottleneck_block(model, in_size_x3, in_size_y3, in_channels3,
        out_size_x3, out_size_y3, out_channels3, stride3, block_num3);
    balance_bottleneck_block(model, in_size_x4, in_size_y4, in_channels4,
        out_size_x4, out_size_y4, out_channels4, stride4, block_num4);
    balance_bottleneck_block(model, in_size_x5, in_size_y5, in_channels5,
        out_size_x5, out_size_y5, out_channels5, stride5, block_num5);
    balance_fc(model, k_size_x6, k_size_y6, in_size_x6, in_size_y6, in_channels6,
        out_size_x6, out_size_y6, out_channels6);

    model.loadBalance();


    // Define network
    auto out1 = conv_layer(model, "conv1", k_size_x1, k_size_y1, in_size_x1, in_size_y1, in_channels1, out_channels1, stride1, out_size_x1, out_size_y1, in_stream, true);
    auto out2 = bottleneck_block(model, "conv2", in_size_x2, in_size_y2, in_channels2, out_size_x2, out_size_y2, out_channels2, stride2, block_num2, maxpool(out1, max_pool_size_x1, max_pool_size_y1));
    auto out3 = bottleneck_block(model, "conv3", in_size_x3, in_size_y3, in_channels3, out_size_x3, out_size_y3, out_channels3, stride3, block_num3, out2);
    auto out4 = bottleneck_block(model, "conv4", in_size_x4, in_size_y4, in_channels4, out_size_x4, out_size_y4, out_channels4, stride4, block_num4, out3);
    auto out5 = bottleneck_block(model, "conv5", in_size_x5, in_size_y5, in_channels5, out_size_x5, out_size_y5, out_channels5, stride5, block_num5, out4);
    auto out6 = fully_connected_layer(model, "fc1", in_channels6, out_channels6, maxpool(out5, max_pool_size_x6, max_pool_size_y6), k_size_x6, k_size_y6);

    out_stream = out6;

    // Compile
    model.compile();

    // Destroy model
    model.destroy();

    return 0;

}

