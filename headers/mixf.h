/****************************************************************************
 * -----------------------------------------                                *
 * C/C++ Mixed Functions Library (libmixf)                                  *
 * -----------------------------------------                                *
 * Copyright 2019-2026 Roberto Mameli                                       *
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License");          *
 * you may not use this file except in compliance with the License.         *
 * You may obtain a copy of the License at                                  *
 *                                                                          *
 *     http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                          *
 * Unless required by applicable law or agreed to in writing, software      *
 * distributed under the License is distributed on an "AS IS" BASIS,        *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 * See the License for the specific language governing permissions and      *
 * limitations under the License.                                           *
 * ------------------------------------------------------------------------ *
 *                                                                          *
 * FILE:        mixf.h                                                      *
 * VERSION:     3.0.0                                                       *
 * AUTHOR(S):   Roberto Mameli                                              *
 * PRODUCT:     Library libmixf - general purpose library                   *
 * DESCRIPTION: Source file for various general purpose functions           *
 *              about:                                                      *
 *              - File and File System Handling                             *
 *              - Time and Date Handling                                    *
 *              - String Handling                                           *
 *              - Configuration Files Handling                              *
 *              - Log Handling                                              *
 *              - License Handling                                          *
 *              - Lock Handling                                             *
 *              - Counters Handling                                         *
 * REV HISTORY: See updated Revision History in file CHANGELOG.md           *
 * NOTE WELL:   If an application needs services and functions from this    *
 *              API, it MUST necessarily:                                   *
 *              - include the library header file                           *
 *                   #include "mixf.h"                                      *
 *              - be linked by including either the shared or the static    *
 *                library libmixf                                           *
 *              ----------------------------------------------------------  *
 *              Please, be aware that this library is neither RE-ENTRANT    *
 *              nor THRHEAD SAFE. However, Log and Counters Handling        *
 *              functions use POSIX mutex to avoid cuncurrent access to     *
 *              log files                                                   *
 *                                                                          *
 ****************************************************************************/

#ifndef MIXF_H_
#define MIXF_H_


/*****************
 * Include Files *
 *****************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif


/*******************************
 * General Purpose Definitions *
 *******************************/
#define MAXFILENAMELEN          FILENAME_MAX /* Platform dependent constant defined in stdio.h */
#define MAXDATETIMELEN          32           /* Maximum length in characters for date and time strings - DO NOT CHANGE */
#define UNDEFINED               255          /* Use this constant for undefined events or errors - DO NOT CHANGE */


/*********************
 * Error Definitions *
 *********************/
#define MIXFOK                  0            /* Returned by several routines that are part of the API - DO NOT CHANGE */
#define MIXFKO                  1
#define MIXFNOACCESS            2
#define MIXFFORMATERROR         3
#define MIXFPARAMUNKNOWN        4
#define MIXFWRONGDEF            5
#define MIXFOVFL                6


/*********************
 * Counters Handling *
 *********************/
#define PEGCTR                  0            /* Used for counter type definitions */
#define ROLLERCTR               1


 /********************
 * Type Definitions *
 ********************/
typedef uint8_t         Error;               /* Type for Error Code returned by libmixf API routines (see Error Definitions above) */
typedef uint8_t         EventCode;           /* Type for Event Codes handled by the libmixf library */

typedef struct Dir_Content                   /* Type used for elements of the list of files */
{                                            /* given back by read_files_input_dir() */
    char                filename[MAXFILENAMELEN];
    struct Dir_Content *next;
} DirContent;

typedef struct eventlist                     /* Type used for the list of events given back by */
{                                            /* parse_cfg_param_file() routine */
    EventCode           event;
    uint16_t            line;
    struct eventlist    *next;
} EventList;




/*******************************************************
 * Function Prototypes - File and File System Handling *
 *******************************************************/

/* check_file_name_validity()
   --------------------------
   Takes an input string, representing a file/directory name to be checked
   and returns MIXFOK if it is a valid file name, MIXFKO otherwise.
   A valid input file starts with a letter, a digit, a dot, a slash or an
   underscore and does not contain any of the following characters:
   |!"£$%()=?'^\[]*+@#;:,<>&   */
Error check_file_name_validity (char *);

/* retrieve_path()
   ---------------
   This function provides a string containing the path of the current working
   directory in the file system. It does not malloc memory for the string.
   Returns MIXFKO in case of errors (e.g. permission to read or search a component
   of the filename was denied), MIXFOK in any other case. */
Error retrieve_path (char *);

/* read_files_input_dir()
   ----------------------
   This function reads all the files in the Directory specified in the first
   parameter and provides a list of all filenames in the second parameter
   (filenames are reported relative to input dir, i.e. without absolute path).
   Only files are considered, directories are discarded. Returns MIXFKO in case of
   errors (e.g. permission to read or search a component of the input dir was
   denied), MIXFOK in any other case. */
Error read_files_input_dir (char *, DirContent **);

/* clear_input_file_list()
   -----------------------
   This function releases memory allocated by read_files_input_dir() for the
   Input File List. */
void clear_input_file_list (DirContent **);



/************************************************
 * Function Prototypes - Time and Date Handling *
 ************************************************/

/* retrieve_time_date()
   --------------------
   This function provides in the first argument a string containing current
   time and date. The string is provided back into the first char * according
   to the format defined in the second argument. This shall be specified
   according to the format described in strftime() man page (e.g. "%c" for
   full date and time, "%T" for time in HH:MM:YY format, "%F" for date in the
   format YYYY-MM-DD, etc.). The routine does not allocate memory for the date
   and time string. */
