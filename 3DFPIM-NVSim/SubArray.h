/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef SUBARRAY_H_
#define SUBARRAY_H_

#include "FunctionUnit.h"
#include "RowDecoder.h"
#include "PredecodeBlock.h"
#include "CompleteDecoder.h"
#include "Precharger.h"
#include "SenseAmp.h"
#include "Cap.h"
#include "typedef.h"


class SubArray: public FunctionUnit {
public:
    SubArray();
    virtual ~SubArray();

    /* Functions */
    void PrintProperty();
    void Initialize(long long _numRow, long long _numColumn,
            int _muxSenseAmp,
            BufferDesignTarget _areaOptimizationLevel);
    void CalculateArea();
    //void CalculateRC();
    void CalculateLatency(double _rampInput);
    void CalculatePower();
    SubArray & operator=(const SubArray &);

    /* Properties */
    bool initialized;   /* Initialization flag */
    bool invalid;       /* Indicate that the current configuration is not valid, pass down to all the sub-components */
    long long numRow;           /* Number of rows */
    long long numRowReal;           /* Number of rows */
    long long numColumn;        /* Number of columns */
    int muxSenseAmp;
    BufferDesignTarget areaOptimizationLevel;

    bool voltageSense;  /* Whether the sense amplifier is voltage-sensing */
    double senseVoltage;/* Minimum sensible voltage */
    double voltagePrecharge;
    long long numSenseAmp;  /* Number of sense amplifiers */
    long long numCap;   /* Number of sense amplifiers */
    double lenWordline; /* Length of wordlines, Unit: m */
    double lenCellline; /* Length of wordlines, Unit: m */
    double lenBitline;  /* Length of bitlines, Unit: m */
    double heightString;    /* Height of the cube, Unit: m */

    double capWordlineLatency;  /* Wordline capacitance, Unit: F */
    double capWordlineDischarge;
    double capWordlineEnergy;   /* Wordline capacitance, Unit: F */
    //double capWordlineShiftLatency;   /* Wordline capacitance, Unit: F */
    //double capWordlineShiftDischarge;
    //double capWordlineShiftEnergy;    /* Wordline capacitance, Unit: F */
    double resWordline; /* Wordline resistance, Unit: ohm */

    double capSelectline;   /* Selectline capacitance, Unit: F */
    double resSelectline;

    double capBitline;  /* Bitline capacitance, Unit: F */
    double resBitline;  /* Bitline resistance, Unit: ohm */

    double capStringline;   /* Bitline capacitance, Unit: F */
    double resStringline;   /* Wordline resistance, Unit: ohm */

    double capDecoderVerticalline;
    double resDecoderVerticalline;

    double capStaircaseLatency;
    double capStaircaseEnergy;
    double resStaircase;

    double resCellAccess; /* Resistance of access device, Unit: ohm */
    double capCellAccess; /* Capacitance of access device, Unit: ohm */
    double resMemCellOff;  /* HRS resistance, Unit: ohm */
    double resMemCellOn;   /* LRS resistance, Unit: ohm */
    double voltageMemCellOff; /* Voltage drop on HRS during read operation, Unit: V */
    double voltageMemCellOn;   /* Voltage drop on LRS druing read operation, Unit: V */
    double resInSerialForSenseAmp; /* Serial resistance of voltage-in voltage sensing as a voltage divider, Unit: ohm */
    double resEquivalentOn;          /* resInSerialForSenseAmp in parallel with resMemCellOn, Unit: ohm */
    double resEquivalentOff;          /* resInSerialForSenseAmp in parallel with resMemCellOn, Unit: ohm */
    double bitlineDelay;    /* Bitline delay, Unit: s */
    double cellDelay;
    double chargeLatency;   /* The bitline charge delay during write operations, Unit: s */
    double columnDecoderLatency;    /* The worst-case mux latency, Unit: s */
    double decoderLatency;

    double maxBitlineCurrent;

    //RowDecoder rowDecoder;
    CompleteDecoder completeDecoder;
    //CompleteDecoder completeDecoderBaseline;
    //CompleteDecoder completeDecoderShift;

    RowDecoder selectDecoder;

    //RowDecoder    senseAmpMuxLev1Decoder;
    //Mux           senseAmpMuxLev1;
    //RowDecoder    senseAmpMuxLev2Decoder;
    //Mux           senseAmpMuxLev2;
    Precharger  precharger;
    SenseAmp    senseAmp;
    Cap         capLoad;

    double chargeEnergy;
    double chargeLeakLatency;

};

#endif /* SUBARRAY_H_ */
