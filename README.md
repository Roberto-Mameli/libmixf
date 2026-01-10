- [Introduction](#introduction)
- [How to compile and install the library](#how-to-compile-and-install-the-library)
- [How to use the *libmixf* library into your C/C++ code](#how-to-use-the-libmixf-library-into-your-cc-code)
- [Library Functions Description](#library-functions-description)
- [Known Issues](#known-issues)
  - [*check\_url\_validity()*](#check_url_validity)
  - [Configuration Files Handling Functions](#configuration-files-handling-functions)
  - [License Handling Functions](#license-handling-functions)

# Introduction
*libmixf* is a library written in C language, which can be linked (either statically or dynamically) to C/C++ programs to provide a set of mixed functions (hence the name) concerning:

- File and File System Handling
- Time and Date Handling
- String Handling
- Configuration Files Handling
- Log Handling
- License Handling
- Lock Handling
- Counters Handling

They are written as general purpose functions, in order to be reused whenever needed.
The latest library version can be downloaded either from author's [personal web site](https://www.roberto-mameli.it/software) or from [GitHub repository](https://github.com/Roberto-Mameli/libmixf.git).


# How to compile and install the library
The library has been developed in Linux environment (RedHat, CentOS, Ubuntu), but since it relies on POSIX standards and gcc compiler and development tooIkit, it is extremely likely that it can be easily ported to most UNIX based operating systems (just recompiling it). It can be compiled without problems with glibc 2.12 or above.

Be aware that this library does not come with an automatically generated makefile (cmake or similar). It contains a manually written makefile, composed by a few lines, that has been successfully tested on RedHat and Debian distributions (RedHat, Centos, Ubuntu, etc.), and that can be easily adapted to other Linux based operating systems.

The library can either be cloned from the GitHub repository or downloaded (either from GitHub or from the web site).

- git clone from GitHub:

  > git clone https://github.com/Roberto-Mameli/libmixf.git
  >
  > cd libmixf

- download:

  > tar -zxvf libmixf2.1.tar.gz
  >
  > cd libmixf2.1

  *or*

  > unzip libmixf-master.zip
  >
  > cd libmixf-master

(this example assumes *libmixf2.1*, however it can be applied also to other versions by simply referring to the correct one).

After that, type the following commands:

    make all
    sudo make install

(be aware that *sudo* is not needed if logged as *root*).

The first command compiles the library and produces in the *libmixf* directory both the static and the shared libraries (respectively *libmixf.a* and *libmixf.so.2.1*).

The second command installs the libraries in the destination folders. Specifically, the header file *mixf.h* is copied into */usr/local/include* (this path may differ depending on the Linux distribution).

Static library *libmixf.a* is copied into the *./lib* folder. Dynamic libraries, instead, are copied to */usr/local/lib* path (or equivalent depending on the Linux distribution). Be aware that, in order to use shared libraries, this path shall be either configured in */etc/ld.so.conf* or in environment variable **$LD_LIBRARY_PATH**.

The first time you install the libraries, a further command might be needed to configure dynamic linker run-time bindings:

    ldconfig

If you apply changes to the library source code and you want to recompile it from scratch, you can clean up all executables by typing:

    make clean


# How to use the *libmixf* library into your C/C++ code
After compiling and installing the *libmixf* library (see previous section), it can be linked statically or dynamically to C/C++ code.

This is obtained as follows:

− include the *mixf.h* header file

  > #include "mixf.h"

− link the executable by including either the shared or the static library libmixf


To compile a generic example file (let's say **_example.c_** ), simply type:

- for shared library linking:

  > gcc -g -c -O2 -Wall -v –I/usr/local/include example.c
  >
  > gcc -g -o example example.c - lmixf

- for static linking:

  > gcc -static example.c - I/usr/local/include -L. -lmixf - o example


In the previous command **_- L._** means that the **_libmixf.a_** file is available in the same directory of the source code **_example.c_** ; if this is not the case just replace the dot after **_L_** with the path to the library file.


# Library Functions Description
For a comprehensive description of the library functions, please refer to the available documntation in the *./docs* subdirectory


# Known Issues

## *check_url_validity()*
Function *check_url_validity()* supports 256 characters as maximum URL length. Furthermore, it does not support full URL verification. Remember that the general URL format is the following:

    [protocol://][username[:password]@]host[:port][</path>][?querystring][#fragment]

Currently there are some minor limitations:
- URL authentication is not recognized, i.e. if the URL contains:

  > [username[:password]@]
  
  it is not considered a valid URL. This is not a great problem, given that basic URL authentication is almost never used since it lacks in terms of security (user and password are contained in clear text).

- [?querystring] and [#fragment] are not completely verified (both should be formatted as concatenation of parameter/value pairs, i.e.
  
  > param1=value1&param2=value2...

  This is not controlled, the routine just checks that those sections contain only allowed characters


## Configuration Files Handling Functions
Only IPv4 IP Addresses are supported (no IPv6)


## License Handling Functions
Those functions use platform dependent parameters to encrypt and decrypt the license file (mainly *hostname* and *hostid*). However, both parameters can be changed rather easily, so the check can be somehow circumvented. This could be a problem in some cases.