void retrieve_time_date (char *, char *);

/* get_time_stamp()
   ----------------
   This function takes a string (first argument) that begins with a Time stamp
   (e.g. a Log Line) and decodes it according to the format specified in the
   second parameter (see man strptime for details about the format).
   It converts this time stamp into a time_t value (i.e. number of seconds from
   epoch) and provides it back as a result (0 if it detects a mismatch in the format). */
time_t get_time_stamp (char *, char *);



/*****************************************
 * Function Prototypes - String Handling *
 *****************************************/

/* filter_and_extract()
   --------------------
   This function takes an input string (first parameter) and if both the following
   conditions apply:
     (1) the input string contains the string specified in the second parameter;
     (2) the input string contains also the string that begins with the third parameter;
   it provides back a char pointer in the input string to the first character of the
   third parameter. If any of the previous conditions doesn't apply, it provides NULL */
char *filter_and_extract (char *, const char *, const char *);

/* strcmp_wildcards()
   ------------------
   This function is an enhanced version of strcmp(). It takes two input strings and
   compare them eventually taking wildcards into account. Specifically, if the second
   string contains a '*', this represents any combination of chars (from 0 up to any
   possible number n). If the second string contains a '?' or a '!', this represents
   exactly one character.
   Such wildcards are allowed only within the second string. If they are contained in
   the first string, they are not considered as wildcards, but exactly as any other
   character (i.e. if first string is "1234*", it can only match with "1234*" and
   anything else).
   The function returns false if the strings match, true otherwise.
   The third parameter is a boolean flag that is set to true if string1 contains
   a wildcard, false otherwise. The fourth parameter is a boolean flag that is set to
   true if the two strings match exactly, false in case the match is due to the
   presence of wildcards in the second string. */
bool strcmp_wildcards (char *, char *, bool *, bool *);

/* remove_blanks()
   ---------------
   This function takes a NULL terminated string and modifies it by removing all blanks,
   tabs and new lines. The behavior is undefined if the string is not NULL terminated.
   Please consider that the input string is modified by this function. */
void remove_blanks (char *);

/* copy_remove_blanks()
   --------------------
   This function takes a NULL terminated string (second parameter) and copies it onto
   another string (first parameter), but removing all blanks, tabs and new lines.
   It differs from remove_blanks() since it preserves the original string (i.e. it does
   not modify the second string). The behaviour is undefined if the string
   is not NULL terminated */
void copy_remove_blanks (char *, char *);

/* only_digits()
   -------------
   This function provides back true if the input string consists only of digits,
   false otherwise */
bool only_digits (char *);

/* check_mail_validity()
   ---------------------
   This function takes a string as input parameter and provides true
   if it represents an e-mail syntactically correct, false otherwise.
   It is assumed that the input string contains up to 128 characters
   otherwise it is considered not valid (there is no specific rule
   that states so, but it seems a reasonable assumption in almost all
   practical situations). */
bool check_mail_validity (char *);

/* check_ipv4_add_validity()
   -------------------------
   This function provides true if the input parameter is a valid IPv4 Address (i.e. a
   string formatted as a.b.c.d, where a, b, c, d are integers between 0 and 255), false
   otherwise. If true the second parameter provides the IP addr converted into a uint32_t */
bool check_ipv4_add_validity (char *, uint32_t *);

/* check_fqdn_validity()
   ---------------------
   This function takes a string as input parameter and provides true
   if it represents a fqdn syntactically correct, false otherwise.
   It is assumed that the input string contains up to 256 characters
   otherwise it is considered not valid (there is no specific rule
   that states so, but it seems a reasonable assumption in almost all
   practical situations). */
bool check_fqdn_validity (char *);

/* check_url_validity()
   --------------------
   This function takes a string as input parameter and provides true
   if it represents an URL syntactically correct, false otherwise.
   It is assumed that the input string contains up to 256 characters,
   otherwise it is considered not valid (there is no specific rule
   that states so, but it seems a reasonable assumption in almost all
   practical situations).
   Observe that the general URL format is the following:
     [protocol://][username[:password]@]host[:port][</path>][?querystring][#fragment]
   This version does not implement full URL verification, since it has
   some minor limitations:
     - it does not support URL authentication, i.e. in case of
       [username[:password]@] the URL is not recognized as valid;
     - [?querystring] and [#fragment] are not completely verified (both
       should be formatted as concatenation of parameter/value pairs, i.e.
       param1=value1&param2=value2... This is not controlled, the routine
       just checks that those sections contain only allowed characters */
bool check_url_validity (char *);

/* generate_token()
   ----------------
   This function takes two input parameters, specifically a string composed
   by a set of characters (charset, in the second parameter) and an integer
   (length, in the third parameter). It provides back in the first parameter
   a random token composed by length characters extracted from the charset.
   There is no specific length limitation in the generated token, length can
   be set arbitrarily high, given that the string in the first parameter is
   allocated correspondingly. The function behaviour is unpredictable in
   case the first string has not enough space.
   The function returns MIXFOK in case of token successfully generated,
   MIXFKO in case of errors (e.g. empty charset or length <=0). */
Error generate_token (char *, char *, int);



/******************************************************
 * Function Prototypes - Configuration Files Handling *
 ******************************************************/

/* reset_param_list()
   ------------------
   Reset the internal list of allowed parameters (call this before initializing the list) */
void reset_param_list (void);

