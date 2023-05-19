/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef MEMCELL_H_
#define MEMCELL_H_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "typedef.h"

using namespace std;

class MemCell {
public:
    MemCell();
    virtual ~MemCell();

    /* Functions */
    void ReadCellFromFile(const std::string & inputFile);
    void ReadDimensionFromFile(const std::string & inputFile);
    void PrintCell();

    /* Properties */
    int processNode;        /* Cell original process technology node, Unit: nm*/
    double area;            /* Cell area, Unit: F^2 */
    double aspectRatio;     /* Cell aspect ratio, H/W */
    double widthInSize; /* Cell width, Unit: F */
    double lengthInSize;    /* Cell height, Unit: F */
    double heightInSize;

    /* For NAND flash */
    double flashEraseVoltage;       /* The erase voltage, Unit: V, highest W/E voltage in ITRS sheet */
    double flashPassVoltage;        /* The voltage applied on the unselected wordline within the same block during programming, Unit: V */
    double flashReadVoltage;        /* The voltage applied on the unselected wordline within the same block during programming, Unit: V */

    double prechargeVoltage;
    double loadVoltage;
    double minSenseVoltage;
    double maxCellReadCurrent;
    double inputDuration;

    int numStairs;
    double stairLength;
    double trenchWidth;
};

#endif /* MEMCELL_H_ */
