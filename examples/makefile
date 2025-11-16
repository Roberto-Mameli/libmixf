####################################################################################
#   -----------------------------------------                                      #
#   C/C++ Mixed Functions Library (libmixf)                                        #
#   -----------------------------------------                                      #
#   Copyright 2019-2025 Roberto Mameli                                             #
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
#   FILE:        libmixf examples makefile                                         #
#   VERSION:     3.0.0                                                             #
#   AUTHOR(S):   Roberto Mameli                                                    #
#   PRODUCT:     Library libmixf - general purpose library                         #
#   DESCRIPTION: Makefile for libmixf examples                                     #
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

shared:
	gcc -g -c -O2 -Wall -v -I../headers ./src/Example1.c
	gcc -g -o ./bin/Example1 ./Example1.o -lmixf
	rm ./Example1.o
	gcc -g -c -O2 -Wall -v -I../headers ./src/Example2.c
	gcc -g -o ./bin/Example2 ./Example2.o -lmixf
	rm ./Example2.o
	gcc -g -c -O2 -Wall -v -I../headers ./src/Example3.c
	gcc -g -o ./bin/Example3 ./Example3.o -lmixf
	rm ./Example3.o
	gcc -g -c -O2 -Wall -v -I../headers ./src/Example4.c
	gcc -g -o ./bin/Example4 ./Example4.o -lmixf
	rm ./Example4.o

static:
	gcc  -v ./src/Example1.c -I../headers -L../lib  -o ./bin/Example1 -lmixf
	gcc  -v ./src/Example2.c -I../headers -L../lib  -o ./bin/Example2 -lmixf
	gcc  -v ./src/Example3.c -I../headers -L../lib  -o ./bin/Example3 -lmixf
	gcc  -v ./src/Example4.c -I../headers -L../lib  -o ./bin/Example4 -lmixf

clean:
	rm ./bin/Example1
	rm ./bin/Example2
	rm ./bin/Example3
	rm ./bin/Example4
