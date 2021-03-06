##################### PHIL Makefile ###########################

SRC_DIR = $(shell pwd)
PRNG_DIR = $(SRC_DIR)/PRNG__Mersenne_Twister
DSFMT_DIR = $(PRNG_DIR)/dSFMT-src-2.2
SNAPPY_DIR = $(SRC_DIR)/snappy
SNAPPY_INC_DIR = $(SNAPPY_DIR)/include
SNAPPY_HDR = $(SNAPPY_INC_DIR)/snappy.h 

INCLUDE_DIRS = -I$(DSFMT_DIR) -I$(SNAPPY_INC_DIR)

CPP = g++ -std=c++11 

################   Multithreading   ###############################
# For multithreading, NCPU should be set here or as an argument to make.
# NCPU must be greater than or equal to the maximum number of threads that will
# be used by OpenMP.  The number of threads used by OpenMP can be set at runtime
# with the environment variable OMP_NUM_THREADS.  The value used for OMP_NUM_THREADS
# must be less than or equal the value of NCPU set during compilation.
# If NCPU > omp_get_max_threads, some relatively small amount of memory will be
# wasted, but it's harmless otherwise.
#
# NCPU is defined to be 1 in Global.h if value not set here 
#
# Autodetect all available processors for linux or mac
# Fall back to single-threaded operation
OS := $(shell uname)
ifeq ($(OS),Linux)
NCPU ?= $(shell grep -c ^processor /proc/cpuinfo)
SNAPPY_LIB = $(SNAPPY_DIR)/lib/libsnappy.so
M64 = -m64
else ifeq ($(shell echo $(OS) | cut -f1 -d_),CYGWIN)
NCPU ?=  $(shell grep -c ^processor /proc/cpuinfo)
SNAPPY_LIB = $(SNAPPY_DIR)/lib/libsnappy.la 
else ifeq ($(OS),Darwin)
CPP = g++-5 -std=c++11
NCPU ?= $(shell sysctl -n hw.ncpu)
SNAPPY_LIB = $(SNAPPY_DIR)/lib/libsnappy.dylib
M64 = -m64
endif
################   Compiler Flags   ##############################
PHIL_EXECUTABLE_NAME = phil
PHIL_memcheck: PHIL_EXECUTABLE_NAME = phil_memcheck

LOGGING_PRESET_0 = 
LOGGING_PRESET_1 = -DPHILWARNING
LOGGING_PRESET_2 = -DPHILSTATUS -DPHILWARNING
LOGGING_PRESET_3 = -DPHILVERBOSE -DPHILSTATUS -DPHILWARNING -DPHILDEBUG

DSFMT_CPPFLAGS = -g -O3 -DDSFMT_MEXP=19937 -DDSFMT_DO_NOT_USE_OLD_NAMES -finline-functions -fomit-frame-pointer -DNDEBUG \
-fno-strict-aliasing --param max-inline-insns-single=1800

DSFMT_SRC = $(DSFMT_DIR)/dSFMT.c
DSFMT_HDR = $(DSFMT_DIR)/dSFMT.h $(DSFMT_DIR)/dSFMT-params19937.h

# use this for development:
## CPPFLAGS = -g -O2 $(LOGGING_PRESET_3) -Wall

# use one of these for profiling
# CPPFLAGS = -g $(M64) -O3 $(LOGGING_PRESET_3) # -DNDEBUG
# CPPFLAGS = -pg $(M64) -O2 -DNDEBUG $(LOGGING_PRESET_3) -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls -fno-omit-frame-pointer
# Use one of these for production:

## Use this to run with multiple threads
CPPFLAGS = -g $(M64) -O3 -fopenmp $(LOGGING_PRESET_3) -DNCPU=$(NCPU) -fno-omit-frame-pointer $(INCLUDE_DIRS) 
PHIL_memcheck: CPPFLAGS = -g $(M64) -O0 -fopenmp $(LOGGING_PRESET_3) -DNCPU=$(NCPU) -fno-omit-frame-pointer $(INCLUDE_DIRS)

## Use this to make reproducible serial runs
# CPPFLAGS = -g $(M64) -O3 $(LOGGING_PRESET_3) -DNCPU=1 #-fast #-Wall

CXX = $(CPP)
CXXFLAGS = $(CPPFLAGS)

LDFLAGS = -L$(SNAPPY_DIR)/lib/

LFLAGS = $(SNAPPY_DIR)/lib/libsnappy.a

#################  MD5 Program ##############################

UNIX	:= $(shell uname)
ifeq ($(UNIX), Linux)
MD5SUM	:= md5sum
else ifeq ($(shell echo $(UNIX) | cut -f1 -d_),CYGWIN)
MD5SUM := md5sum
else
MD5SUM	:= md5 -q
endif

###############################################

%.o:%.c %.h
	$(CPP) $(CPPFLAGS) -c $< $(INCLUDES)