/* init_param_list()
   -----------------
   This function is used to define the maximum number of parameters managed by the
   Configuration Files Handling functions. Allowed values are comprised in the range
   between 1 and 255. This function is not mandatory, i.e. it can also be skipped;
   in this case, the default value for the maximum number of parameters is set to 8.
   This function provides MIXFOK if execution is successful, MIXFKO in case it is
   called twice without invoking reset_param_list() first, MIXFOVFL if the parameter
   is outside the allowed range 1-255 or the system runs out of memory */
Error init_param_list (int);

/* add_numerical_param()
   ---------------------
   This function adds a numerical parameter to the list of parameters allowed in the
   configuration file. The first argument is the parameter name, the second is the Mandatory
   flag; it must be set to true if the parameter is mandatory, false if it is optional in the
   configuration file. Third, fourth and fifth parameter are 3 integers that represent respectively
   the minimum, maximum and default value for the parameter (please observe that default is
   actually meaningful only if the Mandatory flag is false). The last four parameters are event
   codes that are associated respectively to the following events: (1) Event corresponding to a
   Mandatory parameter non provisioned; (2) Event corresponding to an Optional parameter not
   provisioned (default value used instead); (3) Event corresponding to a parameter that is redefined
   (at least twice); (4) Event corresponding to a parameter value out of range or malformed (e.g. not
   a number). Use UNDEFINED for those events that are not meaningful (e.g. event (1) does not make
   sense if Mandatory flag is false).
   This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
   the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
   MIXFWRONGDEF if the condition min <= default <= max is not satisfied and finally
   MIXFKO for any other error (e.g. parameter name empty) */
Error add_numerical_param (char *, bool, int, int, int, EventCode, EventCode, EventCode, EventCode);

/* add_literal_param()
   -------------------
   This function adds a literal parameter to the list of parameters allowed in the
   configuration file. The description is exactly the same as add_numerical_param(), with the
   only difference that there is no minimum or maximum value, but only a default (which is
   defined in the third argument). There is no special check on the possible values of
   literal parameters, therefore the MIXFWRONGDEF error code cannot be returned by this function */
Error add_literal_param (char *, bool, char *, EventCode, EventCode, EventCode, EventCode);

/* add_filename_param()
   --------------------
   This function adds a filename parameter to the list of parameters allowed in the
   configuration file. The description is exactly the same as add_literal_param(); however,
   there is a minor difference between literal and filename parameters, since the
   filename parameter can only get values that are valid filenames (see also
   check_file_name_validity() above. This routine can provide back all error codes as the
   previous ones, including MIXFWRONGDEF (which is given when the default value in the
   third argument is not a valid file name) */
Error add_filename_param (char *, bool, char *, EventCode, EventCode, EventCode, EventCode);

/* add_char_param()
   ----------------
   This function adds a char parameter to the list of parameters allowed in the
   configuration file. A CHAR Parameter is a single character string and shall
   be enclosed within apices in the configuration file (e.g. "a") otherwise it is
   considered malformed. Apices are needed to ensure that also the blank character
   can be easily identified  (" ").
   The first argument is the parameter name, the second is the Mandatory flag;
   it must be set to true if the parameter is mandatory, false if it is optional in the
   configuration file. Third, fourth and fifth parameter are 3 chars that represent respectively
   the minimum, maximum and default value for the parameter (please observe that default is
   actually meaningful only if the Mandatory flag is false). The last four parameters are event
   codes that are associated respectively to the following events: (1) Event corresponding to a
   Mandatory parameter non provisioned; (2) Event corresponding to an Optional parameter not
   provisioned (default value used instead); (3) Event corresponding to a parameter that is redefined
   (at least twice); (4) Event corresponding to a parameter value out of range or malformed (e.g. not
   a char). Use UNDEFINED for those events that are not meaningful (e.g. event (1) does not make
   sense if Mandatory flag is false).
   This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
   the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
   MIXFWRONGDEF if the condition min <= default <= max is not satisfied and finally
   MIXFKO for any other error (e.g. parameter name empty) */
Error add_char_param (char *, bool, char, char, char, EventCode, EventCode, EventCode, EventCode);

/* add_mail_param()
   ----------------
   This function adds a mail parameter to the list of parameters allowed in the
   configuration file. The description is exactly the same as add_literal_param(); however,
   there is a minor difference between literal and mail parameters, since the
   mail parameter can only get values that are valid e-mail addresses.
   This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
   the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
   MIXFWRONGDEF if the default value in the third argument is not a valid e-mail and finally
   MIXFKO for any other error (e.g. parameter name empty) */
Error add_mail_param (char *, bool, char *, EventCode, EventCode, EventCode, EventCode);

/* add_ipv4_param()
   ----------------
   This function adds an ipv4 parameter to the list of parameters allowed in the
   configuration file. The description is exactly the same as add_literal_param(); however,
   there is a minor difference between literal and ipv4 parameters, since the
   ipv4 parameter can only get values that are valid IPv4 addresses.
   This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
   the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
   MIXFWRONGDEF if the default value in the third argument is not a valid IPv4 address and finally
   MIXFKO for any other error (e.g. parameter name empty) */
Error add_ipv4_param (char *, bool, char *, EventCode, EventCode, EventCode, EventCode);

/* add_url_param()
   ---------------
   This function adds an URL parameter to the list of parameters allowed in the
   configuration file. The description is exactly the same as add_literal_param(); however,
   there is a minor difference between literal and URL parameters, since the
   URL parameter can only get values that are valid URLs.
   This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
   the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
   MIXFWRONGDEF if the default value in the third argument is not a valid URL and finally
   MIXFKO for any other error (e.g. parameter name empty) */
