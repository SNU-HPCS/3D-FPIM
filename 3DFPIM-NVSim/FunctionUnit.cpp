/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "FunctionUnit.h"

FunctionUnit::FunctionUnit() {
    height = width = 0;
    area = 0;
    readLatency = 0;
    readDynamicEnergy = 0;
    leakage = 0;
}

FunctionUnit::~FunctionUnit() {
    // TODO Auto-generated destructor stub
}

void FunctionUnit::PrintProperty() {
    cout << "Area = " << height*1e6 << "um x " << width*1e6 << "um = " << area*1e6 << "mm^2" << endl;
    cout << "Timing:" << endl;
    cout << " -  Read Latency = " << readLatency*1e9 << "ns" << endl;
    cout << "Power:" << endl;
    cout << " -  Read Dynamic Energy = " << readDynamicEnergy*1e12 << "pJ" << endl;
    cout << " - Leakage Power = " << leakage*1e3 << "mW" << endl;
}

FunctionUnit & FunctionUnit::operator=(const FunctionUnit &rhs) {
    height = rhs.height;
    width = rhs.width;
    area = rhs.area;
    
    readLatency = rhs.readLatency;
    readDynamicEnergy = rhs.readDynamicEnergy;
    leakage = rhs.leakage;
    return *this;
}
