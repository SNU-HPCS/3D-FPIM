import importlib
import sys
import math
import numpy as np
from configparser import ConfigParser
from pathlib import Path

if __name__ == "__main__":
    assert(len(sys.argv) > 1 and "Require Config Argument")
    parser = ConfigParser()
    parser.read(sys.argv[1])
    ####################################################################

    
    TEMP = eval(parser['temperature']['TEMP'])

    STAIR_LENGTH = eval(parser['staircase']['STAIR_LENGTH'])
    NUM_STAIRS = eval(parser['staircase']['NUM_STAIRS'])
    TRENCH_WIDTH = eval(parser['staircase']['TRENCH_WIDTH'])

    Barrier_width = eval(parser['wordline']['Barrier_width'])
    L_g = eval(parser['wordline']['L_g'])
    L_g_tr = eval(parser['wordline']['L_g_tr'])
    L_spacer = eval(parser['wordline']['L_spacer'])

    r_f = eval(parser['channel']['r_f'])
    t_si = eval(parser['channel']['t_si'])
    r = eval(parser['channel']['r'])

    t_1 = eval(parser['channel']['t_1'])
    t_2 = eval(parser['channel']['t_2'])
    t_3 = eval(parser['channel']['t_3'])
    t_4 = eval(parser['channel']['t_4'])
    t_ox = eval(parser['channel']['t_ox'])

    W_f = eval(parser['channel']['W_f'])
    H_f = eval(parser['channel']['H_f'])

    W_g = eval(parser['channel']['W_g'])
    H_g = eval(parser['channel']['H_g'])

    #########################################

    permittivity = 8.85e-12
    # epsilon of SiO2
    # epsilon of Si3N4
    # epsilon of Si
    # epsilon of SiN
    # epsilon of ALO
    epsilon_sio = eval(parser['material']['epsilon_sio'])
    epsilon_si = eval(parser['material']['epsilon_si'])
    epsilon_sion = eval(parser['material']['epsilon_sion'])
    epsilon_sin = eval(parser['material']['epsilon_sin'])
    epsilon_alo = eval(parser['material']['epsilon_alo'])

    # Wordline width / height
    # assume honeycomb structure
    Tungsten_Resistivity = eval(parser['material']['Tungsten_Resistivity'])
    Polysi_Resistivity = eval(parser['material']['Polysi_Resistivity'])

    HfWf = math.sqrt(pow(W_f, 2) + pow(H_f, 2))
    eta = epsilon_sio * math.sqrt(2. * math.pi * r * (HfWf - r - t_ox) / (4. * W_f * H_f - math.pi * pow(r + t_ox, 2)))

    C_of_gsd1 = 0.
    C_of_gsd2 = epsilon_sio * (4. * W_f * H_f - math.pi * pow(r + t_ox, 2)) / L_spacer

    nu = epsilon_sio * math.sqrt(2 * math.pi * r * (HfWf - r - t_ox) / (4 * H_f * W_f - math.pi * pow((r + t_ox), 2)))
    C_of_gex1 = 8 / math.pi * nu * ((H_f-r-t_ox) * (2*W_f/H_f + 1 - H_f/HfWf) + (W_f-r-t_ox) * (2*H_f/W_f+1-W_f/HfWf))


    C_of_gex2 = 4 * epsilon_sio * (L_spacer - t_ox + r * math.log(L_spacer / t_ox)) * math.sqrt(2 * r / (L_spacer + 2 * r + t_ox))
    C_of_gsd = C_of_gsd1 if L_spacer >= HfWf else C_of_gsd2
    C_of_gex = C_of_gex1 if L_spacer >= HfWf else C_of_gex2

    # per length F
    C_of_gsd_plain = epsilon_sio * 2 * H_f / L_spacer

    # intrinsic fringe cap
    C_if = 4. * epsilon_si * (r + t_ox) * np.log((2*t_ox + r) / (2*t_ox))

    C_ov = 0

    C_gc1 = 2. * math.pi * epsilon_sion * (L_g) / np.log((r + t_1) / r)
    C_gc2 = 2. * math.pi * epsilon_sin * (L_g) / np.log((r + t_1 + t_2) / (r + t_1))
    C_gc3 = 2. * math.pi * epsilon_sio * (L_g) / np.log((r + t_1 + t_2 + t_3) / (r + t_1 + t_2))
    C_gc4 = 2. * math.pi * epsilon_alo * (L_g) / np.log((r + t_1 + t_2 + t_3 + t_4) / (r + t_1 + t_2 + t_3))
    C_gc_fl = 1. / (1. / C_gc1 + 1. / C_gc2 + 1. / C_gc3 + 1. / C_gc4)

    C_gc1 = 2. * math.pi * epsilon_sion * (L_g) / np.log((r + t_1) / r)
    C_gc2 = 2. * math.pi * epsilon_sin * (L_g) / np.log((r + t_1 + t_2) / (r + t_1))
    C_gc3 = 2. * math.pi * epsilon_sio * (L_g) / np.log((r + t_1 + t_2 + t_3) / (r + t_1 + t_2))
    C_gc4 = 2. * math.pi * epsilon_alo * (L_g) / np.log((r + t_1 + t_2 + t_3 + t_4) / (r + t_1 + t_2 + t_3))
    C_gc_tr = 1. / (1. / C_gc1 + 1. / C_gc2 + 1. / C_gc3 + 1. / C_gc4)


    ###################################################################################

    L_g_res = L_g
    wl_unit_cell_resistance = Tungsten_Resistivity / (L_g_res * 2 * H_f) * W_g * 2 + \
                      Tungsten_Resistivity / L_g_res * 3.0 * 2

    wl_staircase_resistance = Tungsten_Resistivity / (L_g_res * 2 * H_f)

    bsl_unit_cell_resistance = wl_unit_cell_resistance 

    contact_unit_cell_resistance = Tungsten_Resistivity / ((r * r) * math.pi)
    string_unit_cell_resistance = Polysi_Resistivity / ((r * r - r_f * r_f) * math.pi)

    cap_per_cell_capacitance = epsilon_sio * math.pi / math.log(W_f * 2 / (r_f + t_si)) * L_spacer

    ################### TO CFG FILE ###################
    Path("./config").mkdir(parents=True, exist_ok=True)

    f = open("./config/stringWire.cfg", "w")
    f.write("-resWirePerUnit: {}\n".format(string_unit_cell_resistance))
    f.write("-capWirePerUnit: {}\n".format(cap_per_cell_capacitance))
    f.close()

    f = open("./config/contactWire.cfg", "w")
    f.write("-resWirePerUnit: {}\n".format(contact_unit_cell_resistance))
    f.close()

    f = open("./config/wordlineWire.cfg", "w")
    f.write("-resWirePerCell: {}\n".format(wl_unit_cell_resistance))
    f.write("-resWirePerStaircase: {}\n".format(wl_staircase_resistance))
    f.close()

    f = open("./config/selectlineWire.cfg", "w")
    f.write("-resWirePerCell: {}\n".format(bsl_unit_cell_resistance))
    f.close()

    f = open("./config/parasitic.cfg", "w")
    f.write("-flCapGc: {}\n".format(C_gc_fl))
    f.write("-flCapOfGex: {}\n".format(C_of_gex))
    f.write("-flCapOfGsd: {}\n".format(C_of_gsd))


    f.write("\n-trCapGc: {}\n".format(C_gc_tr))
    f.write("-trCapOfGex: {}\n".format(C_of_gex1))
    # in two sides
    f.write("-trCapIf: {}\n".format(C_if))
    f.write("-trCapOfGsd: {}\n".format(C_of_gsd))

    f.write("\n-plCapOfGsd: {}\n".format(C_of_gsd_plain))
    f.close()

    #####
    f = open("./config/cellDimension.cfg", "w")
    f.write("-CellLength (m): {}\n".format(H_f * 2))
    f.write("-CellWidth (m): {}\n".format(W_f * 2))
    f.write("-CellHeight (m): {}\n".format(L_spacer + L_g + Barrier_width * 2))
    f.write("-NumStairs: {}\n".format(NUM_STAIRS))
    f.write("-StairLength: {}\n".format(STAIR_LENGTH))
    f.write("-TrenchWidth: {}\n".format(TRENCH_WIDTH))
    f.write("-Temperature (K): {}\n".format(TEMP))
    f.close()
