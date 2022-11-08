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
  override error_flag :=
  override ndebug_flag :=
  override memtrace3_flag := -lmemtrace3
else
  ifneq "$(build_type)" "release"
    $(error `build_type` variable must be either 'debug' or 'release' (actual value was '$(build_type)'))
  endif

  override optimization_flag := -O2
  override error_flag := -Werror
  override ndebug_flag := -DNDEBUG
  override memtrace3_flag :=
endif


CC ?= cc
INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL)

CFLAGS = $(EXTRA_CFLAGS) -std=c11 $(optimization_flag) \
         -Wall -Wextra -Wconversion $(error_flag) \
         -pedantic -pedantic-errors -Wpedantic -Werror=pedantic \
         $(ndebug_flag)


override header_file_paths != find include -mindepth 1 -type f -name '*.h'
override header_file_paths += include/usockit/version.h

override source_file_paths != find src -mindepth 1 -type f -name '*.c'
override object_file_paths := $(source_file_paths:src/%.c=build/$(build_type)/obj/%.o)


.SUFFIXES:

all: usockit
.PHONY: all

include/usockit/version.h: version_name.txt
	mkdir -p $(@D)
	{ \
		printf '/*\n'                                                           && \
		printf ' * Generated at %s\n' "$$(date)"                                && \
		printf ' * DO NOT EDIT!\n'                                              && \
		printf ' */\n'                                                          && \
		printf '\n'                                                             && \
		printf '#ifndef USOCKIT_VERSION_H\n'                                    && \
		printf '#define USOCKIT_VERSION_H\n'                                    && \
		printf '\n'                                                             && \
		printf '#define USOCKIT_VERSION_NAME  "%s"\n' "$$(cat version_name.txt)" && \
		printf '\n'                                                             && \
		printf '#endif /* USOCKIT_VERSION_H */\n'                               ;  \
	} > $@

$(object_file_paths): build/$(build_type)/obj/%.o: $(header_file_paths) src/%.c
	mkdir -p $(@D)
	$(strip $(CC) $(CFLAGS) -Iinclude -c $(lastword $^) -o $@)

build/$(build_type)/bin/artifacts/usockit: $(object_file_paths)
	mkdir -p $(@D)
	$(strip $(CC) $(CFLAGS) $^ -o $@ -pthread $(memtrace3_flag))

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
	rm -rf usockit build include/usockit/version.h
.PHONY: clean
