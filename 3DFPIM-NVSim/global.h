/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#include "InputParameter.h"
#include "Technology.h"
#include "MemCell.h"
#include "Wire.h"

extern InputParameter *inputParameter;
extern Technology *tech;
extern MemCell *cell;
extern Wire *localWire;     /* The wire type of local interconnects (for example, wire in mat) */
extern Wire *globalWire;    /* The wire type of global interconnects (for example, the ones that connect mats) */
extern Wire *selectlineWire;  /* The wire type of string interconnects (for example, the ones that are vertical string of the cube) */
extern Wire *wordlineWire;  /* The wire type of wordline interconnects (for example, the ones that are vertical string of the cube) */
extern Wire *contactWire;  /* The wire type of contact interconnects (for example, the ones that are vertical string of the cube) */
extern Wire *stringWire;  /* The wire type of string interconnects (for example, the ones that are vertical string of the cube) */
extern Wire *bitlineWire;
