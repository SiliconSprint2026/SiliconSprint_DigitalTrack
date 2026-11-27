# SiliconSprint 2026 - CVA6 Packed-SIMD Challenge
# Frozen development environment: Verilator, Spike, RISC-V GCC, riscv-tests.
#
#   docker build -t siliconsprint/cva6-simd:2026 docker/
#   docker run --rm -it -v "$PWD":/work siliconsprint/cva6-simd:2026
#
# All versions below are FROZEN for the event. Do not upgrade them.
FROM ubuntu:24.04

ARG VERILATOR_TAG=v5.008
ARG SPIKE_COMMIT=6e18887745c63f33048e4be3d880c65a72aad0df
ARG RISCV_TESTS_COMMIT=1b3f8bd665d79e5985753a558dae4e3718e2edc8
ARG JOBS=8

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential git ca-certificates autoconf automake libtool \
        python3 python3-pip python3-matplotlib \
        flex bison libfl2 libfl-dev help2man perl ccache \
        zlib1g-dev libyaml-cpp-dev device-tree-compiler \
        gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf \
        make bc time file \
    && rm -rf /var/lib/apt/lists/*

# ---- Verilator (same version + patch as CVA6 v5.3.0 CI) --------------------
COPY verilator-v5.patch /tmp/verilator-v5.patch
RUN git clone --depth 1 --branch ${VERILATOR_TAG} https://github.com/verilator/verilator.git /tmp/verilator \
    && cd /tmp/verilator && git apply /tmp/verilator-v5.patch \
    && autoconf && ./configure --prefix=/opt/verilator \
    && make -j${JOBS} && make install \
    && rm -rf /tmp/verilator

# ---- Spike ISS (golden model + fesvr libraries used by the testharness) ----
RUN git clone https://github.com/riscv-software-src/riscv-isa-sim.git /tmp/spike \
    && cd /tmp/spike && git checkout ${SPIKE_COMMIT} \
    && git submodule update --init --recursive \
    && mkdir build && cd build \
    && ../configure --prefix=/opt/spike --with-boost=no --with-boost-asio=no --with-boost-regex=no \
    && make -j${JOBS} && make install \
    && cp -r /tmp/spike /opt/spike/src \
    && rm -rf /opt/spike/src/build /tmp/spike

# ---- riscv-tests (scalar regression: rv64ui / rv64um / rv64ua, p-env) -------
RUN git clone https://github.com/riscv-software-src/riscv-tests.git /tmp/riscv-tests \
    && cd /tmp/riscv-tests && git checkout ${RISCV_TESTS_COMMIT} \
    && git submodule update --init --depth 1 env \
    && mkdir -p /opt/riscv-tests/isa && cd isa \
    && (make -k -j${JOBS} XLEN=64 RISCV_PREFIX=riscv64-unknown-elf- || true) \
    && for s in rv64ui rv64um rv64ua; do \
         find . -maxdepth 1 -name "${s}-p-*" ! -name '*.dump' -exec cp {} /opt/riscv-tests/isa/ \; ; \
       done \
    && test $(ls /opt/riscv-tests/isa | wc -l) -gt 60 \
    && rm -rf /tmp/riscv-tests

ENV PATH=/opt/verilator/bin:/opt/spike/bin:$PATH \
    VERILATOR_INSTALL_DIR=/opt/verilator \
    SPIKE_INSTALL_DIR=/opt/spike \
    SPIKE_SRC_DIR=/opt/spike/src \
    RISCV=/opt/spike \
    RISCV_PREFIX=riscv64-unknown-elf- \
    RISCV_TESTS_DIR=/opt/riscv-tests/isa \
    NUM_JOBS=8

WORKDIR /work
CMD ["/bin/bash"]
