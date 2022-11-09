/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef WIRE_H_
#define WIRE_H_

#include <iostream>
#include <string>
#include "typedef.h"
#include "SenseAmp.h"

using namespace std;

class Wire {
public:
    Wire();
    virtual ~Wire();

    /* Functions */
    void PrintProperty();
    void Initialize(int _featureSizeInNano, 
            WireType _wireType, WireRepeaterType _wireRepeaterType,
            int _temperature, bool _isLowSwing);
    void CalculateLatencyAndPower(double _wireLength, double *delay, double *dynamicEnergy, double *leakagePower);
    void findOptimalRepeater();
    void findPenalizedRepeater(double _penalty);

    void ReadWireParameterFromFile(const std::string & inputFile);

    // not used for vertical wire
    /* Return delay per unit, Unit: s/m */
    double getRepeatedWireUnitDelay();
    /* Return dynamic energy per unit, Unit: J/m */
    double getRepeatedWireUnitDynamicEnergy();
    /* Return leakage power per unit, Unit: W/m */
    double getRepeatedWireUnitLeakage();
    Wire & operator=(const Wire &);

    /* Properties */
    /* Initialization flag */
    bool initialized;
    /* Process feature size, Unit: nm */
    int featureSizeInNano;
    /* Process feature size, Unit: m */
    double featureSize;
    /* Type of wire */
    WireType wireType;
    /* Type of wire repeater */
    WireRepeaterType wireRepeaterType;

    /* Unit: K */
    int temperature;
    /* Whether to use Low Swing wire with transmitters and receivers */
    bool isLowSwing;

    double barrierThickness;        /* Unit: m */
    double horizontalDielectric;    /* Unit: 1 */
    double wirePitch;               /* Unit: m */
    double aspectRatio;             /* Unit: 1 */
    double ildThickness;            /* Unit: m */
    double wireWidth;               /* Unit: m */
    double wireThickness;           /* Unit: m */
    double wireSpacing;             /* Unit: m */

    double repeaterSize;            /* For repeated wire only, non-repeated wire = 0, Unit: minimum driver size*/
    double repeaterSpacing;         /* For repeated wire only, non-repeated wire = inf, Unit: m */
    double repeaterHeight, repeaterWidth;   /* Unit: m */
    double repeatedWirePitch;       /* For repeated wire only, translate the repeaterSize into meter, Unit: m */

    // final required dat
    double resWirePerUnit;          /* Unit: ohm/m */
    double capWirePerUnit;          /* Unit: F/m */
    double intrinsicCapWirePerUnit;         /* Unit: F/m */
    double couplingCapWirePerUnit;          /* Unit: F/m */

    double resWirePerCell;
    double resWirePerStaircase;

    double resViaWirePerUnit;
    double capViaWirePerString;

    SenseAmp * senseAmp;
};

#endif /* WIRE_H_ */