Error add_url_param  (char *, bool, char *, EventCode, EventCode, EventCode, EventCode);

/* get_num_param_value()
   ---------------------
   This routine provides through the second argument the value of the numerical parameter
   whose name is given by the first argument. It provides return code MIXFOK if the parameter
   is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
   configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
   parameter is not known. The third argument is a flag that is true if the parameter was
   actually provisioned in the configuration file, while is false if it wasn't */
Error get_num_param_value (char *, int *, bool *);

/* get_list_param_value()
   ---------------------
   This routine provides through the second argument the value of the literal parameter
   whose name is given by the first argument. It provides return code MIXFOK if the parameter
   is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
   configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
   parameter is not known. The third argument is a flag that is true if the parameter was
   actually provisioned in the configuration file, while is false if it wasn't
   The string in which the parameter value is copied (second parameter) is not allocated
   by the routine, rather shall be allocated by the caller. */
Error get_list_param_value (char *, char * , bool *);

/* get_fname_param_value()
   -----------------------
   This routine provides through the second argument the value of the filename parameter
   whose name is given by the first argument. It provides return code MIXFOK if the parameter
   is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
   configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
   parameter is not known. The third argument is a flag that is true if the parameter was
   actually provisioned in the configuration file, while is false if it wasn't
   The string in which the parameter value is copied (second parameter) is not allocated
   by the routine, rather shall be allocated by the caller. */
Error get_fname_param_value (char *, char * , bool *);

/* get_char_param_value()
   ----------------------
   This routine provides through the second argument the value of the char parameter
   whose name is given by the first argument. It provides return code MIXFOK if the parameter
   is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
   configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
   parameter is not known. The third argument is a flag that is true if the parameter was
   actually provisioned in the configuration file, while is false if it wasn't */
Error get_char_param_value (char *, char *, bool *);

/* get_mail_param_value()
   ----------------------
   This routine provides through the second argument the value of the email parameter
   whose name is given by the first argument. It provides return code MIXFOK if the parameter
   is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
   configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
   parameter is not known. The third argument is a flag that is true if the parameter was
   actually provisioned in the configuration file, while is false if it wasn't
   The string in which the parameter value is copied (second parameter) is not allocated
   by the routine, rather shall be allocated by the caller. */
Error get_mail_param_value (char *, char * , bool *);

/* get_ipv4_param_value()
   ----------------------
   This routine provides through the second argument the value of the ipv4 parameter
   whose name is given by the first argument. It provides return code MIXFOK if the parameter
   is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
   configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
   parameter is not known. The third argument is a flag that is true if the parameter was
   actually provisioned in the configuration file, while is false if it wasn't
   The string in which the parameter value is copied (second parameter) is not allocated
   by the routine, rather shall be allocated by the caller. */
Error get_ipv4_param_value (char *, char * , bool *);

/* get_url_param_value()
   ---------------------
   This routine provides through the second argument the value of the URL parameter
   whose name is given by the first argument. It provides return code MIXFOK if the parameter
   is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
   configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
   parameter is not known. The third argument is a flag that is true if the parameter was
   actually provisioned in the configuration file, while is false if it wasn't
   The string in which the parameter value is copied (second parameter) is not allocated
   by the routine, rather shall be allocated by the caller. */
Error get_url_param_value  (char *, char * , bool *);

/* parse_cfg_param_file()
   ----------------------
   This routine opens and parses the configuration file specified as first parameter. This
   file shall be composed of lines with the following format:
        PARAM = VALUE
   Also comments are allowed (#), both at the beginning or at the end of a line. If the file
   does not respect this layout, it is considered wrongly formatted. Allowed parameters
   shall be defined before calling this routine through the reset_param_list() and AddXXXParam()
   routines previously defined.
   This function may provide back the following results:
      - MIXFOK
        the file was opened successfully, it is correctly formatted and do not contain
        unrecognized parameters. In this case the second argument provides back the total
        number of lines in the configuration file, while the third is a pointer to a pointer
        to a list of events found during parsing (for each event there is the corresponding
        line number)
      - MIXFNOACCESS
        the function is not able to open the configuration file (it may not exist or the user
        may not have read permission). In this case, both the second and the third parameters are
        not meaningful
      - MIXFFORMATERROR
        the file is wrongly formatted (it does not respect the layout specified above).
        The second parameter provides back the line in which the error occurred, while the third
        is null
      - MIXFPARAMUNKNOWN
        the file contains a parameter that is not recognized, i.e. which was not defined through
        the AddXXXParam() routines previously defined. The second parameter provides back the line
        in which the error occurred, while the third is null */
Error parse_cfg_param_file (char *, uint16_t *, EventList **);

/* clear_event_list()
   ------------------
   This function releases all the memory allocated for the event list by parse_cfg_param_file() */
void clear_event_list (EventList **);



/**************************************
 * Function Prototypes - Log Handling *
 **************************************/

/* define_log_levels()
   -------------------
   This function defines the maximum number of log levels (first parameter)
   and the default value for the log level (second parameter). The maximum number
   of levels shall be between 1 and 8, with 1 being the default value. The default
   value for the log level (second parameter) shall range between 0 and (first
   parameter-1). Please remember that the higher is the log level, the lower
   is the severity (i.e. level 0 events represents very very critical issues,
   while level 7 is for very minor events).
   This function provides MIXFKO in case of wrong parameters (e.g. outside allowed
   ranges), MIXFOK if everything is ok */
