/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef BASICDECODER_H_
#define BASICDECODER_H_

#include "FunctionUnit.h"
#include "OutputDriver.h"

class BasicDecoder: public FunctionUnit {
public:
    BasicDecoder();
    virtual ~BasicDecoder();

    /* Functions */
    void PrintProperty();
    void Initialize(int _numAddressBit, double _capLoad, double _resLoad, bool _normal);
    void CalculateArea();
    void CalculateRC();
    void CalculateLatency(double _rampInput);
    void CalculatePower();

    /* Properties */
    bool initialized;   /* Initialization flag */
    OutputDriver outputDriver;
    double capLoad;     /* Load capacitance, Unit: F */
    double resLoad;     /* Load resistance, Unit: ohm */
    int numNandInput;   /* Type of NAND, NAND2 or NAND3 */
    int numNandGate;    /* Number of NAND Gates */

    double widthNandN, widthNandP;
    double capNandInput, capNandOutput;
    double rampInput, rampOutput;
    /* TO-DO: Basic decoder so far does not take OptPriority input because the output driver is already quite fixed in this module */

    bool normal;
    double vdd;
};

#endif /* BASICDECODER_H_ */
