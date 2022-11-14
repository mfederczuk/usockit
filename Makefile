#!/usr/bin/make -f
# Copyright (c) 2022 Michael Federczuk
# SPDX-License-Identifier: MPL-2.0 AND Apache-2.0

SHELL = /bin/sh

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin


build_type != ./detect-build-type
ifneq "$(.SHELLSTATUS)" "0"
  $(error 'detect-build-type' script exited with status code $(.SHELLSTATUS))
endif

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


override shellquote = '$(subst ','\'',$(1))'
override log = $(shell $(SHELL) -c $(call shellquote,printf '%s\n' $(call shellquote,$(1))) >&2)


$(call log,Build type: '$(build_type)')
$(call log,)
$(call log, Programs & program flags:)
$(call log,  SHELL:            '$(SHELL)')
$(call log,  CC:               '$(CC)')
$(call log,  CFLAGS:           '$(strip $(CFLAGS))')
$(call log,  INSTALL:          '$(INSTALL)')
$(call log,  INSTALL_PROGRAM:  '$(INSTALL_PROGRAM)')
$(call log,)
$(call log, Installation paths:)
$(call log,  prefix:       '$(prefix)')
$(call log,  exec_prefix:  '$(exec_prefix)')
$(call log,  bindir:       '$(bindir)')
$(call log,  DESTDIR:      '$(DESTDIR)')
$(call log,)


.SUFFIXES:

all: usockit
.PHONY: all

include/usockit/version.h: version_name.txt
	mkdir -p $(@D)
	{ \
		datetime="$$(date)"                                            && \
		version_name="$$(cat version_name.txt)"                        && \
		printf '/*\n'                                                  && \
		printf ' * Generated at %s\n' "$$datetime"                     && \
		printf ' * DO NOT EDIT!\n'                                     && \
		printf ' */\n'                                                 && \
		printf '\n'                                                    && \
		printf '#ifndef USOCKIT_VERSION_H\n'                           && \
		printf '#define USOCKIT_VERSION_H\n'                           && \
		printf '\n'                                                    && \
		printf '#define USOCKIT_VERSION_NAME  "%s"\n' "$$version_name" && \
		printf '\n'                                                    && \
		printf '#endif /* USOCKIT_VERSION_H */\n'                      ;  \
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
 ifeq "$(build_type)" "debug"
	   @printf '\nWarning: You are about to install the %s build type! Continue? [y/N] ' $(build_type) >&2
	   @read -r ans && \
		   case "$$ans" in \
			   ([yY]*) ;; \
			   (*) \
				   printf 'Aborted.\n' >&2; \
				   false \
				   ;; \
		   esac
 endif
	mkdir -p $(call shellquote,$(DESTDIR)$(bindir))
	$(strip $(INSTALL_PROGRAM) $< $(call shellquote,$(DESTDIR)$(bindir)))
.PHONY: install

uninstall:
 ifeq "$(origin build_type)" "command line"
	   @printf 'Info: Specifying the `build_type` variable when uninstalling has no effect.\n' >&2
 endif
	rm -f $(call shellquote,$(DESTDIR)$(bindir)/usockit)
.PHONY: uninstall

clean:
	rm -rf usockit build include/usockit/version.h
.PHONY: clean
