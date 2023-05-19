
# 3D-FPIM Compiler

## Overview

This repository includes the retraining framework for 3D-FPIM: An Extreme Energy-Efficient DNN Acceleration System Using 3D NAND Flash-Based In-Situ PIM Unit.[^1]
We added additional ADC aware retraining on top of the existing threshold-based quantization method [^2][^3].

## Organization

## Setup

### Define network and training parameters (constant.py)

- Set the network type in the config.py file (ResNet18, ResNet50, VGG16, VGG19)
- Set the checkpoint path to store the trained network
- Set the training or inference phase (BASELINEQUANT, FOLDINGBN, QACONV, INFERENCE)
- Set hardware configurations
  - GPU settings (or CPU)
  - Batch size
  - Calibration samples
  - Training epochs
- Set the bit-precision for the weight / bias / activation

### Run the network

## Citation

[^1]: H. Lee et al., **3D-FPIM: An Extreme Energy-Efficient DNN Acceleration System Using 3D NAND Flash-Based In-Situ PIM Unit,** *2022 55th IEEE/ACM International Symposium on Microarchitecture (MICRO)*, 2022.
[^2]: Jain, Sambhav, et al. "Trained quantization thresholds for accurate and efficient fixed-point inference of deep neural networks." Proceedings of Machine Learning and Systems 2 (2020).
[^3]: https://github.com/PannenetsF/TQT
