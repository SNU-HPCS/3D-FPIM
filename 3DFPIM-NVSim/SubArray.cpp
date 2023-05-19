/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/

#include "SubArray.h"
#include "formula.h"
#include "global.h"
#include "constant.h"
#include "macros.h"
#include <math.h>
#include <cassert>
#include <cmath>

SubArray::SubArray() {
    // TODO Auto-generated constructor stub
    initialized = false;
    invalid = false;
}

SubArray::~SubArray() {
    // TODO Auto-generated destructor stub
}

void SubArray::Initialize(long long _numRow, long long _numColumn, 
        int _muxSenseAmp, BufferDesignTarget _areaOptimizationLevel) {
    if (initialized)
        cout << "[Subarray] Warning: Already initialized!" << endl;

    numRow = _numRow;
    numColumn = _numColumn;
    muxSenseAmp = _muxSenseAmp;
    areaOptimizationLevel = _areaOptimizationLevel;
    if (inputParameter->pimMode)
        numCap = numColumn / muxSenseAmp;
    else
        numSenseAmp = numColumn / muxSenseAmp;

    // the parameters related to the SLIT
    int linesPerSLIT = 8;                               // the number of lines per SLIT
    int dummyPerSLIT = 1;                               // a dummy row in the middle
    int rowsPerSLIT = 2;                                // the number of lines connected to a bitline in a single SLIT
    int totalSLITs = numRow / rowsPerSLIT;              // the total number of SLITs
    int sharedSLITs;
    if (inputParameter->pimMode)
        sharedSLITs = totalSLITs / 2;                   // odd SLITs / even SLITs share a single driver
    else
        sharedSLITs = 2;                                // the number of SLITs driven by a single decoder
    double TRENCH_WIDTH = cell->trenchWidth;            // the width of a sourceline trench (in between each SLIT)

    int numDrivers;
    if (inputParameter->pimMode)
        numDrivers = totalSLITs / sharedSLITs;
    else
        numDrivers = 1;

    int stringLength = inputParameter->numStack + 2;
    int num_stairs = cell->numStairs;

    // ÷ 4 of numColumn due to the honeycomb structure
    lenCellline = (double)(numColumn / 4) * cell->widthInSize;
    lenWordline = lenCellline + (double)(num_stairs) * cell->stairLength;
    // x 4 of numRow due to the honeycomb structure
    lenBitline = ((double)_numRow * (double)(linesPerSLIT + dummyPerSLIT) / (double)(linesPerSLIT)) * 4 * cell->lengthInSize;
    // + additional length due to the SL trench
    lenBitline += (double)numRow * TRENCH_WIDTH;

    // flash string length = the page count per block plus 2 (select transistors)
    heightString = (double)(stringLength) * cell->heightInSize;

    ////////////////////////////////////////
    maxBitlineCurrent = cell->maxCellReadCurrent * numRow;

    // Calculate the staircase related R / C
    double miller_value;
    if (inputParameter->pimMode)
        miller_value = (1 - cell->flashReadVoltage / cell->flashPassVoltage);
    else
        miller_value = 1;
    capStaircaseLatency = tech->plCapOfGsd * 2 * miller_value * cell->stairLength * num_stairs * (linesPerSLIT + 1);
    // divide by 2 for average case
    capStaircaseEnergy = tech->plCapOfGsd * 2 * miller_value * cell->stairLength * (num_stairs / 2.) * (linesPerSLIT + 1) * sharedSLITs;
    resStaircase = wordlineWire->resWirePerStaircase * cell->stairLength * num_stairs / (linesPerSLIT + 1);

    ////////////////////////////////////////
    // Calculate the bitline related R / C
    // We evaluate two different scenarios 
    // 1) Intrinsic capacitance that affects both charging and discharging
        // Vertical and fringing capacitance of each wire
    double intrinsicCapBitline = (lenBitline) * bitlineWire->intrinsicCapWirePerUnit;
        // Bitline to the WL capacitance
        // assume a worst case scenario => a single BSL is completely overlapped and another BSL is completely unoverlapped
        // 1-1) trCapOfGex => outer fringing capacitance (Uppermost tr to the BL)
        // 1-2) flCapOfGex => outer fringing capacitance (between different tr)
        // 1-3) flCapGc => fully overlapped capacitance (between channel and the WL)
        // 1-4) flCapIf => two inner fringing capacitance
        // 1-5) flCapGc => another fully overlapped capacitance (+ with only a slight region for inner fringe)
    intrinsicCapBitline += ((tech->trCapOfGex +
                             tech->trCapGc +
                             tech->flCapOfGex +
                             tech->flCapOfGex +
                             tech->trCapIf * 2 + 
                             tech->trCapGc) * numRow);
    // 2) coupling capacitance that affects charging 
        // 2-1) vertical coupling capacitance between two wires
    double couplingCapBitline = (lenBitline) * bitlineWire->couplingCapWirePerUnit;
        // 2-2) via to via coupling capacitance
    couplingCapBitline += bitlineWire->capViaWirePerString * numRow;

    // we multiply the coupling bitline by two => assuming that there is no explicit discharge in between operations
    if (inputParameter->pimMode)
        capBitline = intrinsicCapBitline + couplingCapBitline * 2;
    else
        capBitline = intrinsicCapBitline + couplingCapBitline;
    resBitline = (lenBitline) * bitlineWire->resWirePerUnit;

    ////////////////////////////////////////
    // Calculate the stringline related R / C
    // gate capacitance and the outer fringing capacitance
    double capCouplingStringline;
    if (inputParameter->pimMode)
        capCouplingStringline = stringLength * stringWire->capWirePerUnit * 2;
    else
        capCouplingStringline = stringLength * stringWire->capWirePerUnit;
    capStringline = stringLength * (tech->flCapGc + tech->flCapOfGex * 2);
    resStringline = (heightString) * stringWire->resWirePerUnit;

    ////////////////////////////////////////
    // Calculate the wordline related R / C
    // 1) cell capacitance (flCapGc / flCapOfGex)
    // 2) wordline to wordline coupling capacitance (the miller value set to 1)
    miller_value = (1 - cell->flashReadVoltage / cell->flashPassVoltage);

    if (!inputParameter->pimMode) {
        capWordlineLatency = (numColumn / 4.) * (linesPerSLIT + 1) * (tech->flCapGc + tech->flCapOfGex * 2) +
                             (numColumn / 4.) * (linesPerSLIT + 1) * tech->flCapOfGsd * 2 * miller_value;
        // for discharging, the miller value is set to 0
        capWordlineDischarge = (numColumn / 4.) * (linesPerSLIT + 1) * (tech->flCapGc + tech->flCapOfGex * 2);
        capWordlineEnergy = 0;
    }
    else{
        if(inputParameter->baseline){
            capWordlineLatency = (numColumn / 4.) * (linesPerSLIT + 1) * (tech->flCapGc + tech->flCapOfGex * 2) +
                                         (numColumn / 4.) * (linesPerSLIT + 1) * tech->flCapOfGsd * 2 * miller_value;
            // for discharging, the miller value is set to 0
            capWordlineDischarge = (numColumn / 4.) * (linesPerSLIT + 1) * (tech->flCapGc + tech->flCapOfGex * 2);

            // a single driver shares multiple slits
            miller_value = (1 - cell->flashReadVoltage / cell->flashPassVoltage);
            capWordlineEnergy = (numColumn / 4.) * (linesPerSLIT + 1) * (tech->flCapGc + tech->flCapOfGex * 2) * sharedSLITs;
        }
        else {
            capWordlineLatency = (numColumn / 4.) * (linesPerSLIT + 1) * 
                                      (tech->flCapGc + tech->flCapOfGex * 2 + tech->flCapOfGsd * 2 * (1 + miller_value));

            capWordlineDischarge = 0;

            capWordlineEnergy = (numColumn / 4.) * (linesPerSLIT + 1) *
                    (tech->flCapGc + tech->flCapOfGex * 2 + tech->flCapOfGsd * 
                    (2 - (cell->flashReadVoltage / cell->flashPassVoltage))) * sharedSLITs;
        }
    }

    // worst case wordline length
    resWordline = wordlineWire->resWirePerCell * (numColumn / 4.) / (linesPerSLIT + 1);

    miller_value = 1;
    capSelectline = (numColumn / 4) * (linesPerSLIT / 2) * (tech->trCapOfGex * 2 + tech->trCapGc);

    double capSelectLineDischarge = (numColumn / 4) * (linesPerSLIT / 2) * (tech->trCapOfGex * 2 + tech->trCapGc * 1);

    // selectline is only half of the wordline => considered when calculating the resistance
    resSelectline = selectlineWire->resWirePerCell * (numColumn / 4) / (linesPerSLIT / 2);

    if(!inputParameter->pimMode) {
        selectDecoder.Initialize(numRow,
                capSelectline, resSelectline,
                capSelectline * resSelectline / 2,
                areaOptimizationLevel,
                0, false,
                capSelectLineDischarge, 
                capSelectLineDischarge * resSelectline / 2);
    }
    else {
        selectDecoder.Initialize(1,
                capSelectline, resSelectline,
                capSelectline * resSelectline / 2,
                area_first,
                0, false,
                capSelectLineDischarge, 
                capSelectLineDischarge * resSelectline / 2);
    }

    if (selectDecoder.invalid) {
        invalid = true;
        return;
    }

    ////////////////////////////////////////
    // Calculate misc R / C
    /*
    Top View assuming two SLITs share a single decoder
    ============= => SLIT
    || => DecoderVerticalline (Vertical Line in the BL direction)
    ============= => Row Decoder in the middle
    || => DecoderVerticalline (Vertical Line in the BL direction)
    ============= => SLIT
    */
    // that is, there is a vertical line to make multiple SLITs share a single decoder
    resDecoderVerticalline = (lenBitline / totalSLITs) * bitlineWire->resWirePerUnit * (sharedSLITs - 1) / 2;
    capDecoderVerticalline = (lenBitline / totalSLITs) * bitlineWire->capWirePerUnit * (sharedSLITs - 1) / 2;

    if(not inputParameter->pimMode) {
        double tau = resDecoderVerticalline * (capDecoderVerticalline / 2 + capStaircaseLatency + capWordlineLatency) +
                     resStaircase * (capStaircaseLatency / 2 + capWordlineLatency) + 
                     resWordline * capWordlineLatency / 2;

        // a single SLIT total capacitance
        double tau_discharge = resDecoderVerticalline * (capDecoderVerticalline / 2 + capWordlineDischarge) +
                              resStaircase * capWordlineDischarge + 
                              resWordline * capWordlineDischarge / 2;

        double capCompleteDecoder = capDecoderVerticalline + capStaircaseLatency + capWordlineLatency * sharedSLITs;

        completeDecoder.Initialize(numDrivers,
                                   numRow / (sharedSLITs * rowsPerSLIT), 
                                   inputParameter->numStack, 
                                   capCompleteDecoder, 
                                   resWordline, 
                                   tau,
                                   capDecoderVerticalline + capWordlineDischarge * sharedSLITs,
                                   tau_discharge,
                                   areaOptimizationLevel,
                                   cell->flashPassVoltage / resWordline,
                                   false);
        
    }
    else {
        if(inputParameter->baseline) {
            double tau = resDecoderVerticalline * (capDecoderVerticalline / 2 + capStaircaseLatency + capWordlineLatency) +
                           resStaircase * (capStaircaseLatency / 2 + capWordlineLatency) + 
                           resWordline * capWordlineLatency / 2;

            double tau_discharge = resDecoderVerticalline * (capDecoderVerticalline / 2 + capWordlineDischarge) +
                                            resStaircase * capWordlineDischarge + 
                                            resWordline * capWordlineDischarge / 2;
            double capCompleteDecoder = capDecoderVerticalline + capStaircaseEnergy + capWordlineEnergy;

            completeDecoder.Initialize(numDrivers,
                                       1,
                                       inputParameter->numStack, 
                                       capCompleteDecoder,
                                       resWordline,
                                       tau,
                                       capWordlineDischarge * sharedSLITs,
                                       tau_discharge,
                                       areaOptimizationLevel,
                                       0,
                                       false);
        }
        else {
            double tau = resDecoderVerticalline * (capDecoderVerticalline / 2. + capStaircaseLatency + capWordlineLatency) +
                        resStaircase * (capStaircaseLatency / 2. + capWordlineLatency) +
                        resWordline * capWordlineLatency / 2.;

            double tau_discharge = 0;
            double capCompleteDecoder = capDecoderVerticalline + capStaircaseEnergy + capWordlineEnergy;
            completeDecoder.Initialize(numDrivers,
                                            1,
                                            inputParameter->numStack, 
                                            capCompleteDecoder,
                                            resWordline,
                                            tau,
                                            0,
                                            tau_discharge,
                                            areaOptimizationLevel,
                                            0,
                                            false);
        }
        
    }
    if (completeDecoder.invalid) {
        invalid = true;
        return;
    }

    selectDecoder.CalculateRC();
    completeDecoder.CalculateRC();

    if (inputParameter->pimMode) {
        capLoad.Initialize(numCap, inputParameter->capLoad, lenCellline);
        if (capLoad.invalid) {
            invalid = true;
        }
    }
    else {
        senseVoltage = cell->minSenseVoltage;
        senseAmp.Initialize(numSenseAmp, true, senseVoltage, lenCellline / numColumn * muxSenseAmp);
        if (senseAmp.invalid) {
            invalid = true;
        }
        senseAmp.CalculateRC();
    }

    voltagePrecharge = cell->prechargeVoltage;

    double tau_precharge = resBitline * (capBitline / 2 + (capStringline + capCouplingStringline)) +
                           resStringline * ((capStringline + capCouplingStringline) / 2);
    double tau_discharge = resBitline * (intrinsicCapBitline / 2 + capStringline) +
                           resStringline * (capStringline / 2);

    if (inputParameter->pimMode) {
        precharger.Initialize(voltagePrecharge, numColumn, 
            capBitline + (capStringline + capCouplingStringline) * numRow,
            capBitline + (capStringline + capCouplingStringline) * numRow * 0.25 * 0.25,
            resBitline,
            tau_precharge,
            0,
            0);
    }
    else {
        precharger.Initialize(voltagePrecharge, numColumn, 
            capBitline + capStringline + capCouplingStringline, 
            capBitline + capStringline + capCouplingStringline, 
            resBitline,
            tau_precharge,
            intrinsicCapBitline + capStringline,
            tau_discharge);
    }
    precharger.CalculateRC();

    initialized = true;
}

