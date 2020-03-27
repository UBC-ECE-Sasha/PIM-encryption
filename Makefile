DPU_DIR := dpu
HOST_DIR := host
BUILDDIR ?= build
NR_TASKLETS ?= 1
NR_DPUS ?= 1

define conf_filename
	${BUILDDIR}/.NR_DPUS_$(1)_NR_TASKLETS_$(2).conf
endef
CONF := $(call conf_filename,${NR_DPUS},${NR_TASKLETS})

HOST_TARGET := ${BUILDDIR}/encryption_host
DPU_TARGET := ${BUILDDIR}/encryption_dpu

HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCES := $(wildcard ${DPU_DIR}/*.c)

HOST_INCLUDES := $(wildcard includes/${HOST_DIR})
DPU_INCLUDES := $(wildcard includes/${DPU_DIR})

.PHONY: all clean test

__dirs := $(shell mkdir -p ${BUILDDIR})

COMMON_FLAGS := -Wall -Wextra -Werror -g
HOST_FLAGS := ${COMMON_FLAGS} -I${HOST_INCLUDES} -std=c11 -O3 `dpu-pkg-config --cflags --libs dpu` -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS}
DPU_FLAGS := ${COMMON_FLAGS} -I${DPU_INCLUDES} -O2 -DNR_TASKLETS=${NR_TASKLETS}

all: ${HOST_TARGET} ${DPU_TARGET}

${CONF}:
	$(RM) $(call conf_filename,*,*)
	touch ${CONF}

${HOST_TARGET}: ${HOST_SOURCES} ${COMMON_INCLUDES} ${CONF}
	$(CC) -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

${DPU_TARGET}: ${DPU_SOURCES} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES}

clean:
	$(RM) -r $(BUILDDIR)

test: all
	./${HOST_TARGET}

