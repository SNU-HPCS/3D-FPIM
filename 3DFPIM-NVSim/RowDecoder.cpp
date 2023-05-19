/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "RowDecoder.h"
#include "formula.h"
#include "global.h"
#include "macros.h"

RowDecoder::RowDecoder() : FunctionUnit(){
    // TODO Auto-generated constructor stub
    initialized = false;
    invalid = false;
}

RowDecoder::~RowDecoder() {
    // TODO Auto-generated destructor stub
}

void RowDecoder::Initialize(int _numRow, 
        double _capLoad, double _resLoad, double _tau,
        BufferDesignTarget _areaOptimizationLevel, 
        double _minDriverCurrent, bool _normal,
        double _capLoadDischarge, double _tauDischarge) {

    if (initialized)
        cout << "[Row Decoder] Warning: Already initialized!" << endl;

    numRow = _numRow;
    capLoad = _capLoad;
    capLoadDischarge = _capLoadDischarge;
    resLoad = _resLoad;
    tau = _tau;
    tauDischarge = _tauDischarge;
    
    areaOptimizationLevel = _areaOptimizationLevel;

    minDriverCurrent = _minDriverCurrent;

    normal = _normal;
    vdd = normal ? tech->vdd : cell->flashPassVoltage;

    // the selection depends on the numSelect
    // all decoding should have been handled in the predecoder
    //  if the numSelect is less than 8
    if (numRow <= 8) {  /* The predecoder output is used directly */
        numNandInput = 0;   /* no circuit needed */
    } else {
        numNandInput = 2;   /* just NAND two predecoder outputs */
    }

    if (numNandInput > 0) {
        double logicEffortNand;
        double capNand;
        if (numNandInput == 2) {    /* NAND2 */
            widthNandN = 2 * MIN_NMOS_SIZE * tech->featureSize;
            logicEffortNand = (2+tech->pnSizeRatio) / (1+tech->pnSizeRatio);
        } else {                    /* NAND3 */
            widthNandN = 3 * MIN_NMOS_SIZE * tech->featureSize;
            logicEffortNand = (3+tech->pnSizeRatio) / (1+tech->pnSizeRatio);
        }
        widthNandP = tech->pnSizeRatio * MIN_NMOS_SIZE * tech->featureSize;
        capNand = CalculateGateCap(widthNandN, *tech) + 
                CalculateGateCap(widthNandP, *tech);
        outputDriver.Initialize(logicEffortNand, capNand, 
                capLoad, resLoad, tau, true, 
                areaOptimizationLevel, minDriverCurrent, normal, capLoadDischarge, tauDischarge);
    } else {
        double capInv;
        widthNandN = MIN_NMOS_SIZE * tech->featureSize;
        widthNandP = tech->pnSizeRatio * MIN_NMOS_SIZE * tech->featureSize;
        capInv = CalculateGateCap(widthNandN, *tech) + 
                CalculateGateCap(widthNandP, *tech);
        outputDriver.Initialize(1, capInv, capLoad, resLoad, tau, true, 
                areaOptimizationLevel, minDriverCurrent, normal, capLoadDischarge, tauDischarge);
    }

    if (outputDriver.invalid) {
        invalid = true;
        return;
    }


    initialized = true;
}

void RowDecoder::CalculateArea() {
    if (!initialized) {
        cout << "[Row Decoder Area] Error: Require initialization first!" << endl;
    } else {
        outputDriver.CalculateArea();
        if (numNandInput == 0) {    
            /* no circuit needed, use predecoder outputs directly */
            height = outputDriver.height;
            width = outputDriver.width;
        } else {
            double hNand, wNand;
            CalculateGateArea(NAND, numNandInput, widthNandN, 
                    widthNandP, tech->featureSize*40, *tech, &hNand, &wNand);
            height = MAX(hNand, outputDriver.height);
            width = wNand + outputDriver.width;
        }
        width = normal ? width : width * HVT_SCALE; 
        height *= numRow;
        area = height * width;
    }
}

