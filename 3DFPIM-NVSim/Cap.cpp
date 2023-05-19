/*******************************************************************************
* Copyright (c) 2022, Seoul National University. See LICENSE file in the top-
* level directory. 3D-FPIM Project can be copied according to the terms
* contained in the LICENSE file.
*******************************************************************************/


#include "Cap.h"
#include "formula.h"
#include "constant.h"
#include "global.h"
#include "macros.h"

Cap::Cap() {
    initialized = false;
    invalid = false;
}

Cap::~Cap() {
}

void Cap::Initialize(int _numCap, double _capacitance, double _arrWidth){
    numCap = _numCap;
    capacitance = _capacitance;
    arrWidth = _arrWidth;

    initialized = true;
}

// the total array of the capacitor array
void Cap::CalculateArea() {
    if (!initialized) {
        cout << "[Cap] Error: Require initialization first!" << endl;
    } else if (invalid) {
        height = width = area = 1e41;
    } else {
        area = capacitance / (PERMITTIVITY * SIO2EPS) * CAP_THICKNESS * numCap;
        width = arrWidth;
        height = area / width;
    }
}

void Cap::CalculatePower() {
    if (!initialized) {
        cout << "[Cap] Error: Require initialization first!" << endl;
    } else if (invalid) {
        readDynamicEnergy = leakage = 1e41;
    } else {
        readDynamicEnergy = leakage = 0;

        // charge energy => the energy consumed to discharge (charge) the bitline
        readDynamicEnergy += capacitance * cell->loadVoltage * cell->loadVoltage;
        readDynamicEnergy *= numCap;
    }
}

void Cap::PrintProperty() {
    cout << "Cap Properties:" << endl;
    FunctionUnit::PrintProperty();
}

Cap & Cap::operator=(const Cap &rhs) {
    FunctionUnit::operator=(rhs);

    initialized = rhs.initialized;
    invalid = rhs.invalid;

    numCap = rhs.numCap;
    capacitance = rhs.capacitance;
    arrWidth = rhs.arrWidth;

    return *this;
}
