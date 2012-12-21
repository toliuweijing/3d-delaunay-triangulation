CXX=gcc
CXXFLAGS=-Wno-deprecated -O3 -std=c++11
LIBS=
DIRS=

OS := $(shell uname -s)
ifeq ($(OS),Darwin)
LIBS+= -lm -lstdc++ -ltbb -L/opt/local/lib 
LIBS+= -lglew -lgl -lglut -lgomp 
else
LIBS+= -lrt -lm -ltbb -lstdc++ -lGLEW -lGLU -lGL -lglut -lgomp
DIRS+= -I ~/hpc_develop/boost_1_52_0
endif


delaunay: delaunay.cpp predicates.o spatialsort.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(DIRS)  

spatialsort.o: spatialsort.cpp
	$(CXX) $(CXXFLAGS) -c spatialsort.cpp -fopenmp 

predicates.o: predicates.c
	$(CXX) -O3 -c predicates.c

run: delaunay
	./delaunay

clean:
	$(RM) delaunay *.o *.d


