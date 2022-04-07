/****************************************************************************
 * -----------------------------------------                                *
 * C/C++ Mixed Functions Library (libmixf)                                  *
 * -----------------------------------------                                *
 * Copyright 2019-2021 Roberto Mameli                                       *
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
 * FILE:        mixfApi.h                                                   *
 * VERSION:     2.1.0                                                       *
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
 * REV HISTORY: See updated Revision History in file RevHistory.txt         *
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

#ifndef MIXFAPI_H_
#define MIXFAPI_H_

/*******************************
 * General Purpose Definitions *
 *******************************/
#define EVENTARRAYSIZE      255     /* Maximum number of events handled by the library */
#define MAXPARAMARRAYSIZE   255     /* Maximum number of parameters handled by the library */
#define DEFPARAMARRAYSIZE     8     /* Default number of parameters handled by the library */
#define MAXLOGLEVELS          8     /* Maximum number of log levels handled by the library */

#define MICROSTRINGMAXLEN    16
#define SHORTSTRINGMAXLEN    32
#define MEDIUMSTRINGMAXLEN  128
#define MEDIUM2STRINGMAXLEN 256
#define LONGSTRINGMAXLEN    512

#define MAGICCHAR           ' '     /* ASCII code is 32 - introduced to avoid that encryption produces a null terminated string */

/* v.2.0.0 */
#define MAXSCALARCTRNUM    1024     /* Max number of Scalar Counters */
#define MAXVECTORCTRNUM    1024     /* Max number of Vector Counters */
#define MAXVECTORCTRINST  65536     /* Max number of collected Vector Counters Instances */
#define MAXCTRVALUE  4294967295     /* Max value for a counter (2^32-1), after that the counter overflows */
#define MAXAGGRDUMPTIMES    100     /* Max number of dump times in a day (in the form hhmm) for aggregated counters */


/********************
 * Type Definitions *
 ********************/
typedef char        MicroString[MICROSTRINGMAXLEN];
typedef char        ShortString[SHORTSTRINGMAXLEN];
typedef char        MediumString[MEDIUMSTRINGMAXLEN];
typedef char        Medium2String[MEDIUM2STRINGMAXLEN];
typedef char        LongString[LONGSTRINGMAXLEN];

/* Enum type that is used to classify the type of parameters allowed in Configuration Files */
typedef enum paramtype
{
    numerical,
    literal,
    filename,
    character,
    /* New values added in v.2.1.0 */
    email,
    url,
    ipv4
} ParamType;

/* Base type for the array of events used by all log handling routines */
typedef struct eventinfo
{
    MediumString    Descr[4];
    uint8_t         Level,NumParams;
} EventInfo;

/* Base type for the array of parameters used by ParseCfgParamFile() routine */
typedef struct param
{
    ShortString     Name;
    Boolean         Mandatory, Provisioned;
    ParamType       Type;
    EventCode       MandNotProv,    /* Event issued when Mandatory=TRUE and Provisioned=FALSE */
                    OptNotProv,     /* Event issued when Mandatory=FALSE and Provisioned=FALSE */
                    Redefined,      /* Event issued when Provisioned=TRUE and parameter is provisioned at least twice */
                    MalfOrOOR;      /* Event issued when the parameter is malformed or Out Of Range */
    union
    {
        struct
        {   /* Fields specific for numerical type parameters */
            int   Min, Max, Def, Val;
        } Num;
        struct
        {   /* Fields specific for character type parameters */
            char   Min, Max, Def, Val;
        } Car;
        struct
        {   /* Fields specific for literal, filename and mail type parameters */
            MediumString Def, Val;
        } Lit;
        struct
        {   /* Fields specific for url type parameters */
            Medium2String Def, Val;
        }   Url;
        struct
        {   /* Fields specific for ipv4 type parameters */
            ShortString Def, Val;
        }   ipv4;
    } Values;
} Param;

/* Base types for counters handling (v.2.0.0) */
typedef uint8_t         CounterType;    /* Either PEGCTR or ROLLERCTR */

typedef struct ScalarCtrInfo            /* Structure for Scalar counter (either PEGCTR or ROLLERCTR) */
{   /* Each element requires 32+1+4+4 = 41 bytes */
    ShortString         Name;
    CounterType         Type;
    uint32_t            BaseVal,
                        AggrVal;
} scalarCtrInfo;

typedef struct VectorCtrInfo            /* Structure for Vector counter (either PEGCTR or ROLLERCTR) */
{   /* Each element requires 32+32+1+2+8+8+8+8+8+ numinst x(4+4+16) = 107 + numinst x 24 bytes */
    ShortString         Name,
                        InstName;
    CounterType         Type;
    uint16_t            NumInstances;
    uint32_t            *BaseVal,
                        *AggrVal;
    MicroString         *InstIdName;
    FILE                *BaseCtr_fd,
                        *AggrCtr_fd;
} vectorCtrInfo;

#endif /* MIXFAPI_H_ */
