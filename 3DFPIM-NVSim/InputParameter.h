/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef INPUTPARAMETER_H_
#define INPUTPARAMETER_H_

#include <iostream>
#include <string>
#include <stdint.h>

#include "typedef.h"

using namespace std;

class InputParameter {
public:
    InputParameter();
    virtual ~InputParameter();

    /* Functions */
    void ReadInputParameterFromFile(const std::string & inputFile);
    void PrintInputParameter();

    /* Properties */
    OptimizationTarget optimizationTarget;  /* Either read latency, write latency, read energy, write energy, leakage, or area */
    int processNode;                /* Process node (nm) */
    DeviceRoadmap deviceRoadmap;    /* ITRS roadmap: HP, LSTP, or LOP */
    string fileMemCell;             /* Input file name of memory cell type */
    double maxDriverCurrent;        /* The maximum driving current that the wordline/bitline driver can provide */
    double readLatencyConstraint;   /* The allowed variation to the best read latency */
    double readDynamicEnergyConstraint;     /* The allowed variation to the best read dynamic energy */
    double leakageConstraint;       /* The allowed variation to the best leakage energy */
    double areaConstraint;          /* The allowed variation to the best leakage energy */
    double readEdpConstraint;       /* The allowed variation to the best read EDP */

    double capLoad;
    double capLeakage;

    bool isConstraintApplied;       /* If any design constraint is applied */
    bool isPruningEnabled;          /* Whether to prune the results during the exploration */


    long numStack;

    double maxNmosSize;             /* Default value is MAX_NMOS_SIZE in constant.h, however, user might change it, Unit: F */

    string outputFilePrefix;

    int numRow;
    int numColumn;
    int muxSenseAmp;
    int minAreaOptimizationLevel;   /* This one is actually OptPriority type */
    int maxAreaOptimizationLevel;   /* This one is actually OptPriority type */
    int minLocalWireType;           /* This one is actually WireType type */
    int maxLocalWireType;           /* This one is actually WireType type */
    int minGlobalWireType;          /* This one is actually WireType type */
    int maxGlobalWireType;          /* This one is actually WireType type */
    int minLocalWireRepeaterType;       /* This one is actually WireRepeaterType type */
    int maxLocalWireRepeaterType;       /* This one is actually WireRepeaterType type */
    int minGlobalWireRepeaterType;      /* This one is actually WireRepeaterType type */
    int maxGlobalWireRepeaterType;      /* This one is actually WireRepeaterType type */
    int minIsLocalWireLowSwing;     /* This one is actually boolean */
    int maxIsLocalWireLowSwing;     /* This one is actually boolean */
    int minIsGlobalWireLowSwing;        /* This one is actually boolean */
    int maxIsGlobalWireLowSwing;        /* This one is actually boolean */

    bool pimMode;

    int inputPrecision;
    bool lpDecoder;
    double referenceReadLatency;
};

#endif /* INPUTPARAMETER_H_ */
