/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "PredecodeBlock.h"
#include "formula.h"
#include "global.h"
#include <cmath>
#include <bits/stdc++.h>

PredecodeBlock::PredecodeBlock() {
    // TODO Auto-generated constructor stub
    initialized = false;
    rowDecoderStage1A = NULL;
    rowDecoderStage1B = NULL;
    rowDecoderStage1C = NULL;
    rowDecoderStage2 = NULL;
    basicDecoderA1 = NULL;
    basicDecoderA2 = NULL;
    basicDecoderB = NULL;
    basicDecoderC = NULL;
}

PredecodeBlock::~PredecodeBlock() {
    // TODO Auto-generated destructor stub
    if (rowDecoderStage1A)
        delete rowDecoderStage1A;
    if (rowDecoderStage1B)
        delete rowDecoderStage1B;
    if (rowDecoderStage1C)
        delete rowDecoderStage1C;
    if (rowDecoderStage2)
        delete rowDecoderStage2;
    if (basicDecoderA1)
        delete basicDecoderA1;
    if (basicDecoderA2)
        delete basicDecoderA2;
    if (basicDecoderB)
        delete basicDecoderB;
    if (basicDecoderC)
        delete basicDecoderC;
}

void PredecodeBlock::Initialize(int _numAddressBit, double _capLoad, double _resLoad, bool _normal) {
    if (initialized)
        cout << "[Predecoder Block] Warning: Already initialized!" << endl;

    numAddressBit =_numAddressBit;
    normal = _normal;
    if (numAddressBit > 27 ) {
        cout << "[Predecoder Block] Error: Invalid number of address bits" <<endl;
        exit(-1);
    } else if (numAddressBit == 0) {
        height = width = area = 0;
        readLatency = 0;
        readDynamicEnergy = 0;
        leakage = 0;
        initialized = true;
    } else {
        capLoad = _capLoad;
        resLoad = _resLoad;

        numDecoder12 = numDecoder24 = numDecoder38 = 0;
        numOutputAddressBit = 1 << numAddressBit;
        if (numAddressBit == 1) {
            numDecoder12 = 1;
        } else {
            // counter the number of decoder blocks & type
            // numDecoder24 => 2 input 4 output decoder
            // numDecoder38 => 3 input 8 output decoder
            // numOutputAddressBit => total number of address bits
            int numAddressMod3 = numAddressBit % 3;
            if (numAddressMod3 == 2) {
                numDecoder24 = 1;
            } else if (numAddressMod3 == 1) {
                numDecoder24 = 2;
            }
            numDecoder38 = (numAddressBit - 2 * numDecoder24) / 3;
        }
        int numBasicDecoder = numDecoder12 + numDecoder24 + numDecoder38;
        if (numBasicDecoder <= 1){
            rowDecoderStage1A = NULL;
            rowDecoderStage1B = NULL;
            rowDecoderStage1C = NULL;
            rowDecoderStage2 = NULL;
        } else if (numBasicDecoder <= 3) {
            // initialize only 1A stage
            numNandInputStage1A = numBasicDecoder;
            rowDecoderStage2 = NULL;
            rowDecoderStage1B = NULL;
            rowDecoderStage1C = NULL;
            rowDecoderStage1A = new RowDecoder;
            rowDecoderStage1A->Initialize(numOutputAddressBit, capLoad, resLoad, capLoad * resLoad / 2, latency_first, 0, normal);
            rowDecoderStage1A->CalculateRC();
        } else {
            rowDecoderStage2 = new RowDecoder;
            double capLoadStage1A, capLoadStage1B, capLoadStage1C;
            if (numBasicDecoder <= 6) {
                rowDecoderStage2->Initialize(numOutputAddressBit, capLoad, resLoad, capLoad * resLoad / 2, latency_first, 0, normal);
                rowDecoderStage2->CalculateRC();
                numNandInputStage1B = numBasicDecoder / 2;
                numNandInputStage1A = numBasicDecoder - numNandInputStage1B;
                numAddressBitStage1A = numAddressBitStage1B = 1;
                int i = 3 * numNandInputStage1A - numDecoder24;
                numAddressBitStage1A <<= i;
                numAddressBitStage1B <<= 3 * numNandInputStage1B;
                capLoadStage1A = numAddressBitStage1B * rowDecoderStage2->capNandInput;
                capLoadStage1B = numAddressBitStage1A * rowDecoderStage2->capNandInput;
                rowDecoderStage1C = NULL;
                rowDecoderStage1A = new RowDecoder;
                rowDecoderStage1A->Initialize(numAddressBitStage1A, capLoadStage1A, 0 /* TO-DO */, 0, latency_first, 0, normal);
                rowDecoderStage1A->CalculateRC();
                rowDecoderStage1B = new RowDecoder;
                rowDecoderStage1B->Initialize(numAddressBitStage1B, capLoadStage1B, 0 /* TO-DO */, 0, latency_first, 0, normal);
                rowDecoderStage1B->CalculateRC();
            } else if (numBasicDecoder <= 9){
                rowDecoderStage2->Initialize(numOutputAddressBit, capLoad, resLoad, capLoad * resLoad / 2, latency_first, 0, normal);
                rowDecoderStage2->CalculateRC();
                // only Stage1A can possibly contain non-3-input NAND
                if (numBasicDecoder == 7) {
                    numNandInputStage1A = 3;
                    numNandInputStage1B = 2;
                    numNandInputStage1C = 2;
                    // 1B / 1C only contains 3-input NAND
                    // => 2 ** (3 * numNand)
                    numAddressBitStage1B = int(std::pow(2, 3 * numNandInputStage1B));
                    numAddressBitStage1C = int(std::pow(2, 3 * numNandInputStage1C));

                    assert(numAddressBitStage1B == 64);
                    assert(numAddressBitStage1C == 64);
                } else if (numBasicDecoder == 8) {
                    numNandInputStage1A = 3;
                    numNandInputStage1B = 3;
                    numNandInputStage1C = 2;
                    numAddressBitStage1B = int(std::pow(2, 3 * numNandInputStage1B));
                    numAddressBitStage1C = int(std::pow(2, 3 * numNandInputStage1C));

                    assert(numAddressBitStage1B == 512);
                    assert(numAddressBitStage1C == 64);
                } else {
                    numNandInputStage1A = 3;
                    numNandInputStage1B = 3;
                    numNandInputStage1C = 3;
                    numAddressBitStage1B = int(std::pow(2, 3 * numNandInputStage1B));
                    numAddressBitStage1C = int(std::pow(2, 3 * numNandInputStage1C));

                    assert(numAddressBitStage1B == 512);
                    assert(numAddressBitStage1C == 512);
                }
                int i = 3 * numNandInputStage1A - numDecoder24;
                numAddressBitStage1A <<= i;
                capLoadStage1A = numAddressBitStage1B * 
                        numAddressBitStage1C * rowDecoderStage2->capNandInput;
                capLoadStage1B = numAddressBitStage1A * 
                        numAddressBitStage1C * rowDecoderStage2->capNandInput;
                capLoadStage1C = numAddressBitStage1A * 
                        numAddressBitStage1B * rowDecoderStage2->capNandInput;
                rowDecoderStage1A = new RowDecoder;
                rowDecoderStage1A->Initialize(numAddressBitStage1A, capLoadStage1A, 0 /* TO-DO */, 0, latency_first, 0, normal);
                rowDecoderStage1A->CalculateRC();
                rowDecoderStage1B = new RowDecoder;
                rowDecoderStage1B->Initialize(numAddressBitStage1B, capLoadStage1B, 0 /* TO-DO */, 0, latency_first, 0, normal);
                rowDecoderStage1B->CalculateRC();
                rowDecoderStage1C = new RowDecoder;
                rowDecoderStage1C->Initialize(numAddressBitStage1C, capLoadStage1C, 0 /* TO-DO */, 0, latency_first, 0, normal);
                rowDecoderStage1C->CalculateRC();
            }
        }
        if (rowDecoderStage1C != NULL) {
            if (numNandInputStage1C == 2){
                // it always has three input NAND
                capLoadBasicDecoderC = int(std::pow(2, 3)) * rowDecoderStage1C->capNandInput;
            } else {
                capLoadBasicDecoderC = int(std::pow(2, 3)) * int(std::pow(2, 3)) * rowDecoderStage1C->capNandInput;
            }
            basicDecoderC = new BasicDecoder;
            basicDecoderC->Initialize(3, capLoadBasicDecoderC, 0 /* TO-DO */, normal);
        } else {
            basicDecoderC = NULL;
        }
        if (rowDecoderStage1B != NULL) {
            if (numNandInputStage1B == 2){
                capLoadBasicDecoderB = int(std::pow(2, 3)) * rowDecoderStage1B->capNandInput;
            } else {
                capLoadBasicDecoderB = int(std::pow(2, 3)) * int(std::pow(2, 3)) * rowDecoderStage1B->capNandInput;
            }
            basicDecoderB = new BasicDecoder;
            basicDecoderB->Initialize(3, capLoadBasicDecoderB, 0 /* TO-DO */, normal);
        } else {
            basicDecoderB = NULL;
        }
        if (rowDecoderStage1A != NULL) {
            int numCapNandA1, numCapNandA2;
            if (numDecoder24 == 0) {
                numBasicDecoderA1 = numNandInputStage1A;
                numBasicDecoderA2 = 0;
                numCapNandA1 = 1 << ( 3* (numNandInputStage1A - 1));
                capLoadBasicDecoderA1 = numCapNandA1 * rowDecoderStage1A->capNandInput;
                basicDecoderA1 = new BasicDecoder;
                basicDecoderA1->Initialize(3, capLoadBasicDecoderA1, 0 /* TO-DO */, normal);
                basicDecoderA2 = NULL;
            } else if (numDecoder24 == 1) {
                numBasicDecoderA1 = 1;
                numBasicDecoderA2 = numNandInputStage1A - numBasicDecoderA1;
                numCapNandA1 = 1 << (3 * numBasicDecoderA2);
                numCapNandA2 = 1 << (2 + 3 * (numBasicDecoderA2 - 1));
                capLoadBasicDecoderA1 = numCapNandA1 * rowDecoderStage1A->capNandInput;
                capLoadBasicDecoderA2 = numCapNandA2 * rowDecoderStage1A->capNandInput;
                basicDecoderA1 = new BasicDecoder;
                basicDecoderA1->Initialize(2, capLoadBasicDecoderA1, 0 /* TO-DO */, normal);
                basicDecoderA2 = new BasicDecoder;
                basicDecoderA2->Initialize(3, capLoadBasicDecoderA2, 0 /* TO-DO */, normal);
            } else if (numDecoder24 == 2) {
                // if the two 2 4 decoders are all
                if (numNandInputStage1A == 2) {
                    numBasicDecoderA1 = 2;
                    numBasicDecoderA2 = 0;
                    basicDecoderA1 = new BasicDecoder;
                    basicDecoderA1->Initialize(2, int(std::pow(2, 2)) * rowDecoderStage1A->capNandInput, 
                            0 /* TO-DO */, normal);
                    basicDecoderA2 = NULL;
                // if there two 2 4 decoders + one 3 8 decoder
                } else {
                    numBasicDecoderA1 = 2;
                    numBasicDecoderA2 = 1;
                    basicDecoderA1 = new BasicDecoder;
                    // the two input nands are connected against one two input + one three input
                    basicDecoderA1->Initialize(2, int(std::pow(2, 2)) * int(std::pow(2, 3)) * rowDecoderStage1A->capNandInput, 
                            0 /* TO-DO */, normal);
                    basicDecoderA2 = new BasicDecoder;
                    // the three input nands are connected against one two two input
                    basicDecoderA2->Initialize(3, int(std::pow(2, 2)) * int(std::pow(2, 2)) * rowDecoderStage1A->capNandInput, 
                            0 /* TO-DO */, normal);
                }
            }
        }else {
            numBasicDecoderA1 = 1;
            numBasicDecoderA2 = 0;
            basicDecoderA1 = new BasicDecoder;
            basicDecoderA2 = NULL;
            if (numDecoder12 == 1) {
                basicDecoderA1->Initialize(1, capLoad, resLoad, normal);
            } else if (numDecoder24 == 1) {
                basicDecoderA1->Initialize(2, capLoad, resLoad, normal);
            } else if (numDecoder38 == 1)
                basicDecoderA1->Initialize(3, capLoad, resLoad, normal);
        }
    }
    initialized = true;
}

