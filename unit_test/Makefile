CXX = g++
CXXFLAGS += -O3 -g -std=c++11 -lm
CXXFLAGS += -msse3 -mssse3

MACHINE := $(shell uname -m)
ifeq ($(MACHINE),x86_64)
	RAPTORQ__ARCH=x86_64
	OBJDIR=bin64
else
	RAPTORQ__ARCH=i686
	OBJDIR=bin32
endif

RAPTORQ_SDK_PATH = ../src
RAPTORQ_LIB      = ../build/lib
RAPTORQ_SDK_INC  = -I$(RAPTORQ_SDK_PATH) -I../external/eigen3
RAPTORQ_SDK_LDFLAGS = -L$(RAPTORQ_LIB) -lRaptorQ -ldl -lpthread -lgomp -fopenmp

PROGS = tests
PROGS := $(patsubst %,$(OBJDIR)/%,$(PROGS))
all: $(PROGS) | objdirs

objdirs:
	@mkdir -p $(OBJDIR)

# Command line example
$(OBJDIR)/tests-main.o: tests-main.cpp Makefile | objdirs
	$(CXX) ${CXXFLAGS} $(RAPTORQ_SDK_INC) -c -o $@ $<

$(OBJDIR)/tests-ops-store.o: tests-ops-store.cpp Makefile | objdirs
	$(CXX) ${CXXFLAGS} $(RAPTORQ_SDK_INC) -c -o $@ $<

$(OBJDIR)/tests-gf-calc.o: tests-gf-calc.cpp Makefile | objdirs
	$(CXX) ${CXXFLAGS} $(RAPTORQ_SDK_INC) -c -o $@ $<

$(OBJDIR)/tests-util-matrix.o: tests-util-matrix.cpp Makefile | objdirs
	$(CXX) ${CXXFLAGS} $(RAPTORQ_SDK_INC) -c -o $@ $<

$(OBJDIR)/tests-dense-matrix.o: tests-dense-matrix.cpp Makefile | objdirs
	$(CXX) ${CXXFLAGS} $(RAPTORQ_SDK_INC) -c -o $@ $<

$(OBJDIR)/tests-eigen-compatibility.o: tests-eigen-compatibility.cpp Makefile | objdirs
	$(CXX) ${CXXFLAGS} $(RAPTORQ_SDK_INC) -c -o $@ $<

$(OBJDIR)/tests: $(OBJDIR)/tests-main.o $(OBJDIR)/tests-ops-store.o $(OBJDIR)/tests-gf-calc.o $(OBJDIR)/tests-util-matrix.o $(OBJDIR)/tests-dense-matrix.o $(OBJDIR)/tests-eigen-compatibility.o | objdirs
	$(CXX) -o $@ $(OBJDIR)/tests-main.o $(OBJDIR)/tests-ops-store.o $(OBJDIR)/tests-gf-calc.o $(OBJDIR)/tests-util-matrix.o $(OBJDIR)/tests-dense-matrix.o $(OBJDIR)/tests-eigen-compatibility.o $(RAPTORQ_SDK_INC) $(RAPTORQ_SDK_LDFLAGS)

clean:
	rm -f $(OBJDIR)/*.o $(PROGS)
