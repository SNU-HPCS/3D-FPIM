/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef TYPEDEF_H_
#define TYPEDEF_H_

enum DeviceRoadmap
{
    HP,     /* High performance */
    LSTP,   /* Low standby power */
    LOP     /* Low operating power */
};

enum WireType
{
    local_aggressive = 0,   /* Width = 2.5F */
    local_conservative = 1,
    semi_aggressive = 2,    /* Width = 4F */
    semi_conservative = 3,
    global_aggressive = 4,  /* Width = 8F */
    global_conservative = 5,
    dram_wordline = 6,      /* CACTI 6.5 has this one, but we don't plan to support it at this moment */
    selectline_wire = 7,
    wordline_wire = 8,
    contact_wire = 9,
    string_wire = 10,
    bitline_wire = 11
};

enum WireRepeaterType
{
    repeated_none = 0,      /* No repeater */
    repeated_opt = 1,       /* Add Repeater, optimal delay */
    repeated_5 = 2,         /* Add Repeater, 5% delay overhead */
    repeated_10 = 3,        /* Add Repeater, 10% delay overhead */
    repeated_20 = 4,        /* Add Repeater, 20% delay overhead */
    repeated_30 = 5,        /* Add Repeater, 30% delay overhead */
    repeated_40 = 6,        /* Add Repeater, 40% delay overhead */
    repeated_50 = 7         /* Add Repeater, 50% delay overhead */
};

enum BufferDesignTarget
{
    latency_first = 0,              /* The buffer will be optimized for latency */
    latency_area_trade_off = 1,     /* the buffer will be fixed to 2-stage */
    area_first = 2                  /* The buffer will be optimized for area */
};

enum OptimizationTarget
{
    read_latency_optimized = 0,
    read_energy_optimized = 1,
    read_edp_optimized = 2,
    leakage_optimized = 3,
    area_optimized = 4,
    full_exploration = 5
};

#endif /* TYPEDEF_H_ */
