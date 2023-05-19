/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "InputParameter.h"
#include "global.h"
#include "constant.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

InputParameter::InputParameter() {
    // TODO Auto-generated constructor stub
    optimizationTarget = read_latency_optimized;
    processNode = 90;
    maxDriverCurrent = 0;

    maxNmosSize = MAX_NMOS_SIZE;

    muxSenseAmp = 1;

    minAreaOptimizationLevel = latency_first;
    maxAreaOptimizationLevel = area_first;
    minLocalWireType = local_aggressive;
    maxLocalWireType = local_conservative;
    minGlobalWireType = global_aggressive;
    maxGlobalWireType = global_conservative;
    minLocalWireRepeaterType = repeated_none;
    maxLocalWireRepeaterType = repeated_50;     /* The limit is repeated_50 */
    minGlobalWireRepeaterType = repeated_none;
    maxGlobalWireRepeaterType = repeated_50;    /* The limit is repeated_50 */
    minIsLocalWireLowSwing = false;
    maxIsLocalWireLowSwing = true;
    minIsGlobalWireLowSwing = false;
    maxIsGlobalWireLowSwing = true;

    //cacheAccessMode = normal_access_mode;

    readLatencyConstraint = 1e41;
    readDynamicEnergyConstraint = 1e41;
    leakageConstraint = 1e41;
    areaConstraint = 1e41;
    readEdpConstraint = 1e41;
    isConstraintApplied = false;
    isPruningEnabled = false;

    capLoad = 1e41;
    capLeakage = 1e41;

    numStack = 0;
    inputPrecision = 1;

    outputFilePrefix = "output";    /* Default output file name */
}

InputParameter::~InputParameter() {
    // TODO Auto-generated destructor stub
}

