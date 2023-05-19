
# 3D-FPIM Compiler

## Overview

This repository includes the retraining framework for 3D-FPIM: An Extreme Energy-Efficient DNN Acceleration System Using 3D NAND Flash-Based In-Situ PIM Unit.[^1]
We added additional ADC aware retraining on top of the existing threshold-based quantization method [^2][^3].

## Organization

## Setup

### 1. Define network type and 

#### Configure the hardware parameters in the `src/common.h` file.
- The width and height of an MVM unit.
- The depth (i.e., The number of 3D stacked crossbar array in a single MVM unit.) of an MVM unit. 
- The number of MVM units in a core.
- The number of cores in a tile.
- The number of tiles in a chip.
- The size of register file in a core.