void PredecodeBlock::CalculateArea() {
    if (!initialized) {
        cout << "[Predecoder Block] Error: Require initialization first!" << endl;
    } else if (numAddressBit == 0) {
        height = width = area = 0;
    } else {
        double hTemp,wTemp;
        hTemp = wTemp = 0;
        if (basicDecoderA1 != NULL) {
            basicDecoderA1->CalculateArea();
            wTemp = max(wTemp, basicDecoderA1->width);
            hTemp += numBasicDecoderA1 * basicDecoderA1->height;
            if (basicDecoderA2 != NULL) {
                basicDecoderA2->CalculateArea();
                wTemp = max(wTemp, basicDecoderA2->width);
                hTemp += numBasicDecoderA2 * basicDecoderA2->height;
            }
            if (basicDecoderB != NULL) {
                basicDecoderB->CalculateArea();
                wTemp = max(wTemp, basicDecoderB->width);
                hTemp += numNandInputStage1B * basicDecoderB->height;
                if (basicDecoderC != NULL) {
                    basicDecoderC->CalculateArea();
                    wTemp = max(wTemp, basicDecoderC->width);
                    hTemp += numNandInputStage1C * basicDecoderC->height;
                }
            }
        }
        width = wTemp;
        height = hTemp;
        hTemp = wTemp = 0;
        if (rowDecoderStage1A != NULL) {
            rowDecoderStage1A->CalculateArea();
            wTemp = max(wTemp, rowDecoderStage1A->width);
            hTemp += rowDecoderStage1A->height;
            if (rowDecoderStage1B != NULL) {
                rowDecoderStage1B->CalculateArea();
                wTemp = max(wTemp, rowDecoderStage1B->width);
                hTemp += rowDecoderStage1B->height;
                if (rowDecoderStage1C != NULL) {
                    rowDecoderStage1C->CalculateArea();
                    wTemp = max(wTemp, rowDecoderStage1C->width);
                    hTemp += rowDecoderStage1C->height;
                }
            }
            if (rowDecoderStage2 != NULL) {
                rowDecoderStage2->CalculateArea();
                wTemp += rowDecoderStage2->width;
                hTemp = max(hTemp, rowDecoderStage2->height);
            }
        }
        width += wTemp;
        width = normal ? width : width * HVT_SCALE;
        height = max(height, hTemp);
        area = width * height;
    }
}

