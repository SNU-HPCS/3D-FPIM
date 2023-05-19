/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef ROWDECODER_H_
#define ROWDECODER_H_

#include "FunctionUnit.h"
#include "OutputDriver.h"
#include "typedef.h"

class RowDecoder: public FunctionUnit {
public:
    RowDecoder();
    virtual ~RowDecoder();

    /* Functions */
    void PrintProperty();
    void Initialize(int _numRow, 
            double _capLoad, double _resLoad, double _tau,
            BufferDesignTarget _areaOptimizationLevel, 
            double _minDriverCurrent, bool _normal=true, double _capLoadDischarge = 0, double _tauDischarge = 0);

    void CalculateArea();
    void CalculateRC();
    void CalculateLatency(double _rampInput);
    void CalculatePower();
    RowDecoder & operator=(const RowDecoder &);

    /* Properties */
    bool initialized;   /* Initialization flag */
    bool invalid;      /*Invalidatio flag */
    OutputDriver outputDriver;
    int numRow;
    int numNandInput;   /* Type of NAND, NAND2 or NAND3 */
    double capLoad;     /* Load capacitance, i.e. wordline capacitance, Unit: F */
    double capLoadDischarge;        /* Load capacitance, i.e. wordline capacitance, Unit: F */
    double resLoad;     /* Load resistance, Unit: ohm */
    BufferDesignTarget areaOptimizationLevel; /* 0 for latency, 2 for area */
    double minDriverCurrent; /* Minimum driving current should be provided */

    double widthNandN, widthNandP;
    double capNandInput, capNandOutput;
    double rampInput, rampOutput;

    bool normal;
    double vdd;
    double tau;
    double tauDischarge;

    double dischargeLatency;
};

#endif /* ROWDECODER_H_ */
