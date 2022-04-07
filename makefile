####################################################################################
#   -----------------------------------------                                      #
#   C/C++ Mixed Functions Library (libmixf)                                        #
#   -----------------------------------------                                      #
#   Copyright 2019-2021 Roberto Mameli                                             #
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
#   VERSION:     2.1.0                                                             #
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
#   REV HISTORY: See updated Revision History in file RevHistory.txt               #
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

MIXFLIB = libmixf
MIXFSOURCES = ./src/mixfApi.c
MIXFOBJS = ./obj/mixfApi.o
LIBS = -lpthread

INCLUDE = -I. -I./include -I./headers
SOLIB = /usr/local/lib
LIB = ./lib

CC = gcc
RM = rm
AR = ar
CP = cp
MV = mv

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -Wall -v $(INCLUDE) -o $@ $<

all:
	$(CC) -c -fpic -Wall -v $(INCLUDE) $(MIXFSOURCES) -o $(MIXFOBJS) $(LIBS)
	$(CC) -shared -Wl,-soname,$(MIXFLIB).so.2 -o $(MIXFLIB).so.2.1  $(MIXFOBJS) -lc
	$(AR) rcs $(MIXFLIB).a $(MIXFOBJS)

install:
	$(MV) $(MIXFLIB).a $(LIB)
	chown root:root $(LIB)/$(MIXFLIB).a
	chmod 777 $(LIB)/$(MIXFLIB).a
	$(MV) $(MIXFLIB).so.2.1 $(SOLIB)
	ln -sf $(SOLIB)/$(MIXFLIB).so.2 $(SOLIB)/$(MIXFLIB).so
	chown root:root $(SOLIB)/$(MIXFLIB).so.2.1
	chmod 777 $(SOLIB)/$(MIXFLIB).so.2.1
	$(CP) ./headers/*.h /usr/local/include
	chown root:root /usr/local/include/*.h
	ldconfig -n $(SOLIB)

clean:
	$(RM) $(MIXFOBJS) $(LIB)/$(MIXFLIB).* $(SOLIB)/$(MIXFLIB).* $(MIXFLIB).*
