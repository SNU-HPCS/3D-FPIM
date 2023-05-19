/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "OutputDriver.h"
#include "global.h"
#include "formula.h"
#include <math.h>

OutputDriver::OutputDriver() : FunctionUnit(){
    initialized = false;
    invalid = false;
}

OutputDriver::~OutputDriver() {
    // TODO Auto-generated destructor stub
}

void OutputDriver::Initialize(double _logicEffort,
        double _inputCap, double _outputCap, double _outputRes, double _outputTau,
        bool _inv, BufferDesignTarget _areaOptimizationLevel,
        double _minDriverCurrent, bool _normal, double _outputCapDischarge, double _outputTauDis) {
    if (initialized)
        cout << "[Output Driver] Warning: Already initialized!" << endl;

    normal = _normal;

    logicEffort = _logicEffort;
    inputCap = _inputCap;
    outputCap = _outputCap;
    outputCapDischarge = _outputCapDischarge;
    outputTau = _outputTau;
    outputTauDis = _outputTauDis;
    dischargeLatency = 0;
    inv = _inv;
    areaOptimizationLevel = _areaOptimizationLevel;
    minDriverCurrent = _minDriverCurrent;

    vdd = normal ? tech->vdd : cell->flashPassVoltage;

    double minNMOSDriverWidth = minDriverCurrent / tech->currentOnNmos[cell->temperature - 300];
    minNMOSDriverWidth = MAX(MIN_NMOS_SIZE * tech->featureSize, minNMOSDriverWidth);

    if (minNMOSDriverWidth > inputParameter->maxNmosSize * tech->featureSize) {
        invalid = true;
        return;
    }

    int optimalNumStage;

    if (areaOptimizationLevel == latency_first) {
        double F = MAX(1, logicEffort * outputCap / inputCap);  /* Total logic effort */
        optimalNumStage = MAX(0, (int)(log(F) / log(OPT_F) + 0.5) - 1);

        if ((optimalNumStage % 2) ^ inv)    /* If odd, add 1 */
            optimalNumStage += 1;

        if (optimalNumStage > MAX_INV_CHAIN_LEN) {/* Exceed maximum stages */
            if (WARNING)
                cout << "[WARNING] Exceed maximum inverter chain length!" << endl;
            optimalNumStage = MAX_INV_CHAIN_LEN;
        }

        numStage = optimalNumStage;

        double f = pow(F, 1.0 / (optimalNumStage + 1)); /* Logic effort per stage */
        double inputCapLast = outputCap / f;

        widthNMOS[optimalNumStage-1] = MAX(MIN_NMOS_SIZE * tech->featureSize,
                inputCapLast / CalculateGateCap(1/*meter*/, *tech) / (1.0 + tech->pnSizeRatio));

        if (widthNMOS[optimalNumStage-1] > inputParameter->maxNmosSize * tech->featureSize) {
            if (WARNING)
                cout << "[WARNING] Exceed maximum NMOS size!" << endl;
            widthNMOS[optimalNumStage-1] = inputParameter->maxNmosSize * tech->featureSize;
            /* re-Calculate the logic effort */
            double capLastStage = CalculateGateCap((1 + tech->pnSizeRatio) * inputParameter->maxNmosSize * tech->featureSize, *tech);
            F = logicEffort * capLastStage / inputCap;
            f = pow(F, 1.0 / (optimalNumStage));
        }

        if (widthNMOS[optimalNumStage-1] < minNMOSDriverWidth) {
            /* the last level Inv can not provide minimum current so that the Inv chain can't only decided by Logic Effort */
            areaOptimizationLevel = latency_area_trade_off;
        } else {
            widthPMOS[optimalNumStage-1] = widthNMOS[optimalNumStage-1] * tech->pnSizeRatio;

            for (int i = optimalNumStage-2; i >= 0; i--) {
                widthNMOS[i] = widthNMOS[i+1] / f;
                if (widthNMOS[i] < MIN_NMOS_SIZE * tech->featureSize) {
                    if (WARNING)
                        cout << "[WARNING] Exceed minimum NMOS size!" << endl;
                    widthNMOS[i] = MIN_NMOS_SIZE * tech->featureSize;
                }
                widthPMOS[i] = widthNMOS[i] * tech->pnSizeRatio;
            }
        }
    }

    if (areaOptimizationLevel == latency_area_trade_off){
        double newOutputCap = CalculateGateCap(minNMOSDriverWidth, *tech) * (1.0 + tech->pnSizeRatio);
        double F = MAX(1, logicEffort * newOutputCap / inputCap);   /* Total logic effort */
        optimalNumStage = MAX(0, (int)(log(F) / log(OPT_F) + 0.5) - 1);

        if (!((optimalNumStage % 2) ^ inv)) /* If even, add 1 */
            optimalNumStage += 1;

        if (optimalNumStage > MAX_INV_CHAIN_LEN) {/* Exceed maximum stages */
            if (WARNING)
                cout << "[WARNING] Exceed maximum inverter chain length!" << endl;
            optimalNumStage = MAX_INV_CHAIN_LEN;
        }

        numStage = optimalNumStage + 1;

        widthNMOS[optimalNumStage] = minNMOSDriverWidth;
        widthPMOS[optimalNumStage] = widthNMOS[optimalNumStage] * tech->pnSizeRatio;

        double f = pow(F, 1.0 / (optimalNumStage + 1)); /* Logic effort per stage */

        for (int i = optimalNumStage - 1; i >= 0; i--) {
            widthNMOS[i] = widthNMOS[i+1] / f;
            if (widthNMOS[i] < MIN_NMOS_SIZE * tech->featureSize) {
                if (WARNING)
                    cout << "[WARNING] Exceed minimum NMOS size!" << endl;
                widthNMOS[i] = MIN_NMOS_SIZE * tech->featureSize;
            }
            widthPMOS[i] = widthNMOS[i] * tech->pnSizeRatio;
        }
    } else if (areaOptimizationLevel == area_first) {
        optimalNumStage = 1;
        numStage = 1;
        widthNMOS[optimalNumStage - 1] = MAX(MIN_NMOS_SIZE * tech->featureSize, minNMOSDriverWidth);
        if (widthNMOS[optimalNumStage - 1] > AREA_OPT_CONSTRAIN * inputParameter->maxNmosSize * tech->featureSize) {
            invalid = true;
            return;
        }
        widthPMOS[optimalNumStage - 1] = widthNMOS[optimalNumStage - 1] * tech->pnSizeRatio;
    }

    /* Restore the original buffer design style */
    areaOptimizationLevel = _areaOptimizationLevel;

    initialized = true;
}

