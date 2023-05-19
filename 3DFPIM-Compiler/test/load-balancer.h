/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. 3D-FPIM Project can be copied according to the terms
* contained in the LICENSE file.
*******************************************************************************/

#include "3dfpim.h"

void balance_conv 
    (Model model,
    unsigned int k_size_x, 
    unsigned int k_size_y, 
    unsigned int in_size_x, 
    unsigned int in_size_y, 
    unsigned int in_channels, 
    unsigned int out_size_x,
    unsigned int out_size_y,
    unsigned int out_channels,
    unsigned int dac_scaling = 1)
{
    new Layer(model, k_size_x, k_size_y, in_size_x, in_size_y, in_channels,
                out_size_x, out_size_y, out_channels, dac_scaling, false);
}

void balance_fc
    (Model model,
    unsigned int k_size_x, 
    unsigned int k_size_y, 
    unsigned int in_size_x, 
    unsigned int in_size_y, 
    unsigned int in_channels, 
    unsigned int out_size_x,
    unsigned int out_size_y,
    unsigned int out_channels,
    unsigned int dac_scaling = 1)
{
    new Layer(model, k_size_x, k_size_y, in_size_x, in_size_y, in_channels,
                out_size_x, out_size_y, out_channels, dac_scaling, true);
}

void balance_basic_block
    (Model model,
    unsigned int in_size_x, 
    unsigned int in_size_y, 
    unsigned int in_channels, 
    unsigned int out_size_x,
    unsigned int out_size_y,
    unsigned int out_channels,
    unsigned int stride,
    unsigned int block_num)
{
    // iterate over blocks in layer
    for (int i = 0; i < block_num; i++) {
        if (i == 0 && in_channels != out_channels) {
            balance_conv(model, 1, 1, in_size_x, in_size_y, in_channels,
                    out_size_x, out_size_y, out_channels);
        }

        // First conv of block_i
        if (i == 0) {
            balance_conv(model, 3, 3, in_size_x, in_size_y, in_channels,
                    out_size_x, out_size_y, out_channels);
            in_size_x = out_size_x;
            in_size_y = out_size_y;
        }
        else {
            balance_conv(model, 3, 3, in_size_x, in_size_y, out_channels,
                    out_size_x, out_size_y, out_channels);
        }

        // Sesond conv of block_i
        balance_conv(model, 3, 3, in_size_x, in_size_y, out_channels,
                out_size_x, out_size_y, out_channels);
    }
}


void balance_bottleneck_block
    (Model model,
    unsigned int in_size_x, 
    unsigned int in_size_y, 
    unsigned int in_channels, 
    unsigned int out_size_x,
    unsigned int out_size_y,
    unsigned int out_channels,
    unsigned int stride,
    unsigned int block_num)
{
    unsigned int expansion = 4;

    // iterate over blocks in layer
    for (int i = 0; i < block_num; i++) {
        if (i == 0 && in_channels != out_channels * expansion) {
            balance_conv(model, 1, 1, in_size_x, in_size_y, in_channels,
                    out_size_x, out_size_y, out_channels*expansion);
        }

        // First conv of block_i
        if (i == 0) {
            balance_conv(model, 1, 1, in_size_x, in_size_y, in_channels,
                    in_size_x, in_size_y, out_channels);
        }
        else {
            balance_conv(model, 1, 1, in_size_x, in_size_y, out_channels*expansion,
                    in_size_x, in_size_y, out_channels);
        }

        // Sesond conv of block_i
        balance_conv(model, 3, 3, in_size_x, in_size_y, out_channels,
                out_size_x, out_size_y, out_channels);
        in_size_x = out_size_x;
        in_size_y = out_size_y;

        // Third conv of block_i
        balance_conv(model, 1, 1, in_size_x, in_size_y, out_channels,
                out_size_x, out_size_y, out_channels*expansion);
    }
}
