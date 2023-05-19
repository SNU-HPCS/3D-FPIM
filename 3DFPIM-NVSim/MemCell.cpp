/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "MemCell.h"
#include "formula.h"
#include "global.h"
#include "macros.h"
#include <math.h>

MemCell::MemCell() {
    // TODO Auto-generated constructor stub
    minSenseVoltage     = 0.08;
    processNode         = 0;
}

MemCell::~MemCell() {
    // TODO Auto-generated destructor stub
}

void MemCell::ReadDimensionFromFile(const string & inputFile)
{
    FILE *fp = fopen(inputFile.c_str(), "r");
    char line[5000];

    if (!fp) {
        cout << inputFile << " cannot be found!\n";
        exit(-1);
    }

    while (fscanf(fp, "%[^\n]\n", line) != EOF) {
        if (!strncmp("-CellLength", line, strlen("-CellLength"))) {
            sscanf(line, "-CellLength (m): %lf", &lengthInSize);
            continue;
        }
        if (!strncmp("-CellWidth", line, strlen("-CellWidth"))) {
            sscanf(line, "-CellWidth (m): %lf", &widthInSize);
            continue;
        }
        if (!strncmp("-CellHeight", line, strlen("-CellHeight"))) {
            sscanf(line, "-CellHeight (m): %lf", &heightInSize);
            continue;
        }
        if (!strncmp("-NumStairs", line, strlen("-NumStairs"))) {
            sscanf(line, "-NumStairs: %d", &numStairs);
            continue;
        }
        if (!strncmp("-StairLength", line, strlen("-StairLength"))) {
            sscanf(line, "-StairLength: %lf", &stairLength);
            continue;
        }
        if (!strncmp("-TrenchWidth", line, strlen("-TrenchWidth"))) {
            sscanf(line, "-TrenchWidth: %lf", &trenchWidth);
            continue;
        }
    }

    area = widthInSize * lengthInSize;

    fclose(fp);
}

void MemCell::ReadCellFromFile(const string & inputFile)
{
    FILE *fp = fopen(inputFile.c_str(), "r");
    char line[5000];

    if (!fp) {
        cout << inputFile << " cannot be found!\n";
        exit(-1);
    }

    while (fscanf(fp, "%[^\n]\n", line) != EOF) {
        if (!strncmp("-ProcessNode", line, strlen("-ProcessNode"))) {
            sscanf(line, "-ProcessNode: %d", &processNode);
            continue;
        }

        if (!strncmp("-MinSenseVoltage", line, strlen("-MinSenseVoltage"))) {
            sscanf(line, "-MinSenseVoltage (mV): %lf", &minSenseVoltage);
            minSenseVoltage /= 1e3;
            continue;
        }

        if (!strncmp("-FlashPassVoltage (V)", line, strlen("-FlashPassVoltage (V)"))) {
            sscanf(line, "-FlashPassVoltage (V): %lf", &flashPassVoltage);
            continue;
        }

        if (!strncmp("-FlashReadVoltage (V)", line, strlen("-FlashReadVoltage (V)"))) {
            sscanf(line, "-FlashReadVoltage (V): %lf", &flashReadVoltage);
            continue;
        }

        if (!strncmp("-PrechargeVoltage", line, strlen("-PrechargeVoltage"))) {
            sscanf(line, "-PrechargeVoltage: %lf", &prechargeVoltage);
            continue;
        }

        if (!strncmp("-LoadVoltage", line, strlen("-LoadVoltage"))) {
            sscanf(line, "-LoadVoltage: %lf", &loadVoltage);
            continue;
        }

        if (!strncmp("-MaxCellReadCurrent", line, strlen("-MaxCellReadCurrent"))) {
            sscanf(line, "-MaxCellReadCurrent: %lf", &maxCellReadCurrent);
            continue;
        }

        if (!strncmp("-InputDuration", line, strlen("-InputDuration"))) {
            sscanf(line, "-InputDuration: %lf", &inputDuration);
            continue;
        }
    }

    fclose(fp);
}


void MemCell::PrintCell()
{

    cout << "Cell Area (m^2)    : " << area << " (" << lengthInSize << "Fx" << widthInSize << "F)" << endl;
    cout << "Cell Height (m)    : " << heightInSize << endl;

    cout << "Read Voltage       : " << flashReadVoltage << "V" << endl;
    cout << "Pass Voltage       : " << flashPassVoltage << "V" << endl;
}