void PredecodeBlock::CalculateRC() {
    if (!initialized) {
        cout << "[Predecoder Block] Error: Require initialization first!" << endl;
    } else if (numAddressBit > 0) {
        if (basicDecoderA1 != NULL) {
            basicDecoderA1->CalculateRC();
            if (basicDecoderA2 != NULL) {
                basicDecoderA2->CalculateRC();
            }
            if (basicDecoderB != NULL) {
                basicDecoderB->CalculateRC();
                if (basicDecoderC != NULL) {
                    basicDecoderC->CalculateRC();
                }
            }
        }
    }
}

void PredecodeBlock::CalculateLatency(double _rampInput) {
    if (!initialized) {
        cout << "[Predecoder Block] Error: Require initialization first!" << endl;
    } else if (numAddressBit == 0) {
        readLatency = 0;
        rampOutput = _rampInput;
    } else {
        rampInput = _rampInput;
        double delayA1, delayA2, delayB, delayC;
        double maxRampOutput = 0;
        delayA1 = delayA2 = delayB = delayC = 0;
        rampOutput = 0;
        readLatency = 0;
        if (basicDecoderA1 != NULL) {
            basicDecoderA1->CalculateLatency(rampInput);
            delayA1 += basicDecoderA1->readLatency;
            maxRampOutput = basicDecoderA1->rampOutput;
            if (rowDecoderStage1A != NULL) {
                rowDecoderStage1A->CalculateLatency(basicDecoderA1->rampOutput);
                delayA1 += rowDecoderStage1A->readLatency;
                maxRampOutput = rowDecoderStage1A->rampOutput;
                if (rowDecoderStage2 != NULL) {
                    rowDecoderStage2->CalculateLatency(rowDecoderStage1A->rampOutput);
                    delayA1 += rowDecoderStage2->readLatency;
                    maxRampOutput = rowDecoderStage2->rampOutput;
                }
            }
        }
        rampOutput = max(rampOutput, maxRampOutput);
        readLatency = max(readLatency, delayA1);
        maxRampOutput = 0;
        if (basicDecoderA2 != NULL) {
            basicDecoderA2->CalculateLatency(rampInput);
            delayA2 += basicDecoderA1->readLatency;
            rowDecoderStage1A->CalculateLatency(basicDecoderA2->rampOutput);
            delayA2 += rowDecoderStage1A->readLatency;
            maxRampOutput = rowDecoderStage1A->rampOutput;
            if (rowDecoderStage2 != NULL) {
                rowDecoderStage2->CalculateLatency(rowDecoderStage1A->rampOutput);
                delayA2 += rowDecoderStage2->readLatency;
                maxRampOutput = rowDecoderStage2->rampOutput;
            }
        }
        rampOutput = max(rampOutput, maxRampOutput);
        readLatency = max(readLatency, delayA2);
        maxRampOutput = 0;
        if (basicDecoderB !=NULL) {
            basicDecoderB->CalculateLatency(rampInput);
            delayB += basicDecoderB->readLatency;
            rowDecoderStage1B->CalculateLatency(basicDecoderB->rampOutput);
            delayB += rowDecoderStage1B->readLatency;
            rowDecoderStage2->CalculateLatency(rowDecoderStage1B->rampOutput);
            delayB += rowDecoderStage2->readLatency;
            maxRampOutput = rowDecoderStage2->rampOutput;
        }
        rampOutput = max(rampOutput, maxRampOutput);
        readLatency = max(readLatency, delayB);
        maxRampOutput = 0;
        if (basicDecoderC !=NULL) {
            basicDecoderC->CalculateLatency(rampInput);
            delayC += basicDecoderC->readLatency;
            rowDecoderStage1C->CalculateLatency(basicDecoderC->rampOutput);
            delayC += rowDecoderStage1C->readLatency;
            rowDecoderStage2->CalculateLatency(rowDecoderStage1C->rampOutput);
            delayC += rowDecoderStage2->readLatency;
            maxRampOutput = rowDecoderStage2->rampOutput;
        }
        rampOutput = max(rampOutput, maxRampOutput);
        readLatency = max(readLatency, delayC);
    }
}