Error define_log_levels (uint8_t, uint8_t);

/* define_level_descr()
   --------------------
   This function associates a textual description (second parameter) to each log
   level (first parameter). The latter shall be included in the interval 0-(N-1),
   where N is the max number of log levels defined through define_log_levels().
   This function provides MIXFKO in case of wrong parameters (e.g. outside allowed
   ranges), MIXFOK if everything is ok */
Error define_level_descr (uint8_t, char *);

/* get_log_level()
   ---------------
   Provides back the current Log Level */
uint8_t get_log_level (void);

/* set_log_level()
   ---------------
   Sets the current Log Level to the value specified by the parameter
   Before invocation, the Log Level is by default set to 0 (i.e. the highest severity)
   It provides MIXFOK in case of success, MIXFKO if the value is outside the maximum
   log level, i.e. N-1, where N is the max number of log levels defined through
   define_log_levels() */
Error set_log_level (uint8_t);

/* define_num_events()
   -------------------
   Define the maximum number of events for the system. This value shall be within
   1 and 255. This function is mandatory and shall be necessarily called before
   the first occurrence of define_event(). It provides MIXFOK in case of success,
   MIXFKO otherwise */
Error define_num_events (uint8_t);

/* define_event()
   --------------
   This function defines attributes for each event. Specifically, the event code
   specified by the first parameter (which shall be defined in the interval between
   0 and M-1, where M is the num of events defined by define_num_events() ) is associated
   with a severity level specified by the second parameter (which shall be in the
   interval [0,N-1], being N the max number of log levels defined through
   define_log_levels()). The third parameter defines the textual description for the
   event. This string may contain up to 3 placeholders defined as "%1", "%2" and
   "%3", which will be used to identify the position within the string of up
   to 3 parameters (see register_event() below).
   If the function is called multiple times with the same event code, each invocation
   overwrites previous data (only the last definition applies).
   This function provides MIXFOK in case of success, MIXFOVFL if the first or the second
   parameter are out of allowed ranges, MIXFFORMATERROR if the string does not respect
   the format specified above (e.g. it contains an invalid placeholder, such as "%4"
   or a valid placeholder repeated twice) */
Error define_event (EventCode, uint8_t, char *);

/* open_log()
   ----------
   This function opens a log file named <Basename>_<Timestamp>.log (in append mode)
   <Basename> and <Timestamp> are respectively the first and the second argument
   of the function call. <Basename> shall be a valid file name, and is specified with
   respect to the current working directory (it is strongly advised to use absolute
   pathnames here). <Timestamp> is a string that shall be defined according to the
   format described in strftime() man page (e.g. "%F" for date in the format
   YYYY-MM-DD, etc.). If NULL or empty, the file will be simply opened as
   <Basename>.log, without a trailing timestamp. The third argument is a bool;
   if true, it forces the file to be closed and re-opened daily at 00:00).
   This function provides MIXFOK in case of SUCCESS, MIXFKO if the log is already open,
   MIXFFORMATERROR if the first argument is not a valid file name,
   MIXFNOACCESS if the filename cannot be opened. */
Error open_log (char *, char *, bool);

/* close_log()
   -----------
   Closes the Log File */
void close_log (void);

/* register_event()
   ----------------
   Register into the log the event whose event code is specified by first argument.
   This is done only if the event severity is at least equal to the current log level
   and if the log file is open, otherwise the call returns without effect.
   For events that contains parameters in the textual description (see define_event()
   above), they can be specified as strings in the second ("%1" placeholder), third ("%2"
   placeholder) and fourth ("%3" placeholder) argument). This function provides MIXFOK
   in case of SUCCESS, MIXFKO if the log is not open, MIXFNOACCESS if the log file
   cannot be reopened. */
Error register_event (EventCode, char *, char *, char *);



/******************************************
 * Function Prototypes - License Handling *
 ******************************************/

/* check_license()
   ---------------
   This function takes the license file name as first argument; it takes the first line contained
   into this file, decrypts it according to a proprietary algorithm, which uses some internal
   platform dependent parameters (mainly hostname and hostid) and provides the decrypted content
   of the file into the second string parameter. It returns MIXFOK if the file exists and conversion is
   successful, MIXFNOACCESS if the file does not exist, is empty or cannot be opened */
Error check_license (char *, char *);

/* create_license()
   ----------------
   This function takes a clear text string (first parameter) and two other strings, respectively
   the host name and the hostid, and provides back into the first argument the encrypted string.
   It returns MIXFOK if conversion is successful, MIXFKO in case of problems (e.g. if the hostname is empty
   or the hostid is not an 8 digits hex number starting with 0x) */
Error create_license (char *, char *, char *);



/***************************************
 * Function Prototypes - Lock Handling *
 ***************************************/

/* check_lock_present()
   --------------------
   The lock is an empty file whose file name is specified as input argument
   If the lock is not present, this function returns false, otherwise returns true */

bool check_lock_present (char *);

/* set_lock()
   ----------
   The lock is an empty file whose file name is specified as input argument
   Set the lock, returns MIXFOK in case of success, MIXFKO in any other case */
Error set_lock (char *);

/* reset_lock()
   ------------
   The lock is an empty file whose file name is specified as input argument
   Reset the lock, returns MIXFOK in case of success, MIXFKO in any other case */
