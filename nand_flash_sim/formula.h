/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef FORMULA_H_
#define FORMULA_H_

#include "Technology.h"
#include "constant.h"
#include <math.h>

#define MAX(a,b) (((a)> (b))?(a):(b))
#define MIN(a,b) (((a)< (b))?(a):(b))
#define M_PI 3.14159265358979323846

bool isPow2(int n);

/* calculate the gate capacitance */
double CalculateGateCap(double width, Technology tech);

double CalculateGateArea(
        int gateType, int numInput,
        double widthNMOS, double widthPMOS,
        double heightTransistorRegion, Technology tech,
        double *height, double *width);

/* calculate the capacitance of a gate */
void CalculateGateCapacitance(
        int gateType, int numInput,
        double widthNMOS, double widthPMOS,
        double heightTransistorRegion, Technology tech,
        double *capInput, double *capOutput);

double CalculateDrainCap(
        double width, int type,
        double heightTransistorRegion, Technology tech);

/* calculate the capacitance of a FBRAM */
double CalculateFBRAMGateCap(double width, double thicknessFactor, Technology tech);

double CalculateFBRAMDrainCap(double width, Technology tech);

double CalculateGateLeakage(
        int gateType, int numInput,
        double widthNMOS, double widthPMOS,
        double temperature, Technology tech);

double CalculateOnResistance(double width, int type, double temperature, Technology tech);

double CalculateTransconductance(double width, int type, Technology tech);

double horowitz(double tr, double beta, double rampInput, double *rampOutput);

double CalculateWireResistance(
        double resistivity, double wireWidth, double wireThickness, 
        double barrierThickness, double dishingThickness, double alphaScatter);

double CalculateWireCapacitance(
        double permittivity, double wireWidth, double wireThickness, double wireSpacing,
        double ildThickness, double millarValue, double horizontalDielectric,
        double verticalDielectic, double fringeCap);

double CalculateIntrinsicWireCapacitance(
        double permittivity, double wireWidth, 
        double ildThickness,
        double verticalDielectric, double fringeCap);

double CalculateCouplingWireCapacitance(
        double permittivity, double wireThickness, double wireSpacing,
        double millerValue, double horizontalDielectric);

double CalculateVerticalCouplingCapacitance(
        double permittivity, double wireRadius, double wireSpacing,
        double millerValue, double horizontalDielectric);

#endif /* FORMULA_H_ */