void InputParameter::ReadInputParameterFromFile(const std::string & inputFile) {
    FILE *fp = fopen(inputFile.c_str(), "r");
    char line[5000];
    char tmp[5000];

    if (!fp) {
        cout << inputFile << " cannot be found!\n";
        exit(-1);
    }

    while (fscanf(fp, "%[^\n]\n", line) != EOF) {
        if (!strncmp("-OptimizationTarget", line, strlen("-OptimizationTarget"))) {
            sscanf(line, "-OptimizationTarget: %s", tmp);
            if (!strcmp(tmp, "ReadLatency"))
                optimizationTarget = read_latency_optimized;
            else if (!strcmp(tmp, "ReadDynamicEnergy"))
                optimizationTarget = read_energy_optimized;
            else if (!strcmp(tmp, "ReadEDP"))
                optimizationTarget = read_edp_optimized;
            else if (!strcmp(tmp, "LeakagePower"))
                optimizationTarget = leakage_optimized;
            else if (!strcmp(tmp, "Area"))
                optimizationTarget = area_optimized;
            else
                optimizationTarget = full_exploration;
            continue;
        }

        if (!strncmp("-OutputFilePrefix", line, strlen("-OutputFilePrefix"))) {
            sscanf(line, "-OutputFilePrefix: %s", tmp);
            outputFilePrefix = (string)tmp;
            continue;
        }

        if (!strncmp("-ProcessNode", line, strlen("-ProcessNode"))) {
            sscanf(line, "-ProcessNode: %d", &processNode);
            continue;
        }
        if (!strncmp("-Temperature", line, strlen("-Temperature"))) {
            sscanf(line, "-Temperature (K): %d", &temperature);
            continue;
        }
        if (!strncmp("-PIMMode", line, strlen("-PIMMode"))) {
            sscanf(line, "-PIMMode: %d", &pimMode);
            continue;
        }

        if (!strncmp("-MaxDriverCurrent", line, strlen("-MaxDriverCurrent"))) {
            sscanf(line, "-MaxDriverCurrent (uA): %lf", &maxDriverCurrent);
            continue;
        }
        if (!strncmp("-DeviceRoadmap", line, strlen("-DeviceRoadmap"))) {
            sscanf(line, "-DeviceRoadmap: %s", tmp);
            if (!strcmp(tmp, "HP"))
                deviceRoadmap = HP;
            else if (!strcmp(tmp, "LSTP"))
                deviceRoadmap = LSTP;
            else
                deviceRoadmap = LOP;
            continue;
        }

        if (!strncmp("-LocalWireType", line, strlen("-LocalWireType"))) {
            sscanf(line, "-LocalWireType: %s", tmp);
            if (!strcmp(tmp, "LocalAggressive")) {
                minLocalWireType = local_aggressive;
                maxLocalWireType = local_aggressive;
            } else if (!strcmp(tmp, "LocalConservative")) {
                minLocalWireType = local_conservative;
                maxLocalWireType = local_conservative;
            } else if (!strcmp(tmp, "SemiAggressive")) {
                minLocalWireType = semi_aggressive;
                maxLocalWireType = semi_aggressive;
            } else if (!strcmp(tmp, "SemiConservative")) {
                minLocalWireType = semi_conservative;
                maxLocalWireType = semi_conservative;
            } else if (!strcmp(tmp, "GlobalAggressive")) {
                minLocalWireType = global_aggressive;
                maxLocalWireType = global_aggressive;
            } else if (!strcmp(tmp, "GlobalConservative")) {
                minLocalWireType = global_conservative;
                maxLocalWireType = global_conservative;
            } else {    /* no supported yet */
                minLocalWireType = dram_wordline;
                maxLocalWireType = dram_wordline;
            }
            continue;
        }
        if (!strncmp("-LocalWireRepeaterType", line, strlen("-LocalWireRepeaterType"))) {
            sscanf(line, "-LocalWireRepeaterType: %s", tmp);
            if (!strcmp(tmp, "RepeatedOpt")) {
                minLocalWireRepeaterType = repeated_opt;
                maxLocalWireRepeaterType = repeated_opt;
            } else if (!strcmp(tmp, "Repeated5%Penalty")) {
                minLocalWireRepeaterType = repeated_5;
                maxLocalWireRepeaterType = repeated_5;
            } else if (!strcmp(tmp, "Repeated10%Penalty")) {
                minLocalWireRepeaterType = repeated_10;
                maxLocalWireRepeaterType = repeated_10;
            } else if (!strcmp(tmp, "Repeated20%Penalty")) {
                minLocalWireRepeaterType = repeated_20;
                maxLocalWireRepeaterType = repeated_20;
            } else if (!strcmp(tmp, "Repeated30%Penalty")) {
                minLocalWireRepeaterType = repeated_30;
                maxLocalWireRepeaterType = repeated_30;
            } else if (!strcmp(tmp, "Repeated40%Penalty")) {
                minLocalWireRepeaterType = repeated_40;
                maxLocalWireRepeaterType = repeated_40;
            } else if (!strcmp(tmp, "Repeated50%Penalty")) {
                minLocalWireRepeaterType = repeated_50;
                maxLocalWireRepeaterType = repeated_50;
            } else {
                minLocalWireRepeaterType = repeated_none;
                maxLocalWireRepeaterType = repeated_none;
            }
            continue;
        }
        if (!strncmp("-LocalWireUseLowSwing", line, strlen("-LocalWireUseLowSwing"))) {
            sscanf(line, "-LocalWireUseLowSwing: %s", tmp);
            if (!strcmp(tmp, "Yes")) {
                minIsLocalWireLowSwing = 1;
                maxIsLocalWireLowSwing = 1;
            } else {
                minIsLocalWireLowSwing = 0;
                maxIsLocalWireLowSwing = 0;
            }
            continue;
        }

        if (!strncmp("-GlobalWireType", line, strlen("-GlobalWireType"))) {
            sscanf(line, "-GlobalWireType: %s", tmp);
            if (!strcmp(tmp, "LocalAggressive")) {
                minGlobalWireType = local_aggressive;
                maxGlobalWireType = local_aggressive;
            } else if (!strcmp(tmp, "LocalConservative")) {
                minGlobalWireType = local_conservative;
                maxGlobalWireType = local_conservative;
            } else if (!strcmp(tmp, "SemiAggressive")) {
                minGlobalWireType = semi_aggressive;
                maxGlobalWireType = semi_aggressive;
            } else if (!strcmp(tmp, "SemiConservative")) {
                minGlobalWireType = semi_conservative;
                maxGlobalWireType = semi_conservative;
            } else if (!strcmp(tmp, "GlobalAggressive")) {
                minGlobalWireType = global_aggressive;
                maxGlobalWireType = global_aggressive;
            } else if (!strcmp(tmp, "GlobalConservative")) {
                minGlobalWireType = global_conservative;
                maxGlobalWireType = global_conservative;
            } else {    /* no supported yet */
                minGlobalWireType = dram_wordline;
                maxGlobalWireType = dram_wordline;
            }
            continue;
        }
        if (!strncmp("-GlobalWireRepeaterType", line, strlen("-GlobalWireRepeaterType"))) {
            sscanf(line, "-GlobalWireRepeaterType: %s", tmp);
            if (!strcmp(tmp, "RepeatedOpt")) {
                minGlobalWireRepeaterType = repeated_opt;
                maxGlobalWireRepeaterType = repeated_opt;
            } else if (!strcmp(tmp, "Repeated5%Penalty")) {
                minGlobalWireRepeaterType = repeated_5;
                maxGlobalWireRepeaterType = repeated_5;
            } else if (!strcmp(tmp, "Repeated10%Penalty")) {
                minGlobalWireRepeaterType = repeated_10;
                maxGlobalWireRepeaterType = repeated_10;
            } else if (!strcmp(tmp, "Repeated20%Penalty")) {
                minGlobalWireRepeaterType = repeated_20;
                maxGlobalWireRepeaterType = repeated_20;
            } else if (!strcmp(tmp, "Repeated30%Penalty")) {
                minGlobalWireRepeaterType = repeated_30;
                maxGlobalWireRepeaterType = repeated_30;
            } else if (!strcmp(tmp, "Repeated40%Penalty")) {
                minGlobalWireRepeaterType = repeated_40;
                maxGlobalWireRepeaterType = repeated_40;
            } else if (!strcmp(tmp, "Repeated50%Penalty")) {
                minGlobalWireRepeaterType = repeated_50;
                maxGlobalWireRepeaterType = repeated_50;
            } else {
                minGlobalWireRepeaterType = repeated_none;
                maxGlobalWireRepeaterType = repeated_none;
            }
            continue;
        }
        if (!strncmp("-GlobalWireUseLowSwing", line, strlen("-GlobalWireUseLowSwing"))) {
            sscanf(line, "-GlobalWireUseLowSwing: %s", tmp);
            if (!strcmp(tmp, "Yes")) {
                minIsGlobalWireLowSwing = 1;
                maxIsGlobalWireLowSwing = 1;
            } else {
                minIsGlobalWireLowSwing = 0;
                maxIsGlobalWireLowSwing = 0;
            }
            continue;
        }

        if (!strncmp("-MemoryCellInputFile", line, strlen("-MemoryCellInputFile"))) {
            sscanf(line, "-MemoryCellInputFile: %s", tmp);
            fileMemCell = string(tmp);
            continue;
        }

        if (!strncmp("-MaxNmosSize", line, strlen("-MaxNmosSize"))) {
            sscanf(line, "-MaxNmosSize (F): %lf", &maxNmosSize);
            continue;
        }

        if (!strncmp("-ForceSubarray", line, strlen("-ForceSubarray"))) {
            sscanf(line, "-ForceSubarray (Total AxB): %dx%d",
                    &numRow, &numColumn);
            continue;
        }

        if (!strncmp("-MuxSenseAmp", line, strlen("-MuxSenseAmp"))) {
            sscanf(line, "-MuxSenseAmp: %d", &muxSenseAmp);
            continue;
        }

        if (!strncmp("-EnablePruning", line, strlen("-EnablePruning"))) {
            sscanf(line, "-EnablePruning: %s", tmp);
            if (!strcmp(tmp, "Yes"))
                isPruningEnabled = true;
            else
                isPruningEnabled = false;
            continue;
        }

        if (!strncmp("-BufferDesignOptimization", line, strlen("-BufferDesignOptimization"))) {
            sscanf(line, "-BufferDesignOptimization: %s", tmp);
            if (!strcmp(tmp, "latency")) {
                minAreaOptimizationLevel = 0;
                maxAreaOptimizationLevel = 0;
            } else if (!strcmp(tmp, "area")) {
                minAreaOptimizationLevel = 2;
                maxAreaOptimizationLevel = 2;
            } else {
                minAreaOptimizationLevel = 1;
                maxAreaOptimizationLevel = 1;
            }
        }

        if (!strncmp("-ReferenceReadLatency", line, strlen("-ReferenceReadLatency"))) {
                sscanf(line, "-ReferenceReadLatency: %lf", &referenceReadLatency);
                continue;
        }

        if (!strncmp("-InputPrecision", line, strlen("-InputPrecision"))) {
            sscanf(line, "-InputPrecision: %d", &inputPrecision);
            continue;
        }

        if (!strncmp("-Baseline", line, strlen("-Baseline"))) {
            sscanf(line, "-Baseline: %d", &baseline);
            continue;
        }

        if (!strncmp("-FlashNumStack", line, strlen("-FlashNumStack"))) {
            sscanf(line, "-FlashNumStack: %ld", &numStack);
            continue;
        }

        if (!strncmp("-ApplyReadLatencyConstraint", line, strlen("-ApplyReadLatencyConstraint"))) {
            sscanf(line, "-ApplyReadLatencyConstraint: %lf", &readLatencyConstraint);
            isConstraintApplied = true;
            continue;
        }

        if (!strncmp("-ApplyReadDynamicEnergyConstraint", line, strlen("-ApplyReadDynamicEnergyConstraint"))) {
            sscanf(line, "-ApplyReadDynamicEnergyConstraint: %lf", &readDynamicEnergyConstraint);
            isConstraintApplied = true;
            continue;
        }

        if (!strncmp("-ApplyLeakageConstraint", line, strlen("-ApplyLeakageConstraint"))) {
            sscanf(line, "-ApplyLeakageConstraint: %lf", &leakageConstraint);
            isConstraintApplied = true;
            continue;
        }

        if (!strncmp("-ApplyAreaConstraint", line, strlen("-ApplyAreaConstraint"))) {
            sscanf(line, "-ApplyAreaConstraint: %lf", &areaConstraint);
            isConstraintApplied = true;
            continue;
        }

        if (!strncmp("-ApplyReadEdpConstraint", line, strlen("-ApplyReadEdpConstraint"))) {
            sscanf(line, "-ApplyReadEdpConstraint: %lf", &readEdpConstraint);
            isConstraintApplied = true;
            continue;
        }

        if (!strncmp("-capLoad", line, strlen("-capLoad"))) {
            sscanf(line, "-capLoad: %lf", &capLoad);
            continue;
        }

        if (!strncmp("-capLeakage", line, strlen("-capLeakage"))) {
            sscanf(line, "-capLeakage: %lf", &capLeakage);
            continue;
        }
    }

    fclose(fp);
}

void InputParameter::PrintInputParameter() {
    cout << endl << "====================" << endl << "DESIGN SPECIFICATION" << endl << "====================" << endl;
    cout << "Design Target: ";
    cout << "Random Access Memory" << endl;
    cout << "Page Count : " << numStack << endl;
    // TO-DO: tedious work here!!!

    if (optimizationTarget == full_exploration) {
        cout << endl << "Full design space exploration ... might take hours" << endl;
    } else {
        cout << endl << "Searching for the best solution that is optimized for ";
        switch (optimizationTarget) {
        case read_latency_optimized:
            cout << "read latency ..." << endl;
            break;
        case read_energy_optimized:
            cout << "read energy ..." << endl;
            break;
        case read_edp_optimized:
            cout << "read energy-delay-product ..." << endl;
            break;
        case leakage_optimized:
            cout << "leakage power ..." << endl;
            break;
        default:    /* area */
            cout << "area ..." << endl;
        }
    }
}
