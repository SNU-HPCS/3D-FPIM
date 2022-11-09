/*******************************************************************************
* Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
* level directory. This file contains code from NVSim, (c) 2012-2013, 
* Pennsylvania State University and Hewlett-Packard Company. See LICENSE_NVSim 
* file in the current directory. 3D-FPIM Project can be copied according to the 
* terms contained in the LICENSE file.
*******************************************************************************/


#ifndef CONSTANT_H_
#define CONSTANT_H_

// ns scale
#define CLOCK_FREQ 1.0e9
#define CLOCK_PERIOD 1 / CLOCK_FREQ
#define DTC_BATCH 32

#define HVT_SCALE 8

#define WARNING 0

#define INV     0
#define NOR     1
#define NAND    2

#define NMOS    0
#define PMOS    1

#define MAX_INV_CHAIN_LEN   20
#define OPT_F               4

#define MAX_NMOS_SIZE   500
#define MIN_NMOS_SIZE   1.5

#define AREA_OPT_CONSTRAIN 1

#define MAX_TRANSISTOR_HEIGHT 20

#define MIN_GAP_BET_P_AND_N_DIFFS   2
#define MIN_GAP_BET_SAME_TYPE_DIFFS 1.5
#define MIN_GAP_BET_POLY            1.5
#define MIN_GAP_BET_CONTACT_POLY    0.75
#define CONTACT_SIZE                1
#define MIN_WIDTH_POWER_RAIL        2

#define AVG_RATIO_LEAK_2INPUT_NAND 0.48
#define AVG_RATIO_LEAK_3INPUT_NAND 0.31
#define AVG_RATIO_LEAK_2INPUT_NOR  0.95
#define AVG_RATIO_LEAK_3INPUT_NOR  0.62

#define W_SENSE_P       7.5
#define W_SENSE_N       3.75
#define W_SENSE_ISO     12.5
#define W_SENSE_EN      5.0
#define W_SENSE_MUX     9.0
#define VBITSENSEMIN    0.08
#define IV_CONVERTER_AREA 50000 /*TO-DO: technology and design dependent parameter, 649606 is used in PCRAMSim*/

#define STITCHING_OVERHEAD  7.5

#define DRAM_REFRESH_PERIOD 64e-3

#define SHAPER_EFFICIENCY_CONSERVATIVE  0.2
#define SHAPER_EFFICIENCY_AGGRESSIVE    1.0

#define COPPER_RESISTIVITY  2.2e-8          /* Unit: ohm*m at 20 celcius degree */
#define COPPER_RESISTIVITY_TEMPERATURE_COEFFICIENT  0.00393
#define TUNGSTEN_RESISTIVITY    5e-7
#define POLYSI_RESISTIVITY  3.33e-4
#define PERMITTIVITY        8.85e-12        /* Unit: F/m */
#define SIO2EPS             4.2
#define CAP_THICKNESS       3e-9

#define TUNNEL_CURRENT_FLOW 10              /* Unit: A/m^2 */
#define DELTA_V_TH 5.0                      /* Unit: V, Threshold difference of floating gate */

#define RES_ADJ 8.6
#define VOL_SWING .1

#define CONSTRAINT_ASPECT_RATIO_BANK    10

#define BITLINE_LEAKAGE_TOLERANCE   1
#define IR_DROP_TOLERANCE           0.2

#define TOTAL_ADDRESS_BIT   48

#endif /* CONSTANT_H_ */
