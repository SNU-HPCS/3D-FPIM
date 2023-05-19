
# 3D-FPIM Compiler

## Overview

This repository includes the compiler for 3D-FPIM: An Extreme Energy-Efficient DNN Acceleration System Using 3D NAND Flash-Based In-Situ PIM Unit.[^1]
We modified PUMA compiler[^2] and implemented 3D NAND Flash-specific optimizations described in the 3D-FPIM paper.[^1]

## Organization

The compiler is implemented in the `src` directory.
The programming interface is provided in the `include` directory.
Example programs are provided in the `test` directory.

## Setup

### 1. Configure the hardware parameters.

#### Configure the hardware parameters in the `src/common.h` file.
- The width and height of an MVM unit.
- The depth (i.e., The number of 3D stacked crossbar array in a single MVM unit.) of an MVM unit. 
- The number of MVM units in a core.
- The number of cores in a tile.
- The number of tiles in a chip.
- The size of register file in a core.


### 2. Define the network structure.

#### Define the target neural network in the `test` directory.
- Use `conv_layer()` in the `conv-layer.h` file to define a convolutional layer.
- Use `fully_connected_layer()` in the `fully-connected-layer.h` file to define a fully connected layer.
- Use `basic_block()` and `bottleneck_block()` in the `residual-block.h` file to define a basic block and a bottleneck block.


### 3. Load balancing.

#### Balance the load of operations among the MVMUs.
Provide the network structure to the compiler by calling `balance_conv()`, `balance_fc()`, `balance_bottleneck()` functions in the network definition file.
Then, call `model.loadBalance()` function to balance the loads among the MVMUs.


### 4. Compile and run.

#### (1) Build the compiler.
```sh
$ cd src
$ make
```

#### (2) Run the defined network.
```sh
$ cd test
$ make example-network.test
$ ./exmaple-network.test
```

## Citation

[^1]: H. Lee et al., **3D-FPIM: An Extreme Energy-Efficient DNN Acceleration System Using 3D NAND Flash-Based In-Situ PIM Unit,** *2022 55th IEEE/ACM International Symposium on Microarchitecture (MICRO)*, 2022.
[^2]: https://github.com/illinois-impact/puma-compiler
