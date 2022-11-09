/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef FUNCTIONUNIT_H_
#define FUNCTIONUNIT_H_

#include <iostream>

using namespace std;

class FunctionUnit {
public:
    FunctionUnit();
    virtual ~FunctionUnit();

    /* Functions */
    virtual void PrintProperty();
    virtual FunctionUnit & operator=(const FunctionUnit &);

    /* Properties */
    double height;      /* Unit: m */
    double width;       /* Unit: m */
    double area;        /* Unit: m^2 */
    double readLatency;
    double readDynamicEnergy;
    double leakage;     /* Unit: W */
};

#endif /* FUNCTIONUNIT_H_ */