void OutputDriver::CalculateArea() {
    if (!initialized) {
        cout << "[Output Driver] Error: Require initialization first!" << endl;
    } else if (invalid) {
        height = width = area = 1e41;
    } else {
        double totalHeight = 0;
        double totalWidth = 0;
        double h, w;
        for (int i = 0; i < numStage; i++) {
            CalculateGateArea(INV, 1, widthNMOS[i], widthPMOS[i], tech->featureSize*40, *tech, &h, &w);
            totalHeight = MAX(totalHeight, h);
            totalWidth += w;
        }
        height = totalHeight;
        width = totalWidth;
        // scale the area by HVT_SCALE assuming that the decoder requires HV transistor
        width = normal ? width : width * HVT_SCALE;
        area = height * width;
    }
}

void OutputDriver::CalculateRC() {
    if (!initialized) {
        cout << "[Output Driver] Error: Require initialization first!" << endl;
    } else if (invalid) {
        ;  // nothing to do if invalid
    } else if (numStage == 0) {
        capInput[0] = 0;
    } else {
        for (int i = 0; i < numStage; i++) {
            CalculateGateCapacitance(INV, 1, widthNMOS[i], widthPMOS[i], tech->featureSize * MAX_TRANSISTOR_HEIGHT, *tech, &(capInput[i]), &(capOutput[i]));
        }
    }
}

