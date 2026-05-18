# Description of the libmixf library

# Index
- [Description of the libmixf library](#description-of-the-libmixf-library)
- [Index](#index)
- [Introduction](#introduction)
- [How to compile and install the library](#how-to-compile-and-install-the-library)
- [How to use the *libmixf* library into your C/C++ code](#how-to-use-the-libmixf-library-into-your-cc-code)
- [Library Functions Description](#library-functions-description)
  - [Introduction](#introduction-1)
  - [General Purpose Definitions](#general-purpose-definitions)
    - [New libmixf macros and data types](#new-libmixf-macros-and-data-types)
  - [File and File System Handling](#file-and-file-system-handling)
    - [Description](#description)
    - [New libmixf macros and data types](#new-libmixf-macros-and-data-types-1)
    - [New libmixf functions](#new-libmixf-functions)
      - [_Error check\_file\_name\_validity (char \*filename)_](#error-check_file_name_validity-char-filename)
      - [_Error retrieve\_path (char \*Path)_](#error-retrieve_path-char-path)
      - [_Error read\_files\_input\_dir (char \*inputdir, DirContent \*\*list\_of\_files)_](#error-read_files_input_dir-char-inputdir-dircontent-list_of_files)
      - [_void clear\_input\_file\_list(DirContent \*\*ptr)_](#void-clear_input_file_listdircontent-ptr)
    - [Examples](#examples)
      - [_check\_file\_name\_validity()_](#check_file_name_validity)
      - [_retrieve\_path(), read\_files\_input\_dir() and clear\_input\_file\_list()_](#retrieve_path-read_files_input_dir-and-clear_input_file_list)
  - [Time and Date Handling](#time-and-date-handling)
    - [Description](#description-1)
    - [New libmixf macros and data types](#new-libmixf-macros-and-data-types-2)
    - [New libmixf functions](#new-libmixf-functions-1)
      - [_void retrieve\_time\_date(char \*TimeDateString, char \*format)_](#void-retrieve_time_datechar-timedatestring-char-format)
      - [_time\_t get\_time\_stamp(char \*LogLine, char \*format)_](#time_t-get_time_stampchar-logline-char-format)
    - [Examples](#examples-1)
  - [String Handling](#string-handling)
    - [Description](#description-2)
    - [New libmixf functions](#new-libmixf-functions-2)
      - [_char \*filter\_and\_extract(char \*Line, const char \*Filter, const char \*StartWith)_](#char-filter_and_extractchar-line-const-char-filter-const-char-startwith)
      - [_bool strcmp\_wildcards(char \*string1, char \*string2, bool \*WildcardInStr1, bool \*ExactMatch)_](#bool-strcmp_wildcardschar-string1-char-string2-bool-wildcardInStr1-bool-exactmatch)
      - [_void remove\_blanks(char \*buf)_](#void-remove_blankschar-buf)
      - [_void copy\_remove\_blanks(char \*dst, char \*src)_](#void-copy_remove_blankschar-dst-char-src)
      - [_bool only\_digits(char \*inputstr)_](#bool-only_digitschar-inputstr)
      - [_bool check\_mail\_validity(char \*email)_](#bool-check_mail_validitychar-email)
      - [_bool check\_ipv4\_add\_validity(char \*ipAddrStr, uint32\_t \*ipAddr)_](#bool-check_ipv4_add_validitychar-ipaddrstr-uint32_t-ipaddr)
      - [_bool check\_fqdn\_validity(char \*fqdn)_](#bool-check_fqdn_validitychar-fqdn)
      - [_bool check\_url\_validity(char \*url)_](#bool-check_url_validitychar-url)
      - [_Error generate\_token(char \*token, char \*charset, int length)_](#error-generate_tokenchar-token-char-charset-int-length)
    - [Examples](#examples-2)
  - [Configuration Files Handling](#configuration-files-handling)
    - [Description](#description-3)
    - [New libmixf macros and data types](#new-libmixf-macros-and-data-types-3)
    - [New libmixf functions](#new-libmixf-functions-3)
      - [_void reset\_param\_list(void)_](#void-reset_param_listvoid)
      - [_Error init\_param\_list(int maxp)_](#error-init_param_listint-maxp)
      - [_Error add\_numerical\_param(char \*name, bool mand, int min, int max, int def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_](#error-add_numerical_paramchar-name-bool-mand-int-min-int-max-int-def-eventcode-evn1-eventcode-evn2-eventcode-evn3-eventcode-evn4)
      - [_Error add\_literal\_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_](#error-add_literal_paramchar-name-bool-mand-char-def-eventcode-evn1-eventcode-evn2-eventcode-evn3-eventcode-evn4)
      - [_Error add\_filename\_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_](#error-add_filename_paramchar-name-bool-mand-char-def-eventcode-evn1-eventcode-evn2-eventcode-evn3-eventcode-evn4)
      - [_Error add\_char\_param(char \*name, bool mand, char min, char max, char def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_](#error-add_char_paramchar-name-bool-mand-char-min-char-max-char-def-eventcode-evn1-eventcode-evn2-eventcode-evn3-eventcode-evn4)
      - [_Error add\_mail\_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_](#error-add_mail_paramchar-name-bool-mand-char-def-eventcode-evn1-eventcode-evn2-eventcode-evn3-eventcode-evn4)
      - [_Error add\_ipv4\_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_](#error-add_ipv4_paramchar-name-bool-mand-char-def-eventcode-evn1-eventcode-evn2-eventcode-evn3-eventcode-evn4)
      - [_Error add\_url\_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_](#error-add_url_paramchar-name-bool-mand-char-def-eventcode-evn1-eventcode-evn2-eventcode-evn3-eventcode-evn4)
      - [_Error parse\_cfg\_param\_file(char \*CfgFileName, uint16\_t \*totalline, EventList \*\*events)_](#error-parse_cfg_param_filechar-cfgfilename-uint16_t-totalline-eventlist-events)
      - [_Error clear\_event\_list(EventList \*\*ptr)_](#error-clear_event_listeventlist-ptr)
      - [_Error get\_num\_param\_value(char \*param, int \*value, bool \*prov)_](#error-get_num_param_valuechar-param-int-value-bool-prov)
      - [_Error get\_lit\_param\_value(char \*param, char \*value, bool \*prov)_](#error-get_lit_param_valuechar-param-char-value-bool-prov)
      - [_Error get\_fname\_param\_value(char \*param, char \*value, bool \*prov)_](#error-get_fname_param_valuechar-param-char-value-bool-prov)
      - [_Error get\_char\_param\_value(char \*param, char \*value, bool \*prov)_](#error-get_char_param_valuechar-param-char-value-bool-prov)
      - [_Error get\_mail\_param\_value(char \*param, char \*value, bool \*prov)_](#error-get_mail_param_valuechar-param-char-value-bool-prov)
      - [_Error get\_ipv4\_param\_value(char \*param, char \*value, bool \*prov)_](#error-get_ipv4_param_valuechar-param-char-value-bool-prov)
      - [_Error get\_url\_param\_value(char \*param, char \*value, bool \*prov)_](#error-get_url_param_valuechar-param-char-value-bool-prov)
    - [Examples](#examples-3)
  - [Log Handling](#log-handling)
    - [Description](#description-4)
    - [New libmixf functions](#new-libmixf-functions-4)
      - [_Error define\_log\_levels(uint8\_t numloglevels, uint8\_t defloglevel)_](#error-define_log_levelsuint8_t-numloglevels-uint8_t-defloglevel)
      - [_Error define\_level\_descr(uint8\_t level, char \*textdescr)_](#error-define_level_descruint8_t-level-char-textdescr)
      - [_uint8\_t get\_log\_level(void)_](#uint8_t-get_log_levelvoid)
      - [_Error set\_log\_level(uint8\_t level)_](#error-set_log_leveluint8_t-level)
      - [_Error define\_num\_events(uint8\_t maxevents)_](#error-define_num_eventsuint8_t-maxevents)
      - [_Error define\_event(EventCode event, uint8\_t level, char \*descr)_](#error-define_eventeventcode-event-uint8_t-level-char-descr)
      - [_Error open\_log(char \*BaseName, char \*format, bool RotateDaily)_](#error-open_logchar-basename-char-format-bool-rotatedaily)
      - [_void close\_log(void)_](#void-close_logvoid)
      - [_Error register\_event(EventCode event, char \*param1, char \*param2, char \*param3)_](#error-register_eventeventcode-event-char-param1-char-param2-char-param3)
    - [Examples](#examples-4)
  - [License Handling](#license-handling)
    - [Description](#description-5)
    - [New libmixf functions](#new-libmixf-functions-5)
      - [_Error check\_license(char \*LicenseFileName, char \*DecryptedString)_](#error-check_licensechar-licensefilename-char-decryptedstring)
      - [_Error create\_license(char \*String, char \*HostName, char \*HostId)_](#error-create_licensechar-string-char-hostname-char-hostid)
    - [Examples](#examples-5)
  - [Lock Handling](#lock-handling)
    - [Description](#description-6)
    - [New libmixf functions](#new-libmixf-functions-6)
      - [_bool check\_lock\_present(char \*Lock\_FileName)_](#bool-check_lock_presentchar-lock_filename)
      - [_Error set\_lock(char \*Lock\_FileName)_](#error-set_lockchar-lock_filename)
      - [_Error reset\_lock(char \*Lock\_FileName)_](#error-reset_lockchar-lock_filename)
    - [Examples](#examples-6)
  - [Counters Handling](#counters-handling)
    - [Description](#description-7)
    - [New libmixf macros and data types](#new-libmixf-macros-and-data-types-4)
    - [New libmixf functions](#new-libmixf-functions-7)
      - [_Error define\_scalar\_ctr\_num(uint16\_t numcounters)_](#error-define_scalar_ctr_numuint16_t-numcounters)
      - [_Error define\_vector\_ctr\_num(uint16\_t numcounters)_](#error-define_vector_ctr_numuint16_t-numcounters)
      - [_Error define\_scalar\_ctr(uint16\_t ctrId, uint8\_t ctrType, uint32\_t ctrInitial, char \*ctrName)_](#error-define_scalar_ctruint16_t-ctrid-uint8_t-ctrtype-uint32_t-ctrinitial-char-ctrname)
      - [_Error define\_vector\_ctr(uint16\_t ctrId, uint16\_t ctrInst, uint8\_t ctrType, uint32\_t ctrInitial, char \*ctrName, char \*instName)_](#error-define_vector_ctruint16_t-ctrid-uint16_t-ctrinst-uint8_t-ctrtype-uint32_t-ctrinitial-char-ctrname-char-instname)
      - [_Error set\_vector\_ctr\_inst\_name(uint16\_t ctrId, uint16\_t ctrInst, char \*instIdName)_](#error-set_vector_ctr_inst_nameuint16_t-ctrid-uint16_t-ctrinst-char-instidname)
      - [_Error define\_base\_dump(char \*baseDir, char \*baseTimeFormat, char \*baseTimes)_](#error-define_base_dumpchar-basedir-char-basetimeformat-char-basetimes)
      - [_Error define\_aggr\_dump(char \*aggrDir, char \*aggrTimeFormat, char \*aggrTimes)_](#error-define_aggr_dumpchar-aggrdir-char-aggrtimeformat-char-aggrtimes)
      - [_Error start\_counters(void)_](#error-start_countersvoid)
      - [_Error stop\_counters(void)_](#error-stop_countersvoid)
      - [_Error incr\_peg\_scalar\_ctr(uint16\_t ctrId)_](#error-incr_peg_scalar_ctruint16_t-ctrid)
      - [_Error incr\_peg\_vector\_ctr(uint16\_t ctrId, uint16\_t \*ctrInst)_](#error-incr_peg_vector_ctruint16_t-ctrid-uint16_t-ctrinst)
      - [_Error retrieve\_peg\_scalar\_ctr(uint16\_t ctrId, uint32\_t \*ctrBase, uint32\_t \*ctrAggr)_](#error-retrieve_peg_scalar_ctruint16_t-ctrid-uint32_t-ctrbase-uint32_t-ctraggr)
      - [_Error retrieve\_peg\_vector\_ctr(uint16\_t ctrId, uint16\_t ctrInst, uint32\_t \*ctrBase, uint32\_t \*ctrAggr)_](#error-retrieve_peg_vector_ctruint16_t-ctrid-uint16_t-ctrinst-uint32_t-ctrbase-uint32_t-ctraggr)
      - [_Error update\_roller\_scalar\_ctr(uint16\_t ctrId, short delta)_](#error-update_roller_scalar_ctruint16_t-ctrid-short-delta)
      - [_Error update\_roller\_vector\_ctr(uint16\_t ctrId, uint16\_t \*ctrInst, short delta)_](#error-update_roller_vector_ctruint16_t-ctrid-uint16_t-ctrinst-short-delta)
      - [_Error check\_and\_dump\_ctr(void)_](#error-check_and_dump_ctrvoid)
    - [Examples](#examples-7)
  - [Examples](#examples-8)
  - [Known Issues](#known-issues)
    - [URL validation](#url-validation)
    - [Configuration files](#configuration-files)
    - [License handling](#license-handling-1)




# Introduction
`libmixf` is a library written in C language, which can be linked (either statically or dynamically) to C/C++ programs to provide a set of mixed functions (hence the name) concerning:

- File and File System Handling
- Time and Date Handling
- String Handling
- Configuration Files Handling
- Log Handling
- License Handling
- Lock Handling
- Counters Handling

They are written as general-purpose functions, in order to be reused whenever needed.

The latest library version can be downloaded from author's [GitHub repository](https://github.com/Roberto-Mameli/libmixf.git).

In the following we provide a detailed description of the facilities offered by the library.


# How to compile and install the library
The library has been developed in Linux environment (RedHat, CentOS, Ubuntu), but since it relies on `POSIX` standards and `gcc` compiler and development toolkit, it is extremely likely that it can be easily ported to most UNIX based operating systems (just recompiling it). It can be compiled without problems with `glibc 2.12` or above.

Be aware that this library does not come with an automatically generated makefile (`cmake` or similar). It contains a manually written makefile, composed by a few lines, that has been successfully tested on RedHat and Debian distributions (RedHat, Centos, Ubuntu, etc.), and that can be easily adapted to other Linux based operating systems.

The library can either be cloned or downloaded from the GitHub repository:

- git clone from GitHub:

      git clone https://github.com/Roberto-Mameli/libmixf.git
      cd libmixf

- or download from GitHub, then:

      unzip libmixf-master.zip
      cd libmixf-master

After that, type the following commands:

    make all
    sudo make install

(be aware that `sudo` is not needed if logged as `root`).

The first command compiles the library and produces in the `libmixf` directory both the static and the shared libraries (respectively `libmixf.a` and `libmixf.so.x.y`).

The second command installs the libraries in the destination folders. Specifically, the header file `mixf.h` is copied into `/usr/local/include` (this path may differ depending on the Linux distribution).

Static library `libmixf.a` is copied into the `./lib` folder. Dynamic libraries, instead, are copied to `/usr/local/lib` path (or equivalent depending on the Linux distribution). Be aware that, in order to use shared libraries, this path shall be either configured in `/etc/ld.so.conf` or in environment variable `$LD_LIBRARY_PATH`.

The first time you install the libraries, a further command might be needed to configure dynamic linker run-time bindings:

    ldconfig

If you apply changes to the library source code and you want to recompile it from scratch, you can clean up all executables by typing:

    make clean


# How to use the *libmixf* library into your C/C++ code
After compiling and installing the `libmixf` library (see previous section), it can be linked statically or dynamically to C/C++ code.

This is obtained as follows:

- include the `mixf.h` header file

  ```c
  #include "mixf.h"
  ```

- link the executable by including either the shared or the static library `libmixf`


To compile a generic example file (let's say `example.c` ), simply type:

- for shared library linking:

      gcc -g -c -O2 -Wall -v –I/usr/local/include example.c
      gcc -g -o example example.c - lmixf

- for static linking:

      gcc -static example.c - I/usr/local/include -L. -lmixf - o example


In the previous command `- L.` means that the `libmixf.a` file is available in the same directory of the source code `example.c`; if this is not the case just replace the dot after `L` with the path to the library file.


# Library Functions Description

## Introduction

The `libmixf` library provides several functions, organized into distinct function families, specifically:

- File and File System Handling
- Time and Date Handling
- String Handling
- Configuration Files Handling
- Log Handling
- License Handling
- Lock Handling
- Counters Handling

All the function families share a few General Purpose Definitions, which are briefly described below.

In the following, we provide a comprehensive description of all the methods available in each function
family.

## General Purpose Definitions

### New libmixf macros and data types

The following type and macros are defined in header file `mixf.h`. Those definition shall not be changed or overwritten by the source code:

```c
typedef uint8_t Error;
```

This type definition is used to define the return type of most of the `libmixf` library functions. Allowed values for the `Error` type are specified by the following macro definitions:

```c
#define MIXFOK 0
#define MIXFKO 1
#define MIXFNOACCESS 2
#define MIXFFORMATERROR 3
#define MIXFPARAMUNKNOWN 4
#define MIXFWRONGDEF 5
#define MIXFOVFL 6
```

The above definitions apply to all the `libmixf` functions. For specific subsets of functions there may be further definitions; for such cases the relevant explanations are given in the corresponding paragraphs.


## File and File System Handling

### Description
This is a set of functions that provide some facilities for file handling. This family consists of 4 library calls:

- `check_file_name_validity()`
- `retrieve_path()`
- `read_files_input_dir()`
- `clear_input_file_list()`

### New libmixf macros and data types
The following macro provides the maximum allowed length for a filename. It takes its value from the platform dependent macro _FILENAME_MAX_, which is defined in _stdio.h_:

```c
#define MAXFILENAMELEN FILENAME_MAX
```

The following type, which is basically a list of filenames, is defined in `mixf.h` and is returned by one of the `libmixf` functions that are part of the "File and File System Handling" subset (specifically, it is the return type of `read_files_input_dir()` ):

```c
typedef struct Dir_Content
{
    char                filename[MAXFILENAMELEN];
    struct Dir_Content *next;
} DirContent;
```

### New libmixf functions
The following library functions provide some facilities to handle files.

#### _Error check_file_name_validity (char \*filename)_
This function takes an input string, representing a file/directory name to be checked, and returns `MIXFOK` if it represents a valid file name, `MIXFKO` otherwise. The string is considered a valid file name if it satisfies both conditions below:

- it starts with a letter, a digit, a dot a slash or an underscore, and
- it does not contain any of the following characters: " `|!"£$%()=?'^\[]*+@#;:,<>&` "

For example " `myfile` ", " `123.example` ", " `./myfile` " or " `/home/user/myfile` " are all valid file names, while " `my&file` " or " `myfile*` " are considered not valid.

#### _Error retrieve_path (char \*Path)_
This function provides a string that contains the path of the current working directory in the file system (typically, the path from which the executable was launched). The routine does not allocate memory for the string (i.e. the caller shall allocate the string first).

The call returns `MIXFKO` in case of errors (e.g. permission to read or search a component of the filename was denied), `MIXFOK` in any other case.

#### _Error read_files_input_dir (char \*inputdir, DirContent \*\*list\_of\_files)_
This function reads all the files in the directory specified in `inputdir` and returns the corresponding filenames in `list_of_files` (filenames are reported relative to input dir, i.e. without absolute path). Only files are considered, directories are discarded.

The output is constituted by a single linked list of elements of type `DirContent` (see above). The list is dynamically allocated within the routine and must be released after usage by means of `clear_input_file_list()` (see below) to avoid memory leakage.

The function returns also an `Error` value, specifically `MIXFKO` in case of errors (e.g. permission denied), `MIXFOK` in any other case.

#### _void clear_input_file_list(DirContent \*\*ptr)_
This function destroys the Input File List created by `read_files_input_dir()` and releases the corresponding memory. To avoid memory leakage, it must always be called when the list is no longer needed.

### Examples

#### _check_file_name_validity()_
The following piece of code takes an input string from the keyboard and states if it represents a valid file name or not.

```c
#include "mixf.h"

char filename [255];

printf ("Enter filename: ");
scanf ("%s",filename);

if ( (check_file_name_validity(filename)==MIXFOK) )
    printf ("Valid file name\n");
else
    printf ("Invalid file name\n");
```

#### _retrieve_path(), read_files_input_dir() and clear_input_file_list()_
The following example reads first the current directory, then provides the list of files contained within it.

```c
#include "mixf.h"
int main(int argc, char *argv[], char *envp[])
{
    char path[255];
    DirContent *ptr, *p;

    if ( retrieve_path(path)!=MIXFOK )
    {
        printf ("Unable to retrieve current path\n");
        exit (-1);
    }

    printf ("Current path is %s\n",path);

    if ( read_files_input_dir(path,&ptr)!=MIXFOK )
    {
        printf ("Unable to read directory content\n");
        exit (-1);
    }
    
    printf ("This is the list of files in the current path:\n\n");
    
    for (p=ptr; p!=NULL; p=p->next)
        printf ("\t%s\n",p->filename);
    
    clear_input_file_list(&ptr);
    
    exit (0);

}
```

## Time and Date Handling

### Description

This family provides functions to handle formatted time and date values.
It exposes two library calls:

- `retrieve_time_date()`
- `get_time_stamp()`

### New libmixf macros and data types

The following macro defines the maximum length of a timestamp string:

```c
#define MAXDATETIMELEN 32
```

### New libmixf functions

#### _void retrieve_time_date(char \*TimeDateString, char \*format)_
Retrieves the current date and time into `TimeDateString` using the format specified in `format`. The format string follows standard `strftime()` syntax (for example `%c`, `%T`, `%F`, etc.). The caller must allocate the output buffer before calling this function.

#### _time_t get_time_stamp(char \*LogLine, char \*format)_
Parses a timestamp from the beginning of `LogLine` according to the format string in `format` (see `strptime()` for supported format specifiers). It returns the corresponding `time_t` value, or `0` on format mismatch.

### Examples

```c
#include "mixf.h"
#include <stdio.h>
#include <time.h>

#define FORMAT "%d/%m/%Y %H:%M:%S"

int main(void)
{
    char date_time[MAXDATETIMELEN];
    time_t ts;

    retrieve_time_date(date_time, FORMAT);
    printf("Current date and time is %s", date_time);

    ts = get_time_stamp(date_time, FORMAT);
    printf("Seconds since epoch: %ld", (long)ts);

    return 0;
}
```

## String Handling

### Description

This family provides string utilities for filtering, wildcard matching,
blank removal, digit checking, address/URL validation and token generation.

### New libmixf functions

This family provides several functions aimed at specific string operations:

- `filter_and_extract()`
- `strcmp_wildcards()`
- `remove_blanks()`
- `copy_remove_blanks()`
- `only_digits()`
- `check_mail_validity()`
- `check_ipv4_add_validity()`
- `check_fqdn_validity()`
- `check_url_validity()`
- `generate_token()`

#### _char \*filter_and_extract(char \*Line, const char \*Filter, const char \*StartWith)_
Searches the input string `Line` for the filter string `Filter` and, when found, returns a pointer to the first occurrence of `StartWith` inside `Line`. Returns `NULL` if the filter does not match or `StartWith` is not found.

#### _bool strcmp_wildcards(char \*string1, char \*string2, bool \*WildcardInStr1, bool \*ExactMatch)_
Compares two strings with wildcard support in `string2`. Supported wildcards are `*` (zero or more characters) and `?` or `!` (exactly one character).
The function returns `false` when the strings match and `true` otherwise (note that it follows the same convention of `strcmp()`, i.e. provides 0 in case of match). The output parameter `WildcardInStr1` is set to `true` when `string1` contains a wildcard character; `ExactMatch` is set to `true` when the match is exact (i.e. no wildcard expansion was needed).

#### _void remove_blanks(char \*buf)_
Removes blanks, tabs and newlines from `buf` in-place.

#### _void copy_remove_blanks(char \*dst, char \*src)_
Copies `src` into `dst` while removing blanks, tabs and newlines. The source string `src` is preserved.

#### _bool only_digits(char \*inputstr)_
Returns `true` when `inputstr` contains only digit characters, `false` otherwise.

#### _bool check_mail_validity(char \*email)_
Returns `true` when `email` is a syntactically valid e-mail address,
`false` otherwise. An e-mail is considered syntactically correct if it is formatted as follows:

```
name@domain.tld
```
and all the following conditions apply (tld = top level domain):

- there is only one '@' character;
- name is not empty;
- name does not begin or end with a dot ('.');
- name contains only valid characters (uppercase and lowercase letters, digits, dot '.' , hyphen '-' and underscore '_');
- the substring at the right of '@' is not empty;
- the substring at the right of '@' contains only valid characters (uppercase and lowercase letters, digits, dot '.' , hyphen '-' and underscore '_');
- the substring at the right of '@' contains at least a dot '.'
- the substring at the right of '@' does not contain consecutive dots;

Further, it is assumed that the input string is shorter than 128 characters, otherwise it is considered not valid (this is an implementation assumption, however it seems reasonable in almost all practical situations).

#### _bool check_ipv4_add_validity(char \*ipAddrStr, uint32\_t \*ipAddr)_
Returns `true` when `ipAddrStr` is a valid IPv4 address (a string formatted as `a.b.c.d`, where `a`, `b`, `c`, `d` are integers between `0` and `255`). If valid, `ipAddr` receives the corresponding 32-bit representation.

#### _bool check_fqdn_validity(char \*fqdn)_
Returns `true` when `fqdn` is a syntactically valid Fully Qualified Domain Name (FQDN), `false` otherwise. It is assumed that the input string contains up to 256 characters; inputs exceeding this length are considered not valid.

#### _bool check_url_validity(char \*url)_
Returns `true` when `url` is a syntactically valid URL,
`false` otherwise. The function supports up to 256 characters and has some
minor limitations in URL authentication, query and fragment verification.
Specifically, for a string to be considered a valid URL, several conditions must be satisfied. First, it must begin with a protocol, followed by colon and double slash ('://')

```
[protocol://]
```
The following list provides supported protocols:

```
mailto
http
https
ftp
ftps
sftp
gopher
news
telnet
aim
```
In case of " `mailto://` ", the URL is considered syntactically correct if the protocol indication is followed by a valid e-mail (see `check_mail_validity()` above).

For all remaining protocols, the general URL format is the following:

```
[protocol://][username[:password]@]host[:port][</path>][?querystring][#fragment]
```
The `host` part shall either be a valid IPv4 address (see `check_ipv4_add_validity()` above) or alternatively satisfy the following constraints:

- host is not empty
- host does not begin or end with a dot ('.')
- host contains only valid characters (uppercase and lowercase letters, digits, dot '.' , hyphen '-' and underscore '_');
- host contains at least a dot '.'
- host does not contain consecutive dots;

The `port` parameter is optional, if present it shall be an integer between `0` and `65535`.

The `path` section is also optional, if present it shall satisfy the following constraints:

- it does not begin with dot ('.') or slash ('/');
- it is composed by the concatenation of valid strings separated by slashes ('/');
- valid strings mentioned in the previous point are those which do not contain any of the following characters: " `|!"£$%()=?'^\[]*+@#;:,<>&` "

This version does not implement full URL verification, since it has some minor limitations:

- it does not support URL authentication, i.e. if the string contains `[username[:password]@]` the URL is not recognized as valid;
- `[?querystring]` and `[#fragment]` are not completely verified (both should be formatted as concatenation of parameter/value pairs, i.e. `param1=value1&param2=value2`.
    This is not controlled, the function just checks that those sections contain only allowed characters).



#### _Error generate_token(char \*token, char \*charset, int length)_
Generates a random token of `length` characters extracted from `charset` and writes it into the buffer `token` (which must be allocated and subsequently released by the caller). There is no specific length limitation on the token; `length` can be set arbitrarily high, provided that `token` has sufficient space allocated.
Returns `MIXFOK` on success, `MIXFKO` on error (e.g. empty charset or `length` <= 0).

### Examples

The following code snippet shows usage of `filter_and_extract()` library call to analyze two example log lines. It also shows another usage example for the `get_time_stamp()` function (see Time and Date Handling function family):
```c
#include "mixf.h"
#include <stdio.h>

int main(void)
{
    char src[] = "23/06/2021 11:33:42 - ERROR 127 - INTERNAL ERROR (mycode.c:1234)";
    char *ptr;
    time_t ts;

    ts = get_time_stamp(src, "%d/%m/%Y %H:%M:%S");
    printf("Timestamp: %ld", (long)ts);

    ptr = filter_and_extract(src, "INTERNAL", "(");
    if (ptr != NULL)
        printf("Extracted: %s", ptr);
    else
        printf("No match");

    return 0;
}
```

The following example takes two input strings and provides as output the result of the comparison through `strcmp_wildcards()`:

```c
#include "mixf.h"

#define STRINGLEN 100

int main()
{
    char string1[STRINGLEN],
    string2[STRINGLEN];
    bool WildcardInStr1, ExactMatch;
    
    printf ("Enter string1: ");
    scanf ("%s",string1);

    printf ("Enter string2: ");
    scanf ("%s",string2);

    printf ("Compare string1 (%s) with string2 (%s).\n",string1,string2);
    if ( strcmp_wildcards(string1, string2, &WildcardInStr1, &ExactMatch) == false )
    {
        printf ("Strings match\n");
        if (WildcardInStr1)
            printf ("\tString1 contains a wildcard. comparison between strings is based on exact match\n");
        if (ExactMatch)
            printf ("\tString1 and String2 match exactly\n");
    }
    else
        printf ("Strings do not match\n");
}
```

The example below shows the different behaviour of `remove_blanks()` and `copy_remove_blanks()` library calls:

```c
#include "mixf.h"
#include <stdio.h>

int main(void)
{
    char src[] = "Thi s Is An Examp le Str i ng";
    char dst[255];

    copy_remove_blanks(dst, src);
    printf("Source: %s", src);
    printf("Destination: %s", dst);

    remove_blanks(src);
    printf("In-place: %s", src);

    return 0;
}
```

The code below takes an input string and recognizes whether it contains a valid e-mail, a valid URL or a valid IPv4 address, or any of them:

```c
#define STRINGLEN 65

char String[STRINGLEN];
uint32_t ipaddr;

printf ("Please insert either a valid e-mail, a valid URL or a valid IPv4 address: ");
scanf ("%s",String);

if (check_mail_validity(String))
    {
        printf ("OK, this is a valid e-mail.");
        exit (0);
    }

if (check_ipv4_add_validity(String, &ipaddr))
    {
        printf ("OK, this is a valid IPv4 address.");
        /* . in this case ipaddr contains the 32 bit address */
        exit (0);
    }
if (check_url_validity(String))
    {
        printf ("OK, this is a valid URL.");
        exit (0);
    }
```

Finally, the following code generates a random token of `TOKENLEN` characters extracted by a `charset` composed of digits and capital letters:

```c
#define TOKENLEN 12

char token[TOKENLEN+1];
char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

generate_token(token, charset, TOKENLEN);
printf ("The random generated token is: %s\n",token);
```

## Configuration Files Handling

### Description

This set of functions provides methods and facilities to manage configuration files parsing and parameters handling. A configuration file is a text file, composed by several lines formatted as follows:

```
Parameter = Value
```

Comments beginning with `#` are allowed.

The configuration API uses the following snake_case identifiers:

- `reset_param_list()`
- `init_param_list()`
- `add_numerical_param()`
- `add_literal_param()`
- `add_filename_param()`
- `add_char_param()`
- `add_mail_param()`
- `add_ipv4_param()`
- `add_url_param()`
- `parse_cfg_param_file()`
- `clear_event_list()`
- `get_num_param_value()`
- `get_lit_param_value()`
- `get_fname_param_value()`
- `get_char_param_value()`
- `get_mail_param_value()`
- `get_ipv4_param_value()`
- `get_url_param_value()`

These functions return `Error` codes such as `MIXFOK`, `MIXFKO`, `MIXFNOACCESS`,
`MIXFFORMATERROR`, `MIXFPARAMUNKNOWN` and `MIXFWRONGDEF`.

### New libmixf macros and data types

```c
#define UNDEFINED 255
typedef uint8_t EventCode;
```

Configuration parsing also uses the `EventList` linked list type, which is
returned by `parse_cfg_param_file()` and must be freed with
`clear_event_list()`.

```c
typedef struct eventlist
{
    EventCode        event;
    uint16_t         line;
    struct eventlist *next;
} EventList;
```

### New libmixf functions

#### _void reset_param_list(void)_
Resets internal parameter definitions before initializing the allowed parameter list.

#### _Error init_param_list(int maxp)_
Defines the maximum number of parameters (1-255) through `maxp`. If not called, the default is 8. Returns `MIXFOK`, `MIXFKO` if called twice without reset, and `MIXFOVFL` when the requested maximum is out of range.

#### _Error add_numerical_param(char \*name, bool mand, int min, int max, int def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_
Adds a numerical parameter to the list of parameters allowed in the configuration file.

`name` is the parameter name; `mand` is the Mandatory flag and must be set to `true` if the parameter is mandatory, `false` if it is optional in the configuration file.

`min`, `max` and `def` are three integers that represent respectively the minimum, maximum and default value for the parameter (please observe that the default value is actually meaningful only if `mand` is `false`).

The last four parameters are event codes that are associated respectively to the following events:

    (1) evn1: Event corresponding to a Mandatory parameter not provisioned;
    (2) evn2: Event corresponding to an Optional parameter not provisioned (default value used instead);
    (3) evn3: Event corresponding to a parameter that is redefined (at least twice);
    (4) evn4: Event corresponding to a parameter value out of range or malformed (e.g. not a number).

Use `UNDEFINED` for those events that are not meaningful (e.g. `evn1` does not make sense if `mand` is `false`).

This function provides `MIXFOK` if parameter is successfully added to the list, `MIXFOVFL` if the maximum number of allowed parameters is exceeded (max value defined in `init_param_list()`), `MIXFWRONGDEF` if the condition `min <= def <= max` is not satisfied and finally `MIXFKO` for any other error (e.g. parameter name empty).


#### _Error add_literal_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_
This function adds a literal parameter to the list of parameters allowed in the configuration file. Function arguments are similar to those described in `add_numerical_param()`, with the notable difference that the min and max values are not present.

#### _Error add_filename_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_
This function adds a filename parameter to the list of parameters allowed in the configuration file. A filename parameter is similar to a literal parameter; the only difference is that filename parameters can only get values that are valid filenames (see also `check_file_name_validity()` for details about valid filenames characteristics).

#### _Error add_char_param(char \*name, bool mand, char min, char max, char def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_
This function adds a char parameter to the list of parameters allowed in the configuration file. A CHAR Parameter is a single character string and shall be enclosed within apices in the configuration file (e.g. `"a"`) otherwise it is considered malformed. Apices are needed to ensure that also the blank character can be easily identified (`" "`). Function arguments shall be interpreted as in the case of `add_numerical_param()` (in this case `min` and `max`, representing respectively the minimum and the maximum value, shall be provided).

#### _Error add_mail_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_
This function adds an e-mail parameter to the list of parameters allowed in the configuration file. An e-mail parameter is similar to a literal parameter, but it can only get values that are valid e-mail addresses (see also `check_mail_validity()` for details).

#### _Error add_ipv4_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_
Adds an IPv4 parameter to the list of parameters allowed in the configuration file. An IPv4 parameter is similar to a literal parameter, but it is constrained to assume values that are valid IPv4 addresses (i.e. values must satisfy `check_ipv4_add_validity()`).

#### _Error add_url_param(char \*name, bool mand, char \*def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)_
This function adds a URL parameter to the list of parameters allowed in the configuration file. A URL parameter is similar to a literal parameter, with the notable difference that URL parameters can only get values that are valid URLs (see `check_url_validity()` for details).

#### _Error parse_cfg_param_file(char \*CfgFileName, uint16\_t \*totalline, EventList \*\*events)_
Parses the configuration file `CfgFileName`. On success, sets the total line count in `totalline` and returns an `EventList *` in `events`. `clear_event_list()` must be used to free the returned list.

As previously outlined, this file shall be composed of lines with the following format:

```
Parameter = Value
```

Comments are allowed as well (`#`), both at the beginning or at the end of a line.

If the file does not respect this layout, it is considered wrongly formatted.

Allowed parameters shall be defined before calling this routine through the `reset_param_list()` and the
various `add_xxx_param()` library calls previously defined.

This function may provide back the following results:

- `MIXFOK`
    the file was successfully opened, it is correctly formatted and does not contain unrecognized
    parameters. In this case `totalline` provides back the total number of lines in the
    configuration file, while `events` is a pointer to a pointer to a list of events found during parsing
    (for each event there is the corresponding line number). Please see below for a detailed
    explanation of events found during parsing;
- `MIXFNOACCESS`
    the function is not able to open the configuration file (it may not exist or the user may not have
    read permission). In this case, both `totalline` and `events` are not meaningful;
- `MIXFFORMATERROR`
    the file is wrongly formatted (it does not respect the layout specified above). `totalline`
    provides back the line in which the error occurred, while `events` is null;
- `MIXFPARAMUNKNOWN`
    the file contains a parameter that is not recognized, i.e. which was not defined through the
    `add_xxx_param()` routines previously defined. `totalline` provides back the line in which
    the error occurred, while `events` is null.

In case of successful parsing, `parse_cfg_param_file()` provides `MIXFOK` and a single linked list of events in `events`. The list is allocated by `parse_cfg_param_file()` itself, and is composed of elements
defined as `EventList` structures (see section [New libmixf macros and data types](#new-libmixf-macros-and-data-types-3)), where each element
contains an event code and a line number. The events that are reported in this list are those specified at
parameter definition. For example, if a parameter is redefined, the list contains an event that identifies the
parameter redefinition and the line number in the configuration file in which the parameter is repeated. If
the parameter is missing, the event list contains an event that notifies this, with associated line number 0.


The list of events allocated by this function shall be explicitly released by `clear_event_list()`, in order to avoid
memory leakage.

#### _Error clear_event_list(EventList \*\*ptr)_
Frees the event list allocated by `parse_cfg_param_file()`.

#### _Error get_num_param_value(char \*param, int \*value, bool \*prov)_
This routine retrieves the value of the numerical parameter whose name is given by `param` and provides it to the caller through `value`. It provides return code `MIXFOK` if the parameter is successfully read,
`MIXFNOACCESS` if the parameter is defined but not actually read from the configuration file (i.e. `parse_cfg_param_file()` was not invoked), `MIXFPARAMUNKNOWN` if the parameter is not known (i.e. it has not been defined through `add_numerical_param()`). The output parameter `prov` is a flag that is `true` if the parameter was actually provisioned in the configuration file, `false` if it wasn't (i.e. in case of optional parameter that assumes its default value).

#### _Error get_lit_param_value(char \*param, char \*value, bool \*prov)_
This routine provides in `value` the value of the literal parameter whose name is given by `param`. Behavior and return codes are exactly the same as `get_num_param_value()`. Observe that the buffer used in `value` to host the return parameter must be allocated by the caller.

#### _Error get_fname_param_value(char \*param, char \*value, bool \*prov)_
This routine provides in `value` the value of the filename parameter whose name is given by `param`. Behavior and return codes are exactly the same as `get_lit_param_value()`. Observe that the buffer used in `value` to host the return parameter must be allocated by the caller.

#### _Error get_char_param_value(char \*param, char \*value, bool \*prov)_
This routine provides in `value` the value of the char parameter whose name is given by `param`. Behavior and return codes are exactly the same as `get_num_param_value()`.

#### _Error get_mail_param_value(char \*param, char \*value, bool \*prov)_
This routine provides in `value` the value of the e-mail parameter whose name is given by `param`. Behavior and return codes are exactly the same as `get_lit_param_value()`. Observe that the buffer used in `value` to host the return parameter must be allocated by the caller.

#### _Error get_ipv4_param_value(char \*param, char \*value, bool \*prov)_
This routine provides in `value` the value of the IPv4 parameter whose name is given by `param`. Behavior and return codes are exactly the same as `get_lit_param_value()`. Observe that the buffer used in `value` to host the return parameter must be allocated by the caller.

#### _Error get_url_param_value(char \*param, char \*value, bool \*prov)_
This routine provides in `value` the value of the URL parameter whose name is given by `param`. Behavior and return codes are exactly the same as `get_lit_param_value()`. Observe that the buffer used in `value` to host the return parameter must be allocated by the caller.

### Examples

See `examples/bin/Example1` for a complete usage example that combines
parameter definition, configuration parsing, and license handling.

## Log Handling

### Description

Log handling lets applications define log levels, event templates and register
log entries. Logs can be written into timestamped files with optional daily rotation.

The log API uses these functions:

- `define_log_levels()`
- `define_level_descr()`
- `get_log_level()`
- `set_log_level()`
- `define_num_events()`
- `define_event()`
- `open_log()`
- `close_log()`
- `register_event()`

### New libmixf functions

#### _Error define_log_levels(uint8\_t numloglevels, uint8\_t defloglevel)_
Defines the maximum number of log levels **_(1-8)_** through `numloglevels`, and the default log level through `defloglevel` (between **_0_** and **_N-1_**), where **_N_** is the value assigned to `numloglevels`. Returns `MIXFKO` for invalid parameters, `MIXFOK` otherwise.

Please remember that the higher is the log level, the lower is the severity (i.e. level **_0_** events represents very critical issues, while level **_7_** is for very minor events).
This function is optional, if not invoked the number of severity levels is set to **_1_** by default.

#### _Error define_level_descr(uint8\_t level, char \*textdescr)_
Associates the textual description `textdescr` with severity level `level`, specified in the range between **_0_** and **_N-1_**, being **_N_** the number of log levels defined by `define_log_levels()`.

Returns `MIXFKO` for invalid parameters, `MIXFOK` otherwise.

#### _uint8_t get_log_level(void)_
Returns the current log level.

#### _Error set_log_level(uint8\_t level)_
Sets the current log level to `level`, specified in the range between **_0_** and **_N-1_**, being **_N_** the number of log levels defined by `define_log_levels()`.

Before invocation, the Log Level is set to the default value specified by the second parameter of `define_log_levels()` (or **_0_**, i.e. the highest severity, in case `define_log_levels()` is not invoked).

#### _Error define_num_events(uint8\_t maxevents)_
Defines the maximum number of event codes (1-255) through `maxevents`. Must be called before
`define_event()`.

#### _Error define_event(EventCode event, uint8\_t level, char \*descr)_
Defines event code `event` in the range **_(1, M-1)_**, where **_M_** is the number of events defined in `define_num_events()`. It associates the event with severity level `level` and description template `descr`. The template may include placeholders `%1`, `%2`, `%3` for runtime parameter substitution.

If the function is called multiple times with the same event code, each invocation overwrites previous data (only the last definition applies).

This function provides **_MIXFOK_** in case of success, **_MIXFOVFL_** if `event` or `level` are out of allowed ranges, **_MIXFFORMATERROR_** if `descr` does not respect the format specified above (e.g. it contains an invalid placeholder, such as " **_%4_** " or a valid placeholder repeated twice).

#### _Error open_log(char \*BaseName, char \*format, bool RotateDaily)_
Opens a log file using `BaseName` as basename and `format` as an optional timestamp format string, according to the naming pattern `<BaseName>_<timestamp>.log`.

`format` is a string that shall be defined according to the format described in `strftime()` man page (e.g. `"%F"` for date in the format `YYYY-MM-DD`, etc.).

If `format` is empty or `NULL`, the log file is opened as `<BaseName>.log`. When `RotateDaily` is `true`, the file is automatically closed and re-opened daily at 00:00.

This function provides `MIXFOK` in case of success, `MIXFKO` if the log is already open, `MIXFFORMATERROR` if `BaseName` is not a valid file name, `MIXFNOACCESS` if the filename cannot be opened.

#### _void close_log(void)_
Closes the currently open log file.

#### _Error register_event(EventCode event, char \*param1, char \*param2, char \*param3)_
Writes a log entry for `event` using optional parameter strings `param1`, `param2`, `param3`.

This is done only if the event severity (specified at event definition) is at least equal to the current log level and if the log file is open, otherwise the call returns without effect.

A typical line registered in the log appears as follows:

```
hh:mm:ss – <SeverityDescr>(<Severity>) – Event <EventCode> - <EventDescr>
```
for example:

```
14:12:47 – CRITICAL(0) – Event 18 – Fatal Error in Process Id 2564
```
For events that contain parameters in the textual description (see `define_event()` above), they can be specified as strings in `param1` (`"%1"` placeholder), `param2` (`"%2"` placeholder) and `param3` (`"%3"` placeholder).

Returns `MIXFOK`, `MIXFKO` if the log is not open, or `MIXFNOACCESS` if the log
cannot be reopened after rotation.

### Examples
The following code snippet defines 2 different severity levels ( `ERROR` and `INFO` ) and `4` events, with event codes from `0` to `3`. Each event is assigned a proper severity level and a textual Description. Event `1` description also contains a variable parameter, identified through the " `%1` " placeholder in the Description.

After that, log file is defined and opened. The file name is characterized by the following format:

```
/logs/ApplicationLog_ddmmyyyy.log
```
and is configured with daily rotation enabled (i.e. it is closed and re-opened every day at 00:00)
```c
#include "mixf.h"
#include <stdio.h>

int main(void)
{
    Error res;

    define_log_levels(2, 1);
    define_level_descr(0, "ERROR");
    define_level_descr(1, "INFO");
    define_num_events(4);

    define_event(0, 1, "Operation completed successfully");
    define_event(1, 0, "Unable to start process id %1");
    define_event(2, 1, "All tasks completed - Exiting");
    define_event(3, 0, "Missing Input Data");

    set_log_level(1);
    res = open_log("/logs/ApplicationLog", "%d%m%Y", true);
    if (res != MIXFOK) {
        fprintf(stderr, "Unable to open log file: %d", res);
        return 1;
    }

    register_event(1, "2564", NULL, NULL);
    close_log();
    return 0;
}
```

## License Handling

### Description

License handling includes two functions to create and verify basic encrypted
license tokens based on host-specific parameters.

- `check_license()`
- `create_license()`

Both functions return `Error` codes.

### New libmixf functions

#### _Error check_license(char \*LicenseFileName, char \*DecryptedString)_
Reads the first line of the license file `LicenseFileName`, decrypts it using host-specific parameters (hostname and hostid), and writes the decrypted string into `DecryptedString`.
Returns `MIXFOK` on success or `MIXFNOACCESS` when the file cannot be opened.

#### _Error create_license(char \*String, char \*HostName, char \*HostId)_
Encrypts the clear text string `String` using `HostName` and `HostId` as host-specific parameters and writes the encrypted result back into `String`. Returns `MIXFOK` on success or `MIXFKO` on error (e.g. if `HostName` is empty or `HostId` is not an 8-digit hex number starting with `0x`).

### Examples

See `examples/bin/Example1` for a usage example.

## Lock Handling

### Description

This family provides a basic filesystem lock mechanism via zero-byte files.

- `check_lock_present()`
- `set_lock()`
- `reset_lock()`

### New libmixf functions

#### _bool check_lock_present(char \*Lock\_FileName)_
The lock is an empty file whose file name is specified by `Lock_FileName`. This function returns `true` when the lock file exists, `false` otherwise.

#### _Error set_lock(char \*Lock\_FileName)_
Creates the lock file specified by `Lock_FileName` and returns `MIXFOK` on success, `MIXFKO` otherwise.

#### _Error reset_lock(char \*Lock\_FileName)_
Removes the lock file specified by `Lock_FileName` and returns `MIXFOK` on success, `MIXFKO` in any other case.

### Examples

```c
#include "mixf.h"
#include <stdio.h>
#include <unistd.h>

#define LOCKFILE "./.exchange.file.lock"

int main(void)
{
    if (check_lock_present(LOCKFILE)) {
        printf("Lock is already present");
        return 1;
    }

    if (set_lock(LOCKFILE) != MIXFOK) {
        fprintf(stderr, "Unable to set lock");
        return 1;
    }

    /* ... work ... */

    if (reset_lock(LOCKFILE) != MIXFOK) {
        fprintf(stderr, "Unable to reset lock");
        return 1;
    }

    return 0;
}
```

## Counters Handling

### Description

Counters handling supports scalar and vector counters for runtime metrics collection.
Counters are dumped periodically into CSV files at configurable intervals.

Library `libmixf` supports 2 different counter types:

- Peg Counters
    Peg Counters are characterized by the following properties:
       o they have initial value always set to 0;
       o they can only increase;
       o they are reset every time that counters are dumped to file (i.e. at each base/aggregate interval).

- Roller Counters
    Roller Counters are slightly different from Peg Counters:
       o they can have a non-null initial value;
       o they can increase and decrease (but never become negative);
       o when they are dumped to file, they are not reset.
    Roller Counters might be useful to keep track of other types of relevant information during the application lifetime (see example below).

_Example:_

_An example of Roller Counter applicable for a RESTful API server may be represented by
the current number of open connections. The value of this counter collected at each base interval represents
a snapshot of the number of active clients at the moment. This value can increase or decrease over time
according to the number of clients that open/close connections._

A further classification distinguishes counters into two categories:

- Scalar Counters
    Scalar Counters are global counters, i.e. they refer to the whole application and are not related to a specific instance/object.
- Vector Counters
    Vector counters are collected separately for a set of objects/instances of the same type within the application.

_Example:_

_To clarify the difference between Scalar and Vector counters, let's consider a RESTful API server.
The Total Number of Received Requests is a typical example of Scalar
Counter. However, if we introduce the Total Number of Received Requests for each specific Client, this
represents an example of Vector Counter. In this case, we have a class of counters (i.e. Total Number of
Received Requests per Client), and an instance of the Counter dedicated to each specific Client._

`libmixf` library supports up to **_1024_** Scalar Counters and up to **_1024_** Vector Counters, with the further
constraint that the total number of Vector Counter instances shall be not greater than **_65536_** (i.e. **_2^16_**). This
means that the application can collect e.g. **_4_** Vector Counters (each of **_16384_** instances), **_1024_** Vector
Counters (each of **_64_** instances) or even **_32768_** Vector Counters (each of **_2_** instances).

Each counter maintains two independent accumulators:

- **Base value**: accumulated since the last base dump interval; reset to zero after each base dump for `PEGCTR` counters.
- **Aggregated value**: accumulated since the last aggregated dump interval; reset to zero after each aggregated dump for `PEGCTR` counters. `ROLLERCTR` counters are never reset in either accumulator.

The counter API uses these identifiers:

- `define_scalar_ctr_num()`
- `define_vector_ctr_num()`
- `define_scalar_ctr()`
- `define_vector_ctr()`
- `set_vector_ctr_inst_name()`
- `define_base_dump()`
- `define_aggr_dump()`
- `start_counters()`
- `stop_counters()`
- `incr_peg_scalar_ctr()`
- `incr_peg_vector_ctr()`
- `retrieve_peg_scalar_ctr()`
- `retrieve_peg_vector_ctr()`
- `update_roller_scalar_ctr()`
- `update_roller_vector_ctr()`
- `check_and_dump_ctr()`

### New libmixf macros and data types

```c
#define PEGCTR    0
#define ROLLERCTR 1
```

These macros are used to specify the counter type when calling `define_scalar_ctr()` and `define_vector_ctr()`.

### New libmixf functions
In order to collect counters within an application, you need to follow the steps outlined below:

1. Define relevant counters;
2. Define base and aggregated (if any) intervals;
3. Start counters;
4. Update counters through corresponding library calls upon relevant events occurrence;
5. Dump counters files;
6. Stop counters when the application is terminated.

Library calls take care of all those actions, as better explained below.


#### _Error define_scalar_ctr_num(uint16\_t numcounters)_

Defines the maximum number of Scalar Counters that the application intends to use. `numcounters` specifies the desired number and shall be in the range `[0, 1024]`.

Upon successful execution, all internal Scalar Counter structures are reset and any previous counter definition is lost. This function **must be called before** `start_counters()`: if invoked after counters have been started, it returns `MIXFKO` immediately without modifying any internal state.

Possible return values:
- `MIXFOK`: the maximum number of Scalar Counters has been defined successfully.
- `MIXFKO`: `numcounters` exceeds the allowed range, or `start_counters()` has already been called.


#### _Error define_vector_ctr_num(uint16\_t numcounters)_

Defines the maximum number of Vector Counters that the application intends to use. `numcounters` specifies the desired number and shall be in the range `[0, 1024]`.

Be aware that the library supports at most **65536** total Vector Counter instances across all defined Vector Counters. This constraint is not enforced here but will be checked later in `define_vector_ctr()`.

Upon successful execution, all internal Vector Counter structures are reset and any previous counter definition is lost. As with `define_scalar_ctr_num()`, this function **must be called before** `start_counters()`.

Possible return values:
- `MIXFOK`: the maximum number of Vector Counters has been defined successfully.
- `MIXFKO`: `numcounters` exceeds the allowed range, or `start_counters()` has already been called.


#### _Error define_scalar_ctr(uint16\_t ctrId, uint8\_t ctrType, uint32\_t ctrInitial, char \*ctrName)_

Configures a specific Scalar Counter. The four parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Scalar Counter to define, in the range `[0, M-1]` where `M` is the value previously set through `define_scalar_ctr_num()`.
- **`ctrType`** (`uint8_t`): either `PEGCTR` or `ROLLERCTR` (see the macro definitions above).
- **`ctrInitial`** (`uint32_t`): meaningful only for `ROLLERCTR` counters, where it sets the starting value for both the base and the aggregated accumulator. For `PEGCTR` counters this parameter is ignored and the counter is always initialised to 0.
- **`ctrName`** (`char *`): a descriptive string used as the column header in the CSV output files. Names longer than 32 characters are silently truncated.

This function **must be called before** `start_counters()`.

Possible return values:
- `MIXFOK`: the Scalar Counter has been defined successfully.
- `MIXFKO`: `ctrId` is outside the allowed range, `ctrType` is invalid, or `start_counters()` has already been called.


#### _Error define_vector_ctr(uint16\_t ctrId, uint16\_t ctrInst, uint8\_t ctrType, uint32\_t ctrInitial, char \*ctrName, char \*instName)_

Configures a specific Vector Counter, i.e. a family of counters collected independently for each of a set of homogeneous objects or instances. The six parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Vector Counter to define, in the range `[0, N-1]` where `N` is the value previously set through `define_vector_ctr_num()`.
- **`ctrInst`** (`uint16_t`): the number of objects/instances for which this counter shall be collected independently. Must be at least 1. The cumulative total of instances across all defined Vector Counters cannot exceed **65536**; if this limit would be exceeded the function returns `MIXFOVFL`.
- **`ctrType`** (`uint8_t`): either `PEGCTR` or `ROLLERCTR`.
- **`ctrInitial`** (`uint32_t`): meaningful only for `ROLLERCTR` counters. The specified value is applied uniformly to **all instances** of this counter. For `PEGCTR` counters this parameter is ignored and all instances are initialised to 0.
- **`ctrName`** (`char *`): a descriptive name for the counter class (e.g. `"Total Bytes Sent"`). Names longer than 32 characters are silently truncated.
- **`instName`** (`char *`): a name describing the type of object that instances represent (e.g. `"TCP Conn. ID"`). Used as a label in the CSV output files. Names longer than 32 characters are silently truncated.

_Example:_ to track the total bytes sent over up to 512 TCP connections using a Roller Counter with ID 12:

```c
define_vector_ctr(12, 512, ROLLERCTR, 0, "Total Bytes Sent", "TCP Conn. ID");
```

This function **must be called before** `start_counters()`.

Possible return values:
- `MIXFOK`: the Vector Counter has been defined successfully.
- `MIXFKO`: `ctrId` or `ctrInst` is outside the allowed range, `ctrType` is invalid, or `start_counters()` has already been called.
- `MIXFOVFL`: the cumulative number of Vector Counter instances would exceed 65536.


#### _Error set_vector_ctr_inst_name(uint16\_t ctrId, uint16\_t ctrInst, char \*instIdName)_

Assigns an individual name to a specific instance of a Vector Counter. This is useful, for example, when a counter tracks metrics per TCP connection and the instance name stores the actual connection identifier. The three parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Vector Counter, as defined through `define_vector_ctr_num()`.
- **`ctrInst`** (`uint16_t`): identifier of the specific instance, in the range `[0, P-1]` where `P` is the number of instances set in the corresponding `define_vector_ctr()` call.
- **`instIdName`** (`char *`): the name string to assign to the instance. Names longer than 16 characters are silently truncated. If `NULL`, the call has no effect and the existing name is preserved.

Unlike most counter configuration functions, `set_vector_ctr_inst_name()` **can be called even after** `start_counters()` has been invoked, which allows instance names to be updated dynamically at runtime (e.g. when a new connection is accepted or when an object is replaced).

Possible return values:
- `MIXFOK`: the instance name has been set (or the call was a no-op because `instIdName` was `NULL`).
- `MIXFKO`: `ctrId` or `ctrInst` is outside the allowed range.


#### _Error define_base_dump(char \*baseDir, char \*baseTimeFormat, char \*baseTimes)_

Configures the base-interval counter dump. The base interval is the shortest collection granularity: at each base dump time the library writes a row in the CSV output files and, for `PEGCTR` counters, resets the base accumulator to zero. Calling this function is **mandatory** before invoking `start_counters()`. The three parameters are:

- **`baseDir`** (`char *`): path (absolute or relative) to the directory where CSV files will be written. If `NULL` or empty, the current working directory is used. At midnight (00:00) the files are automatically closed and new ones are opened, so one set of files is generated per calendar day. The naming convention is:
  - `scalar_<timestamp>.csv` — all Scalar Counters;
  - `vector_<ID>_<timestamp>.csv` — instances of Vector Counter `<ID>` (one file per counter).
- **`baseTimeFormat`** (`char *`): a `strftime()`-compatible format string that controls the `<timestamp>` portion of the file names (e.g. `"%F"` for `YYYY-MM-DD`, `"%d%m%Y"` for `ddmmyyyy`). If `NULL` or empty, `"%d%m%Y"` is used.
- **`baseTimes`** (`char *`): a comma-separated list of minute values (two-digit `"mm"` format, `00`–`59`) that specify when within each hour counters shall be dumped to file. For example, to dump every 5 minutes: `"00,05,10,15,20,25,30,35,40,45,50,55"`.

Possible return values:
- `MIXFOK`: the base dump configuration has been accepted.
- `MIXFKO`: any parameter is invalid (malformed directory path, invalid time format, minute value outside `[00–59]`), or `start_counters()` has already been called.


#### _Error define_aggr_dump(char \*aggrDir, char \*aggrTimeFormat, char \*aggrTimes)_

Configures the optional aggregated-interval counter dump. The aggregation interval is a coarser granularity on top of the base interval: at each aggregated dump time the library writes a row in the aggregated CSV files and, for `PEGCTR` counters, resets the aggregated accumulator to zero. Calling this function is **optional**: if omitted, no aggregated files are produced.

The three parameters follow the same pattern as `define_base_dump()`:

- **`aggrDir`** (`char *`): path to the directory for aggregated CSV files. Files are rotated daily at midnight. The naming convention is:
  - `scalar_aggr_<timestamp>.csv` — all Scalar Counters;
  - `vector_<ID>_aggr_<timestamp>.csv` — instances of Vector Counter `<ID>`.
- **`aggrTimeFormat`** (`char *`): a `strftime()`-compatible format string for the file name timestamp. If `NULL` or empty, `"%d%m%Y"` is used.
- **`aggrTimes`** (`char *`): a comma-separated list of `hhmm`-formatted times (hours `00`–`23`, minutes `00`–`59`) specifying when aggregated counters shall be dumped. For example, to dump every two hours: `"0000,0200,0400,0600,0800,1000,1200,1400,1600,1800,2000,2200"`. At most **100** dump times can be specified.

Possible return values:
- `MIXFOK`: the aggregated dump configuration has been accepted.
- `MIXFKO`: any parameter is invalid, or `start_counters()` has already been called.
- `MIXFOVFL`: more than 100 dump times have been specified.


#### _Error start_counters(void)_

Opens all counter output files (base and aggregated, if `define_aggr_dump()` was called) and activates counter collection. After a successful call, all update and retrieve functions become operational.

This function initialises the internal POSIX mutexes used to protect concurrent access to the CSV files, and writes a header row into each newly created file.

Possible return values:
- `MIXFOK`: counter collection has started successfully.
- `MIXFKO`: `start_counters()` had already been called, or `define_base_dump()` had not been called.
- `MIXFNOACCESS`: one or more output files could not be opened (e.g. the target directory does not exist or is not writable).


#### _Error stop_counters(void)_

Terminates counter collection. Specifically, the function dumps the current counter values to the output files as a final row, closes all open CSV file descriptors, destroys the internal POSIX mutexes, and releases all dynamically allocated memory. **After this call, all counter definitions are lost** and the library is returned to its initial state.

Possible return values:
- `MIXFOK`: counter collection has been stopped and resources have been released successfully.
- `MIXFKO`: `start_counters()` had not been called before.


#### _Error incr_peg_scalar_ctr(uint16\_t ctrId)_

Increments a `PEGCTR` Scalar Counter by one. Both the base accumulator and the aggregated accumulator are incremented simultaneously. `ctrId` shall be in the range `[0, M-1]` where `M` was set through `define_scalar_ctr_num()`.

Note that if either accumulator reaches the maximum value `2^32 - 1`, it **wraps around to 0** on the next increment (natural unsigned overflow). The function returns `MIXFOVFL` in this case but the increment is applied regardless.

Possible return values:
- `MIXFOK`: the counter has been incremented successfully.
- `MIXFKO`: `ctrId` is out of range, the counter is of type `ROLLERCTR`, or `start_counters()` has not been called.
- `MIXFOVFL`: at least one accumulator (base or aggregated) has wrapped around the maximum value (`2^32 - 1`). The increment was applied and the new value is 0.


#### _Error incr_peg_vector_ctr(uint16\_t ctrId, uint16\_t \*ctrInst)_

Increments a `PEGCTR` Vector Counter by one. Both the base and aggregated accumulators are incremented. The two parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Vector Counter, in the range `[0, N-1]`.
- **`ctrInst`** (`uint16_t *`): pointer to the instance to increment, in the range `[0, P-1]` where `P` was set in `define_vector_ctr()`. If `NULL`, **all instances** of the counter are incremented by one.

The same wrap-around behaviour as `incr_peg_scalar_ctr()` applies.

Possible return values:
- `MIXFOK`: the counter instance(s) have been incremented successfully.
- `MIXFKO`: `ctrId` or the instance value pointed to by `ctrInst` is out of range, the counter is of type `ROLLERCTR`, or `start_counters()` has not been called.
- `MIXFOVFL`: at least one accumulator of at least one affected instance has wrapped around the maximum value. The increment was applied.


#### _Error retrieve_peg_scalar_ctr(uint16\_t ctrId, uint32\_t \*ctrBase, uint32\_t \*ctrAggr)_

Reads the current accumulated values of a `PEGCTR` Scalar Counter without modifying it. The three parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Scalar Counter, in the range `[0, M-1]`.
- **`ctrBase`** (`uint32_t *`): output parameter set to the current value of the base accumulator (i.e. the value accumulated since the last base dump interval).
- **`ctrAggr`** (`uint32_t *`): output parameter set to the current value of the aggregated accumulator (i.e. the value accumulated since the last aggregated dump interval).

Possible return values:
- `MIXFOK`: both output parameters have been populated successfully.
- `MIXFKO`: `ctrId` is out of range, the counter is of type `ROLLERCTR`, or `start_counters()` has not been called.


#### _Error retrieve_peg_vector_ctr(uint16\_t ctrId, uint16\_t ctrInst, uint32\_t \*ctrBase, uint32\_t \*ctrAggr)_

Reads the current accumulated values of a specific instance of a `PEGCTR` Vector Counter without modifying it. The four parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Vector Counter, in the range `[0, N-1]`.
- **`ctrInst`** (`uint16_t`): identifier of the specific instance to read, in the range `[0, P-1]`. Unlike `incr_peg_vector_ctr()`, passing a specific instance is mandatory here; there is no `NULL` shorthand to retrieve all instances at once.
- **`ctrBase`** (`uint32_t *`): output parameter set to the current base accumulator value for the specified instance.
- **`ctrAggr`** (`uint32_t *`): output parameter set to the current aggregated accumulator value for the specified instance.

Possible return values:
- `MIXFOK`: both output parameters have been populated successfully.
- `MIXFKO`: `ctrId` or `ctrInst` is out of range, the counter is of type `ROLLERCTR`, or `start_counters()` has not been called.


#### _Error update_roller_scalar_ctr(uint16\_t ctrId, short delta)_

Applies a signed delta to a `ROLLERCTR` Scalar Counter. Both the base and the aggregated accumulators are updated simultaneously. The two parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Scalar Counter, in the range `[0, M-1]`.
- **`delta`** (`short`): the signed increment to apply. A positive value increases the counter; a negative value decreases it. A value of 0 is accepted (the counter is left unchanged and `MIXFOK` is returned).

Unlike `PEGCTR` counters, `ROLLERCTR` counters **do not wrap**. Instead they are **saturated**:
- if the update would cause the value to exceed `2^32 - 1`, the counter is capped to `2^32 - 1`;
- if the update would cause the value to go below 0, the counter is capped to 0.

In both saturation cases the function returns `MIXFOVFL` and the counter is set to the capped value. Note that base and aggregated accumulators are updated independently, so saturation may affect one without affecting the other.

Possible return values:
- `MIXFOK`: the counter has been updated successfully.
- `MIXFKO`: `ctrId` is out of range, the counter is of type `PEGCTR`, or `start_counters()` has not been called.
- `MIXFOVFL`: at least one accumulator (base or aggregated) reached a saturation bound. The counter has been capped accordingly.


#### _Error update_roller_vector_ctr(uint16\_t ctrId, uint16\_t \*ctrInst, short delta)_

Applies a signed delta to a `ROLLERCTR` Vector Counter. The three parameters are:

- **`ctrId`** (`uint16_t`): identifier of the Vector Counter, in the range `[0, N-1]`.
- **`ctrInst`** (`uint16_t *`): pointer to the instance to update, in the range `[0, P-1]`. If `NULL`, **all instances** of the counter are updated by the same `delta` value.
- **`delta`** (`short`): the signed increment to apply. The same saturation semantics as `update_roller_scalar_ctr()` apply to every affected instance.

Possible return values:
- `MIXFOK`: the counter instance(s) have been updated successfully.
- `MIXFKO`: `ctrId` or the instance value pointed to by `ctrInst` is out of range, the counter is of type `PEGCTR`, or `start_counters()` has not been called.
- `MIXFOVFL`: at least one accumulator of at least one affected instance reached a saturation bound.


#### _Error check_and_dump_ctr(void)_

Checks the current wall-clock time against the configured base and aggregated dump schedules and, if a dump time has been reached, writes a timestamped row into the appropriate CSV files.

For `PEGCTR` counters, the base accumulator is reset to zero after each base dump; the aggregated accumulator is reset to zero after each aggregated dump. `ROLLERCTR` counters are never reset in either accumulator.

Additionally, the function handles daily **file rotation**: at midnight (00:00) all open CSV files are closed and new ones are opened with an updated timestamp in the file name, ensuring that each calendar day produces its own set of files.

This function must be **called periodically** within the application's main loop (e.g. once per minute) to ensure that dump times are not missed. It is safe to call it more frequently since it performs the dump only when a scheduled time is actually reached.

Possible return values:
- `MIXFOK`: the check was performed successfully (no dump may have occurred if no dump time was due).
- `MIXFKO`: counters have not been defined or `start_counters()` has not been called.
- `MIXFNOACCESS`: a CSV file could not be written or a rotated file could not be reopened.

### Examples

See `examples/bin/Example2` for a full example that combines lock handling
and counters collection.

## Examples

The `examples` directory contains sample programs illustrating library usage.
Build them with the provided `make` targets.

## Known Issues

### URL validation

`check_url_validity()` supports up to 256 characters and does not implement full
URL verification. In particular:

- URL authentication sections (`username[:password]@`) are not accepted.
- Query and fragment sections are only lightly validated; the routine checks
  allowed characters but not full parameter/value structure.

### Configuration files

- Only IPv4 addresses are supported in IPv4-specific parameter types; IPv6 is
  not supported.

### License handling

- License checks rely on hostname and hostid values, which can be modified and
  are not a hardened licensing mechanism.
