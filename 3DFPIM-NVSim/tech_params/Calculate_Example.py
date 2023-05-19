import math
import numpy as np

######## Unit ########
# Meter / Farrat / Ohm / Kelvin
######################

# Should set all the parameters (FIXME)

# We set the temperature to
# 1) 360 in non-PIM mode (for worst case validation)
# 2) 300 in PIM mode (for typical case performance)
TEMPERATURE = 360

# We obtain all the values from [46]
# Following parameters should be modified for you own device
# STAIR_LENGTH      => the 2D length of each staircase
# NUM_STAIRS        => this value is not the same as the number of stacks (multiple stacks can be connected to a single staircase)
#                      set to 17 for Samsung 64 Layer
# Barrier_width     => barrier thickness of the wordline
# L_g               => thickness of a wordline
# L_spacer          => spacing between wordlines
# r_f               => radius of the macaroni filler
# r_si              => radius of the silicon channel
# t_1 ~ t_4         => radius of the O/N/O doping (there are four doping layers in Samsung 64 Layer)
# W_g               => vertical pitch between the strings (refer to [50])
# H_g               => horizontal pitch between the strings  (refer to [50])
# TRENCH_WIDTH      => distance between the source lines

STAIR_LENGTH = FIXME
NUM_STAIRS = FIXME
Barrier_width = FIXME

L_g = FIXME
L_spacer = FIXME - Barrier_width * 2
r_f = FIXME
t_si = FIXME

t_1 = FIXME
t_2 = FIXME
t_3 = FIXME
t_4 = FIXME

W_g = FIXME
H_g = FIXME

TRENCH_WIDTH = FIXME

######################################################################################################################

permittivity = 8.85e-12
# We obtained the parameters from [47,48]
# epsilon of SiO2
# epsilon of Si3N4
# epsilon of Si
# epsilon of SiN
# epsilon of ALO
epsilon_sio = 4.2 * permittivity
epsilon_si = 11.7 * permittivity
epsilon_sion = 5.3 * permittivity
epsilon_sin = 7.5 * permittivity
epsilon_alo = 5.7 * permittivity

Tungsten_Resistivity = 5e-7 * (1 + 0.0045 * (TEMPERATURE - 293))
Polysi_Resistivity = 1./3. * 1e-3

t_ox = t_1 + t_2 + t_3 + t_4
r = r_f + t_si

HfWf = math.sqrt(pow(W_g, 2) + pow(H_g, 2))
eta = epsilon_sio * math.sqrt(2. * math.pi * r * (HfWf - r - t_ox) / (4. * W_g * H_g - math.pi * pow(r + t_ox, 2)))

C_of_gsd1 = 0.
C_of_gsd2 = epsilon_sio * (4. * W_g * H_g - math.pi * pow(r + t_ox, 2)) / L_spacer

nu = epsilon_sio * math.sqrt(2 * math.pi * r * (HfWf - r - t_ox) / (4 * H_g * W_g - math.pi * pow((r + t_ox), 2)))
C_of_gex1 = 8 / math.pi * nu * ((H_g-r-t_ox) * (2*W_g/H_g + 1 - H_g/HfWf) + (W_g-r-t_ox) * (2*H_g/W_g+1-W_g/HfWf))


C_of_gex2 = 4 * epsilon_sio * (L_spacer - t_ox + r * math.log(L_spacer / t_ox)) * math.sqrt(2 * r / (L_spacer + 2 * r + t_ox))
C_of_gsd = C_of_gsd1 if L_spacer >= HfWf else C_of_gsd2
C_of_gex = C_of_gex1 if L_spacer >= HfWf else C_of_gex2

# per length F
C_of_gsd_plain = epsilon_sio * 2 * H_g / L_spacer

# intrinsic fringe cap
C_if = 4. * epsilon_si * (r + t_ox) * np.log((2*t_ox + r) / (2*t_ox))

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
wl_unit_cell_resistance = Tungsten_Resistivity / (L_g_res * 2 * H_g) * (W_g - (r + t_ox)) * 2 + \
                  Tungsten_Resistivity / L_g_res * 3.0 * 2

wl_staircase_resistance = Tungsten_Resistivity / (L_g_res * 2 * H_g)

bsl_unit_cell_resistance = wl_unit_cell_resistance 

contact_unit_cell_resistance = Tungsten_Resistivity / ((r * r) * math.pi)
string_unit_cell_resistance = Polysi_Resistivity / ((r * r - r_f * r_f) * math.pi)

cap_per_cell_capacitance = epsilon_sio * math.pi / math.log(W_g * 2 / (r_f + t_si)) * L_spacer

################### TO CFG FILE ###################
f = open("stringWire.cfg", "w")
f.write("-resWirePerUnit: {}\n".format(string_unit_cell_resistance))
f.write("-capWirePerUnit: {}\n".format(cap_per_cell_capacitance))
f.close()

f = open("contactWire.cfg", "w")
f.write("-resWirePerUnit: {}\n".format(contact_unit_cell_resistance))
f.close()

f = open("wordlineWire.cfg", "w")
f.write("-resWirePerCell: {}\n".format(wl_unit_cell_resistance))
f.write("-resWirePerStaircase: {}\n".format(wl_staircase_resistance))
f.close()

f = open("selectlineWire.cfg", "w")
f.write("-resWirePerCell: {}\n".format(bsl_unit_cell_resistance))
f.close()

f = open("parasitic.cfg", "w")
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
f = open("cellDimension.cfg", "w")
f.write("-CellLength (m): {}\n".format(H_g * 2))
f.write("-CellWidth (m): {}\n".format(W_g * 2))
f.write("-CellHeight (m): {}\n".format(L_spacer + L_g + Barrier_width * 2))
f.write("-NumStairs: {}\n".format(NUM_STAIRS))
f.write("-StairLength: {}\n".format(STAIR_LENGTH))
f.write("-TrenchWidth: {}\n".format(TRENCH_WIDTH))
f.close()
