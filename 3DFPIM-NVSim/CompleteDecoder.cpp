/*******************************************************************************
* Copyright (c) 2022, Seoul National University. See LICENSE file in the top-
* level directory. 3D-FPIM Project can be copied according to the terms
* contained in the LICENSE file.
*******************************************************************************/


#include "CompleteDecoder.h"
#include "formula.h"
#include "global.h"
#include "macros.h"
#include <cmath>
#include <bits/stdc++.h>

CompleteDecoder::CompleteDecoder() : FunctionUnit(){
    // TODO Auto-generated constructor stub
    initialized = false;
    invalid = false;
}

CompleteDecoder::~CompleteDecoder() {
    // TODO Auto-generated destructor stub
}

void CompleteDecoder::Initialize(int _numDrivers, int _numBlocks, int _numStairs,
        double _capLoad, double _resLoad, double _tau,
        double _capLoadDischarge,
        double _tauDischarge,
        BufferDesignTarget _areaOptimizationLevel, 
        double _minDriverCurrent, bool _normal) {

    if (initialized)
        cout << "[Complete Decoder] Warning: Already initialized!" << endl;

    numDrivers = _numDrivers;

    capLoad = _capLoad;
    capLoadDischarge = _capLoadDischarge;
    resLoad = _resLoad;
    tau = _tau;
    tauDischarge = _tauDischarge;
    areaOptimizationLevel = _areaOptimizationLevel;
    minDriverCurrent = _minDriverCurrent;

    numBlocks = _numBlocks;
    numStairs = _numStairs;

    normal = _normal;
    vdd = normal ? tech->vdd : cell->flashPassVoltage;

    ////////////////////////////////////////////////////////////////////////

    //// Final rowdecoder for combining block decoder and wl decoder
    int numAddressRow = int(std::ceil(log2(numBlocks))) + int(std::ceil(log2(numStairs)));
    int numRows = std::pow(2, numAddressRow);

    finalRowDecoder.Initialize(numRows, capLoad, resLoad, tau,
                               latency_area_trade_off, minDriverCurrent, normal, capLoadDischarge, tauDischarge);

    if (finalRowDecoder.invalid){
        invalid = true;
        return;
    }

    // input capacitance of the final row decoder
    double capFinalRowDecoderIn = finalRowDecoder.capNandInput;

    ////////////////////////////////////////////////////////////////////////

    //// The total number of blocks and wordlines
    int totalBlocks = std::pow(2, int(std::ceil(log2(numBlocks))));
    int totalWls = std::pow(2, int(std::ceil(log2(numStairs))));

    int numAddressBlock = int(std::ceil(log2(numBlocks)));
    // multiply capFinalRowDecoderIn by totalWls (=> the number of connected NAND gates)
    blockRowDecoder.Initialize(totalBlocks, capFinalRowDecoderIn * totalWls * numDrivers, 0, 0,
                               areaOptimizationLevel, minDriverCurrent, normal);

    if (blockRowDecoder.invalid){
        invalid = true;
        return;
    }

    double capBlockRowDecoderIn = blockRowDecoder.capNandInput;

    // Predecoder for block

    int numAddressBlockPredecoderBlock1 = numAddressBlock;
    if (numAddressBlockPredecoderBlock1 < 0) {
        invalid = true;
        initialized = true;
        return;
    }
    int numAddressBlockPredecoderBlock2 = 0;
    if (numAddressBlockPredecoderBlock1 > 3) {
        numAddressBlockPredecoderBlock2 = numAddressBlockPredecoderBlock1 / 2;
        numAddressBlockPredecoderBlock1 = numAddressBlockPredecoderBlock1 - numAddressBlockPredecoderBlock2;
    }

    int numPredecoderBlocks1 = std::pow(2, numAddressBlockPredecoderBlock1);
    int numPredecoderBlocks2 = std::pow(2, numAddressBlockPredecoderBlock2);

    blockPredecoderBlock1.Initialize(numAddressBlockPredecoderBlock1, 
                                     capBlockRowDecoderIn * numPredecoderBlocks2, 0, normal);
    blockPredecoderBlock2.Initialize(numAddressBlockPredecoderBlock2, 
                                     capBlockRowDecoderIn * numPredecoderBlocks1, 0, normal);

    ////////////////////////////////////////////////////////////////////////

    // Predecoder for block
    
    int numAddressWl = int(std::ceil(log2(numStairs)));
    // multiply capFinalRowDecoderIn by totalBlocks (=> the number of connected NAND gates)
    wlRowDecoder.Initialize(totalWls, capFinalRowDecoderIn * totalBlocks * numDrivers, 0, 0,
                               areaOptimizationLevel, minDriverCurrent, normal);

    if (wlRowDecoder.invalid){
        invalid = true;
        return;
    }

    double capWlRowDecoderIn = wlRowDecoder.capNandInput;

    //// PREDECODER for wordline

    int numAddressWlPredecoderBlock1 = numAddressWl;
    if (numAddressWlPredecoderBlock1 < 0) {
        invalid = true;
        initialized = true;
        return;
    }
    int numAddressWlPredecoderBlock2 = 0;
    if (numAddressWlPredecoderBlock1 > 3) {
        numAddressWlPredecoderBlock2 = numAddressWlPredecoderBlock1 / 2;
        numAddressWlPredecoderBlock1 = numAddressWlPredecoderBlock1 - numAddressWlPredecoderBlock2;
    }

    int numPredecoderWl1 = std::pow(2, numAddressWlPredecoderBlock1);
    int numPredecoderWl2 = std::pow(2, numAddressWlPredecoderBlock2);

    wlPredecoderBlock1.Initialize(numAddressWlPredecoderBlock1, 
                                  capWlRowDecoderIn * numPredecoderWl2, 0, normal);
    wlPredecoderBlock2.Initialize(numAddressWlPredecoderBlock2, 
                                  capWlRowDecoderIn * numPredecoderWl1, 0, normal);

    initialized = true;
}

