/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/

#include "Precharger.h"
#include "formula.h"
#include "global.h"

Precharger::Precharger() {
    // TODO Auto-generated constructor stub
    initialized = false;
    enableLatency = 0;
}

Precharger::~Precharger() {
    // TODO Auto-generated destructor stub
}

void Precharger::Initialize(double _voltagePrecharge, 
        int _numColumn, double _capBitline,
        double _capEnergy,
        double _resBitline, double _tau_precharge, double _capDischarge, double _tau_discharge){

    if (initialized)
        cout << "[Precharger] Warning: Already initialized!" << endl;

    voltagePrecharge = _voltagePrecharge;
    numColumn  = _numColumn;

    // calculate the precharge latency
    capBitline = _capBitline;
    resBitline = _resBitline;
    capEnergy = _capEnergy;
    tau_precharge = _tau_precharge;

    capDischarge = _capDischarge;
    tau_discharge = _tau_discharge;
    
    // calculate the driver latency
    capWireLoadPerColumn = cell->widthInSize * 
            bitlineWire->capWirePerUnit;
    resWireLoadPerColumn = cell->widthInSize * 
            bitlineWire->resWirePerUnit;

    widthInvNmos = MIN_NMOS_SIZE * tech->featureSize;
    widthInvPmos = widthInvNmos * tech->pnSizeRatio;
    widthPMOSBitlineEqual      = MIN_NMOS_SIZE * tech->featureSize;
    widthPMOSBitlinePrecharger = 6 * tech->featureSize;

    capLoadInv  = CalculateGateCap(widthPMOSBitlineEqual, *tech) + 
            2 * CalculateGateCap(widthPMOSBitlinePrecharger, *tech)
            + CalculateDrainCap(widthInvNmos, NMOS, 
            tech->featureSize*40, *tech)
            + CalculateDrainCap(widthInvPmos, PMOS, 
            tech->featureSize*40, *tech);
    capOutputBitlinePrecharger = CalculateDrainCap
            (widthPMOSBitlinePrecharger, PMOS, 
            tech->featureSize*40, *tech) + CalculateDrainCap
            (widthPMOSBitlineEqual, PMOS, 
            tech->featureSize*40, *tech);
    double capInputInv         = 
            CalculateGateCap(widthInvNmos, *tech) + 
            CalculateGateCap(widthInvPmos, *tech);
    capLoadPerColumn           = capInputInv + capWireLoadPerColumn;
    double capLoadOutputDriver = numColumn * capLoadPerColumn;
    outputDriver.Initialize(1, capInputInv, 
            capLoadOutputDriver, 0 /* TO-DO */, 0,
            true, area_first, 0);  /* Always Latency First */

    initialized = true;
}

void Precharger::CalculateArea() {
    if (!initialized) {
        cout << "[Precharger] Error: Require initialization first!" << endl;
    } else {
        outputDriver.CalculateArea();
        double hBitlinePrechareger, wBitlinePrechareger;
        double hBitlineEqual, wBitlineEqual;
        double hInverter, wInverter;
        CalculateGateArea(INV, 1, 0, 
                widthPMOSBitlinePrecharger, 
                tech->featureSize*40, 
                *tech, &hBitlinePrechareger, 
                &wBitlinePrechareger);
        CalculateGateArea(INV, 1, 0, widthPMOSBitlineEqual, 
                tech->featureSize*40, *tech, 
                &hBitlineEqual, &wBitlineEqual);
        CalculateGateArea(INV, 1, widthInvNmos, 
                widthInvPmos, tech->featureSize*40, 
                *tech, &hInverter, &wInverter);
        width = 2 * wBitlinePrechareger + wBitlineEqual;
        width = MAX(width, wInverter);
        width *= numColumn;
        width = MAX(width, outputDriver.width);
        height = MAX(hBitlinePrechareger, hBitlineEqual);
        height += hInverter;
        height = MAX(height, outputDriver.height);
        area = height * width;
    }
}

void Precharger::CalculateRC() {
    if (!initialized) {
        cout << "[Precharger] Error: Require initialization first!" << endl;
    } else {
        outputDriver.CalculateRC();
        // more accurate RC model would include drain 
        // Capacitances of Precharger and Equalization PMOS transistors
    }
}