void SubArray::CalculateArea() {
    if (!initialized) {
        cout << "[Subarray] Error: Require initialization first!" << endl;
    } else if (invalid) {
        height = width = area = 1e41;
    } else {
        width = lenWordline;
        height = lenBitline;
        //cout << "array area: " << width * height * 8 * 1e12 << endl;
        area = width * height;

        completeDecoder.CalculateArea();
        area += completeDecoder.area;

        selectDecoder.CalculateArea();
        if (inputParameter->pimMode)
            selectDecoder.area *= numRow;
        
        area += selectDecoder.area;

        precharger.CalculateArea();
        area += precharger.area;

        if (inputParameter->pimMode) {
            capLoad.CalculateArea();
            area += capLoad.area;
        }
        else {
            senseAmp.CalculateArea();
            area += senseAmp.area;
        }
    }
}

void SubArray::CalculateLatency(double _rampInput) {
    if (!initialized) {
        cout << "[Subarray] Error: Require initialization first!" << endl;
    } else if (invalid) {
        readLatency = 1e41;
    } else {
        completeDecoder.CalculateLatency(_rampInput);
        selectDecoder.CalculateLatency(_rampInput);

        // precharging latency
        precharger.CalculateLatency(_rampInput);

        if(!inputParameter->pimMode)
            senseAmp.CalculateLatency(1e41);

        decoderLatency = MAX(completeDecoder.readLatency, selectDecoder.readLatency);

        cellDelay = cell->inputDuration;

        precharger.prechargeLatency = precharger.enableLatency + precharger.prechargeLatency;
        precharger.dischargeLatency = precharger.enableLatency + precharger.dischargeLatency;

        if (!inputParameter->pimMode) {
            readLatency =
                    // precharging latency (driver latency + bitline driving latency)
                    precharger.prechargeLatency * int(inputParameter->inputPrecision) +
                    MAX(MAX(precharger.dischargeLatency, completeDecoder.dischargeLatency), selectDecoder.dischargeLatency) * int(inputParameter->inputPrecision) +
                    decoderLatency +
                    senseAmp.readLatency;
        }
        else {
            if(not inputParameter->baseline) {
                readLatency =
                        // precharging latency (driver latency + bitline driving latency)
                        precharger.prechargeLatency * int(inputParameter->inputPrecision) +
                        MAX(precharger.dischargeLatency, selectDecoder.dischargeLatency) * int(inputParameter->inputPrecision) +
                        decoderLatency +
                        cellDelay * pow(2, int(inputParameter->inputPrecision)) +
                        CLOCK_PERIOD * max(double(numRow) / DTC_BATCH, 1.) * int(inputParameter->inputPrecision);
            }
            else {
                readLatency =
                        // precharging latency (driver latency + bitline driving latency)
                        precharger.prechargeLatency * int(inputParameter->inputPrecision) +
                        MAX(MAX(precharger.dischargeLatency, completeDecoder.dischargeLatency), selectDecoder.dischargeLatency) * int(inputParameter->inputPrecision) +
                        decoderLatency +
                        cellDelay * pow(2, int(inputParameter->inputPrecision)) +
                        CLOCK_PERIOD * max(double(numRow) / DTC_BATCH, 1.) * int(inputParameter->inputPrecision);
            }
        }
    }
}

