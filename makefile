####################################################################################
#   -----------------------------------------                                      #
#   C/C++ Mixed Functions Library (libmixf)                                        #
#   -----------------------------------------                                      #
#   Copyright 2019-2026 Roberto Mameli                                             #
#                                                                                  #
#   Licensed under the Apache License, Version 2.0 (the "License");                #
#   you may not use this file except in compliance with the License.               #
#   You may obtain a copy of the License at                                        #
#                                                                                  #
#       http://www.apache.org/licenses/LICENSE-2.0                                 #
#                                                                                  #
#   Unless required by applicable law or agreed to in writing, software            #
#   distributed under the License is distributed on an "AS IS" BASIS,              #
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.       #
#   See the License for the specific language governing permissions and            #
#   limitations under the License.                                                 #
#   ------------------------------------------------------------------------       #
#                                                                                  #
#   FILE:        libmixf makefile                                                  #
#   VERSION:     3.0.0                                                             #
#   AUTHOR(S):   Roberto Mameli                                                    #
#   PRODUCT:     Library libmixf - general purpose library                         #
#   DESCRIPTION: Source file for various general purpose functions                 #
#                about:                                                            #
#                - File and File System Handling                                   #
#                - Time and Date Handling                                          #
#                - String Handling                                                 #
#                - Configuration Files Handling                                    #
#                - Log Handling                                                    #
#                - License Handling                                                #
#                - Lock Handling                                                   #
#                - Counters Handling                                               #
#   REV HISTORY: See updated Revision History in file CHANGELOG.md                 #
#   NOTE WELL:   If an application needs services and functions from this          #
#                API, it MUST necessarily:                                         #
#                - include the library header file                                 #
#                     #include "mixf.h"                                            #
#                - be linked by including either the shared or the static          #
#                  library libmixf                                                 #
#                ----------------------------------------------------------        #
#                Please, be aware that this library is neither RE-ENTRANT          #
#                nor THRHEAD SAFE. However, Log and Counters Handling              #
#                functions use POSIX mutex to avoid cuncurrent access to           #
#                log files                                                         #
#                                                                                  #
####################################################################################

# ---- Toolchain ----
CC         ?= gcc
AR         ?= ar
RANLIB     ?= ranlib
RM         ?= rm -f
INSTALL    ?= install

# ---- Project ----
NAME       := mixf
VERSION    := 3.0.0

# ---- Directories ----
SRCDIR     := src
HDRDIR     := headers
INCDIR     := include
OBJDIR     := obj
LIBDIR     := lib
EXAMPLEDIR := examples

PREFIX     ?= /usr/local
SYS_LIBDIR := $(PREFIX)/lib
SYS_INCDIR := $(PREFIX)/include

# ---- Sources ----
SRC        := $(wildcard $(SRCDIR)/*.c)
OBJ        := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC))
DEP        := $(OBJ:.o=.d)
HDR        := $(HDRDIR)/mixf.h
EXAMPLES   := Example1 Example2 Example3 Example4

# ---- Libraries ----
STATIC_LIB := $(LIBDIR)/lib$(NAME).a
SHARED_LIB := $(LIBDIR)/lib$(NAME).so.$(VERSION)
SONAME     := lib$(NAME).so

# ---- Flags ----
#CFLAGS     += -v -O2 -Wall -Wextra -fPIC -pthread -MMD -MP
CFLAGS     += -v -O2 -Wall -fPIC -pthread -MMD -MP
CPPFLAGS   += -I$(INCDIR) -I$(HDRDIR)

LDFLAGS    += -pthread
SOFLAGS    := -shared -Wl,-soname,$(SONAME)


# ---- Targets ----

.PHONY: all clean install uninstall dirs staticexamples dynamicexamples cleanexamples

all: dirs $(STATIC_LIB) $(SHARED_LIB)

# ---- Create directories ----
dirs:
	mkdir -p $(OBJDIR)
	mkdir -p $(LIBDIR)

# ---- Pattern rule (.c -> .o) ----
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# ---- Static library ----
$(STATIC_LIB): $(OBJ)
	$(AR) rcs $@ $^
	$(RANLIB) $@

# ---- Shared library ----
$(SHARED_LIB): $(OBJ)
	$(CC) $(SOFLAGS) $(OBJ) $(LDFLAGS) -o $@

# ---- Install ----
install: all
	$(INSTALL) -d $(DESTDIR)$(SYS_LIBDIR)
	$(INSTALL) -d $(DESTDIR)$(SYS_INCDIR)
	$(INSTALL) -m 644 $(STATIC_LIB) $(DESTDIR)$(SYS_LIBDIR)
	$(INSTALL) -m 755 $(SHARED_LIB) $(DESTDIR)$(SYS_LIBDIR)
	ln -sf $(notdir $(SHARED_LIB)) $(DESTDIR)$(SYS_LIBDIR)/$(SONAME)
	$(INSTALL) -m 644 $(HDR) $(DESTDIR)$(SYS_INCDIR)

# ---- Uninstall ----
uninstall:
	$(RM) $(DESTDIR)$(SYS_LIBDIR)/lib$(NAME).a || true
	$(RM) $(DESTDIR)$(SYS_LIBDIR)/$(SONAME)* || true
	$(RM) $(DESTDIR)$(SYS_INCDIR)/mixf.h || true

# ---- Examples with static linking ----
staticexamples: $(LIB_STATIC)
	@mkdir -p $(EXAMPLEDIR)/bin
	@for e in $(EXAMPLES); do \
		$(CC) $(CFLAGS) -Iheaders \
		    $(EXAMPLEDIR)/src/$$e.c \
		    lib/libmixf.a \
		    -o $(EXAMPLEDIR)/bin/$$e-static ; \
	done

# ---- Examples with dynamic linking ----
dynamicexamples: $(SHARED_LIB)
	@mkdir -p $(EXAMPLEDIR)/bin
	@for e in $(EXAMPLES); do \
		$(CC) $(CFLAGS) -Iheaders \
		    $(EXAMPLEDIR)/src/$$e.c \
		    -Llib -lmixf \
		    -Wl,-rpath,'$$ORIGIN/../../lib' \
		    -o $(EXAMPLEDIR)/bin/$$e-dynamic ; \
	done

# ---- Clean ----
clean:
	$(RM) $(OBJ) || true
	$(RM) $(DEP) || true
	$(RM) $(STATIC_LIB) || true
	$(RM) $(SHARED_LIB) || true
	$(RM) $(SONAME) || true

cleanexamples:
	$(RM) $(EXAMPLEDIR)/bin/*

# ---- Include auto-deps ----
-include $(DEP)
