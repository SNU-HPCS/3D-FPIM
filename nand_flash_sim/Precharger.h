/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef PRECHARGER_H_
#define PRECHARGER_H_

#include "FunctionUnit.h"
#include "OutputDriver.h"

class Precharger: public FunctionUnit {
public:
    Precharger();
    virtual ~Precharger();

    /* Functions */
    void PrintProperty();
    void Initialize(double _voltagePrecharge, int _numColumn, double _capBitline, double _capEnergy, double _resBitline, double _tau_precharge, double _capDischarge, double _tau_discharge);
    void CalculateArea();
    void CalculateRC();
    void CalculateLatency(double _rampInput);
    void CalculatePower();
    Precharger & operator=(const Precharger &);

    /* Properties */
    bool initialized;   /* Initialization flag */
    OutputDriver outputDriver;
    double voltagePrecharge;  /* Precharge Voltage */
    double capBitline, resBitline;
    double capEnergy;
    double capLoadInv;
    double capOutputBitlinePrecharger;
    double capWireLoadPerColumn, resWireLoadPerColumn;
    double enableLatency;
    double prechargeLatency;
    double dischargeLatency;
    int numColumn;          /* Number of columns */
    double widthPMOSBitlinePrecharger, widthPMOSBitlineEqual;
    double widthInvNmos, widthInvPmos;
    double capLoadPerColumn;
    double rampInput, rampOutput;

    double tau_precharge;
    double tau_discharge;

    double capDischarge;

};

#endif /* PRECHARGER_H_ */