void SubArray::CalculatePower() {
    if (!initialized) {
        cout << "[Subarray] Error: Require initialization first!" << endl;
    } else if (invalid) {
        readDynamicEnergy = leakage = 1e41;
    } else {
        // precharging power
        precharger.CalculatePower();

        completeDecoder.CalculatePower();

        selectDecoder.CalculatePower();

        if(inputParameter->pimMode)
            capLoad.CalculatePower();
        else
            senseAmp.CalculatePower();

        /* Calculate the NAND flash string length, which is the page count per block plus 2 (two select transistors) */
        int stringLength = inputParameter->numStack + 2;

        // wordline => 4 types
        // 1) selected wordline => power: 0, #: numRow
        // 2) unselected wordline => power: actual, 
        //          #: (stringLength - 3) * numRow
        // 3) select wordline => power: selectDecoder, #: numRow
        // 4) select drain => power: selectDecoder, #: numRow
        //
        // note that, multiply by numRow is removed inside the row decoder!!!
        // this is to reduce the overhead of the row decoding itself! 

        double actualWordlinePassEnergy;
        double actualWordlineReadEnergy;
        
        if (!inputParameter->pimMode) {
            actualWordlineReadEnergy = completeDecoder.wlReadDynamicEnergy 
                    / completeDecoder.vdd / completeDecoder.vdd
                    * cell->flashReadVoltage * cell->flashReadVoltage;
            actualWordlinePassEnergy = completeDecoder.wlReadDynamicEnergy
                    / completeDecoder.vdd / completeDecoder.vdd
                    * cell->flashPassVoltage * cell->flashPassVoltage;
                    // approximate calculate, the wordline is charged 
                    // to Vpass instead of Vdd

            completeDecoder.shiftEnergy = 0;
            completeDecoder.readDynamicEnergy = 
                actualWordlinePassEnergy * (stringLength - 3) + 
                actualWordlineReadEnergy + 
                completeDecoder.decodingDynamicEnergy;
        }
        else {
            if(not inputParameter->baseline){
                actualWordlinePassEnergy = completeDecoder.wlReadDynamicEnergy
                        / completeDecoder.vdd / completeDecoder.vdd
                        * cell->flashPassVoltage * cell->flashPassVoltage;
                actualWordlineReadEnergy = completeDecoder.wlReadDynamicEnergy
                        / completeDecoder.vdd / completeDecoder.vdd
                        * cell->flashReadVoltage * cell->flashReadVoltage;
                //actualWordlinePassMaxEnergy = completeDecoder.wlReadDynamicEnergy
                //        / completeDecoder.vdd / completeDecoder.vdd
                //        * cell->flashPassVoltage * cell->flashPassVoltage;

                completeDecoder.shiftEnergy = 
                        actualWordlinePassEnergy + 
                        actualWordlineReadEnergy + 
                        completeDecoder.decodingDynamicEnergy;
                
                // 
                completeDecoder.readDynamicEnergy = 
                        actualWordlinePassEnergy * (stringLength - 3) + 
                        actualWordlineReadEnergy + 
                        completeDecoder.decodingDynamicEnergy;
            }
            else{
                actualWordlinePassEnergy = completeDecoder.wlReadDynamicEnergy
                        / completeDecoder.vdd / completeDecoder.vdd
                        * cell->flashPassVoltage * cell->flashPassVoltage;
                actualWordlineReadEnergy = completeDecoder.wlReadDynamicEnergy
                        / completeDecoder.vdd / completeDecoder.vdd
                        * cell->flashPassVoltage * cell->flashPassVoltage;
                //actualWordlinePassMaxEnergy = completeDecoder.wlReadDynamicEnergy
                //        / completeDecoder.vdd / completeDecoder.vdd
                //        * cell->flashPassVoltage * cell->flashPassVoltage;

                completeDecoder.shiftEnergy = 
                        actualWordlinePassEnergy + 
                        actualWordlineReadEnergy + 
                        completeDecoder.decodingDynamicEnergy;
                
                // 
                completeDecoder.readDynamicEnergy = 
                        actualWordlinePassEnergy * (stringLength - 3) + 
                        actualWordlineReadEnergy + 
                        completeDecoder.decodingDynamicEnergy;
            }

            leakage = 0;

            // all rows are activated at once
            selectDecoder.readDynamicEnergy *= numRow;
            // mult by 0.25 * 0.25 => analytic assuming ratio of zero bits in the activation / weight
            selectDecoder.readDynamicEnergy *= 0.25 * 0.25;

            readDynamicEnergy += completeDecoder.readDynamicEnergy 
                              + selectDecoder.readDynamicEnergy * int(inputParameter->inputPrecision)
                              + precharger.readDynamicEnergy * int(inputParameter->inputPrecision)
                              + capLoad.readDynamicEnergy;
            leakage += completeDecoder.leakage 
                    + selectDecoder.leakage
                    + precharger.leakage;
        }
    }
}

