## [Overview]

This repository includes the circuit-level simulator for 3D-FPIM: An Extreme Energy-Efficient DNN Acceleration System Using 3D NAND Flash-Based In-Situ PIM Unit[^1].
We modified NVSim[^2] and implemented 3D NAND Flash-specific parameters to obtain the 3D NAND Flash read latency.
Also, we configure the simulator to enable a matrix-vectory-multiply operation and obtain the corresponding latency and energy consumption.
Note that we configure the simulator to evaluate the latency for a "single subarray".



## [Compile]
```sh 
$ make
```
## [Setup]

The nand flash simulator requires setting three different files to configure the nand flash structure.

### 1. Cell file (ex. 3D\_NAND.cell)

#### [Used in both PIM \& non-PIM mode]
- FlashPassVoltage (V): pass voltage of the target nand flash
- FlashReadVoltage (V): read voltage of the target nand flash
- PrechargeVoltage (V): bitline precharge voltage of the target nand flash

#### [Used in both PIM mode only]
- LoadVoltage (V): load capacitor precharge voltage (set to 1.8 V / 2)
- Min/MaxCellReadCurrent): the minimum and maximum current of the weight value
- InputDuration: the duration of the unit input value (set to 1ns in 3D-FPIM)

### 2. Cfg file (ex. 3D\_NAND\_128G.cfg)

#### [Used in PIM \& non-PIM mode]
- ProcessNode: technology of the row decoder and precharger (22nm)
- PIMMode: set to 1 for PIM mode
- FlashNumStack: the number of memory stacks
- MuxSenseAmp: the number of bitlines that share a single sense amplifier
- DeviceRoadmap: HP / LOP / LSTP
- MemoryCellInputFile: target cell file
- Temperature (K): temperature in Kelvin

#### [Used in PIM mode only]
- Baseline: set to 1 for NR and set to 0 for SR
- InputPrecision: bit precision of an input
- capLoad: capacitance of the load capacitor

#### [Used in non-PIM mode only]
- ReferenceReadLatency: reference read latency (for validation)
- BitsPerCell: bit precision fo each cell

### 3. Calculate file (ex. tech\_params/example.cfg)
- TEMP: target temperature in Kelvin
- STAIR\_LENGTH: the 2D length of each staircase
- NUM\_STAIRS: this value is not the same as the number of stacks (multiple stacks can be connected to a single staircase)
- Barrier\_width: barrier thickness of the wordline
- L\_g: thickness of a wordline
- L\_spacer: spacing between wordlines
- r\_f: radius of the macaroni filler
- r\_si: radius of the silicon channel
- t\_1 ~ t\_4: radius of the O/N/O doping (there are four doping layers in Samsung 64 Layer)
- W\_g: vertical pitch between the strings (refer to [50])
- H\_g: horizontal pitch between the strings  (refer to [50])
- TRENCH\_WIDTH: distance between the source lines

- epsilon\_\*: electric permittivity parameters
- \*\_Resistivity: resistivity of the tungsten (wordline) and polysilicon channel

Set the above parameters and run
```sh 
$ python Calculate.py example.cfg
```

## [Run]
```sh 
$ ./3DNAND_SIM target.cfg
```

## [Misc]

We utilized HSPICE to extract performance, latency, and area parameters for an ADC and an switched integrator.
Refer to the 3D-FPIM paper (Table II).


## Citation

[^1]: H. Lee et al., **3D-FPIM: An Extreme Energy-Efficient DNN Acceleration System Using 3D NAND Flash-Based In-Situ PIM Unit,** *2022 55th IEEE/ACM International Symposium on Microarchitecture (MICRO)*, 2022.
[^2]: https://github.com/SEAL-UCSB/NVSim
