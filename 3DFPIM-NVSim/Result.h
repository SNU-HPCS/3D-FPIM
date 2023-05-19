/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef RESULT_H_
#define RESULT_H_

#include "SubArray.h"
#include "Wire.h"

class Result {
public:
    Result();
    virtual ~Result();

    /* Functions */
    void print();
    void reset();
    void compareAndUpdate(Result &newResult);

    OptimizationTarget optimizationTarget;  /* Exploration should not be assigned here */

    SubArray * subarray;
    Wire * localWire;       /* TO-DO: this one has the same name as one of the global variables */
    Wire * globalWire;
    Wire * selectlineWire;
    Wire * wordlineWire;
    Wire * contactWire;
    Wire * stringWire;
    Wire * bitlineWire;

    double limitReadLatency;            /* The maximum allowable read latency, Unit: s */
    double limitReadDynamicEnergy;      /* The maximum allowable read dynamic energy, Unit: J */
    double limitReadEdp;                /* The maximum allowable read EDP, Unit: s-J */
    double limitArea;                   /* The maximum allowable area, Unit: m^2 */
    double limitLeakage;                /* The maximum allowable leakage power, Unit: W */
};

#endif /* RESULT_H_ */