void CompleteDecoder::CalculateArea() {
    if (!initialized) {
        cout << "[Complete Decoder Area] Error: Require initialization first!" << endl;
    } else {
        finalRowDecoder.CalculateArea();
        
        blockPredecoderBlock1.CalculateArea();
        
        blockPredecoderBlock2.CalculateArea();
        blockRowDecoder.CalculateArea();
        
        wlPredecoderBlock1.CalculateArea();
        wlPredecoderBlock2.CalculateArea();

        wlRowDecoder.CalculateArea();

        // we have multiple drivers as they are activated at once
        // different from the conventional NAND flash
        double allDecoderBlocks = finalRowDecoder.area * numDrivers + 
                                  blockPredecoderBlock1.area +
                                  blockPredecoderBlock2.area +
                                  blockRowDecoder.area + 
                                  wlPredecoderBlock1.area + 
                                  wlPredecoderBlock2.area +
                                  wlRowDecoder.area;

        area = allDecoderBlocks;
    }
}

void CompleteDecoder::CalculateRC() {
    if (!initialized) {
        cout << "[Row Decoder RC] Error: Require initialization first!" << endl;
    } else {
        finalRowDecoder.CalculateRC();
        blockPredecoderBlock1.CalculateRC();
        blockPredecoderBlock2.CalculateRC();
        blockRowDecoder.CalculateRC();
        wlPredecoderBlock1.CalculateRC();
        wlPredecoderBlock2.CalculateRC();
        wlRowDecoder.CalculateRC();
    }
}

