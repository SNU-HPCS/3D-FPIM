/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <math.h>
#include "InputParameter.h"
#include "MemCell.h"
#include "RowDecoder.h"
#include "Precharger.h"
#include "OutputDriver.h"
#include "SenseAmp.h"
#include "Technology.h"
#include "BasicDecoder.h"
#include "PredecodeBlock.h"
#include "SubArray.h"
//#include "Mat.h"
//#include "BankWithoutHtree.h"
#include "Wire.h"
#include "Result.h"
#include "formula.h"
#include "macros.h"

using namespace std;

InputParameter *inputParameter;
Technology *tech;
MemCell *cell;
Wire *localWire;
Wire *selectlineWire;
Wire *globalWire;
Wire *contactWire;
Wire *stringWire;
Wire *wordlineWire;
Wire *bitlineWire;

int main(int argc, char *argv[])
{
    cout << fixed << setprecision(3);
    string inputFileName;

    if (argc == 2) {
        inputFileName = argv[1];
        cout << "User-defined configuration file (" << inputFileName << ") is loaded" << endl;
    } else {
        cout << "[NAND Flash (PIM) Error]: Please use the correct format as follows" << endl;
        cout << "  Use the default configuration: " << argv[0] << endl;
        cout << "  Use the customized configuration: " << argv[0] << " <.cfg file>"  << endl;
        exit(-1);
    }
    cout << endl;

    inputParameter = new InputParameter();
    RESTORE_SEARCH_SIZE;
    inputParameter->ReadInputParameterFromFile(inputFileName);

    tech = new Technology();
    tech->Initialize(inputParameter->processNode, inputParameter->deviceRoadmap);

    Technology techHigh;
    double alpha = 0;
    if (inputParameter->processNode > 200){
        // TO-DO: technology node > 200 nm
    } else if (inputParameter->processNode > 120) { // 120 nm < technology node <= 200 nm
        techHigh.Initialize(200, inputParameter->deviceRoadmap);
        alpha = (inputParameter->processNode - 120.0) / 60;
    } else if (inputParameter->processNode > 90) { // 90 nm < technology node <= 120 nm
        techHigh.Initialize(120, inputParameter->deviceRoadmap);
        alpha = (inputParameter->processNode - 90.0) / 30;
    } else if (inputParameter->processNode > 65) { // 65 nm < technology node <= 90 nm
        techHigh.Initialize(90, inputParameter->deviceRoadmap);
        alpha = (inputParameter->processNode - 65.0) / 25;
    } else if (inputParameter->processNode > 45) { // 45 nm < technology node <= 65 nm
        techHigh.Initialize(65, inputParameter->deviceRoadmap);
        alpha = (inputParameter->processNode - 45.0) / 20;
    } else if (inputParameter->processNode >= 32) { // 32 nm < technology node <= 45 nm
        techHigh.Initialize(45, inputParameter->deviceRoadmap);
        alpha = (inputParameter->processNode - 32.0) / 13;
    } else if (inputParameter->processNode >= 22) { // 22 nm < technology node <= 32 nm
        techHigh.Initialize(32, inputParameter->deviceRoadmap);
        alpha = (inputParameter->processNode - 22.0) / 10;
    } else {
        //TO-DO: technology node < 22 nm
    }

    tech->InterpolateWith(techHigh, alpha);

    cell = new MemCell();
    cell->ReadCellFromFile(inputParameter->fileMemCell);
    cell->ReadDimensionFromFile("tech_params/cellDimension.cfg");

    cell->PrintCell();

    int areaOptimizationLevel;                          /* actually BufferDesignTarget */
    int localWireType, globalWireType;                  /* actually WireType */
    int localWireRepeaterType, globalWireRepeaterType;  /* actually WireRepeaterType */
    int isLocalWireLowSwing, isGlobalWireLowSwing;      /* actually boolean value */

    /* for cache data array, memory array */
    Result bestDataResults[(int)full_exploration];  /* full_exploration is always set as the last element in the enum, so if full_exploration is 8, what we want here is a 0-7 array, which is correct */
    SubArray *dataSubArray;
    for (int i = 0; i < (int)full_exploration; i++) {
        FILTER_PIM_MODE(i);
        bestDataResults[i].optimizationTarget = (OptimizationTarget)i;
    }

    /* for cache tag array only */
    Result bestTagResults[(int)full_exploration];   /* full_exploration is always set as the last element in the enum, so if full_exploration is 8, what we want here is a 0-7 array, which is correct */
    for (int i = 0; i < (int)full_exploration; i++) {
        FILTER_PIM_MODE(i);
        bestTagResults[i].optimizationTarget = (OptimizationTarget)i;
    }

    localWire = new Wire();
    globalWire = new Wire();
    selectlineWire = new Wire();
    wordlineWire = new Wire();
    contactWire = new Wire();
    stringWire = new Wire();
    bitlineWire = new Wire();

    long long numSolution = 0;

    inputParameter->PrintInputParameter();

    INITIAL_BASIC_WIRE;
    for (areaOptimizationLevel = inputParameter->minAreaOptimizationLevel; areaOptimizationLevel <= inputParameter->maxAreaOptimizationLevel; areaOptimizationLevel++) {
        CALCULATE(dataSubArray, data);
        if (!dataSubArray->invalid) {
            Result tempResult;
            numSolution++;
            UPDATE_BEST_DATA;
        }
        delete dataSubArray;
    }
    if (numSolution > 0) {
        SubArray * trialSubArray;
        Result tempResult;
        /* refine local wire type */
        REFINE_LOCAL_WIRE_FORLOOP {
            localWire->Initialize(inputParameter->processNode, (WireType)localWireType,
                    (WireRepeaterType)localWireRepeaterType, inputParameter->temperature,
                    (bool)isLocalWireLowSwing);
            for (int i = 0; i < (int)full_exploration; i++) {
                FILTER_PIM_MODE(i);
                LOAD_GLOBAL_WIRE(bestDataResults[i]);
                TRY_AND_UPDATE(bestDataResults[i], data);
            }
        }
        /* refine global wire type */
        REFINE_GLOBAL_WIRE_FORLOOP {
            globalWire->Initialize(inputParameter->processNode, (WireType)globalWireType,
                    (WireRepeaterType)globalWireRepeaterType, inputParameter->temperature,
                    (bool)isGlobalWireLowSwing);
            for (int i = 0; i < (int)full_exploration; i++) {
                FILTER_PIM_MODE(i);
                LOAD_LOCAL_WIRE(bestDataResults[i]);
                TRY_AND_UPDATE(bestDataResults[i], data);
            }
        }
    }

    if (inputParameter->optimizationTarget == full_exploration && inputParameter->isPruningEnabled) {
        /* pruning is enabled */
        Result **** pruningResults;
        /* pruningResults[x][y][z] points to the result which is optimized for x, with constraint on y with z overhead */
        pruningResults = new Result***[(int)full_exploration];  /* full_exploration is always set as the last element in the enum, so if full_exploration is 8, what we want here is a 0-7 array, which is correct */
        for (int i = 0; i < (int)full_exploration; i++) {
            FILTER_PIM_MODE(i);
            pruningResults[i] = new Result**[(int)full_exploration];
            for (int j = 0; j < (int)full_exploration; j++) {
                FILTER_PIM_MODE(j);
                pruningResults[i][j] = new Result*[3];      /* 10%, 20%, and 30% overhead */
                for (int k = 0; k < 3; k++)
                    pruningResults[i][j][k] = new Result;
            }
        }

        /* assign the constraints */
        for (int i = 0; i < (int)full_exploration; i++) {
            FILTER_PIM_MODE(i);
            for (int j = 0; j < (int)full_exploration; j++) {
                FILTER_PIM_MODE(j);
                for (int k = 0; k < 3; k++) {
                    pruningResults[i][j][k]->optimizationTarget = (OptimizationTarget)i;
                    *(pruningResults[i][j][k]->localWire) = *(bestDataResults[i].localWire);
                    *(pruningResults[i][j][k]->globalWire) = *(bestDataResults[i].globalWire);
                    switch ((OptimizationTarget)j) {
                    case read_latency_optimized:
                        pruningResults[i][j][k]->limitReadLatency = bestDataResults[j].subarray->readLatency * (1 + (k + 1.0) / 10);
                        break;
                    case read_energy_optimized:
                        pruningResults[i][j][k]->limitReadDynamicEnergy = bestDataResults[j].subarray->readDynamicEnergy * (1 + (k + 1.0) / 10);
                        break;
                    case read_edp_optimized:
                        pruningResults[i][j][k]->limitReadEdp = bestDataResults[j].subarray->readLatency * bestDataResults[j].subarray->readDynamicEnergy * (1 + (k + 1.0) / 10);
                        break;
                    case area_optimized:
                        pruningResults[i][j][k]->limitArea = bestDataResults[j].subarray->area * (1 + (k + 1.0) / 10);
                        break;
                    case leakage_optimized:
                        pruningResults[i][j][k]->limitLeakage = bestDataResults[j].subarray->leakage * (1 + (k + 1.0) / 10);
                        break;
                    default:
                        /* nothing should happen here */
                        cout << "Warning: should not happen" << endl;
                    }
                }
            }
        }

        cout << "Pruning done" << endl;
        /* Run pruning here */
        /* TO-DO */

        /* delete */
        for (int i = 0; i < (int)full_exploration; i++) {
            FILTER_PIM_MODE(i);
            for (int j = 0; j < (int)full_exploration; j++) {
                FILTER_PIM_MODE(j);
                for (int k = 0; k < 3; k++)
                    delete pruningResults[i][j][k];
                delete [] pruningResults[i][j];
            }
            delete [] pruningResults[i];
        }
    }

    /* If design constraint is applied */
    if (inputParameter->optimizationTarget != full_exploration && inputParameter->isConstraintApplied) {
        double allowedDataReadLatency = bestDataResults[read_latency_optimized].subarray->readLatency * (inputParameter->readLatencyConstraint + 1);
        double allowedDataReadDynamicEnergy = bestDataResults[read_energy_optimized].subarray->readDynamicEnergy * (inputParameter->readDynamicEnergyConstraint + 1);
        double allowedDataLeakage = bestDataResults[leakage_optimized].subarray->leakage * (inputParameter->leakageConstraint + 1);
        double allowedDataArea = bestDataResults[area_optimized].subarray->area * (inputParameter->areaConstraint + 1);
        double allowedDataReadEdp = bestDataResults[read_edp_optimized].subarray->readLatency
                * bestDataResults[read_edp_optimized].subarray->readDynamicEnergy * (inputParameter->readEdpConstraint + 1);
        for (int i = 0; i < (int)full_exploration; i++) {
            FILTER_PIM_MODE(i);
            APPLY_LIMIT(bestDataResults[i]);
        }

        numSolution = 0;
        INITIAL_BASIC_WIRE;
        for (areaOptimizationLevel = inputParameter->minAreaOptimizationLevel; areaOptimizationLevel <= inputParameter->maxAreaOptimizationLevel; areaOptimizationLevel++) {
            CALCULATE(dataSubArray, data);
            if (!dataSubArray->invalid && dataSubArray->readLatency <= allowedDataReadLatency 
                    && dataSubArray->readDynamicEnergy <= allowedDataReadDynamicEnergy
                    && dataSubArray->leakage <= allowedDataLeakage && dataSubArray->area <= allowedDataArea
                    && dataSubArray->readLatency * dataSubArray->readDynamicEnergy <= allowedDataReadEdp) {
                Result tempResult;
                numSolution++;
                UPDATE_BEST_DATA;
            }
            delete dataSubArray;
        }
    }

    if (inputParameter->optimizationTarget != full_exploration) {
        if (numSolution > 0) {
            bestDataResults[inputParameter->optimizationTarget].print();
        } else {
            cout << "No valid solutions." << endl;
        }
        cout << endl << "Finished!" << endl;
    } else {
        if (inputParameter->isPruningEnabled) {
            cout << "The results are pruned" << endl;
        } else {
            int solutionMultiplier = 1;
            cout << numSolution * solutionMultiplier << " solutions in total" << endl;
        }
    }

    if (localWire) delete localWire;
    if (globalWire) delete globalWire;

    return 0;
}