OBJ =   Phil.o Global.o Age_Map.o Timestep_Map.o Utils.o Params.o Date.o Random.o \
	Geo_Utils.o Cell.o Grid.o Large_Grid.o Large_Cell.o Small_Grid.o Small_Cell.o Travel.o \
	Decision.o Policy.o Manager.o \
	Antiviral.o Antivirals.o AV_Decisions.o AV_Policies.o AV_Manager.o AV_Health.o \
	Vaccine_Health.o Vaccine_Dose.o Vaccine.o Vaccines.o \
	Vaccine_Priority_Decisions.o Vaccine_Priority_Policies.o Vaccine_Manager.o \
	Person.o Place.o Place_List.o Population.o \
	Activities.o Demographics.o Health.o \
	Classroom.o Hospital.o Household.o Neighborhood.o Office.o School.o Workplace.o \
	Disease.o Infection.o Epidemic.o \
	EvolutionFactory.o Evolution.o Strain.o Trajectory.o StrainTable.o \
	Multistrain_Timestep_Map.o \
	IntraHost.o DefaultIntraHost.o FixedIntraHost.o RSAIntraHost.o\
	Abstract_Grid.o Abstract_Cell.o \
	Seasonality_Timestep_Map.o Seasonality.o \
	Past_Infection.o MSEvolution.o Piecewise_Linear.o \
	Compression.o Report.o
	# ODEIntraHost.o ODE.o

SRC = $(OBJ:.o=.cc)

HDR = $(OBJ:.o=.h)

MD5 := PHIL.md5

all: PHIL PHIL.tar.gz fsz $(MD5)

PHIL: $(SNAPPY_LIB) $(OBJ) dSFMT.o
	$(CPP) -o $(PHIL_EXECUTABLE_NAME) $(CPPFLAGS) $(LDFLAGS) $(OBJ) dSFMT.o $(LFLAGS) -ldl
	mkdir -p ../bin
	cp $(PHIL_EXECUTABLE_NAME) ../bin

#LIBPHIL: $(SNAPPY_LIB) $(OBJ) dSFMT.o
#	$(CPP) -shared -fPIC  -o libphil.so $(CPPFLAGS) $(LDFLAGS) $(OBJ) dSFMT.o $(LFLAGS) -ldl

PHIL_Unit_Tracker: 
	cd TestSuite/Tracker; $(CPP) -g -O0 -fopenmp -DUNIT_TEST=1 -I../../ Tracker_Unit_Test.cc -c -o Tracker_Unit_Test.o
	cd TestSuite/Tracker; $(CPP) -g -O0 -fopenmp -o PHIL_Unit_Tracker -DUNIT_TEST=1 -I../../ ../../Global.o Tracker_Unit_Test.o


PHIL_memcheck: PHIL

dSFMT.o:
	$(CPP) $(DSFMT_CPPFLAGS) $(DSFMT_SRC) -c $(DSFMT_HDR) 

fsz: $(SNAPPY_LIB) Compression.o Global.h fsz.cc
	$(CPP) -o fsz $(CPPFLAGS) $(LDFLAGS) Compression.o $(LFLAGS) fsz.cc
	mkdir -p ../bin
	cp fsz ../bin

DEPENDS: $(SRC) $(HDR)
	$(CPP) -MM $(SRC) > DEPENDS

include DEPENDS

$(SNAPPY_LIB):
	cd $(SNAPPY_DIR); tar --strip-components=1 -xzvf snappy-1.0.5.tar.gz; CPP=$(CPP) CXX=$(CPP) ./configure --prefix=$(SNAPPY_DIR); make && make install; mv .libs lib;


PHIL.tar.gz: $(SRC) $(HDR)
	tar -czf PHIL.tar.gz $(HDR) $(SRC) Makefile

PHIL.md5 : PHIL.tar.gz
	$(MD5SUM) $< > $@

##############################################

print:
	enscript $(SRC) $(HDR)

clean:
	rm -f gmock.a gmock_main.a *.o PHIL ../bin/PHIL fsz ../bin/fsz *~
	(cd $(SNAPPY_DIR); make clean)
	(cd $(SNAPPY_DIR); rm -r lib)

tar: clean
	cd ..
	tar cvf PHIL-`date +"%Y-%m-%d"`.tar PHIL

dist:
	make clean
	(cd ../..; tar cvf PHIL-`date +"%Y-%m-%d"`.tar PHIL/src/Makefile \
	PHIL/src/*.txt PHIL/src/*.cc PHIL/src/*.h PHIL/src/params* \
	PHIL/bin/* PHIL/src/OUT.RT/* ; cd PHIL/src)
	make

VER = 1.0.1

release:
	make clean
	(cd ../..; tar cvzf PHIL-V${VER}-`date +"%Y-%m-%d"`.tgz --exclude CVS \
	PHIL/Makefile PHIL/LICENSE PHIL/bin PHIL/input_files PHIL/region/ \
	PHIL/src/Makefile PHIL/src/*.h  PHIL/src/*.cc PHIL/src/mt*.c PHIL/tests)
	rm -f gmock.a gmock_main.a *.o PHIL ../bin/PHIL *~

tags:
	find . -name \*.[ch]* | xargs etags