void CompleteDecoder::CalculateLatency(double _rampInput) {
    if (!initialized) {
        cout << "[Row Decoder Latency] Error: Require initialization first!" << endl;
    } else {
        blockPredecoderBlock1.CalculateLatency(_rampInput);
        blockPredecoderBlock2.CalculateLatency(_rampInput);
        blockRowDecoder.CalculateLatency(MAX(blockPredecoderBlock1.rampOutput, blockPredecoderBlock2.rampOutput));

        wlPredecoderBlock1.CalculateLatency(_rampInput);
        wlPredecoderBlock2.CalculateLatency(_rampInput);
        wlRowDecoder.CalculateLatency(MAX(wlPredecoderBlock1.rampOutput, wlPredecoderBlock2.rampOutput));

        finalRowDecoder.CalculateLatency(MAX(blockRowDecoder.rampOutput, wlRowDecoder.rampOutput));

        rampOutput = finalRowDecoder.rampOutput;

        double blockPredecoderLatency = MAX(blockPredecoderBlock1.readLatency, blockPredecoderBlock2.readLatency);
        blockPredecoderLatency += blockRowDecoder.readLatency;
        double wlPredecoderLatency = MAX(wlPredecoderBlock1.readLatency, wlPredecoderBlock2.readLatency);
        wlPredecoderLatency += wlRowDecoder.readLatency;

        double predecoderLatency = MAX(blockPredecoderLatency, wlPredecoderLatency);

        readLatency = predecoderLatency + finalRowDecoder.readLatency;
        dischargeLatency = finalRowDecoder.dischargeLatency;
    }
}

void CompleteDecoder::CalculatePower() {
    if (!initialized) {
        cout << "[Row Decoder Power] Error: Require initialization first!" << endl;
    } else {
        finalRowDecoder.CalculatePower();
        blockPredecoderBlock1.CalculatePower();
        blockPredecoderBlock2.CalculatePower();
        blockRowDecoder.CalculatePower();
        wlPredecoderBlock1.CalculatePower();
        wlPredecoderBlock2.CalculatePower();
        wlRowDecoder.CalculatePower();

        // Should support both
        // Decoding Energy
        // WL Read Energy
        decodingDynamicEnergy = blockPredecoderBlock1.readDynamicEnergy +
                                blockPredecoderBlock2.readDynamicEnergy +
                                blockRowDecoder.readDynamicEnergy + 
                                wlPredecoderBlock1.readDynamicEnergy + 
                                wlPredecoderBlock2.readDynamicEnergy +
                                wlRowDecoder.readDynamicEnergy;

        wlReadDynamicEnergy = finalRowDecoder.readDynamicEnergy * numDrivers;

        leakage = finalRowDecoder.leakage * numDrivers + 
                  blockPredecoderBlock1.leakage +
                  blockPredecoderBlock2.leakage +
                  blockRowDecoder.leakage + 
                  wlPredecoderBlock1.leakage + 
                  wlPredecoderBlock2.leakage +
                  wlRowDecoder.leakage;
    }
}

void CompleteDecoder::PrintProperty() {
    cout << "Complete Decoder Properties:" << endl;
    FunctionUnit::PrintProperty();
}

CompleteDecoder & CompleteDecoder::operator=(const CompleteDecoder &rhs) {
    FunctionUnit::operator=(rhs);

    initialized = rhs.initialized;
    invalid = rhs.invalid;

    finalRowDecoder = rhs.finalRowDecoder;

    blockPredecoderBlock1 = rhs.blockPredecoderBlock1;
    blockPredecoderBlock2 = rhs.blockPredecoderBlock2;

    blockRowDecoder = rhs.blockRowDecoder;

    wlPredecoderBlock1 = rhs.wlPredecoderBlock1;
    wlPredecoderBlock2 = rhs.wlPredecoderBlock2;
    wlRowDecoder = rhs.wlRowDecoder;

    capLoad = rhs.capLoad;
    capLoadDischarge = rhs.capLoadDischarge;
    resLoad = rhs.resLoad;
    areaOptimizationLevel = rhs.areaOptimizationLevel;
    minDriverCurrent = rhs.minDriverCurrent;

    normal = rhs.normal;
    vdd = rhs.vdd;

    decodingDynamicEnergy = rhs.decodingDynamicEnergy;
    wlReadDynamicEnergy = rhs.wlReadDynamicEnergy;
    shiftEnergy = rhs.shiftEnergy;

    rampOutput = rhs.rampOutput;

    tau = rhs.tau;
    tauDischarge = rhs.tauDischarge;
    dischargeLatency = rhs.dischargeLatency;

    return *this;
}