Error reset_lock (char *);



/*******************************************
 * Function Prototypes - Counters Handling *
 *******************************************/

/* define_scalar_ctr_num()
   -----------------------
   Define the maximum number of Scalar counters, between 0 and 1024.
   Scalar counters are global counters, i.e. they refer to the whole
   piece of code and are not related to a specific object/instance.
   The function returns MIXFOK in case of success, MIXFKO in any other case
   In case of success, internal counter structures are reset
   and any previous counter definition is lost
   Please observe that this function cannot be called after start_counters() */
Error define_scalar_ctr_num (uint16_t);

/* define_vector_ctr_num()
   -----------------------
   Define the maximum number of Vector counters, between 0 and 1024.
   Vector counters are collected for a set of objects/instances of the
   same type. For example the number of bytes sent over a specific TCP
   connection shall be considered a Vector counter (the counter is one,
   i.e. the number of bytes sent, but given N the number of TCP
   connections, it comes in N instances). Be aware that the library
   can collect up to 65536 total instances of Vector counters.
   Here follows some examples of allowed combinations:
    - 2 Vector counters (each with up to 32768 instances)
    - 3 Vector counters (e.g. the first with up to 32768 instances,
        the others up to 16536 instances)
    - 1024 Vector counters of 64 instances each
   or any other combination, given that the total amount of collected
   counter instances does not exceed 65536.
   The function returns MIXFOK in case of success, MIXFKO in any other case
   In case of success, internal counter structures are reset
   and any previous counter definition is lost
   Please observe that this function cannot be called after start_counters() */
Error define_vector_ctr_num (uint16_t);

/* define_scalar_ctr()
   -------------------
   The first parameter is the Scalar Counter ID and shall be defined
   in the interval (0,M-1), where M is the the maximum number of
   Scalar counters defined through define_scalar_ctr_num(). The second
   parameter specifies the counter type (PEGCTR or ROLLERCTR).
   PEG Counters are counters that are characterized by the following
   properties: they have initial value set to 0, they can only increase
   and they are reset every time that counters are dumped to file
   (i.e. at each base interval). ROLLER Counters are slightly different:
   they can have a non null initial value, they can increase and decrease
   (but never become negative) and when they are dumped to file,
   they are not reset. The third parameter represents the initial value
   for the counter and is valid only for ROLLER Counters (it is meaningless
   in case that counter type is PEGCTR).
   The fourth parameter is a string that provides the counter name (up to 32
   characters, otherwise it is truncated).
   This function provides MIXFKO either in case of wrong parameters (e.g. outside allowed
   ranges) or in case of counters already started, MIXFOK if everything is ok */
Error define_scalar_ctr (uint16_t, uint8_t, uint32_t, char*);

/* define_vector_ctr()
   -------------------
   The first parameter is the Vector Counter ID and shall be defined
   in the interval (0,N-1), where N is the the maximum number of
   Vector counters defined through define_vector_ctr_num(). The second
   parameter is the maximum number of instances allowed for the
   Vector Counter ID specified by parameter 1. It shall be at least 1.
   The maximum value is limited by the fact that the maximum
   number of instances cannot exceed 65536. The third
   parameter specifies the counter type (PEGCTR or ROLLERCTR).
   PEG Counters are counters that are characterized by the following
   properties: they have initial value set to 0, they can only increase
   and they are reset every time that counters are dumped to file
   (i.e. at each base interval). ROLLER Counters are slightly different:
   they can have a non null initial value, they can increase and decrease
   (but never become negative) and when they are dumped to file,
   they are not reset. The fourth parameter represents the initial value
   for the counter and is valid only for ROLLER Counters (it is meaningless
   in case that counter type is PEGCTR). This value is assigned to all
   intances of the counter.
   The fifth parameter is a string that provides the counter name (up to 32
   characters, otherwise it is truncated). The sixth parameter is another
   string (up to 32 characters length, otherwise it is truncated) that
   provides the name of the object associated to instances.
   Example: referring to the number of bytes sent for different TCP
            connections, assuming that this is a ROLLER Counter whose ID is 12
            and that the maximum number of TCP Connections is 512, the call
            might be:
                 define_vector_ctr(12,512,ROLLERCTR,0,"Total Bytes Sent","TCP Conn. ID")
            In this case the initial value for the counter has been set to 0 (for
            all the 512 TCP connections)
   This function provides MIXFKO either in case of wrong parameters(e.g.outside allowed
   ranges) or in case of counters already started, MIXFOVFL if the number of cumulative
   instances of Vector Counters up to function call exceeds 65536, MIXFOK if everything is ok  */
Error define_vector_ctr (uint16_t, uint16_t, uint8_t, uint32_t, char*, char*);

/* set_vector_ctr_inst_name()
   --------------------------
   This function is used to associate a name to an instance of a Vector Counter
   The first parameter specifies the Vector Counter ID, the second one represents
   the Instance Id. They shall be defined within the limits specified through
   define_vector_ctr_num() and define_vector_ctr(), otherwise MIXFKO is returned.
   The third parameter is a string that specifies the name of the concerned
   instance (e.g. if the counter is used to collect Total Bytes Sent for
   several TCP connections, this name could be used to store the connection ID).
   If NULL, the call has no effect (i.e. the name is not changed), otherwise it
   is overwritten. Please observe that up to 16 chars are allowed, if more
   the name is truncated.
   Please observe that this function can be called even if counters have been
   already started */
Error set_vector_ctr_inst_name(uint16_t, uint16_t, char*);

