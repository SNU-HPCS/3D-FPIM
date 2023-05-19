/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef TECHNOLOGY_H_
#define TECHNOLOGY_H_

#include <iostream>
#include <string>
#include <cstring>
#include "typedef.h"

using namespace std;

class Technology {
public:
    Technology();
    virtual ~Technology();

    /* Functions */
    void PrintProperty();
    void Initialize(int _featureSizeInNano, DeviceRoadmap _deviceRoadmap);
    void InterpolateWith(Technology rhs, double _alpha);
    void ReadTechParameterFromFile(const std::string & inputFile);

    /* Properties */
    bool initialized;   /* Initialization flag */
    int featureSizeInNano; /*Process feature size, Unit: nm */
    double featureSize; /* Process feature size, Unit: m */
    DeviceRoadmap deviceRoadmap;    /* HP, LSTP, or LOP */



    // 


    // external input value
    /* Supply voltage, Unit: V */
    double vdd;
    /* Threshold voltage, Unit: V */
    double vth;


    /* Velocity saturation voltage, Unit: V */
    double vdsatNmos;
    /* Velocity saturation voltage, Unit: V */
    double vdsatPmos;
    /* Physical gate length, Unit: m */
    double phyGateLength;
    /* Ideal gate capacitance, Unit: F/m */
    double capIdealGate;
    /* Fringe capacitance, Unit: F/m */
    double capFringe;
    /* Junction bottom capacitance, Cj0, Unit: F/m^2 */
    double capJunction;
    /* Overlap capacitance, Cover in MASTAR, Unit: F/m */
    double capOverlap;
    /* Junction sidewall capacitance, Cjsw, Unit: F/m */
    double capSidewall;
    /* Junction drain to channel capacitance, Cjswg, Unit: F/m */
    double capDrainToChannel;
    /* Cox_elec in MASTAR, Unit: F/m^2 */
    double capOx;
    /* Bottom junction built-in potential(PB in BSIM4 model), Unit: V */
    double buildInPotential;
    /* ueff for NMOS in MASTAR, Unit: m^2/V/s */
    double effectiveElectronMobility;
    /* ueff for PMOS in MASTAR, Unit: m^2/V/s */
    double effectiveHoleMobility;
    /* PMOS to NMOS size ratio */
    double pnSizeRatio;
    /* Extra resistance due to vdsat */
    double effectiveResistanceMultiplier;



    
    /* NMOS saturation current, Unit: A/m */
    double currentOnNmos[101];
    /* PMOS saturation current, Unit: A/m */
    double currentOnPmos[101];
    /* NMOS off current (from 300K to 400K), Unit: A/m */
    double currentOffNmos[101];
    /* PMOS off current (from 300K to 400K), Unit: A/m */
    double currentOffPmos[101];

    double capPolywire;

    // added new 

    //////////////

    double flCapGc;
    double flCapOfGex;
    double flCapOfGsd;
    double flCapSide;
    double flCD;

    double trCapGc;
    double trCapOfGex;
    double trCapOfGsd;
    double trCapIf;
    double trCapSide;
    double trCD;

    double plCapOfGsd;
};

#endif /* TECHNOLOGY_H_ */
