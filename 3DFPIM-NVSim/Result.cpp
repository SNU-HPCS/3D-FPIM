/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "Result.h"
#include "global.h"
#include "formula.h"
#include "macros.h"
#include "constant.h"

#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

Result::Result() {
    // TODO Auto-generated constructor stub
    subarray = new SubArray();
    localWire = new Wire();
    globalWire = new Wire();
    selectlineWire = new Wire();
    wordlineWire = new Wire();
    contactWire = new Wire();
    stringWire = new Wire();
    bitlineWire = new Wire();

    /* initialize the worst case */
    subarray->readLatency = 1e41;
    subarray->readDynamicEnergy = 1e41;
    subarray->leakage = 1e41;
    subarray->height = 1e41;
    subarray->width = 1e41;
    subarray->area = 1e41;

    /* No constraints */
    limitReadLatency = 1e41;
    limitReadDynamicEnergy = 1e41;
    limitReadEdp = 1e41;
    limitArea = 1e41;
    limitLeakage = 1e41;

    /* Default read latency optimization */
    optimizationTarget = read_latency_optimized;
}

Result::~Result() {
    // TODO Auto-generated destructor stub
    if (subarray)
        delete subarray;
    if (Result::localWire)
        delete Result::localWire;
    if (Result::globalWire)
        delete Result::globalWire;
    if (Result::selectlineWire)
        delete Result::selectlineWire;
    if (Result::wordlineWire)
        delete Result::wordlineWire;
    if (Result::contactWire)
        delete Result::contactWire;
    if (Result::stringWire)
        delete Result::stringWire;
    if (Result::bitlineWire)
        delete Result::bitlineWire;
}

void Result::reset() {
    subarray->readLatency = 1e41;
    subarray->readDynamicEnergy = 1e41;
    subarray->leakage = 1e41;
    subarray->height = 1e41;
    subarray->width = 1e41;
    subarray->area = 1e41;
}

void Result::compareAndUpdate(Result &newResult) {
    if (newResult.subarray->readLatency <= limitReadLatency 
            && newResult.subarray->readDynamicEnergy <= limitReadDynamicEnergy 
            && newResult.subarray->readLatency * 
            newResult.subarray->readDynamicEnergy 
            <= limitReadEdp
            && newResult.subarray->area 
            <= limitArea 
            && newResult.subarray->leakage <= limitLeakage) {


        bool toUpdate = false;
        switch (optimizationTarget) {
        case read_latency_optimized:
            if  (newResult.subarray->readLatency < subarray->readLatency)
                toUpdate = true;
            break;
        case read_energy_optimized:
            if  (newResult.subarray->readDynamicEnergy < subarray->readDynamicEnergy)
                toUpdate = true;
            break;
        case read_edp_optimized:
            if  (newResult.subarray->readLatency * newResult.subarray->readDynamicEnergy < subarray->readLatency * subarray->readDynamicEnergy)
                toUpdate = true;
            break;
        case area_optimized:
            if  (newResult.subarray->area < subarray->area)
                toUpdate = true;
            break;
        case leakage_optimized:
            if  (newResult.subarray->leakage < subarray->leakage)
                toUpdate = true;
            break;
        default:    /* Exploration */
            /* should not happen */
            ;
        }
        if (toUpdate) {
            *subarray = *(newResult.subarray);
            *localWire = *(newResult.localWire);
            *globalWire = *(newResult.globalWire);
            *selectlineWire = *(newResult.selectlineWire);
            *wordlineWire = *(newResult.wordlineWire);
            *contactWire = *(newResult.contactWire);
            *stringWire = *(newResult.stringWire);
            *bitlineWire = *(newResult.bitlineWire);
        }
    }
}


