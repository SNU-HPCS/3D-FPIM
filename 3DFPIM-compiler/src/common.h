/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from puma-compiler, (c) 2019,
* University of Illinois. See LICENSE_PUMA file in the parent directory.
* 3D-FPIM Project can be copied according to the terms contained in the
* LICENSE file.
*******************************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

#include "3dfpim.h"

/* Constants */

// The size of column/row/depth of the crossbar array
#define MVMU_DIM                        128
#define MVMU_DPT                        64

#define N_CONSTANT_MVMUS_PER_CORE       1
#define N_CORES_PER_TILE                24
#define MAX_LOAD_STORE_WIDTH            8
#define MAX_SEND_RECV_WIDTH             8
#define N_INPUT_REGISTERS               MVMU_DIM * N_CONSTANT_MVMUS_PER_CORE
#define N_OUTPUT_REGISTERS              MVMU_DIM * N_CONSTANT_MVMUS_PER_CORE
#define INPUT_REGISTERS_START_ADDRESS   0
#define OUTPUT_REGISTERS_START_ADDRESS  (INPUT_REGISTERS_START_ADDRESS + N_INPUT_REGISTERS)
#define REGISTER_FILE_START_ADDRESS     (OUTPUT_REGISTERS_START_ADDRESS + N_OUTPUT_REGISTERS)
#define REGISTER_FILE_SIZE              (N_OUTPUT_REGISTERS) * 4
#define REGISTERS_PER_CORE              (N_INPUT_REGISTERS + N_OUTPUT_REGISTERS + REGISTER_FILE_SIZE)

#define STACK_REUSE_LATENCY             105
#define STACK_SHIFT_LATENCY             170
#define PRECHARGE_LATENCY               32
#define ADC_LATENCY                     256

#define N_MAX_TILE                      14 * 16

/* tensors.h */
class AbstractTensor;
class AbstractVector;
class AbstractMatrix;
class AbstractImagePixelStream;
class InputVectorTile;
class InputVectorImpl;
class InputImagePixelStreamImpl;
class VectorImpl;
class ImagePixelStreamImpl;
class OutputVectorTile;
class OutputVectorImpl;
class OutputImagePixelStreamImpl;
class ConstantMatrixTile;
class ConstantMatrixImpl;
class ConvolutionalConstantMatrixImpl;
class FCConstantMatrixImpl;

/* operations.h */
class Operation;
class ProducerOperation;
class ConsumerOperation;
class TileMemoryWriteOperation;
class TileMemoryReadOperation;
class InputOperation;
class OutputOperation;
class CoreOperation;
class TileOperation;
class MVMOperation;
class CoalescedMVMSet;
class CoalescedMergedMVMSet;
class MergedMVMSet;
class SlidingMergedMVMSet;
class ALUVectorOperation;
class SetImmediateOperation;
class CopyOperation;
class LoadOperation;
class StoreOperation;
class MVMGuardOperation;
class SendOperation;
class ReceiveOperation;
class WriteInputOperation;
class ReadOutputOperation;
class PseudoInputOperation;
class PseudoOutputOperation;

/* allocator.h */
class CoreAllocator;
class SpillTracker;

/* model.h */
class ModelImpl;

/* partitioner.h */
class Partitioner;

/* placer.h */
class Placer;

/* memalloc.h */
class MemoryAllocator;

/* coalescer.h */
class Coalescer;

/* linearizer.h */
class Linearizer;

/* regalloc.h */
class RegisterAllocator;

/* codegen.h */
class CodeGenerator;

#endif

