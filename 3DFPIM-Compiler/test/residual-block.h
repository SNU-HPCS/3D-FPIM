/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. 3D-FPIM Project can be copied according to the terms
* contained in the LICENSE file.
*******************************************************************************/

#ifndef _PUMA_TEST_RESIDUAL_BLOCK_
#define _PUMA_TEST_RESIDUAL_BLOCK_

#include "3dfpim.h"
#include "conv-layer.h"

static ImagePixelStream bottleneck_block
    (Model model, std::string layerName, 
    unsigned int in_size_x, 
    unsigned int in_size_y, 
    unsigned int in_channels, 
    unsigned int out_size_x,
    unsigned int out_size_y,
    unsigned int out_channels, 
    unsigned int stride,
    unsigned int block_num,
    ImagePixelStream in_stream
) {

    unsigned int expansion = 4;

    ImagePixelStream conv_out;
    ImagePixelStream shortcut_out;
    ImagePixelStream block_out;

    // iterate over blocks in layer
    for (int i = 0; i < block_num; i++) {
        std::string blockName = layerName + "_" + std::to_string(i+1) + "_";

        // Shortcut path of block_i
        if (i == 0 && in_channels != out_channels * expansion) {
            shortcut_out = conv_layer(model, blockName + "sc",
                    1, 1, in_size_x, in_size_y, in_channels, out_channels*expansion, stride,
                    out_size_x, out_size_y, in_stream);
        }
        else {
            if (i == 0) shortcut_out = in_stream;
            else shortcut_out = block_out;
        }

        // First conv of block_i
        if (i == 0) {
            conv_out = conv_layer(model, blockName + std::to_string(1),
                    1, 1, in_size_x, in_size_y, in_channels, out_channels, 1,
                    in_size_x, in_size_y, in_stream);

            // only the first block performs downsampling
        }
        else {
            conv_out = conv_layer(model, blockName + std::to_string(1),
                    1, 1, in_size_x, in_size_y, out_channels*expansion, out_channels, 1,
                    in_size_x, in_size_y, block_out);
        }

        // Second conv of block_i
        if(i == 0){
            conv_out = conv_layer(model, blockName + std::to_string(2),
                    3, 3, in_size_x, in_size_y, out_channels, out_channels, stride,
                    out_size_x, out_size_y, conv_out);
        }
        else{
            conv_out = conv_layer(model, blockName + std::to_string(2),
                    3, 3, in_size_x, in_size_y, out_channels, out_channels, 1,
                    out_size_x, out_size_y, conv_out);
        }
        in_size_x = out_size_x;
        in_size_y = out_size_y;

        // Third conv of block_i (No activation function)
        conv_out = convnoact_layer(model, blockName + std::to_string(3),
                1, 1, in_size_x, in_size_y, out_channels, out_channels*expansion, 1,
                out_size_x, out_size_y, conv_out);

        // Output of block_i
        block_out = sig(shortcut_out + conv_out);
    }

    return block_out;
}

static ImagePixelStream basic_block
    (Model model, std::string layerName, 
    unsigned int in_size_x, 
    unsigned int in_size_y, 
    unsigned int in_channels, 
    unsigned int out_size_x,
    unsigned int out_size_y,
    unsigned int out_channels, 
    unsigned int stride,
    unsigned int block_num,
    ImagePixelStream in_stream) {

    ImagePixelStream conv_out;
    ImagePixelStream shortcut_out;
    ImagePixelStream block_out;

    // iterate over blocks in layer
    for (int i = 0; i < block_num; i++) {
        std::string blockName = layerName + "_" + std::to_string(i+1) + "_";

        // Shortcut path of block_i
        if (i == 0 && in_channels != out_channels) {
            shortcut_out = conv_layer(model, blockName + "sc",
                    1, 1, in_size_x, in_size_y, in_channels, out_channels, stride,
                    out_size_x, out_size_y, in_stream);
        }
        else {
            if (i == 0) shortcut_out = in_stream;
            else shortcut_out = block_out;
        }

        // First conv of block_i
        if (i == 0) {
            conv_out = conv_layer(model, blockName + std::to_string(1),
                    3, 3, in_size_x, in_size_y, in_channels, out_channels, stride,
                    out_size_x, out_size_y, in_stream);

            // only the first block performs downsampling
            stride = 1;
            in_size_x = out_size_x;
            in_size_y = out_size_y;
        }
        else {
            conv_out = conv_layer(model, blockName + std::to_string(1),
                    3, 3, in_size_x, in_size_y, out_channels, out_channels, stride,
                    out_size_x, out_size_y, block_out);
        }

        // Second conv of block_i
        conv_out = convnoact_layer(model, blockName + std::to_string(2),
                3, 3, in_size_x, in_size_y, out_channels, out_channels, stride,
                out_size_x, out_size_y, conv_out);

        // Output of block_i
        block_out = sig(shortcut_out + conv_out);
    }

    return block_out;
}

#endif