void PredecodeBlock::CalculatePower() {
    if (!initialized) {
        cout << "[Predecoder Block] Error: Require initialization first!" << endl;
    } else if (numAddressBit == 0) {
        leakage = readDynamicEnergy = 0;
    } else {
        leakage = readDynamicEnergy = 0;
        if (basicDecoderA1 != NULL) {
            basicDecoderA1->CalculatePower();
            leakage += basicDecoderA1->leakage;
            readDynamicEnergy += basicDecoderA1->readDynamicEnergy;
            if (basicDecoderA2 != NULL) {
                basicDecoderA2->CalculatePower();
                leakage += basicDecoderA2->leakage;
                readDynamicEnergy += basicDecoderA2->readDynamicEnergy;
            }
            if (basicDecoderB != NULL) {
                basicDecoderB->CalculatePower();
                leakage += basicDecoderB->leakage;
                readDynamicEnergy += basicDecoderB->readDynamicEnergy;
                if (basicDecoderC != NULL) {
                    basicDecoderC->CalculatePower();
                    leakage += basicDecoderC->leakage;
                    readDynamicEnergy += basicDecoderC->readDynamicEnergy;
                }
            }
        }
        if (rowDecoderStage1A != NULL) {
            rowDecoderStage1A->CalculatePower();
            leakage += rowDecoderStage1A->leakage;
            readDynamicEnergy += rowDecoderStage1A->readDynamicEnergy;
            if (rowDecoderStage1B != NULL) {
                rowDecoderStage1B->CalculatePower();
                leakage += rowDecoderStage1B->leakage;
                readDynamicEnergy += rowDecoderStage1B->readDynamicEnergy;
                if (rowDecoderStage1C != NULL) {
                    rowDecoderStage1C->CalculatePower();
                    leakage += rowDecoderStage1C->leakage;
                    readDynamicEnergy += rowDecoderStage1C->readDynamicEnergy;
                }
            }
            if (rowDecoderStage2 != NULL) {
                leakage += rowDecoderStage2->leakage;
                readDynamicEnergy += rowDecoderStage2->readDynamicEnergy;
            }
        }
    }
}

