# Build the Spike vsimd extension as a shared library.
#   make            -> build/libvsimd.so
#   make test       -> run tests/build/directed/vadd.elf on Spike
SPIKE_INSTALL_DIR ?= /opt/spike
CXX      ?= g++
# Note: do NOT add include/fesvr to the path - its syscall.h shadows the system one.
CXXFLAGS := -std=c++2a -O2 -fPIC -Wall \
            -I$(SPIKE_INSTALL_DIR)/include \
            -I$(SPIKE_INSTALL_DIR)/include/riscv
# Do NOT link libriscv: the spike executable already contains it and resolves
# our symbols at load time (linking it again registers every plugin twice).
LDFLAGS  := -shared

SRC ?= vsimd.cc
LIB := build/libvsimd.so

$(LIB): $(SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

test: $(LIB)
	spike --isa=rv64gc --extlib=$(abspath $(LIB)) --extension=vsimd \
	      ../tests/build/directed/vadd.elf

clean:
	rm -rf build

.PHONY: test clean
