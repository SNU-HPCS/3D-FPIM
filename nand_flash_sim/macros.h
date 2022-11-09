/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef MACROS_H_
#define MACROS_H_


#define INITIAL_BASIC_WIRE { \
    WireType basicWireType; \
    WireRepeaterType basicWireRepeaterType; \
    bool isBasicLowSwing; \
    if (inputParameter->minLocalWireType == inputParameter->maxLocalWireType) \
        basicWireType = (WireType)inputParameter->minLocalWireType; \
    else \
        basicWireType = local_aggressive; \
    if (inputParameter->minLocalWireRepeaterType == inputParameter->maxLocalWireRepeaterType) \
        basicWireRepeaterType = (WireRepeaterType)inputParameter->minLocalWireRepeaterType; \
    else \
        basicWireRepeaterType = repeated_none; \
    if (inputParameter->minIsLocalWireLowSwing == inputParameter->maxIsLocalWireLowSwing) \
        isBasicLowSwing = inputParameter->minIsLocalWireLowSwing; \
    else \
        isBasicLowSwing = false; \
    localWire->Initialize(inputParameter->processNode, basicWireType, basicWireRepeaterType, inputParameter->temperature, isBasicLowSwing); \
    selectlineWire->Initialize(inputParameter->processNode, selectline_wire, repeated_none, inputParameter->temperature, false); \
    wordlineWire->Initialize(inputParameter->processNode, wordline_wire, repeated_none, inputParameter->temperature, false); \
    contactWire->Initialize(inputParameter->processNode, contact_wire, repeated_none, inputParameter->temperature, false); \
    stringWire->Initialize(inputParameter->processNode, string_wire, repeated_none, inputParameter->temperature, false); \
    bitlineWire->Initialize(inputParameter->processNode, bitline_wire, repeated_none, inputParameter->temperature, false); \
    if (inputParameter->minGlobalWireType == inputParameter->maxGlobalWireType) \
        basicWireType = (WireType)inputParameter->minGlobalWireType; \
    else \
        basicWireType = global_aggressive; \
    if (inputParameter->minGlobalWireRepeaterType == inputParameter->maxGlobalWireRepeaterType) \
        basicWireRepeaterType = (WireRepeaterType)inputParameter->minGlobalWireRepeaterType; \
    else \
        basicWireRepeaterType = repeated_none; \
    if (inputParameter->minIsGlobalWireLowSwing == inputParameter->maxIsGlobalWireLowSwing) \
        isBasicLowSwing = inputParameter->minIsGlobalWireLowSwing; \
    else \
        isBasicLowSwing = false; \
    globalWire->Initialize(inputParameter->processNode, basicWireType, basicWireRepeaterType, inputParameter->temperature, isBasicLowSwing); \
}



#define REFINE_LOCAL_WIRE_FORLOOP \
    for (localWireType = inputParameter->minLocalWireType; localWireType <= inputParameter->maxLocalWireType; localWireType++) \
    for (localWireRepeaterType = inputParameter->minLocalWireRepeaterType; localWireRepeaterType <= inputParameter->maxLocalWireRepeaterType; localWireRepeaterType++) \
    for (isLocalWireLowSwing = inputParameter->minIsLocalWireLowSwing; isLocalWireLowSwing <= inputParameter->maxIsLocalWireLowSwing; isLocalWireLowSwing++) \
    if ((WireRepeaterType)localWireRepeaterType == repeated_none || (bool)isLocalWireLowSwing == false)


#define REFINE_GLOBAL_WIRE_FORLOOP \
    for (globalWireType = inputParameter->minGlobalWireType; globalWireType <= inputParameter->maxGlobalWireType; globalWireType++) \
    for (globalWireRepeaterType = inputParameter->minGlobalWireRepeaterType; globalWireRepeaterType <= inputParameter->maxGlobalWireRepeaterType; globalWireRepeaterType++) \
    for (isGlobalWireLowSwing = inputParameter->minIsGlobalWireLowSwing; isGlobalWireLowSwing <= inputParameter->maxIsGlobalWireLowSwing; isGlobalWireLowSwing++) \
    if ((WireRepeaterType)globalWireRepeaterType == repeated_none || (bool)isGlobalWireLowSwing == false)




#define LOAD_GLOBAL_WIRE(oldResult) { \
    globalWire->Initialize(inputParameter->processNode, (oldResult).globalWire->wireType, (oldResult).globalWire->wireRepeaterType, \
            inputParameter->temperature, (oldResult).globalWire->isLowSwing); \
}


#define LOAD_LOCAL_WIRE(oldResult) \
    localWire->Initialize(inputParameter->processNode, (oldResult).localWire->wireType, (oldResult).localWire->wireRepeaterType, \
            inputParameter->temperature, (oldResult).localWire->isLowSwing);



#define TRY_AND_UPDATE(oldResult, memoryType) { \
    trialSubArray = new SubArray(); \
    trialSubArray->Initialize(inputParameter->numRow, inputParameter->numColumn, inputParameter->muxSenseAmp, \
                              (oldResult).subarray->areaOptimizationLevel); \
    trialSubArray->CalculateArea(); \
    trialSubArray->CalculateLatency(1e41); \
    trialSubArray->CalculatePower(); \
    *(tempResult.subarray) = *trialSubArray; \
    *(tempResult.localWire) = *localWire; \
    *(tempResult.selectlineWire) = *selectlineWire; \
    *(tempResult.wordlineWire) = *wordlineWire; \
    *(tempResult.contactWire) = *contactWire; \
    *(tempResult.stringWire) = *stringWire; \
    *(tempResult.bitlineWire) = *bitlineWire; \
    *(tempResult.globalWire) = *globalWire; \
    oldResult.compareAndUpdate(tempResult); \
    delete trialSubArray; \
}


