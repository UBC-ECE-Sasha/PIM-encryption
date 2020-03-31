DPU_DIR := dpu
HOST_DIR := host
BUILDDIR ?= build
NR_TASKLETS ?= 1
NR_DPUS ?= 1

define conf_filename
	${BUILDDIR}/.NR_DPUS_$(1)_NR_TASKLETS_$(2).conf
endef
CONF := $(call conf_filename,${NR_DPUS},${NR_TASKLETS})

HOST_TARGET := ${BUILDDIR}/pimcrypto
DPU_ENCRYPT_TARGET := ${BUILDDIR}/dpu_encrypt
DPU_DECRYPT_TARGET := ${BUILDDIR}/dpu_decrypt

HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCES := $(wildcard ${DPU_DIR}/*.c)

COMMON_INCLUDES := include

.PHONY: all clean test

__dirs := $(shell mkdir -p ${BUILDDIR})

COMMON_FLAGS := -Wall -Wextra -Werror -g -I${COMMON_INCLUDES}
HOST_FLAGS := ${COMMON_FLAGS} -std=c11 -O3 `dpu-pkg-config --cflags --libs dpu` -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS}
DPU_FLAGS := ${COMMON_FLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS}

all: ${HOST_TARGET} ${DPU_ENCRYPT_TARGET} ${DPU_DECRYPT_TARGET}

${CONF}:
	$(RM) $(call conf_filename,*,*)
	touch ${CONF}

${HOST_TARGET}: ${HOST_SOURCES} ${COMMON_INCLUDES} ${CONF}
	$(CC) -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

${DPU_ENCRYPT_TARGET}: ${DPU_SOURCES} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES}

${DPU_DECRYPT_TARGET}: ${DPU_SOURCES} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -DDECRYPT -o $@ ${DPU_SOURCES}

ctags: ${HOST_SOURCES} ${DPU_SOURCES} ${COMMON_INCLUDES}
	ctags ${HOST_SOURCES} ${DPU_SOURCES} $(wildcard ${COMMON_INCLUDES}/*.h)

clean:
	$(RM) -r $(BUILDDIR)

test: all
	./${HOST_TARGET} test.txt test.txt.enc