/* define_base_dump()
   ------------------
   This function provides information needed to store counters (both scalar and vector
   as well as PEG and ROLLER counters) at each base interval. The first parameter is
   a string that provides the path (either absolute or relative) to the directory in which
   counters are collected. Files are closed and re-opened daily at 00:00. They are named
   as follows:
      scalar_<timestamp>.csv             -> dump of all Scalar Counters in CSV format
      vector_<vector ID>_<timestamp>.csv -> dump of Vector Counter having ID <vector ID>
                                            in CSV format (there is a separate file for
                                            each <vector ID>)
   The second parameter is a string formatted according to strftime() man page (e.g.
   "%F" for date in the format YYYY-MM-DD, etc.). It specifies the format of the <timestamp>
   above. If NULL or empty, the time stamp will be formatted as ddmmyyyy.
   Finally, the third string is a comma separated value of minutes corresponding to the
   desired dump times ("mm" format, with mm between 00 and 60). For example, in order to
   dump counters every 5 minutes, this string will be formatted as follows:
      "00,05,10,15,20,25,30,35,40,45,50,55"
   This function returns:
      - MIXFKO: if either the first parameter is not a valid file name or
             the second or third parameters are wrongly formatted or
             contain some error (e.g. invalid dump time). This code is also
             used if counters collection has been already started through start_counters()
      - MIXFOK: if everything is correct
   Please note that calling this function is MANDATORY before starting statistic collection
   Remember also that PEG counters are set to zero at every base interval                   */
Error define_base_dump(char*, char*, char*);

/* define_aggr_dump()
   ------------------
   This function provides information needed to store counters (both scalar and vector
   as well as PEG and ROLLER counters) at each aggregation interval. The first parameter is
   a string that provides the path (either absolute or relative) to the directory in which
   counters are collected. Files are closed and re-opened daily at 00:00. They are named
   as follows:
      scalar_aggr_<timestamp>.csv             -> dump of all Scalar Counters in CSV format
      vector_<vector ID>_aggr_<timestamp>.csv -> dump of Vector Counter having ID <vector ID>
                                                 in CSV format (there is a separate file for
                                                 each <vector ID>)
   The second parameter is a string formatted according to strftime() man page (e.g.
   "%F" for date in the format YYYY-MM-DD, etc.). It specifies the format of the <timestamp>
   above. If NULL or empty, the time stamp will be formatted as ddmmyyyy.
   Finally, the third string is a comma separated value of hours/minutes corresponding to the
   desired aggregated dump times ("hhmm" format, with hh between 00 and 23 and mm between 00 and
   59). For example, in order to dump counters every 2 hours at clock times, this string
   will be formatted as follows:
      "0000,0200,0400,0600,0800,1000,1200,1400,1600,1800,2000,2200"
   Please observe that up to 100 dump times can be defined.
   This function returns:
      - MIXFKO:   if either the first parameter is not a valid file name or
                  the second or third parameters are wrongly formatted or
                  contain some error (e.g. invalid dump time). This code is also
                  used if counters collection has been already started through start_counters()
      - MIXFOVFL: if more than 100 dump times have been specified
      - MIXFOK:   if everything is correct
   Please note that calling this function is OPTIONAL before starting statistic collection
   (if not called, counters will not be aggregated)
   Remember also that aggregated PEG counters are set to zero at every aggregation interval  */
Error define_aggr_dump(char*, char*, char*);

/* start_counters()
   ----------------
   Open all counters files (base and aggregated, if defined) and start counting events
   This function may return:
      - MIXFNOACCESS: if any of the base or aggregated files cannot be opened
      - MIXFKO:       if counter collection has alredy been started before through start_counters()
                      or define_base_dump() has not been called
      - MIXFOK:       if everything is OK                                                */
Error start_counters (void);

/* stop_counters()
   ---------------
   Stops collecting counters, dumps the last values collected up to that time,
   closes all files and releases internal resources.
   Please observe that after this call all internal counters definition are lost
   It returns MIXFKO if counters have not been started before, MIXFOK in all other cases  */
Error stop_counters (void);

/* incr_peg_scalar_ctr()
   ---------------------
   This function increases a Peg Scalar Counter by one.
   The only parameter is the Scalar Counter ID and shall be defined
   in the interval (0,M-1), where M is the the maximum number of
   Scalar counters defined through define_scalar_ctr_num().
   Possible return values are:
      - MIXFKO:   the counter ID does not exist, is outside
                  the allowed range or the specified counter
                  is a Roller Counter or counters have
                  not been started
      - MIXFOVFL: the counter has wrapped around the maximum
                  value (i.e. 2^32 -1). This applies both to base
                  value and to aggregate value. Note that the counter
                  is increased anyway (i.e. the new value is 0)
      - MIXFOK:   the counter has been increased without errors       */
Error incr_peg_scalar_ctr (uint16_t);

/* incr_peg_vector_ctr()
   ---------------------
   This function increases a Peg Vector Counter by one.
   The first parameter is the Scalar Counter ID and shall be defined
   in the interval (0,N-1), where N is the the maximum number of
   Vector counters defined through define_vector_ctr_num().
   The second parameter is a pointer to a Instance ID. If NULL
   all the instances related to the concerned Vector counter
   are increased by 1, otherwise only the specified Instance Id
   is increased. It must be included in the interval (0,P-1)
   where P is the number of instances specified in
   define_vector_ctr().
   Possible return values are:
      - MIXFKO:   the counter ID does not exist, is outside
                  the allowed range, the specified counter
                  is a Roller Counter, the instance ID
                  is outside the allowed interval or counters
                  have not been started
      - MIXFOVFL: the counter has wrapped around the maximum
                  value (i.e. 2^32 -1). This applies both to base
                  value and to aggregate value. Note that the counter
                  is increased anyway (i.e. the new value is 0)
      - MIXFOK:   the counter has been increased without errors        */