void PredecodeBlock::PrintProperty() {
    cout << "Predecoding Block Properties:" << endl;
    FunctionUnit::PrintProperty();
}

PredecodeBlock & PredecodeBlock::operator=(const PredecodeBlock &rhs) {
    FunctionUnit::operator=(rhs);

    initialized = rhs.initialized;
    numNandInputStage1A = rhs.numNandInputStage1A;
    numNandInputStage1B = rhs.numNandInputStage1B;
    numNandInputStage1C = rhs.numNandInputStage1C;
    numAddressBitStage1A = rhs.numAddressBitStage1A;
    numAddressBitStage1B = rhs.numAddressBitStage1B;
    numAddressBitStage1C = rhs.numAddressBitStage1C;
    capLoad = rhs.capLoad;
    resLoad = rhs.resLoad;
    numAddressBit = rhs.numAddressBit;
    numOutputAddressBit = rhs.numOutputAddressBit;
    numDecoder12 = rhs.numDecoder12;
    numDecoder24 = rhs.numDecoder24;
    numDecoder38 = rhs.numDecoder38;
    numBasicDecoderA1 = rhs.numBasicDecoderA1;
    numBasicDecoderA2 = rhs.numBasicDecoderA2;
    capLoadBasicDecoderA1 = rhs.capLoadBasicDecoderA1;
    capLoadBasicDecoderA2 = rhs.capLoadBasicDecoderA2;
    capLoadBasicDecoderB = rhs.capLoadBasicDecoderB;
    capLoadBasicDecoderC = rhs.capLoadBasicDecoderC;
    rampInput = rhs.rampInput;
    rampOutput = rhs.rampOutput;

    normal = rhs.normal;
    return *this;
}