void RowDecoder::CalculateRC() {
    if (!initialized) {
        cout << "[Row Decoder RC] Error: Require initialization first!" << endl;
    } else {
        outputDriver.CalculateRC();
        if (numNandInput == 0) {    /* no circuit needed, use predecoder outputs directly */
            capNandInput = capNandOutput = 0;
        
        } else {
            CalculateGateCapacitance(NAND, numNandInput, widthNandN, 
                    widthNandP, tech->featureSize * MAX_TRANSISTOR_HEIGHT, 
                    *tech, &capNandInput, &capNandOutput);
        }
    }
}

void RowDecoder::CalculateLatency(double _rampInput) {
    if (!initialized) {
        cout << "[Row Decoder Latency] Error: Require initialization first!" << endl;
    } else {
        if (numNandInput == 0) {    
            /* no circuit needed, use predecoder outputs directly */
            outputDriver.CalculateLatency(_rampInput);
            readLatency = outputDriver.readLatency;
            rampOutput = outputDriver.rampOutput;
        } else {
            rampInput = _rampInput;

            double resPullDown;
            double capLoad;
            double tr;  /* time constant */
            double gm;  /* transconductance */
            double beta;    /* for horowitz calculation */
            double rampInputForDriver;

            resPullDown = CalculateOnResistance(widthNandN, NMOS, 
                    inputParameter->temperature, *tech) * numNandInput;
            capLoad = capNandOutput + outputDriver.capInput[0];
            tr = resPullDown * capLoad;
            gm = CalculateTransconductance(widthNandN, NMOS, *tech);
            beta = 1 / (resPullDown * gm);
            readLatency = horowitz(tr, beta, rampInput, &rampInputForDriver);

            outputDriver.CalculateLatency(rampInputForDriver);
            readLatency += outputDriver.readLatency;

            rampOutput = outputDriver.rampOutput;
        }
        dischargeLatency = outputDriver.dischargeLatency;
    }
}

void RowDecoder::CalculatePower() {
    if (!initialized) {
        cout << "[Row Decoder Power] Error: Require initialization first!" << endl;
    } else {
        outputDriver.CalculatePower();
        leakage = outputDriver.leakage;
        if (numNandInput == 0) {    /* no circuit needed, use predecoder outputs directly */
            readDynamicEnergy = outputDriver.readDynamicEnergy;

        } else {
            /* Leakage power */
            leakage += CalculateGateLeakage(NAND, numNandInput, widthNandN, widthNandP,
                    inputParameter->temperature, *tech) * vdd;
            /* Dynamic energy */
            double capLoad = capNandOutput + outputDriver.capInput[0];
            readDynamicEnergy = capLoad * vdd * vdd;
            readDynamicEnergy += (outputDriver.readDynamicEnergy);
            readDynamicEnergy *= 1;
        }
        leakage *= numRow;
    }
}

void RowDecoder::PrintProperty() {
    cout << "Row Decoder Properties:" << endl;
    FunctionUnit::PrintProperty();
}

RowDecoder & RowDecoder::operator=(const RowDecoder &rhs) {
    FunctionUnit::operator=(rhs);

    initialized = rhs.initialized;
    invalid = rhs.invalid;

    outputDriver = rhs.outputDriver;
    numRow = rhs.numRow;
    numNandInput = rhs.numNandInput;
    capLoad = rhs.capLoad;
    capLoadDischarge = rhs.capLoadDischarge;
    resLoad = rhs.resLoad;
    areaOptimizationLevel = rhs.areaOptimizationLevel;
    minDriverCurrent = rhs.minDriverCurrent;

    widthNandN = rhs.widthNandN;
    widthNandP = rhs.widthNandP;
    capNandInput = rhs.capNandInput;
    capNandOutput = rhs.capNandOutput;
    rampInput = rhs.rampInput;
    rampOutput = rhs.rampOutput;

    normal = rhs.normal;
    vdd = rhs.vdd;
    
    tau = rhs.tau;
    tauDischarge = rhs.tauDischarge;

    dischargeLatency = rhs.dischargeLatency;

    return *this;
}