void Result::print() {

    cout << endl << "=============" 
            << endl << "CONFIGURATION" 
            << endl << "=============" << endl;
    //cout << "Mat Organization: " 
    //        << mat->numRowSubarray 
    //        << " x " << mat->numColumnSubarray << endl;
    //cout << " - Row Activation   : " 
    //        << mat->numActiveSubarrayPerColumn 
    //        << " / " << mat->numRowSubarray << endl;
    //cout << " - Column Activation: " 
    //        << mat->numActiveSubarrayPerRow 
    //        << " / " << mat->numColumnSubarray << endl;
    cout << " - SubArray Size    : " 
            << subarray->numRow 
            << " Rows x " << subarray->numColumn << " Columns" << endl;
    cout << "Mux Level:" << endl;
    cout << "Local Wire:" << endl;
    cout << " - Wire Type : ";
    switch (localWire->wireType) {
    case local_aggressive:
        cout << "Local Aggressive" << endl;
        break;
    case local_conservative:
        cout << "Local Conservative" << endl;
        break;
    case semi_aggressive:
        cout << "Semi-Global Aggressive" << endl;
        break;
    case semi_conservative:
        cout << "Semi-Global Conservative" << endl;
        break;
    case global_aggressive:
        cout << "Global Aggressive" << endl;
        break;
    case global_conservative:
        cout << "Global Conservative" << endl;
        break;
    default:
        cout << "DRAM Wire" << endl;
    }
    cout << " - Repeater Type: ";
    switch (localWire->wireRepeaterType) {
    case repeated_none:
        cout << "No Repeaters" << endl;
        break;
    case repeated_opt:
        cout << "Fully-Optimized Repeaters" << endl;
        break;
    case repeated_5:
        cout << "Repeaters with 5% Overhead" << endl;
        break;
    case repeated_10:
        cout << "Repeaters with 10% Overhead" << endl;
        break;
    case repeated_20:
        cout << "Repeaters with 20% Overhead" << endl;
        break;
    case repeated_30:
        cout << "Repeaters with 30% Overhead" << endl;
        break;
    case repeated_40:
        cout << "Repeaters with 40% Overhead" << endl;
        break;
    case repeated_50:
        cout << "Repeaters with 50% Overhead" << endl;
        break;
    default:
        cout << "Unknown" << endl;
    }
    cout << " - Low Swing : ";
    if (localWire->isLowSwing)
        cout << "Yes" << endl;
    else
        cout << "No" << endl;
    cout << "Global Wire:" << endl;
    cout << " - Wire Type : ";
    switch (globalWire->wireType) {
    case local_aggressive:
        cout << "Local Aggressive" << endl;
        break;
    case local_conservative:
        cout << "Local Conservative" << endl;
        break;
    case semi_aggressive:
        cout << "Semi-Global Aggressive" << endl;
        break;
    case semi_conservative:
        cout << "Semi-Global Conservative" << endl;
        break;
    case global_aggressive:
        cout << "Global Aggressive" << endl;
        break;
    case global_conservative:
        cout << "Global Conservative" << endl;
        break;
    default:
        cout << "DRAM Wire" << endl;
    }
    cout << " - Repeater Type: ";
    switch (globalWire->wireRepeaterType) {
    case repeated_none:
        cout << "No Repeaters" << endl;
        break;
    case repeated_opt:
        cout << "Fully-Optimized Repeaters" << endl;
        break;
    case repeated_5:
        cout << "Repeaters with 5% Overhead" << endl;
        break;
    case repeated_10:
        cout << "Repeaters with 10% Overhead" << endl;
        break;
    case repeated_20:
        cout << "Repeaters with 20% Overhead" << endl;
        break;
    case repeated_30:
        cout << "Repeaters with 30% Overhead" << endl;
        break;
    case repeated_40:
        cout << "Repeaters with 40% Overhead" << endl;
        break;
    case repeated_50:
        cout << "Repeaters with 50% Overhead" << endl;
        break;
    default:
        cout << "Unknown" << endl;
    }
    cout << " - Low Swing : ";
    if (globalWire->isLowSwing)
        cout << "Yes" << endl;
    else
        cout << "No" << endl;
    cout << "Buffer Design Style: ";
    switch (subarray->areaOptimizationLevel) {
    case latency_first:
        cout << "Latency-Optimized" << endl;
        break;
    case area_first:
        cout << "Area-Optimized" << endl;
        break;
    default:    /* balance */
        cout << "Balanced" << endl;
    }

    cout << "=============" << endl << "   RESULT" << endl << "=============" << endl;

    cout << scientific;
    cout << " |--- SubArray Area = " << (subarray->area) * 1e6 << " mm^2" << endl;
    cout << "    |--- Cell Area Inlcuding the Staircase = " 
            << (subarray->lenBitline) * 1e3 << " x "
            << (subarray->lenWordline) * 1e3 << " = " 
            << (subarray->lenBitline * 
            subarray->lenWordline) * 1e6 << " mm^2" <<endl;

    cout << "    |--- Decoder Area = " 
            << (subarray->completeDecoder.area * 2) * 1e6 << " mm^2" << endl;

    cout << "    |--- SelectDecoder Area = " 
            << (subarray->selectDecoder.area) * 1e6 << " mm^2" << endl;

    if(!inputParameter->pimMode) {
        cout << "    |--- SenseAmp Area = " 
                << (subarray->senseAmp.area) << endl;
    }
    else {
        cout << "    |--- SenseAmp Area (Cap Load) = " 
                << (subarray->capLoad.area) * 1e6 << " mm^2" << endl;
    }

    cout << "    |--- Precharger Area = " 
            << (subarray->precharger.area) * 1e6 << " mm^2" << endl;


    cout << " - Area Efficiency = " 
            << (subarray->lenBitline * 
            subarray->lenCellline) / subarray->area * 100 << "%" << endl;

    cout << "Timing:" << endl;

    if(!inputParameter->pimMode) {
        cout << fixed << "|--- Error = " 
                << abs(subarray->readLatency - inputParameter->referenceReadLatency) / inputParameter->referenceReadLatency * 100. << endl;
        cout << scientific;
        cout << "|--- SubArray Latency = " 
                << subarray->readLatency * 1e9 << " ns" << endl;

        cout << "   |--- Decoding Latency = " 
                << subarray->decoderLatency * 1e9 << " ns" << endl;
        cout << "       |--- Row Decoder Latency = " 
                << subarray->completeDecoder.readLatency * 1e9 << " ns" << endl;
        cout << "       |--- Select Latency = " 
                << (subarray->selectDecoder.readLatency) * 1e9 << " ns" << endl;
        cout << "   |--- Senseamp Latency = " 
                << (subarray->senseAmp.readLatency) * 1e9 << " ns" << endl;
        cout << "   |--- Precharge Latency = " 
                << (subarray->precharger.prechargeLatency +
                    subarray->precharger.enableLatency) * 1e9 << " ns" << endl;
        cout << "       |--- Enable Latency = " 
                << subarray->precharger.enableLatency * 1e9 << " ns" << endl;
        cout << "   |--- Discharge Latency = " 
                << MAX(MAX(subarray->precharger.dischargeLatency + 
                   subarray->precharger.enableLatency,
                   subarray->completeDecoder.dischargeLatency),
                   subarray->selectDecoder.dischargeLatency) * 1e9 << " ns" << endl;
        cout << "      |--- Discharge Precharger = " << subarray->precharger.dischargeLatency + subarray->precharger.enableLatency * 1e9 << " ns" << endl;
        cout << "      |--- Discharge Complete Decoder = " << subarray->completeDecoder.dischargeLatency * 1e9 << " ns" << endl;
        cout << "      |--- Discharge Select Decoder = " << subarray->selectDecoder.dischargeLatency * 1e9 << " ns" << endl;
    }
    else {
        ////////////////////////////////////////////
        double rowDecoderInitLatency;
        double rowDecoderShiftLatency;
        double rowDecoderDischargeLatency;
        double cellDelay;
        double selectDecoderLatency;
        double dacLatency;
        double prechargeLatency;
        if(inputParameter->lpDecoder){
            rowDecoderShiftLatency = int(std::ceil((subarray->completeDecoder.readLatency) * CLOCK_FREQ));
        }
        else{
            rowDecoderShiftLatency = int(std::ceil((subarray->completeDecoder.readLatency) * CLOCK_FREQ));
            rowDecoderInitLatency = int(std::ceil((subarray->completeDecoder.readLatency) * CLOCK_FREQ));
            rowDecoderDischargeLatency = int(std::ceil((subarray->completeDecoder.dischargeLatency) * CLOCK_FREQ));
        }
        
        cellDelay = int(std::ceil((subarray->cellDelay) * CLOCK_FREQ)) * pow(2, int(inputParameter->inputPrecision));
        selectDecoderLatency = int(std::ceil(((subarray->selectDecoder.readLatency + subarray->selectDecoder.dischargeLatency) * CLOCK_FREQ))) * 
                        int(inputParameter->inputPrecision);
        dacLatency = int(std::ceil((CLOCK_PERIOD * max(double(subarray->numRow) / DTC_BATCH, 1.) * CLOCK_FREQ))) * int(inputParameter->inputPrecision);

        prechargeLatency = int(std::ceil((subarray->precharger.prechargeLatency * CLOCK_FREQ))) * int(inputParameter->inputPrecision);

        if(!inputParameter->lpDecoder) {
            cout << "   |--- Row Decoder Latency (NR) = " 
                    << int(rowDecoderInitLatency + selectDecoderLatency + dacLatency + prechargeLatency + rowDecoderDischargeLatency) << " ns" << endl;
        }
        else {
            cout << "   |--- Row Decoder Latency (SR) = " 
                    << int(rowDecoderShiftLatency + selectDecoderLatency + dacLatency + prechargeLatency) << " ns" << endl;
        }
        cout << "   |--- Row Decoder Latency (CR) = " 
                << int(selectDecoderLatency + dacLatency + prechargeLatency) << " ns" << endl;
        cout << "   |--- Capacitor Drive Latency = " 
                << int(cellDelay) << " ns" << endl;

        cout << scientific << "Energy:" << endl;

        /////////////////// PRINT AGAIN /////////////////////////
        double rowDecoderInitEnergy;
        double rowDecoderShiftEnergy;
        double selectDecoderEnergy;
        double senseAmpEnergy;
        double prechargeEnergy;
        double currentEnergy;
        double leakage;
        if(inputParameter->lpDecoder){
            rowDecoderInitEnergy = subarray->completeDecoder.readDynamicEnergy;
            rowDecoderShiftEnergy = subarray->completeDecoder.shiftEnergy;
            selectDecoderEnergy = subarray->selectDecoder.readDynamicEnergy * int(inputParameter->inputPrecision);
            senseAmpEnergy = subarray->capLoad.readDynamicEnergy;
            prechargeEnergy = subarray->precharger.readDynamicEnergy * int(inputParameter->inputPrecision);
            // We manually scale the current energy considering the average input and weight value (conservative)
            currentEnergy = 0.25 * 0.25 * subarray->maxBitlineCurrent * prechargeLatency * CLOCK_PERIOD * cell->prechargeVoltage * subarray->numColumn;
            leakage = subarray->leakage;
        }
        else{
            rowDecoderInitEnergy = subarray->completeDecoder.readDynamicEnergy;
            selectDecoderEnergy = subarray->selectDecoder.readDynamicEnergy * int(inputParameter->inputPrecision);
            senseAmpEnergy = subarray->capLoad.readDynamicEnergy;
            prechargeEnergy = subarray->precharger.readDynamicEnergy * int(inputParameter->inputPrecision);
            // We manually scale the current energy considering the average input and weight value (conservative)
            currentEnergy = 0.25 * 0.25 * subarray->maxBitlineCurrent * prechargeLatency * CLOCK_PERIOD * cell->prechargeVoltage * subarray->numColumn;
            leakage = subarray->leakage;
        }

        cout << "   |--- CELL + Others = "
             << (prechargeEnergy + currentEnergy) * 1e12 << " fJ" << endl;
        cout << "   |--- Load Capacitor = "
             << senseAmpEnergy * 1e12 << " fJ" << endl;
        if(!inputParameter->lpDecoder) {
            cout << "   |---  Row Decoder Energy (NR) = "
                 << (rowDecoderInitEnergy + selectDecoderEnergy) * 1e12 << " fJ" << endl;
        }
        else {
            cout << "   |--- Row Decoder Energy (SR) = "
                 << (rowDecoderShiftEnergy + selectDecoderEnergy) * 1e12 << " fJ" << endl;
        }
        cout << "   |--- Row Decoder Energy (CR) = "
             << selectDecoderEnergy * 1e12 << " fJ" << endl;
        cout << "   |--- Leakage = " << leakage * 1e9 << " nW" << endl;
        cout << "#################################################" << endl;
    }
}