Error incr_peg_vector_ctr (uint16_t, uint16_t*);

/* update_roller_scalar_ctr()
   --------------------------
   This function updates a Roller Scalar Counter by a specified value,
   either positive or negative. The first parameter is the Scalar
   Counter ID and shall be defined in the interval (0,M-1), where M
   is the the maximum number of Scalar counters defined through
   define_scalar_ctr_num(). The second parameter represents either
   the increase (if positive) or the decrease (if negative). 0
   is a valid value (i.e. it is accepted even if the corresponding
   counter is not modified in such case).
   Possible return values are:
      - MIXFKO:   the counter ID does not exist, is outside
                  the allowed range, the specified counter
                  is a Peg Counter or counters have not been started
      - MIXFOVFL: the counter should either exceed the maximum
                  value (i.e. 2^32 -1), or decrease below 0. This
                  applies both to base value and to aggregate value.
                  Note that differenly from peg counters, a roller
                  counter does not wrap (i.e. it is capped either to
                  2^32-1 or to 0)
      - MIXFOK:   the counter has been updated without errors          */
Error update_roller_scalar_ctr (uint16_t, short);

/* update_roller_vector_ctr()
   --------------------------
   This function updates a Roller Vector Counter by a specified value,
   either positive or negative. The first parameter is the Vector
   Counter ID and shall be defined in the interval (0,N-1), where N
   is the the maximum number of Vector counters defined through
   define_vector_ctr_num(). The second parameter is a pointer to an
   Instance ID. If NULL all the instances related to the concerned
   Vector counter are updated, otherwise only the specified Instance Id
   is affected. It must be included in the interval (0,P-1) where P
   is the number of instances specified in define_vector_ctr()
   The third parameter represents either the increase (if positive)
   or the decrease (if negative). 0 s a valid value (i.e. it is accepted
   even if the corresponding counter is not modified in such case).
   Possible return values are:
      - MIXFKO:   the counter ID does not exist, is outside
                  the allowed range, the specified counter
                  is a Peg Counter, the instance ID
                  is outside the allowed interval or counters
                  have not been started
      - MIXFOVFL: the counter should either exceed the maximum
                  value (i.e. 2^32 -1), or decrease below 0. This
                  applies both to base value and to aggregate value.
                  Note that differenly from peg counters, a roller
                  counter does not wrap (i.e. it is capped either to
                  2^32-1 or to 0)
      - MIXFOK:   the counter has been updated without errors            */
Error update_roller_vector_ctr (uint16_t, uint16_t*, short);

/* retrieve_peg_scalar_ctr()
   -------------------------
   This function provides back the current value of the Peg Scalar
   Counter defined by the first parameter (i.e. the Scalar Counter
   ID), which shall be defined in the interval (0,M-1),being M
   the maximum number of Scalar counters defined through
   define_scalar_ctr_num(). The second and third parameters are
   pointers to unsigned 32 bit integers, which are set respectively
   set to the current base and aggregated values of the counter
   Possible return values are:
      - MIXFKO:   the counter ID does not exist, is outside
                  the allowed range the specified counter
                  is a Roller Counter or counters have not been started
      - MIXFOK:   the counter has been extracted without errors.
                  second and third parameter contain respectively
                  the current base and aggregated values             */
Error retrieve_peg_scalar_ctr (uint16_t, uint32_t *, uint32_t *);

/* retrieve_peg_vector_ctr()
   -------------------------
   This function provides back the current value of an instance of
   the Peg Vector Counter defined by the first parameter (i.e. the
   Vector Counter ID) and the second parameter (i.e. the Instance ID).
   The first parameter shall be defined in the interval (0,N-1),
   being N the maximum number of Vector counters defined through
   define_vector_ctr_num(), while the second parameter shall be
   defined within (0,P-1) where P is the number of instances
   specified in define_vector_ctr().
   The third and fourth parameters are pointers to unsigned 32 bit
   integers, which are set respectively set to the current base and
   aggregated values of the concerned counter/instance.
   Possible return values are:
      - MIXFKO:   the counter ID does not exist, is outside
                  the allowed range, the specified counter
                  is a Roller Counter, the instance ID
                  is outside the allowed interval
                  or counters have not been started
      - MIXFOK:   the counter has been extracted without errors.
                  third and fourth parameter contain respectively
                  the current base and aggregated values              */
Error retrieve_peg_vector_ctr (uint16_t, uint16_t, uint32_t *, uint32_t *);

/* check_and_dump_ctr()
   --------------------
   When this function is invoked, it checks current time against the next
   Base and Aggr Dump Times. If they coincide, it writes a row in the
   corresponding scalar and vector files. In order for counters to be
   regularly dumped to files, this function shall be called at regular
   intervals during code execution.
   It returns MIXFKO if counters have not been defined/started,
   MIXFNOACCESS if it is not able to write counters to file, MIXFOK in any
   other case.                                                          */
Error check_and_dump_ctr (void);


#ifdef __cplusplus
} //end extern "C"
#endif

#endif /* MIXF_H_ */
