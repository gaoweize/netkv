# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = netkv

# all source are stored in SRCS-y
SRCS-y := main.cc demo.cc common.cc env.cc inap.cc conf.cc

# Build using pkg-config variables if possible
ifneq ($(shell pkg-config --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PKGCONF ?= pkg-config

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 -g $(shell $(PKGCONF) --cflags libdpdk) # 
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk) -lhiredis
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk) -lhiredis

CFLAGS += -DALLOW_EXPERIMENTAL_API -pthread -lrt

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CXX) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) 

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CXX) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC) 

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true