void OutputDriver::CalculateLatency(double _rampInput) {
    if (!initialized) {
        cout << "[Output Driver] Error: Require initialization first!" << endl;
    } else if (invalid) {
        readLatency = 1e41;
    } else {
        rampInput = _rampInput;
        double resPullDown;
        double capLoad;
        double tr;  /* time constant */
        double tr_dis;  /* time constant */
        double gm;  /* transconductance */
        double beta;    /* for horowitz calculation */
        double temp;
        readLatency = 0;
        for (int i = 0; i < numStage - 1; i++) {
            resPullDown = CalculateOnResistance(widthNMOS[i], NMOS, cell->temperature, *tech);
            capLoad = capOutput[i] + capInput[i+1];
            tr = resPullDown * capLoad;
            gm = CalculateTransconductance(widthNMOS[i], NMOS, *tech);
            beta = 1 / (resPullDown * gm);
            readLatency += horowitz(tr, beta, rampInput, &temp);
            rampInput = temp;   /* for next stage */
        }
        /* Last level inverter */
        resPullDown = CalculateOnResistance(widthNMOS[numStage-1], NMOS, cell->temperature, *tech);
        tr = resPullDown * (capOutput[numStage-1] + outputCap) + outputTau;
        tr_dis = resPullDown * (capOutput[numStage-1] + outputCapDischarge) + outputTauDis;
        // outputLatencyCap * outputRes / 2;
        gm = CalculateTransconductance(widthNMOS[numStage-1], NMOS, *tech);
        beta = 1 / (resPullDown * gm);
        dischargeLatency = readLatency + horowitz(tr_dis, beta, rampInput, &rampOutput);
        readLatency += horowitz(tr, beta, rampInput, &rampOutput);
        rampInput = _rampInput;
    }
}

void OutputDriver::CalculatePower() {
    if (!initialized) {
        cout << "[Output Driver] Error: Require initialization first!" << endl;
    } else if (invalid) {
        readDynamicEnergy = leakage = 1e41;
    } else {
        /* Leakage power */
        leakage = 0;
        for (int i = 0; i < numStage; i++) {
            leakage += CalculateGateLeakage(INV, 1, widthNMOS[i], widthPMOS[i], cell->temperature, *tech)
                    * vdd;
        }
        /* Dynamic energy */
        readDynamicEnergy = 0;
        double capLoad;
        for (int i = 0; i < numStage - 1; i++) {
            capLoad = capOutput[i] + capInput[i+1];
            readDynamicEnergy += capLoad * vdd * vdd;
        }
        /* outputCap here means the final load capacitance */
        capLoad = capOutput[numStage-1] + outputCap;
        readDynamicEnergy += capLoad * vdd * vdd;
    }
}

void OutputDriver::PrintProperty() {
    cout << "Output Driver Properties:" << endl;
    FunctionUnit::PrintProperty();
    cout << "Number of inverter stage: " << numStage << endl;
}

OutputDriver & OutputDriver::operator=(const OutputDriver &rhs) {
    FunctionUnit::operator=(rhs);
    initialized = rhs.initialized;
    invalid = rhs.invalid;
    logicEffort = rhs.logicEffort;
    inputCap = rhs.inputCap;
    outputCap = rhs.outputCap;
    outputCapDischarge = rhs.outputCapDischarge;
    outputTau = rhs.outputTau;
    inv = rhs.inv;
    numStage = rhs.numStage;
    areaOptimizationLevel = rhs.areaOptimizationLevel;
    minDriverCurrent = rhs.minDriverCurrent;

    for(int i = 0; i < MAX_INV_CHAIN_LEN; i++){
        widthNMOS[i] = rhs.widthNMOS[i];
        widthPMOS[i] = rhs.widthPMOS[i];
        capInput[i] = rhs.capInput[i];
        capOutput[i] = rhs.capOutput[i];
    }

    rampInput = rhs.rampInput;
    rampOutput = rhs.rampOutput;

    outputTau = rhs.outputTau;
    outputTauDis = rhs.outputTauDis;
    dischargeLatency = rhs.dischargeLatency;
    normal = rhs.normal;

    return *this;
}