void Precharger::CalculateLatency(double _rampInput){
    if (!initialized) {
        cout << "[Precharger] Error: Require initialization first!" << endl;
    } else {
        rampInput= _rampInput;
        outputDriver.CalculateLatency(rampInput);
        enableLatency = outputDriver.readLatency;
        double resPullDown;
        double tr;  /* time constant */
        double gm;  /* transconductance */
        double beta;    /* for horowitz calculation */
        double temp;
        resPullDown = CalculateOnResistance(widthInvNmos, 
                NMOS, inputParameter->temperature, *tech);
        tr = resPullDown * capLoadInv;
        gm = CalculateTransconductance(widthInvNmos, NMOS, *tech);
        beta = 1 / (resPullDown * gm);
        enableLatency += horowitz(tr, beta, outputDriver.rampOutput, &temp);
        readLatency = 0;
        prechargeLatency = 0;
        dischargeLatency = 0;
        double resPullUp = CalculateOnResistance
                (widthPMOSBitlinePrecharger, PMOS,
                inputParameter->temperature, *tech);
        double tau_pre = resPullUp * (capBitline + 
                capOutputBitlinePrecharger) + tau_precharge;
        double tau_dis = resPullUp * (capDischarge + 
                capOutputBitlinePrecharger) + tau_discharge;
        gm = CalculateTransconductance(widthPMOSBitlinePrecharger, PMOS, *tech);
        beta = 1 / (resPullUp * gm);
        readLatency += horowitz(tau_pre, beta, temp, &rampOutput);
        prechargeLatency += horowitz(tau_pre, beta, temp, &rampOutput);
        dischargeLatency += horowitz(tau_dis, beta, temp, &rampOutput);
    }
}

void Precharger::CalculatePower() {
    if (!initialized) {
        cout << "[Precharger] Error: Require initialization first!" << endl;
    } else {
        outputDriver.CalculatePower();
        /* Leakage power */
        leakage = outputDriver.leakage;
        leakage += numColumn * voltagePrecharge * 
                CalculateGateLeakage(INV, 1, widthInvNmos, 
                widthInvPmos, inputParameter->temperature, *tech);
        leakage += numColumn * voltagePrecharge * 
                CalculateGateLeakage(INV, 1, 0, widthPMOSBitlinePrecharger,
                inputParameter->temperature, *tech);

        /* Dynamic energy */
        /* We don't count bitline precharge energy into account because it is a charging process */
        readDynamicEnergy = outputDriver.readDynamicEnergy;
        readDynamicEnergy += capLoadInv * 
                voltagePrecharge * voltagePrecharge * numColumn;

        // multiply by 0.25 * 0.25 (sparsity of the input / weight)
        readDynamicEnergy += capEnergy *
                voltagePrecharge * voltagePrecharge * numColumn;
    }
}

void Precharger::PrintProperty() {
    cout << "Precharger Properties:" << endl;
    FunctionUnit::PrintProperty();
}

Precharger & Precharger::operator=(const Precharger &rhs) {
    FunctionUnit::operator=(rhs);
    initialized = rhs.initialized;
    outputDriver = rhs.outputDriver;
    capBitline = rhs.capBitline;
    capEnergy = rhs.capEnergy;
    resBitline = rhs.resBitline;
    capLoadInv = rhs.capLoadInv;
    capOutputBitlinePrecharger = rhs.capOutputBitlinePrecharger;
    capWireLoadPerColumn = rhs.capWireLoadPerColumn;
    resWireLoadPerColumn = rhs.resWireLoadPerColumn;
    enableLatency = rhs.enableLatency;
    prechargeLatency = rhs.prechargeLatency;
    dischargeLatency = rhs.dischargeLatency;
    numColumn = rhs.numColumn;
    widthPMOSBitlinePrecharger = rhs.widthPMOSBitlinePrecharger;
    widthPMOSBitlineEqual = rhs.widthPMOSBitlineEqual;
    capLoadPerColumn = rhs.capLoadPerColumn;
    rampInput = rhs.rampInput;
    rampOutput = rhs.rampOutput;

    tau_precharge = rhs.tau_precharge;
    tau_discharge = rhs.tau_discharge;

    return *this;
}
