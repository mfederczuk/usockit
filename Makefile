#!/usr/bin/make -f
# Copyright (c) 2022 Michael Federczuk
# SPDX-License-Identifier: MPL-2.0 AND Apache-2.0

SHELL = /bin/sh

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin


build_type = debug

ifeq "$(build_type)" "debug"
  override optimization_flag :=
  override ndebug_flag :=
else
  ifneq "$(build_type)" "release"
    $(error `build_type` variable must be either 'debug' or 'release' (actual value was '$(build_type)'))
  endif

  override optimization_flag := -O2
  override ndebug_flag := -DNDEBUG
endif


CC ?= cc
INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL)

CFLAGS = $(EXTRA_CFLAGS) -std=c11 $(optimization_flag) \
         -Wall -Wextra -Wconversion -Werror=infinite-recursion \
         -pedantic -pedantic-errors -Wpedantic -Werror=pedantic \
         $(ndebug_flag)


override header_file_paths != find include -mindepth 1 -type f -name '*.h'
override source_file_paths != find src -mindepth 1 -type f -name '*.c'
override object_file_paths := $(source_file_paths:src/%.c=build/$(build_type)/obj/%.o)


.SUFFIXES:

all: usockit
.PHONY: all

$(object_file_paths): build/$(build_type)/obj/%.o: $(header_file_paths) src/%.c
	mkdir -p $(@D)
	$(strip $(CC) $(CFLAGS) -Iinclude -c $(lastword $^) -o $@)

build/$(build_type)/bin/artifacts/usockit: $(object_file_paths)
	mkdir -p $(@D)
	$(strip $(CC) $(CFLAGS) $^ -o $@ -pthread)

usockit: build/$(build_type)/bin/artifacts/usockit
	ln -sf $< $@

install: build/$(build_type)/bin/artifacts/usockit
	mkdir -p $(DESTDIR)$(bindir)
	$(strip $(INSTALL_PROGRAM) $< $(DESTDIR)$(bindir))
.PHONY: install

uninstall:
	rm -f $(DESTDIR)$(bindir)/usockit
.PHONY: uninstall

clean:
	rm -rf usockit build
.PHONY: clean