void SubArray::PrintProperty() {
    cout << "Subarray Properties:" << endl;
    FunctionUnit::PrintProperty();
    cout << "numRow:" << numRow << " numColumn:" << numColumn << endl;
    cout << "lenWordline * lenBitline = " << lenWordline*1e6 << "um * " << lenBitline*1e6 << "um = " << lenWordline * lenBitline * 1e6 << "mm^2" << endl;
    cout << "Complete Decoder Area:" << completeDecoder.height*1e6 << "um x " << completeDecoder.width*1e6 << "um = " << completeDecoder.area*1e6 << "mm^2" << endl;
    if(inputParameter->pimMode)
        cout << "Load Capacitor Area:" << capLoad.height*1e6 << "um x " << capLoad.width*1e6 << "um = " << capLoad.area*1e6 << "mm^2" << endl;
    else
        cout << "Sense Amplifier Area:" << senseAmp.height*1e6 << "um x " << senseAmp.width*1e6 << "um = " << senseAmp.area*1e6 << "mm^2" << endl;
    cout << "Subarray Area Efficiency = " << lenWordline * lenBitline / area * 100 <<"%" << endl;
    cout << "bitlineDelay: " << bitlineDelay*1e12 << "ps" << endl;
    cout << "chargeLatency: " << chargeLatency*1e12 << "ps" << endl;
}

