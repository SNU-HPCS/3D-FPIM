/*******************************************************************************
* Copyright (c) 2022, Seoul National University. See LICENSE file in the top-
* level directory. 3D-FPIM Project can be copied according to the terms
* contained in the LICENSE file.
*******************************************************************************/


#ifndef CAP_H_
#define CAP_H_

#include "FunctionUnit.h"

class Cap: public FunctionUnit {
public:
    Cap();
    virtual ~Cap();

    /* Functions */
    void PrintProperty();
    void Initialize(int _numCap, double _capacitance, double _arrWidth);
    void CalculateArea();
    void CalculatePower();
    Cap & operator=(const Cap &);

    // related to the validity
    bool initialized;   /* Initialization flag */
    bool invalid;       /* Indicate that the current configuration is not valid */

    //
    int numCap;
    double capacitance; /* Unit: F */
    // arrWidth equals to the width of the memory array
    double arrWidth;
};



#endif /* CAP_H_ */
