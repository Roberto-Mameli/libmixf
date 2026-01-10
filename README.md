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
#                                                                                  #
####################################################################################


----------------
(1) INTRODUCTION
----------------
libmixf is a library written in C language, which can be linked (either statically
or dynamically) to C/C++ programs to provide a set of mixed functions (hence the
name) concerning:

   - File and File System Handling
   - Time and Date Handling
   - String Handling
   - Configuration Files Handling
   - Log Handling
   - License Handling
   - Lock Handling
   - Counters Handling

They are written as general purpose functions, in order to be reused whenever needed.
The latest library version can be downloaded either from author's personal web site
(https://www.roberto-mameli.it/software) or from GitHub repository
(https://github.com/Roberto-Mameli/libmixf.git).


--------------------------
(2) HOW TO COMPILE LIBRARY
--------------------------
The library has been developed in Linux environment (RedHat, CentOS), but since
it relies on POSIX standards and gcc compiler and development tooIkit, it is extremely
likely that it can be easily ported to most UNIX based operating systems (just
recompiling it). It can be compiled without problems with glibc 2.12 or above.

Be aware that this library does not come with an automatically generated makefile
(cmake or similar). It contains a manually written makefile, composed by a few lines,
that works on RedHat based operating systems (RedHat, Centos, etc.), and that can be
easily adapted to other Linux based operating systems.

After having downloaded the library, extract the source files into a directory
arbitrarily chosen in your Linux Box:

    tar -zxvf libmixf2.1.tar.gz
    cd libmixf2.1

(this example assumes libmixf2.1, however it can be applied also to other versions by
simply referring to the correct one).

After that, type the following commands:

    make all
    make install

The first compiles the library and produces in the libmixf2.1 directory both the
static and the shared libraries (respectively libmixf.a and libmixf.so.2.1).

The second installs the libraries in the destination folders. Specifically,
the header file mixf.h is copied into /usr/local/include (this path is the one
usually used in RedHat based operating systems, it may differ in other Linux
distributions).

Static library libmixf.a is copied into the libmixf2.1/lib folder. Dynamic libraries,
instead, are copied to /usr/local/lib path (usually used in RedHat based operating
systems, it may differ in other Linux distributions). Be aware that, in order to use
shared libraries, this path shall be either configured in /etc/ld.so.conf or in
environment variable $LD_LIBRARY_PATH. The first time you install the libraries, a
further command might be needed to configure dynamic linker run-time bindings:

    ldconfig

If you apply changes to the library source code and you want to recompile it from
scratch, you can clean up all executables by typing:

    make clean


----------------------------------------------------
(3) HOW TO USE THE LIBRARY IN YOUR C/C++ SOURCE CODE
----------------------------------------------------
Let’s assume that libraries are correctly compiled and installed (see previous section).

In order to use library functions within C/C++ source code, the following shall be done:

    - include the mixf.h header file
        #include "mixf.h"

    - link the executable by including either the shared or the static library libmixf

To compile a generic example file (let's say example.c), simply type:

    gcc -g -c -O2 -Wall -v –I/usr/local/include example.c
    gcc -g -o example example.c -lmixf

for shared library linking or:

    gcc -static example.c -I/usr/local/include -L. -lmixf -o example

for static linking. In the previous command -L. means that the libmixf.a file is
available in the same directory of the source code example.c; if this is not the case
just replace the dot after L with the path to the library file.


---------------------------------
(4) LIBRARY FUNCTIONS DESCRIPTION
---------------------------------
For a comprehensive description of the library functions, please refer to the available
documntation in the ./docs subdirectory


----------------
(5) KNOWN ISSUES
----------------
(1) check_url_validity()
----------------------
- Function check_url_validity() supports 255 characters as maximum URL length. Furthermore,
  it does not support full URL verification. Remember that the general URL format is the
  following:

    [protocol://][username[:password]@]host[:port][</path>][?querystring][#fragment]

  Currently there are some minor limitations:
    - URL authentication is not recognized, i.e. if the URL contains
        [username[:password]@]
      it is not considered a valid URL. This is not a great problem, given that basic URL
      authentication is almost never used since it lacks in terms of security (user
      and password are contained in clear text).
    - [?querystring] and [#fragment] are not completely verified (both should be
      formatted as concatenation of parameter/value pairs, i.e.
        param1=value1&param2=value2...
      This is not controlled, the routine just checks that those sections contain
      only allowed characters

(2) Configuration Files Handling Functions
------------------------------------------
- Only IPv4 IP Addresses are supported (no IPv6)

(3) License Handling Functions
------------------------------
Those functions use platform dependent parameters to encrypt and decrypt the license
file (mainly hostname and hostid). However, both parameters can be changed rather
easily, so the check can be somehow circumvented. This could be a problem in some
cases.