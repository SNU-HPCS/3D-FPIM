/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/

#ifndef SENSEAMP_H_
#define SENSEAMP_H_

#include "FunctionUnit.h"

class SenseAmp: public FunctionUnit {
public:
    SenseAmp();
    virtual ~SenseAmp();

    /* Functions */
    void PrintProperty();
    void Initialize(long long _numColumn, bool _currentSense, double _senseVoltage /* Unit: V */, double _pitchSenseAmp);
    void CalculateArea();
    void CalculateRC();
    void CalculateLatency(double _rampInput);
    void CalculatePower();
    SenseAmp & operator=(const SenseAmp &);

    /* Properties */
    bool initialized;   /* Initialization flag */
    bool invalid;       /* Indicate that the current configuration is not valid */
    long long numColumn;        /* Number of columns */
    bool currentSense;  /* Whether the sensing scheme is current-based */
    double senseVoltage;    /* Minimum sensible voltage */
    double capLoad;     /* Load capacitance of sense amplifier */
    double pitchSenseAmp;   /* The maximum width allowed for one sense amplifier layout */
    int numPerLine;
};

#endif /* SENSEAMP_H_ */
