DPU_DIR := dpu
HOST_DIR := host
COMMON_DIR := common
export BUILDDIR ?= build
export EXPERIMENTDIR ?= experiment

#
# Configuration options
#
# NR_TASKLETS 	 How many tasklets to use (2-24)
# NOBULK 	 If defined, use dpu_copy_to instead of dpu_push_xfer
# TRANSFER_SIZE  Set the size of transfers from MRAM to WRAM. Max is 2048 (note: decent performance requires at least 16B per tasklet)
# PERFCOUNT_TYPE Valid options are COUNT_INSTRUCTIONS and COUNT_CYCLES

# Defaults (and build directories) are exported so that they will be available to the recursive make in the experiment rule
export NR_TASKLETS ?= 14
export TRANSFER_SIZE ?= 2048
export PERFCOUNT_TYPE ?= COUNT_INSTRUCTIONS

define conf_filename
	${BUILDDIR}/.$(1)_NR_TASKLETS_$(2)_TRANSFER_SIZE_$(3)_PERFCOUNT_TYPE_$(4)$(5).conf
endef


HOST_TARGET := ${BUILDDIR}/pimcrypto
DPU_ENCRYPT_TARGET := ${BUILDDIR}/dpu_encrypt
DPU_DECRYPT_TARGET := ${BUILDDIR}/dpu_decrypt

HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCES := $(wildcard ${DPU_DIR}/*.c)
COMMON_SOURCES := $(wildcard ${COMMON_DIR}/*.c)

HOST_INCLUDES := $(wildcard ${HOST_DIR}/*.h)
DPU_INCLUDES := $(wildcard ${DPU_DIR}/*.h)
COMMON_INCLUDES := $(wildcard ${COMMON_DIR}/*.h)

.PHONY: all clean experiment

__dirs := $(shell mkdir -p ${BUILDDIR})

COMMON_FLAGS := -Wall -Wextra -Werror -I${COMMON_DIR} -I${CURDIR} -DNR_TASKLETS=${NR_TASKLETS} -DTRANSFER_SIZE=${TRANSFER_SIZE} -DPERFCOUNT_TYPE=${PERFCOUNT_TYPE}

ifdef NOBULK
	COMMON_FLAGS += -DNOBULK
	# Make it appear nicely in CONF
	NOBULK = _NOBULK
endif

ifdef EXPERIMENT
	EXPERIMENT := EXPERIMENT
	COMMON_FLAGS += -O3 -DEXPERIMENT
else
	EXPERIMENT := DEBUG
	COMMON_FLAGS += -g -O0
endif

CONF := $(call conf_filename,${EXPERIMENT},${NR_TASKLETS},${TRANSFER_SIZE},${PERFCOUNT_TYPE},${NOBULK})

HOST_FLAGS := ${COMMON_FLAGS} -I${HOST_DIR} `dpu-pkg-config --cflags --libs dpu` -DDPU_ENCRYPT_BINARY='"${DPU_ENCRYPT_TARGET}"' -DDPU_DECRYPT_BINARY='"${DPU_DECRYPT_TARGET}"'
DPU_FLAGS := ${COMMON_FLAGS} -I${DPU_DIR}

all: ${HOST_TARGET} ${DPU_ENCRYPT_TARGET} ${DPU_DECRYPT_TARGET}

${CONF}:
	$(RM) $(call conf_filename,*,*,*,*,*)
	touch ${CONF}

${HOST_TARGET}: ${HOST_SOURCES} ${HOST_INCLUDES} ${COMMON_SOURCES} ${COMMON_INCLUDES} ${CONF}
	$(CC) -o $@ ${HOST_SOURCES} ${COMMON_SOURCES} ${HOST_FLAGS}

${DPU_ENCRYPT_TARGET}: ${DPU_SOURCES} ${DPU_INCLUDES} ${COMMON_SOURCES} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES} ${COMMON_SOURCES}

${DPU_DECRYPT_TARGET}: ${DPU_SOURCES} ${DPU_INCLUDES} ${COMMON_SOURCES} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -DDECRYPT -o $@ ${DPU_SOURCES} ${COMMON_SOURCES}

experiment:
	$(eval export BUILDDIR := ${EXPERIMENTDIR})
	$(eval export EXPERIMENT := EXPERIMENT)
	$(MAKE)

ctags: ${HOST_SOURCES} ${DPU_SOURCES} ${COMMON_SOURCES} ${COMMON_INCLUDES}
	ctags ${HOST_SOURCES} ${DPU_SOURCES} ${COMMON_SOURCES} $(wildcard ${COMMON_INCLUDES}/*.h)

clean:
	$(RM) -r $(BUILDDIR)
	$(RM) -r $(EXPERIMENTDIR)