#define CALCULATE(subarray, memoryType) { \
    (subarray) = new SubArray(); \
    (subarray)->Initialize(inputParameter->numRow, inputParameter->numColumn, \
                           inputParameter->muxSenseAmp, \
                           (BufferDesignTarget)areaOptimizationLevel); \
    (subarray)->CalculateArea(); \
    (subarray)->CalculateLatency(1e41); \
    (subarray)->CalculatePower(); \
}


#define UPDATE_BEST_DATA { \
    *(tempResult.subarray) = *dataSubArray; \
    *(tempResult.bitlineWire) = *bitlineWire; \
    *(tempResult.stringWire) = *stringWire; \
    *(tempResult.wordlineWire) = *wordlineWire; \
    *(tempResult.selectlineWire) = *selectlineWire; \
    *(tempResult.contactWire) = *contactWire; \
    *(tempResult.localWire) = *localWire; \
    *(tempResult.globalWire) = *globalWire; \
    for (int i = 0; i < (int)full_exploration; i++) { \
        FILTER_PIM_MODE(i); \
        bestDataResults[i].compareAndUpdate(tempResult); \
    } \
}

#define FILTER_PIM_MODE(index) { \
    if(inputParameter->pimMode == false) { \
        if(index == (int)read_energy_optimized) break; \
        if(index == (int)read_edp_optimized) break; \
        if(index == (int)leakage_optimized) break; \
    } \
}


#define RESTORE_SEARCH_SIZE { \
    inputParameter->minAreaOptimizationLevel = latency_first; \
    inputParameter->maxAreaOptimizationLevel = area_first; \
    inputParameter->minLocalWireType = local_aggressive; \
    inputParameter->maxLocalWireType = semi_conservative; \
    inputParameter->minGlobalWireType = semi_aggressive; \
    inputParameter->maxGlobalWireType = global_conservative; \
    inputParameter->minLocalWireRepeaterType = repeated_none; \
    inputParameter->maxLocalWireRepeaterType = repeated_50;     /* The limit is repeated_50 */ \
    inputParameter->minGlobalWireRepeaterType = repeated_none; \
    inputParameter->maxGlobalWireRepeaterType = repeated_50;    /* The limit is repeated_50 */ \
    inputParameter->minIsLocalWireLowSwing = false; \
    inputParameter->maxIsLocalWireLowSwing = true; \
    inputParameter->minIsGlobalWireLowSwing = false; \
    inputParameter->maxIsGlobalWireLowSwing = true; \
}


#define APPLY_LIMIT(result) { \
    (result).reset(); \
    (result).limitReadLatency = allowedDataReadLatency; \
    (result).limitReadDynamicEnergy = allowedDataReadDynamicEnergy; \
    (result).limitReadEdp = allowedDataReadEdp; \
    (result).limitArea = allowedDataArea; \
    (result).limitLeakage = allowedDataLeakage; \
}


#define OUTPUT_TO_FILE { \
    if (inputParameter->designTarget == cache) { \
        for (int i = 0; i < (int)full_exploration; i++) { \
            FILTER_PIM_MODE(i); \
            tempResult.printAsCacheToCsvFile(bestTagResults[i], inputParameter->cacheAccessMode, outputFile); \
        } \
    } else { \
        tempResult.printToCsvFile(outputFile); \
        outputFile << endl; \
    } \
}

#define TO_SECOND(x) \
    ((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
    << \
    ((x) < 1e-9 ? "ps" : (x) < 1e-6 ? "ns" : (x) < 1e-3 ? "us" : (x) < 1 ? "ms" : "s")

#define TO_BPS(x) \
    ((x) < 1e3 ? (x) : (x) < 1e6 ? (x) / 1e3 : (x) < 1e9 ? (x) / 1e6 : (x) < 1e12 ? (x) / 1e9 : (x) / 1e12) \
    << \
    ((x) < 1e3 ? "B/s" : (x) < 1e6 ? "KB/s" : (x) < 1e9 ? "MB/s" : (x) < 1e12 ? "GB/s" : "TB/s")

#define TO_JOULE(x) \
    ((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
    << \
    ((x) < 1e-9 ? "pJ" : (x) < 1e-6 ? "nJ" : (x) < 1e-3 ? "uJ" : (x) < 1 ? "mJ" : "J")

#define TO_WATT(x) \
    ((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
    << \
    ((x) < 1e-9 ? "pW" : (x) < 1e-6 ? "nW" : (x) < 1e-3 ? "uW" : (x) < 1 ? "mW" : "W")

#define TO_METER(x) \
    ((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
    << \
    ((x) < 1e-9 ? "pm" : (x) < 1e-6 ? "nm" : (x) < 1e-3 ? "um" : (x) < 1 ? "mm" : "m")

#define TO_SQM(x) \
    ((x) < 1e-12 ? (x) * 1e18 : (x) < 1e-6 ? (x) * 1e12 : (x) < 1 ? (x) * 1e6 : (x)) \
    << \
    ((x) < 1e-12 ? "nm^2" : (x) < 1e-6 ? "um^2" : (x) < 1 ? "mm^2" : "m^2")

#endif /* MACROS_H_ */
