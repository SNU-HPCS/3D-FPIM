/*******************************************************************************
* Copyright (c) 2022, Seoul National University. See LICENSE file in the top-
* level directory. 3D-FPIM Project can be copied according to the terms
* contained in the LICENSE file.
*******************************************************************************/

#ifndef COMPLETEDECODER_H_
#define COMPLETEDECODER_H_

#include "FunctionUnit.h"
#include "OutputDriver.h"
#include "typedef.h"
#include "RowDecoder.h"
#include "PredecodeBlock.h"

class CompleteDecoder: public FunctionUnit {
public:
    CompleteDecoder();
    virtual ~CompleteDecoder();

    /* Functions */
    void PrintProperty();
    void Initialize(int _numDrivers, int _numBlocks, int _numStairs,
            double _capLoad, double _resLoad, double _tau,
            double _capLoadDischarge,
            double _tauDischarge,
            BufferDesignTarget _areaOptimizationLevel, 
            double _minDriverCurrent, bool _normal);
    void CalculateArea();
    void CalculateRC();
    void CalculateLatency(double _rampInput);
    void CalculatePower();
    CompleteDecoder & operator=(const CompleteDecoder &);

    /* Properties */
    bool initialized;   /* Initialization flag */
    bool invalid;      /*Invalidatio flag */

    RowDecoder finalRowDecoder;

    PredecodeBlock blockPredecoderBlock1;
    PredecodeBlock blockPredecoderBlock2;
    RowDecoder blockRowDecoder;

    PredecodeBlock wlPredecoderBlock1;
    PredecodeBlock wlPredecoderBlock2;
    RowDecoder wlRowDecoder;

    double capLoad;     /* Load capacitance, i.e. wordline capacitance, Unit: F */
    double capLoadDischarge;
    double tau;
    double tauDischarge;
    double resLoad;     /* Load resistance, Unit: ohm */
    BufferDesignTarget areaOptimizationLevel; /* 0 for latency, 2 for area */
    double minDriverCurrent; /* Minimum driving current should be provided */

    int numBlocks;
    int numStairs;

    bool normal;
    double vdd;

    double decodingDynamicEnergy;
    double wlReadDynamicEnergy;
    double shiftEnergy;
    double rampOutput;

    double dischargeLatency;

    int numDrivers;
};

#endif /* ROWDECODER_H_ */