SubArray & SubArray::operator=(const SubArray &rhs) {
    FunctionUnit::operator=(rhs);

    initialized = rhs.initialized;
    numRow = rhs.numRow;
    numColumn = rhs.numColumn;
    areaOptimizationLevel = rhs.areaOptimizationLevel;

    voltageSense = rhs.voltageSense;
    numCap = rhs.numCap;
    numSenseAmp = rhs.numSenseAmp;
    lenWordline = rhs.lenWordline;
    lenCellline = rhs.lenCellline;
    lenBitline = rhs.lenBitline;
    heightString = rhs.heightString;

    capWordlineLatency = rhs.capWordlineLatency;
    capWordlineEnergy = rhs.capWordlineEnergy;
    capWordlineDischarge = rhs.capWordlineDischarge;
    resWordline = rhs.resWordline;

    capSelectline = rhs.capSelectline;
    resSelectline = rhs.resSelectline;
    
    capBitline = rhs.capBitline;
    resBitline = rhs.resBitline;
    
    capStringline = rhs.capStringline;
    resStringline = rhs.resStringline;

    capDecoderVerticalline = rhs.capDecoderVerticalline;
    resDecoderVerticalline = rhs.resDecoderVerticalline;

    capStaircaseLatency = rhs.capStaircaseLatency;
    capStaircaseEnergy = rhs.capStaircaseEnergy;
    resStaircase = rhs.resStaircase;
    
    resCellAccess = rhs.resCellAccess;
    capCellAccess = rhs.capCellAccess;
    bitlineDelay = rhs.bitlineDelay;
    chargeLatency = rhs.chargeLatency;
    decoderLatency = rhs.decoderLatency;
    resInSerialForSenseAmp = rhs.resInSerialForSenseAmp;
    resEquivalentOn = rhs.resEquivalentOn;
    resEquivalentOff = rhs.resEquivalentOff;
    cellDelay = rhs.cellDelay;
    resMemCellOff = rhs.resMemCellOff;
    resMemCellOn = rhs.resMemCellOn;

    completeDecoder = rhs.completeDecoder;
    selectDecoder = rhs.selectDecoder;

    precharger = rhs.precharger;
    senseAmp = rhs.senseAmp;
    capLoad = rhs.capLoad;

    chargeLeakLatency = rhs.chargeLeakLatency;

    maxBitlineCurrent = rhs.maxBitlineCurrent;

    chargeEnergy = rhs.chargeEnergy;

    return *this;
}
