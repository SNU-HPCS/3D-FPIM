#
# Copyright (c) 2022 Seoul National University. See LICENSE file in the top-
# level directory. This file contains code from puma-compiler, (c) 2019,
# University of Illinois. See LICENSE_PUMA file in the parent directory.
# 3D-FPIM Project can be copied according to the terms contained in the
# LICENSE file.
#

CXX=g++
CXXFLAGS=-std=c++11 -O0 -g
LD_FLAGS=
INCLUDE=-I../include
LIB=-L../src -l3dfpim

DEP=$(wildcard *.h)
SRC=$(wildcard *.cpp)
TEST=$(SRC:.cpp=.test)

default: $(TEST)

%.test: %.o
	g++ $(CXXFLAGS) $(LD_FLAGS) -o $@ $< $(LIB)

%.o: %.cpp $(DEP)
	g++ $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f *.o *.test *.dot *.pdf *.3dfpim *.3dfpim.py *.graph *.out *.weights

