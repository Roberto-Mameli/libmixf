/****************************************************************************
 * -----------------------------------------                                *
 * C/C++ Mixed Functions Library (libmixf)                                  *
 * -----------------------------------------                                *
 * Copyright 2019-2025 Roberto Mameli                                       *
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
 * FILE:        mixfCounters.c                                              *
 * VERSION:     3.0.0                                                       *
 * AUTHOR(S):   Roberto Mameli                                              *
 * PRODUCT:     Library libmixf - general purpose library                   *
 * DESCRIPTION: This source file contains the implementation of a subset of *
 *              the functions provided by libmixf library, specifically:    *
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


/**************************
 *                        *
 *   Linux system files   *
 *                        *
 **************************/
#define _XOPEN_SOURCE 500   /* glibc (2.12 or above) needs this for proper handling of the following
                               library functions: lstat(), gethostid(), gethostname() and strptime() */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <pthread.h>


/****************************
 *                          *
 *   Library system files   *
 *                          *
 ****************************/
#include "mixf.h"
#include "mixfApi.h"


/*********************************
 *                               *
 *   Global private variables    *
 *                               *
 *********************************/
/* Private variables for Counters Handling */
static uint16_t         numScalarCtr = 0,                     /* Number of Scalar Counters, between 0 and MAXSCALARCTRNUM */
                        numVectorCtr = 0;                     /* Number of Vector Counters, between 0 and MAXVECTORCTRNUM */
static uint32_t         cumVectorInst = 0;                    /* Cumulative Number of Instances for Vector Counters (<=MAXVECTORCTRINST) */
static ScalarCtrInfo    scalarCtr[MAXSCALARCTRNUM];           /* Array of Scalar Counters */
static VectorCtrInfo    vectorCtr[MAXVECTORCTRNUM];           /* Array of Vector Counters */
static MediumString     BaseCtrDir = "",                      /* Path to the directory in which Base counters are collected */
                        AggrCtrDir = "";                      /* Path to the directory in which Aggr counters are collected */
static LongString       BaseDumpTimes = "";                   /* String containing base dump times (third parameter of define_base_dump) */
static ExtendedString   AggrDumpTimes = "";                   /* String containing aggr dump times (third parameter of define_aggr_dump) */
static MicroString      BaseCtrTimeStampFormat = "%d%m%Y",    /* String containing base dump time stamp format (second parameter of define_base_dump) */
                        AggrCtrTimeStampFormat = "%d%m%Y",    /* String containing aggr dump time stamp format (second parameter of define_aggr_dump) */
                        BaseDumpOpenDate = "",                /* Date when the current base ctr file was opened, local to the library */
                        AggrDumpOpenDate = "";                /* Date when the current aggr ctr file was opened, local to the library */
char                    *BaseNextDump = NULL,                 /* Pointer within BaseDumpTimes to the next dump time */
                        *AggrNextDump = NULL;                 /* Pointer within AggrDumpTimes to the next dump time */
static bool             BaseCtrActive = false,                /* Flag used to understand whether the base ctr file is open or not */
                        AggrCtrActive = false;                /* Flag used to understand whether the aggr ctr file is open or not */
static FILE             *BaseCtr_fd = NULL;                   /* File descriptor for base scalar counters */
static FILE             *AggrCtr_fd = NULL;                   /* File descriptor for aggregated scalar counters */
static pthread_mutex_t  BaseMutex;                            /* Mutex used to handle cuncurrent access to Base Counter file */
static pthread_mutex_t  AggrMutex;                            /* Mutex used to handle cuncurrent access to Aggr Counter file */


/*******************************
 *                             *
 *      Static Functions       *
 * (only visible in this file) *
 *                             *
 *******************************/
/*
 * This in an internal function that closes and reopens all existing base counters file
 * without handling locks. It cannot be invoked from outside the library, it is part of the
 * implementation of the check_and_dump_ctr() function, which invokes it when it needs
 * to rotate counters file at 00:00.
 * BE AWARE that it doesn't set/clear locks, which are managed within the calling
 * function.
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if base counters are not open,
 * MIXFNOACCESS if the filenames cannot be reopened.
 */
static Error CloseReopenBaseCounters(void)
{
    /* Local variables */
    LongString   DumpFile;
    ShortString  TimeStamp;
    int          i,j;

    /* Do not manage MUTEX. They are handled by calling function */
    if (BaseCtrActive==false)   /* Base counters not running - return MIXFKO error */
        return (MIXFKO);

    /* Evaluates and keeps the date of counters reopening in ddmmyyyy format */
    /* This is stored for Rotation */
    retrieve_time_date(BaseDumpOpenDate,"%d%m%Y");

    /* Close the existing base counters files */
    fclose (BaseCtr_fd);
    BaseCtr_fd = NULL;
    for (i = 0; i<numVectorCtr; i++)
    {
        fclose (vectorCtr[i].BaseCtr_fd);
        vectorCtr[i].BaseCtr_fd = NULL;
    }

    /* Evaluate Time Stamp */
    retrieve_time_date(TimeStamp, BaseCtrTimeStampFormat);

    /* Open first scalar counters file... */
    sprintf(DumpFile, "%sscalar_%s.csv", BaseCtrDir, TimeStamp);
    if ( (BaseCtr_fd=fopen(DumpFile,"a")) == NULL)
        return (MIXFNOACCESS);
    fprintf(BaseCtr_fd, "Date,Time,");
    for (j = 0; j < (numScalarCtr - 1); j++)
        fprintf(BaseCtr_fd, "%s,", scalarCtr[j].Name);
    fprintf(BaseCtr_fd, "%s\n", scalarCtr[numScalarCtr - 1].Name);

    /* ... then open again vector counters files */
    for (i = 0; i<numVectorCtr; i++)
    {
        sprintf(DumpFile, "%svector_%d_%s.csv", BaseCtrDir, i, TimeStamp);
        if ((vectorCtr[i].BaseCtr_fd = fopen(DumpFile, "a")) == NULL)
            return (MIXFNOACCESS);
        fprintf(vectorCtr[i].BaseCtr_fd, "Vector Counter: %s - Instances: %s\nDate,Time,", vectorCtr[i].Name, vectorCtr[i].InstName);
        for (j = 0; j < (vectorCtr[i].NumInstances - 1); j++)
            fprintf(vectorCtr[i].BaseCtr_fd, "%s,", vectorCtr[i].InstIdName[j]);
        fprintf(vectorCtr[i].BaseCtr_fd, "%s\n", vectorCtr[i].InstIdName[j]);
    }

    return (MIXFOK);
}


/*
 * This in an internal function that closes and reopens all existing aggr counters file
 * without handling locks. It cannot be invoked from outside the library, it is part of the
 * implementation of the check_and_dump_ctr() function, which invokes it when it needs
 * to rotate counters file at 00:00.
 * BE AWARE that it doesn't set/clear locks, which are managed within the calling
 * function.
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if aggr counters are not open,
 * MIXFNOACCESS if the filenames cannot be reopened.
 */
static Error CloseReopenAggrCounters(void)
{
    /* Local variables */
    LongString  DumpFile;
    ShortString TimeStamp;
    int         i,j;

    /* Do not manage MUTEX. They are handled by calling function */
    if (AggrCtrActive==false)   /* Aggr counters not running - return MIXFKO error */
        return (MIXFKO);

    /* Evaluates and keeps the date of counters reopening in ddmmyyyy format */
    /* This is stored for Rotation */
    retrieve_time_date(AggrDumpOpenDate,"%d%m%Y");

    /* Close the existing base counters files */
    fclose (AggrCtr_fd);
    AggrCtr_fd = NULL;
    for (i = 0; i<numVectorCtr; i++)
    {
        fclose (vectorCtr[i].AggrCtr_fd);
        vectorCtr[i].AggrCtr_fd = NULL;
    }

    /* Evaluate Time Stamp */
    retrieve_time_date(TimeStamp, AggrCtrTimeStampFormat);

    /* Open first scalar counters file... */
    sprintf(DumpFile, "%sscalar_aggr_%s.csv", AggrCtrDir, TimeStamp);
    if ( (AggrCtr_fd=fopen(DumpFile,"a")) == NULL)
        return (MIXFNOACCESS);
    fprintf(AggrCtr_fd, "Date,Time,");
    for (j = 0; j < (numScalarCtr - 1); j++)
        fprintf(AggrCtr_fd, "%s,", scalarCtr[j].Name);
    fprintf(AggrCtr_fd, "%s\n", scalarCtr[numScalarCtr - 1].Name);

    /* ... then open again vector counters files */
    for (i = 0; i<numVectorCtr; i++)
    {
        sprintf(DumpFile, "%svector_%d_aggr_%s.csv", AggrCtrDir, i, TimeStamp);
        if ((vectorCtr[i].AggrCtr_fd = fopen(DumpFile, "a")) == NULL)
            return (MIXFNOACCESS);
        fprintf(vectorCtr[i].AggrCtr_fd, "Vector Counter: %s - Instances: %s\nDate,Time,", vectorCtr[i].Name, vectorCtr[i].InstName);
        for (j = 0; j < (vectorCtr[i].NumInstances - 1); j++)
            fprintf(vectorCtr[i].AggrCtr_fd, "%s,", vectorCtr[i].InstIdName[j]);
        fprintf(vectorCtr[i].AggrCtr_fd, "%s\n", vectorCtr[i].InstIdName[j]);
    }

    return (MIXFOK);
}


/***********************************
 *                                 *
 *        Visible Functions        *
 * (can be used outside this file  *
 *  and are part of the library)   *
 *                                 *
 ***********************************/
/* ---------------------------
 * Counters Handling (v.2.0.0)
 * --------------------------- */
/*
 * Define the maximum number of Scalar counters, between 0 and 1024.
 * Scalar counters are global counters, i.e. they refer to the whole
 * piece of code and are not related to a specific object/instance.
 * The function returns MIXFOK in case of success, MIXFKO in any other case
 * In case of success, internal counter structures are reset
 * and any previous counter definition is lost
 * Please observe that this function cannot be called after start_counters()
 */
Error define_scalar_ctr_num(uint16_t numcounters)
{
    int i;

    if (BaseCtrActive == true)      /*if counters have already been started return error */
        return (MIXFKO);

    if (numcounters > MAXSCALARCTRNUM)
        return (MIXFKO);

    numScalarCtr = numcounters;

    /* Reset internal counter structures */
    for (i = 0; i < MAXSCALARCTRNUM; i++)
    {
        scalarCtr[i].Name[0] = '\0';
        scalarCtr[i].Type = 0;
        scalarCtr[i].BaseVal = 0;
        scalarCtr[i].AggrVal = 0;
    }
    if (BaseCtr_fd != NULL)
    {
        fclose(BaseCtr_fd);
        BaseCtr_fd = NULL;
    }
    if (AggrCtr_fd != NULL)
    {
        fclose(AggrCtr_fd);
        AggrCtr_fd = NULL;
    }

    return (MIXFOK);
}


/*
 * Define the maximum number of Vector counters, between 0 and 1024.
 * Vector counters are collected for a set of objects/instances of the
 * same type. For example the number of bytes sent over a specific TCP
 * connection shall be considered a Vector counter (the counter is one,
 * i.e. the number of bytes sent, but given N the number of TCP
 * connections, it comes in N instances). Be aware that the library
 * can collect up to 65536 total instances of Vector counters.
 * Here follows some examples of allowed combinations:
 *   - 2 Vector counters (each with up to 32768 instances)
 *   - 3 Vector counters (e.g. the first with up to 32768 instances,
 *       the others up to 16536 instances)
 *   - 1024 Vector counters of 64 instances each
 * or any other combination, given that the total amount of collected
 * counter instances does not exceed 65536.
 * The function returns MIXFOK in case of success, MIXFKO in any other case
 * In case of success, internal counter structures are reset
 * and any previous counter definition is lost
 * Please observe that this function cannot be called after start_counters()
 */
Error define_vector_ctr_num(uint16_t numcounters)
{
    int     i;

    if (BaseCtrActive == true)      /*if counters have already been started return error */
        return (MIXFKO);

    if (numcounters > MAXVECTORCTRNUM)
        return (MIXFKO);

    numVectorCtr = numcounters;
    cumVectorInst = 0;

    /* Reset internal counter structures */
    for (i = 0; i < MAXVECTORCTRNUM; i++)
    {
        vectorCtr[i].Name[0] = '\0';
        vectorCtr[i].InstName[0] = '\0';
        vectorCtr[i].Type = 0;
        vectorCtr[i].NumInstances = 0;
        if (vectorCtr[i].BaseVal)
            free(vectorCtr[i].BaseVal);
        vectorCtr[i].BaseVal = NULL;
        if (vectorCtr[i].AggrVal)
            free(vectorCtr[i].AggrVal);
        vectorCtr[i].AggrVal = NULL;
        if (vectorCtr[i].InstIdName)
            free(vectorCtr[i].InstIdName);
        vectorCtr[i].InstIdName = NULL;
        if (vectorCtr[i].BaseCtr_fd != NULL)
        {
            fclose(vectorCtr[i].BaseCtr_fd);
            vectorCtr[i].BaseCtr_fd = NULL;
        }
        if (vectorCtr[i].AggrCtr_fd != NULL)
        {
            fclose(vectorCtr[i].AggrCtr_fd);
            vectorCtr[i].AggrCtr_fd = NULL;
        }
    }
    return (MIXFOK);
}


/* The first parameter is the Scalar Counter ID and shall be defined
 * in the interval (0,M-1), where M is the the maximum number of
 * Scalar counters defined through define_scalar_ctr_num(). The second
 * parameter specifies the counter type (PEGCTR or ROLLERCTR).
 * PEG Counters are counters that are characterized by the following
 * properties: they have initial value set to 0, they can only increase
 * and they are reset every time that counters are dumped to file
 * (i.e. at each base interval). ROLLER Counters are slightly different:
 * they can have a non null initial value, they can increase and decrease
 * (but never become negative) and when they are dumped to file,
 * they are not reset. The third parameter represents the initial value
 * for the counter and is valid only for ROLLER Counters (it is meaningless
 * in case that counter type is PEGCTR).
 * The fourth parameter is a string that provides the counter name (up to 32
 * characters, otherwise it is truncated).
 * This function provides MIXFKO either in case of wrong parameters (e.g. outside allowed
 * ranges) or in case of counters already started, MIXFOK if everything is ok */
Error define_scalar_ctr(uint16_t ctrId, uint8_t ctrType, uint32_t ctrInitial, char* ctrName)
{
    if (BaseCtrActive == true)      /*if counters have already been started return error */
        return (MIXFKO);

    if (ctrId >= numScalarCtr)
        return (MIXFKO);

    if ((ctrType != PEGCTR) && (ctrType != ROLLERCTR))
        return (MIXFKO);

    if (strlen(ctrName) > SHORTSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 32 characters */
        strncpy(scalarCtr[ctrId].Name, ctrName, SHORTSTRINGMAXLEN);
        scalarCtr[ctrId].Name[SHORTSTRINGMAXLEN] = '\0';
    }
    else
        strcpy(scalarCtr[ctrId].Name, ctrName);
    scalarCtr[ctrId].Type = ctrType;
    if (ctrType == PEGCTR)
    {   /* PEG Counter - Initial value always NULL */
        scalarCtr[ctrId].BaseVal = 0;
        scalarCtr[ctrId].AggrVal = 0;
    }
    else
    {   /* ROLLER Counter - Initial value set by caller */
        scalarCtr[ctrId].BaseVal = ctrInitial;
        scalarCtr[ctrId].AggrVal = ctrInitial;
    }

    return (MIXFOK);
}


/* The first parameter is the Vector Counter ID and shall be defined
 * in the interval (0,N-1), where N is the the maximum number of
 * Vector counters defined through define_vector_ctr_num(). The second
 * parameter is the maximum number of instances allowed for the
 * Vector Counter ID specified by parameter 1. It shall be at least 1.
 * The maximum value is limited by the fact that the maximum
 * number of instances cannot exceed 65536. The third
 * parameter specifies the counter type(PEGCTR or ROLLERCTR).
 * PEG Counters are counters that are characterized by the following
 * properties: they have initial value set to 0, they can only increase
 * and they are reset every time that counters are dumped to file
 * (i.e.at each base interval).ROLLER Counters are slightly different :
 * they can have a non null initial value, they can increaseand decrease
 * (but never become negative) and when they are dumped to file,
 * they are not reset.The fourth parameter represents the initial value
 * for the counter and is valid only for ROLLER Counters(it is meaningless
 * in case that counter type is PEGCTR).This value is assigned to all
 * intances of the counter.
 * The fifth parameter is a string that provides the counter name (up to 32
 * characters, otherwise it is truncated).The sixth parameter is another
 * string(up to 32 characters length, otherwise it is truncated) that
 * provides the name of the object associated to instances.
 * Example: referring to the number of bytes sent for different TCP
 * connections, assuming that this is a ROLLER Counter whose ID is 12
 * andthat the maximum number of TCP Connections is 512, the call
 * might be :
 *      define_vector_ctr(12, 512, ROLLERCTR, 0, "Total Bytes Sent", "TCP Conn. ID")
 * In this case the initial value for the counter has been set to 0 (for
 * all the 512 TCP connections)
 * This function provides MIXFKO either in case of wrong parameters(e.g.outside allowed
 * ranges) or in case of counters already started, MIXFOVFL if the number of cumulative
 * instances of Vector Counters up to function call exceeds 65536, MIXFOK if everything is ok
 */
Error define_vector_ctr(uint16_t ctrId, uint16_t ctrInst, uint8_t ctrType, uint32_t ctrInitial, char* ctrName, char* instName)
{
    uint32_t    cum;
    int         i;

    if (BaseCtrActive == true)      /*if counters have already been started return error */
        return (MIXFKO);

    if ( (ctrId >= numVectorCtr) || (ctrInst < 1) )
        return (MIXFKO);

    cum = cumVectorInst + ctrInst;

    if (cum > MAXVECTORCTRINST)
        return (MIXFOVFL);

    if ((ctrType != PEGCTR) && (ctrType != ROLLERCTR))
        return (MIXFKO);

    if (strlen(ctrName) > SHORTSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 32 characters */
        strncpy(vectorCtr[ctrId].Name, ctrName, SHORTSTRINGMAXLEN);
        vectorCtr[ctrId].Name[SHORTSTRINGMAXLEN] = '\0';
    }
    else
        strcpy(vectorCtr[ctrId].Name, ctrName);

    if (strlen(instName) > SHORTSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 32 characters */
        strncpy(vectorCtr[ctrId].InstName, instName, SHORTSTRINGMAXLEN);
        vectorCtr[ctrId].InstName[SHORTSTRINGMAXLEN] = '\0';
    }
    else
        strcpy(vectorCtr[ctrId].InstName, instName);

    vectorCtr[ctrId].Type = ctrType;
    vectorCtr[ctrId].NumInstances = ctrInst;
    vectorCtr[ctrId].BaseVal = (uint32_t*)calloc((size_t)ctrInst, (size_t) sizeof(uint32_t));
    vectorCtr[ctrId].AggrVal = (uint32_t*)calloc((size_t)ctrInst, (size_t) sizeof(uint32_t));
    vectorCtr[ctrId].InstIdName = (MicroString *)calloc((size_t)ctrInst, MICROSTRINGMAXLEN);

    for (i = 0; i < ctrInst; i++)
    {
        vectorCtr[ctrId].InstIdName[i][0] = '\0';   /* Instance ID Name initially set to empty string */
        if (ctrType == PEGCTR)  /* PEG Counter - Initial values always NULL for all instances */
            vectorCtr[ctrId].BaseVal[i] = vectorCtr[ctrId].AggrVal[i] = 0;
        else                    /* ROLLER Counter - Initial values set for all instances */
            vectorCtr[ctrId].BaseVal[i] = vectorCtr[ctrId].AggrVal[i] = ctrInitial;
    }

    /* Update cumulated number of instances */
    cumVectorInst = cum;

    return (MIXFOK);
}


/*
 * This function is used to associate a name to an instance of a Vector Counter
 * The first parameter specifies the Vector Counter ID, the second one represents
 * the Instance Id. They shall be defined within the limits specified through
 * define_vector_ctr_num() and define_vector_ctr(), otherwise MIXFKO is returned.
 * The third parameter is a string that specifies the name of the concerned
 * instance (e.g. if the counter is used to collect Total Bytes Sent for
 * several TCP connections, this name could be used to store the connection ID).
 * If NULL, the call has no effect (i.e. the name is not changed), otherwise it
 * is overwritten. Please observe that up to 16 chars are allowed, if more
 * the name is truncated.
 * Please observe that this function can be called even if counters have been
 * already started
 */
Error set_vector_ctr_inst_name(uint16_t ctrId, uint16_t ctrInst, char *instIdName)
{
    if ((ctrId >= numVectorCtr) || (ctrInst >= vectorCtr[ctrId].NumInstances) )
        return (MIXFKO);

    if (instIdName == NULL) /* If instIdName is not specified */
        return (MIXFOK);

    if (strlen(instIdName) >= MICROSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 16 characters */
        strncpy(vectorCtr[ctrId].InstIdName[ctrInst], instIdName, MICROSTRINGMAXLEN);
        vectorCtr[ctrId].InstIdName[ctrInst][MICROSTRINGMAXLEN] = '\0';
    }
    else
        strcpy(vectorCtr[ctrId].InstIdName[ctrInst], instIdName);

    return (MIXFOK);
}


/*
 * This function increases a Peg Scalar Counter by one.
 * The only parameter is the Scalar Counter ID and shall be defined
 * in the interval (0,M-1), where M is the the maximum number of
 * Scalar counters defined through define_scalar_ctr_num().
 * Possible return values are:
 *    - MIXFKO:   the counter ID does not exist, is outside
 *                the allowed range, the specified counter
 *                is a Roller Counter or counters have
 *                not been started
 *    - MIXFOVFL: the counter has wrapped around the maximum
 *                value (i.e. 2^32 -1). This applies both to base
 *                value and to aggregate value. Note that the counter
 *                is increased anyway (i.e. the new value is 0)
 *    - MIXFOK:   the counter has been increased without errors
 */
Error incr_peg_scalar_ctr(uint16_t ctrId)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == false)     /* if counters have not been started return error */
        return (MIXFKO);

    if ( (ctrId >= numScalarCtr) || (scalarCtr[ctrId].Type != PEGCTR) )
        return (MIXFKO);

    if (scalarCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through define_scalar_ctr() */
        return (MIXFKO);

    if ((scalarCtr[ctrId].BaseVal == MAXCTRVALUE) || (scalarCtr[ctrId].AggrVal == MAXCTRVALUE))
        res = MIXFOVFL;

    scalarCtr[ctrId].BaseVal += 1;
    scalarCtr[ctrId].AggrVal += 1;

    return(res);

}


/*
 * This function increases a Peg Vector Counter by one.
 * The first parameter is the Scalar Counter ID and shall be defined
 * in the interval (0,N-1), where N is the the maximum number of
 * Vector counters defined through define_vector_ctr_num().
 * The second parameter is a pointer to a Instance ID. If NULL
 * all the instances related to the concerned Vector counter
 * are increased by 1, otherwise only the specified Instance Id
 * is increased. It must be included in the interval (0,P-1)
 * where P is the number of instances specified in
 * define_vector_ctr().
 * Possible return values are:
 *    - MIXFKO:   the counter ID does not exist, is outside
 *                the allowed range, the specified counter
 *                is a Roller Counter, the instance ID
 *                is outside the allowed interval or counters
 *                have not been started
 *    - MIXFOVFL: the counter has wrapped around the maximum
 *                value (i.e. 2^32 -1). This applies both to base
 *                value and to aggregate value. Note that the counter
 *                is increased anyway (i.e. the new value is 0)
 *    - MIXFOK:   the counter has been increased without errors
 */
Error incr_peg_vector_ctr(uint16_t ctrId, uint16_t* ctrInst)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == false)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numVectorCtr) || (vectorCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (vectorCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through define_vector_ctr() */
        return (MIXFKO);

    if (ctrInst != NULL)
    {   /* Update a specific instance */
        if ((*ctrInst) >= vectorCtr[ctrId].NumInstances)
            return (MIXFKO);
        if ((vectorCtr[ctrId].BaseVal[*ctrInst] == MAXCTRVALUE) || (vectorCtr[ctrId].AggrVal[*ctrInst] == MAXCTRVALUE))
            res = MIXFOVFL;
        vectorCtr[ctrId].BaseVal[*ctrInst] += 1;
        vectorCtr[ctrId].AggrVal[*ctrInst] += 1;
    }   /* if (ctrInst != NULL) */
    else
    {   /* Update all instances */
        int     i;
        for (i = 0; i < vectorCtr[ctrId].NumInstances; i++)
        {
            if ((vectorCtr[ctrId].BaseVal[i] == MAXCTRVALUE) || (vectorCtr[ctrId].AggrVal[i] == MAXCTRVALUE))
                res = MIXFOVFL;
            vectorCtr[ctrId].BaseVal[i] += 1;
            vectorCtr[ctrId].AggrVal[i] += 1;
        }   /* for (i = 0; i < vectorCtr[ctrId]... */

    }   /* else if (ctrInst != NULL) */

    return(res);
}


/*
 * This function provides back the current value of the Peg Scalar
 * Counter defined by the first parameter (i.e. the Scalar Counter
 * ID), which shall be defined in the interval (0,M-1),being M the
 * maximum number of Scalar counters defined through
 * define_scalar_ctr_num(). The second and third parameters are
 * pointers to unsigned 32 bit integers, which are set respectively
 * set to the current base and aggregated values of the counter
 * Possible return values are:
 *    - MIXFKO:   the counter ID does not exist, is outside
 *                the allowed range, the specified counter
 *                is a Roller Counter or counters have not been started
 *    - MIXFOK:   the counter has been extracted without errors.
 *                second and third parameter contain respectively
 *                the current base and aggregated values
 */
Error retrieve_peg_scalar_ctr(uint16_t ctrId, uint32_t* ctrBase, uint32_t* ctrAggr)
{
    if (BaseCtrActive == false)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numScalarCtr) || (scalarCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (scalarCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through define_scalar_ctr() */
        return (MIXFKO);

    *ctrBase = scalarCtr[ctrId].BaseVal;
    *ctrAggr = scalarCtr[ctrId].AggrVal;

    return (MIXFOK);
}


/*
 * This function provides back the current value of an instance of
 * the Peg Vector Counter defined by the first parameter (i.e. the
 * Vector Counter ID) and the second parameter (i.e. the Instance ID).
 * The first parameter shall be defined in the interval (0,N-1),
 * being N the maximum number of Vector counters defined through
 * define_vector_ctr_num(), while the second parameter shall be
 * defined within (0,P-1) where P is the number of instances
 * specified in define_vector_ctr().
 * The third and fourth parameters are pointers to unsigned 32 bit
 * integers, which are set respectively set to the current base and
 * aggregated values of the concerned counter/instance.
 * Possible return values are:
 *    - MIXFKO:   the counter ID does not exist, is outside
 *                the allowed range, the specified counter
 *                is a Roller Counter, the instance ID
 *                is outside the allowed interval
 *                or counters have not been started
 *    - MIXFOK:   the counter has been extracted without errors.
 *                third and fourth parameter contain respectively
 *                the current base and aggregated values
 */
Error retrieve_peg_vector_ctr(uint16_t ctrId, uint16_t ctrInst, uint32_t* ctrBase, uint32_t* ctrAggr)
{
    if (BaseCtrActive == false)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numVectorCtr) || (vectorCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (vectorCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through define_vector_ctr() */
        return (MIXFKO);

    if (ctrInst >= vectorCtr[ctrId].NumInstances)
        return (MIXFKO);

    *ctrBase = vectorCtr[ctrId].BaseVal[ctrInst];
    *ctrAggr = vectorCtr[ctrId].AggrVal[ctrInst];

    return (MIXFOK);
}


/*
 * This function updates a Roller Scalar Counter by a specified value,
 * either positive or negative. The first parameter is the Scalar
 * Counter ID and shall be defined in the interval (0,M-1), where M
 * is the the maximum number of Scalar counters defined through
 * define_scalar_ctr_num(). The second parameter represents either
 * the increase (if positive) or the decrease (if negative). 0
 * is a valid value (i.e. it is accepted even if the corresponding
 * counter is not modified in such case).
 * Possible return values are:
 *    - MIXFKO:   the counter ID does not exist, is outside
 *                the allowed range, the specified counter
 *                is a Peg Counter or counters have not been started
 *    - MIXFOVFL: the counter should either exceed the maximum
 *                value (i.e. 2^32 -1), or decrease below 0. This
 *                applies both to base value and to aggregate value.
 *                Note that differenly from peg counters, a roller
 *                counter does not wrap (i.e. it is capped either to
 *                2^32-1 or to 0)
 *    - MIXFOK:   the counter has been updated without errors
 */
Error update_roller_scalar_ctr(uint16_t ctrId, short delta)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == false)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numScalarCtr) || (scalarCtr[ctrId].Type != ROLLERCTR))
        return (MIXFKO);

    if (scalarCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through define_scalar_ctr() */
        return (MIXFKO);

    if ( (delta>0) && ((scalarCtr[ctrId].BaseVal+delta) < scalarCtr[ctrId].BaseVal) )
    {   /* delta is positive and the base counter would wrap over the maximum allowed value */
        scalarCtr[ctrId].BaseVal = MAXCTRVALUE;
        res = MIXFOVFL;
    }
    else if ((delta < 0) && ((scalarCtr[ctrId].BaseVal + delta) > scalarCtr[ctrId].BaseVal))
    {   /* delta is negative and the base counter would become negative */
        scalarCtr[ctrId].BaseVal = 0;
        res = MIXFOVFL;
    }
    else
        scalarCtr[ctrId].BaseVal += delta;

    if ((delta > 0) && ((scalarCtr[ctrId].AggrVal + delta) < scalarCtr[ctrId].AggrVal))
    {   /* delta is positive and the aggregate counter would wrap over the maximum allowed value */
        scalarCtr[ctrId].AggrVal = MAXCTRVALUE;
        res = MIXFOVFL;
    }
    else if ((delta < 0) && ((scalarCtr[ctrId].AggrVal + delta) > scalarCtr[ctrId].AggrVal))
    {   /* delta is negative and the aggregate counter would become negative */
        scalarCtr[ctrId].AggrVal = 0;
        res = MIXFOVFL;
    }
    else
        scalarCtr[ctrId].AggrVal += delta;

    return(res);
}


/*
 * This function updates a Roller Vector Counter by a specified value,
 * either positive or negative. The first parameter is the Vector
 * Counter ID and shall be defined in the interval (0,N-1), where N
 * is the the maximum number of Vector counters defined through
 * define_vector_ctr_num(). The second parameter is a pointer to an
 * Instance ID. If NULL all the instances related to the concerned
 * Vector counter are updated, otherwise only the specified Instance Id
 * is affected. It must be included in the interval (0,P-1) where P
 * is the number of instances specified in define_vector_ctr()
 * The third parameter represents either the increase (if positive)
 * or the decrease (if negative). 0 s a valid value (i.e. it is accepted
 * even if the corresponding counter is not modified in such case).
 * Possible return values are:
 *    - MIXFKO:   the counter ID does not exist, is outside
 *                the allowed range, the specified counter
 *                is a Peg Counter, the instance ID
 *                is outside the allowed interval or counters
 *                have not been started
 *    - MIXFOVFL: the counter should either exceed the maximum
 *                value (i.e. 2^32 -1), or decrease below 0. This
 *                applies both to base value and to aggregate value.
 *                Note that differenly from peg counters, a roller
 *                counter does not wrap (i.e. it is capped either to
 *                2^32-1 or to 0)
 *    - MIXFOK:   the counter has been updated without errors
 */
Error update_roller_vector_ctr(uint16_t ctrId, uint16_t *ctrInst, short delta)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == false)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numVectorCtr) || (vectorCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (vectorCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through define_vector_ctr() */
        return (MIXFKO);

    if (ctrInst != NULL)
    {   /* Update a specific instance */
        if ((*ctrInst) >= vectorCtr[ctrId].NumInstances)
            return (MIXFKO);

        if ( (delta>0) && ((vectorCtr[ctrId].BaseVal[*ctrInst]+delta) < vectorCtr[ctrId].BaseVal[*ctrInst]) )
        {   /* delta is positive and the base counter would wrap over the maximum allowed value */
            vectorCtr[ctrId].BaseVal[*ctrInst] = MAXCTRVALUE;
            res = MIXFOVFL;
        }
        else if ((delta < 0) && ((vectorCtr[ctrId].BaseVal[*ctrInst] + delta) > vectorCtr[ctrId].BaseVal[*ctrInst]))
        {   /* delta is negative and the base counter would become negative */
            vectorCtr[ctrId].BaseVal[*ctrInst] = 0;
            res = MIXFOVFL;
        }
        else
            vectorCtr[ctrId].BaseVal[*ctrInst] += delta;

        if ((delta > 0) && ((vectorCtr[ctrId].AggrVal[*ctrInst] + delta) < vectorCtr[ctrId].AggrVal[*ctrInst]))
        {   /* delta is positive and the aggregate counter would wrap over the maximum allowed value */
            vectorCtr[ctrId].AggrVal[*ctrInst] = MAXCTRVALUE;
            res = MIXFOVFL;
        }
        else if ((delta < 0) && ((vectorCtr[ctrId].AggrVal[*ctrInst] + delta) > vectorCtr[ctrId].AggrVal[*ctrInst]))
        {   /* delta is negative and the aggregate counter would become negative */
            vectorCtr[ctrId].AggrVal[*ctrInst] = 0;
            res = MIXFOVFL;
        }
        else
            vectorCtr[ctrId].AggrVal[*ctrInst] += delta;
    }   /* if (ctrInst != NULL) */
    else
    {   /* Update all instances */
        int     i;

        for (i = 0; i < vectorCtr[ctrId].NumInstances; i++)
        {
            if ( (delta>0) && ((vectorCtr[ctrId].BaseVal[i]+delta) < vectorCtr[ctrId].BaseVal[i]) )
            {   /* delta is positive and the base counter would wrap over the maximum allowed value */
                vectorCtr[ctrId].BaseVal[i] = MAXCTRVALUE;
                res = MIXFOVFL;
            }
            else if ((delta < 0) && ((vectorCtr[ctrId].BaseVal[i] + delta) > vectorCtr[ctrId].BaseVal[i]))
            {   /* delta is negative and the base counter would become negative */
                vectorCtr[ctrId].BaseVal[i] = 0;
                res = MIXFOVFL;
            }
            else
                vectorCtr[ctrId].BaseVal[i] += delta;

            if ((delta > 0) && ((vectorCtr[ctrId].AggrVal[i] + delta) < vectorCtr[ctrId].AggrVal[i]))
            {   /* delta is positive and the aggregate counter would wrap over the maximum allowed value */
                vectorCtr[ctrId].AggrVal[i] = MAXCTRVALUE;
                res = MIXFOVFL;
            }
            else if ((delta < 0) && ((vectorCtr[ctrId].AggrVal[i] + delta) > vectorCtr[ctrId].AggrVal[i]))
            {   /* delta is negative and the aggregate counter would become negative */
                vectorCtr[ctrId].AggrVal[i] = 0;
                res = MIXFOVFL;
            }
            else
                vectorCtr[ctrId].AggrVal[i] += delta;
        }   /* for (i = 0; i < vectorCtr[ctrId].NumInstances; i++) */

    }   /* else if (ctrInst != NULL) */

    return (res);
}


/*
 * This function provides information needed to store counters (both scalar and vector
 * as well as PEG and ROLLER counters) at each base interval. The first parameter is
 * a string that provides the path (either absolute or relative) to the directory in which
 * counters are collected. Files are closed and re-opened daily at 00:00. They are named
 * as follows:
 *     scalar_<timestamp>.csv             -> dump of all Scalar Counters in CSV format
 *     vector_<vector ID>_<timestamp>.csv -> dump of Vector Counter having ID <vector ID>
 *                                           in CSV format (there is a separate file for
 *                                           each <vector ID>)
 * The second parameter is a string formatted according to strftime() man page (e.g.
 * "%F" for date in the format YYYY-MM-DD, etc.). It specifies the format of the <timestamp>
 * above. If NULL or empty, the time stamp will be formatted as ddmmyyyy.
 * Finally, the third string is a comma separated value of minutes corresponding to the
 * desired dump times ("mm" format, with mm between 00 and 60). For example, in order to
 * dump counters every 5 minutes, this string will be formatted as follows:
 *     "00,05,10,15,20,25,30,35,40,45,50,55"
 * This function returns:
 *     - MIXFKO: if either the first parameter is not a valid file name or
 *            the second or third parameters are wrongly formatted or
 *            contain some error (e.g. invalid dump time). This code is also
 *            used if counters collection has been already started through start_counters()
 *     - MIXFOK: if everything is correct
 * Please note that calling this function is MANDATORY before starting statistic collection
 * Remember also that PEG counters are set to zero at every base interval                    */
Error define_base_dump(char* baseDir, char* baseTimeFormat, char* baseTimes)
{
    /* Local variables */
    ExtendedString Dir;
    char           minutes[3] = { '\0','\0','\0' };
    short          s;
    int            i,len;

    if (BaseCtrActive==true)    /* Base Counters already started, returns MIXFKO */
        return (MIXFKO);

    if (baseTimes==NULL)
        return (MIXFKO);

    len = strlen(baseTimes);
    /* First check third parameter and evaluate if it is correct */
    for (i=0;i<len;i+=3)
    {
        minutes[0] = baseTimes[i];
        if (baseTimes[i+1] == '\0')
            return (MIXFKO);
        minutes[1] = baseTimes[i+1];
        minutes[2] = '\0';
        if ((baseTimes[i+2] != '\0') && (baseTimes[i+2] != ','))
            return (MIXFKO);
        s = atoi(minutes);
        if ((s < 0) || (s > 59))
            return (MIXFKO);
    }   /* for (i=0;i<len;i+=3) */

    /* Third parameter is correct - Now check the first one */
    if ((baseDir == NULL) || (baseDir[0] == '\0') ) /* Base Directory has not been specified. Use current directory */
        strcpy(Dir, "./");
    else
    {   /* Base directory specified. First check if the file name is valid */
        if (check_file_name_validity(baseDir) != MIXFOK)
            return (MIXFKO);
        strcpy(Dir, baseDir);
        s = strlen(Dir);
        if ((s != 0) && (Dir[s-1] != '/'))
            strcat(Dir, "/");
    }

    /* Now, Dir contains a valid base dir and baseTimes a valid sequence of dump times */
    /* Evaluate current time stamp format provided as second parameter (if any) */
    if ((baseTimeFormat == NULL) || (baseTimeFormat[0] == '\0'))
        strcpy (BaseCtrTimeStampFormat,"%d%m%Y");
    else
        strcpy(BaseCtrTimeStampFormat, baseTimeFormat);

    strcpy(BaseDumpTimes, baseTimes);
    strcpy(BaseCtrDir, Dir);

    return (MIXFOK);
}


/*
 * This function provides information needed to store counters (both scalar and vector
 * as well as PEG and ROLLER counters) at each aggregation interval. The first parameter is
 * a string that provides the path (either absolute or relative) to the directory in which
 * counters are collected. Files are closed and re-opened daily at 00:00. They are named
 * as follows:
 *     scalar_aggr_<timestamp>.csv             -> dump of all Scalar Counters in CSV format
 *     vector_<vector ID>_aggr_<timestamp>.csv -> dump of Vector Counter having ID <vector ID>
 *                                                in CSV format (there is a separate file for
 *                                                each <vector ID>)
 * The second parameter is a string formatted according to strftime() man page (e.g.
 * "%F" for date in the format YYYY-MM-DD, etc.). It specifies the format of the <timestamp>
 * above. If NULL or empty, the time stamp will be formatted as ddmmyyyy.
 * Finally, the third string is a comma separated value of hours/minutes corresponding to the
 * desired aggregated dump times ("hhmm" format, with hh between 00 and 23 and mm between 00 and
 * 59). For example, in order to dump counters every 2 hours at clock times, this string
 * will be formatted as follows:
 *     "0000,0200,0400,0600,0800,1000,1200,1400,1600,1800,2000,2200"
 * Please observe that up to 100 dump times can be defined.
 * This function returns:
 *     - MIXFKO:   if either the first parameter is not a valid file name or
 *                 the second or third parameters are wrongly formatted or
 *                 contain some error (e.g. invalid dump time). This code is also
 *                 used if counters collection has been already started through start_counters()
 *     - MIXFOVFL: if more than 100 dump times have been specified
 *     - MIXFOK:   if everything is correct
 * Please note that calling this function is OPTIONAL before starting statistic collection
 * (if not called, counters will not be aggregated)
 * Remember also that aggregated PEG counters are set to zero at every aggregation interval  */
Error define_aggr_dump(char* aggrDir, char* aggrTimeFormat, char* aggrTimes)
{
    /* Local variables */
    ExtendedString Dir;
    char           minutes[3] = { '\0','\0','\0' },
                   hours[3] = { '\0','\0','\0' };
    short          s, count = 0;
    int            i,len;

    if (BaseCtrActive==true)    /* Base Counters already started, returns MIXFKO */
        return (MIXFKO);

    if (aggrTimes==NULL)
        return (MIXFKO);

    len = strlen(aggrTimes);
    /* First check third parameter and evaluate if it is correct */
    for (i=0;i<len;i+=5)
    {
        count++;

        hours[0] = aggrTimes[i];
        if (aggrTimes[i+1] == '\0')
            return (MIXFKO);
        hours[1] = aggrTimes[i]+1;
        hours[2] = '\0';

        if (aggrTimes[i+2] == '\0')
            return (MIXFKO);

        minutes[0] = aggrTimes[i+2];
        if (aggrTimes[i+3] == '\0')
            return (MIXFKO);
        minutes[1] = aggrTimes[i+3];
        minutes[2] = '\0';
        if ((aggrTimes[i+4] != '\0') && (aggrTimes[i+4] != ','))
            return (MIXFKO);

        s = atoi(hours);
        if ((s < 0) || (s > 23))
            return (MIXFKO);

        s = atoi(minutes);
        if ((s < 0) || (s > 59))
            return (MIXFKO);
    }   /* for (i=0;i<len;i+=5) */

    if (count > MAXAGGRDUMPTIMES)
        return (MIXFOVFL);

    /* Third parameter is correct - Now check the first one */
    if ((aggrDir == NULL) || (aggrDir[0] == '\0') ) /* Aggr Directory has not been specified. Use current directory */
        strcpy(Dir, "./");
    else
    {   /* Aggr directory specified. First check if the file name is valid */
        if (check_file_name_validity(aggrDir) != MIXFOK)
            return (MIXFKO);
        strcpy(Dir, aggrDir);
        s = strlen(Dir);
        if ((s != 0) && (Dir[s-1] != '/'))
            strcat(Dir, "/");
    }

    /* Now, Dir contains a valid base dir and aggrTimes a valid sequence of dump times */
    /* Evaluate current time stamp format provided as second parameter (if any) */
    if ((aggrTimeFormat == NULL) || (aggrTimeFormat[0] == '\0'))
        strcpy (AggrCtrTimeStampFormat,"%d%m%Y");
    else
        strcpy(AggrCtrTimeStampFormat, aggrTimeFormat);

    strcpy(AggrDumpTimes, aggrTimes);
    strcpy(AggrCtrDir, Dir);

    return (MIXFOK);
}


/*
 * Open all counters files (base and aggregated, if defined) and start counting events
 * This function may return:
 *  - MIXFNOACCESS: if any of the base or aggregated files cannot be opened
 *  - MIXFKO:       if counter collection has alredy been started before through start_counters()
 *                  or define_base_dump() has not been called
 *  - MIXFOK:       if everything is OK
 */
Error start_counters(void)
{
    /* Local variables */
    LongString  DumpFile;
    ShortString TimeStamp;
    struct stat FileStat;
    int         i,j;

    if ( (BaseCtrActive==true) || (BaseCtrDir[0]=='\0') )   /* Either counters already started or define_base_dump() not called */
        return (MIXFKO);

    /* Initialize global locks */
    pthread_mutex_init(&BaseMutex, NULL);
    pthread_mutex_init(&AggrMutex, NULL);

    /* Retrieve current time stamp according to defined format */
    retrieve_time_date(TimeStamp, BaseCtrTimeStampFormat);

    /* Open first the single scalar counter base file */
    /* File is open in append mode, in case that it is initially empty */
    /* it prints first an header row containing all scalar counters name */
    sprintf(DumpFile, "%sscalar_%s.csv", BaseCtrDir, TimeStamp);
    pthread_mutex_lock(&BaseMutex);
    if ((BaseCtr_fd=fopen(DumpFile,"a")) == NULL)
    {
        pthread_mutex_unlock(&BaseMutex);
        return (MIXFNOACCESS);
    }
    stat(DumpFile, &FileStat);
    if (FileStat.st_size == 0)
    {
        fprintf(BaseCtr_fd, "Date,Time,");
        for (j = 0; j < (numScalarCtr - 1); j++)
            fprintf(BaseCtr_fd, "%s,", scalarCtr[j].Name);
        fprintf(BaseCtr_fd, "%s\n", scalarCtr[numScalarCtr - 1].Name);
    }

    /* Now open all the vector counters base files */
    /* As above, files are opened in append mode and if initially empty */
    /* it prints first an header row containing all scalar counters name */
    for (i = 0; i<numVectorCtr; i++)
    {
        sprintf(DumpFile, "%svector_%d_%s.csv", BaseCtrDir, i, TimeStamp);
        if ((vectorCtr[i].BaseCtr_fd = fopen(DumpFile, "a")) == NULL)
        {   /* Not able to open the i-th vector base file, close all files already open and exit with MIXFNOACCESS */
            for (j = i - 1; j >= 0; j--)
                fclose(vectorCtr[j].BaseCtr_fd);
            fclose(BaseCtr_fd);
            pthread_mutex_unlock(&BaseMutex);
            return (MIXFNOACCESS);
        }   /* if ((vectorCtr[i].AggrCtr_fd ... */
        stat(DumpFile, &FileStat);
        if (FileStat.st_size == 0)
        {
            fprintf(vectorCtr[i].BaseCtr_fd, "Vector Counter: %s - Instances: %s\nDate,Time,", vectorCtr[i].Name, vectorCtr[i].InstName);
            for (j = 0; j < (vectorCtr[i].NumInstances - 1); j++)
                fprintf(vectorCtr[i].BaseCtr_fd, "%s,", vectorCtr[i].InstIdName[j]);
            fprintf(vectorCtr[i].BaseCtr_fd, "%s\n", vectorCtr[i].InstIdName[j]);
        }   /* if (FileStat.st_size == 0) */
    }   /* for (i = 0; i<numVectorCtr; i++) */
    fflush (NULL);
    /* All base files are open - clear base mutex */
    pthread_mutex_unlock(&BaseMutex);

    /* Now check whether aggregation has been initialized through define_aggr_dump() */
    if (AggrCtrDir[0] == '\0')
    {   /* Aggregation not initialized - Store the BaseDumpOpenDate for file rotation, set BaseCtrActive and exit */
        retrieve_time_date(BaseDumpOpenDate, "%d%m%Y");
        BaseCtrActive = true;
        BaseNextDump = NULL;
        return (MIXFOK);
    }

    /* Aggregation has been initialized through define_aggr_dump() */
    /* Now Open the single scalar counter Aggr file */
    /* File is open in append mode, in case that it is initially empty */
    /* it prints first an header row containing all scalar counters name */
    sprintf(DumpFile, "%sscalar_aggr_%s.csv", AggrCtrDir, TimeStamp);
    pthread_mutex_lock(&AggrMutex);
    if ((AggrCtr_fd = fopen(DumpFile, "a")) == NULL)
    {   /* Something went wrong - close all previously opened files */
        for (j = 0; j < numScalarCtr; j++)
            fclose(vectorCtr[j].BaseCtr_fd);
        fclose(BaseCtr_fd);
        pthread_mutex_unlock(&AggrMutex);
        return (MIXFNOACCESS);
    }
    stat(DumpFile, &FileStat);
    if (FileStat.st_size == 0)
    {
        fprintf(AggrCtr_fd, "Date,Time,");
        for (j = 0; j < (numScalarCtr - 1); j++)
            fprintf(AggrCtr_fd, "%s,", scalarCtr[j].Name);
        fprintf(AggrCtr_fd, "%s\n", scalarCtr[numScalarCtr - 1].Name);
    }

    /* Now open all the vector counters Aggr files */
    /* As above, files are opened in append mode and if initially empty */
    /* it prints first an header row containing all scalar counters name */
    for (i = 0; i < numVectorCtr; i++)
    {
        sprintf(DumpFile, "%svector_%d_aggr_%s.csv", AggrCtrDir, i, TimeStamp);
        if ((vectorCtr[i].AggrCtr_fd = fopen(DumpFile, "a")) == NULL)
        {   /* Not able to open the i-th vector Aggr file, close all files already open and exit with MIXFNOACCESS */
            for (j = i - 1; j >= 0; j--)
                fclose(vectorCtr[j].AggrCtr_fd);
            for (j = 0; j < numScalarCtr; j++)
                fclose(vectorCtr[j].BaseCtr_fd);
            fclose(AggrCtr_fd);
            fclose(BaseCtr_fd);
            pthread_mutex_unlock(&AggrMutex);
            return (MIXFNOACCESS);
        }   /* if ((vectorCtr[i].AggrCtr_fd ... */
        stat(DumpFile, &FileStat);
        if (FileStat.st_size == 0)
        {
            fprintf(vectorCtr[i].AggrCtr_fd, "Vector Counter: %s - Instances: %s\nDate,Time,", vectorCtr[i].Name, vectorCtr[i].InstName);
            for (j = 0; j < (vectorCtr[i].NumInstances - 1); j++)
                fprintf(vectorCtr[i].AggrCtr_fd, "%s,", vectorCtr[i].InstIdName[j]);
            fprintf(vectorCtr[i].AggrCtr_fd, "%s\n", vectorCtr[i].InstIdName[j]);
        }   /* if (FileStat.st_size == 0) */
    }   /* for (i = 0; i<numVectorCtr; i++) */
    fflush (NULL);
    /* All Aggr files are open - clear Aggr mutex */
    pthread_mutex_unlock(&AggrMutex);

    /* Store the DumpOpenDate for file rotation and set flags */
    retrieve_time_date(BaseDumpOpenDate, "%d%m%Y");
    strcpy(AggrDumpOpenDate, BaseDumpOpenDate);
    BaseCtrActive = AggrCtrActive = true;
    BaseNextDump = AggrNextDump = NULL;

    return (MIXFOK);

}


/*
 * Stops collecting counters, dumps the last values collected up to that time,
 * closes all files and releases internal resources.
 * Please observe that after this call all internal counters definition are lost
 * It returns MIXFKO if counters have not been started before, MIXFOK in all other cases */
Error stop_counters(void)
{
    int     i;

    if (BaseCtrActive == false)     /* Counters not started */
        return (MIXFKO);

    /* Set both locks for base and aggregated counters */
    pthread_mutex_lock(&BaseMutex);
    pthread_mutex_lock(&AggrMutex);

    /* Now close all files, free all allocated memory structures and reset all data */
    for (i = 0; i < MAXSCALARCTRNUM; i++)
    {
        scalarCtr[i].Name[0] = '\0';
        scalarCtr[i].Type = 0;
        scalarCtr[i].BaseVal = 0;
        scalarCtr[i].AggrVal = 0;
    }

    if (BaseCtr_fd != NULL)
    {
        fclose(BaseCtr_fd);
        BaseCtr_fd = NULL;
    }
    if (AggrCtr_fd != NULL)
    {
        fclose(AggrCtr_fd);
        AggrCtr_fd = NULL;
    }

    for (i = 0; i < MAXVECTORCTRNUM; i++)
    {
        vectorCtr[i].Name[0] = '\0';
        vectorCtr[i].InstName[0] = '\0';
        vectorCtr[i].Type = 0;
        vectorCtr[i].NumInstances = 0;
        if (vectorCtr[i].BaseVal)
            free(vectorCtr[i].BaseVal);
        vectorCtr[i].BaseVal = NULL;
        if (vectorCtr[i].AggrVal)
            free(vectorCtr[i].AggrVal);
        vectorCtr[i].AggrVal = NULL;
        if (vectorCtr[i].InstIdName)
            free(vectorCtr[i].InstIdName);
        vectorCtr[i].InstIdName = NULL;
        if (vectorCtr[i].BaseCtr_fd != NULL)
        {
            fclose(vectorCtr[i].BaseCtr_fd);
            vectorCtr[i].BaseCtr_fd = NULL;
        }
        if (vectorCtr[i].AggrCtr_fd != NULL)
        {
            fclose(vectorCtr[i].AggrCtr_fd);
            vectorCtr[i].AggrCtr_fd = NULL;
        }
    }   /* for (i = 0; i < MAXVECTORCTRNUM; i++) */

    /* Finally, restore all default values for global variables */
    numScalarCtr = numVectorCtr = 0;
    cumVectorInst = 0;
    BaseCtrDir[0] = '\0';
    AggrCtrDir[0] = '\0';
    BaseDumpTimes[0] = '\0';
    AggrDumpTimes[0] = '\0';
    BaseNextDump = AggrNextDump = NULL;
    BaseCtrActive = AggrCtrActive = false;

    /* Release Locks */
    pthread_mutex_unlock(&AggrMutex);
    pthread_mutex_unlock(&BaseMutex);

    /* and finally destroy them */
    pthread_mutex_destroy(&AggrMutex);
    pthread_mutex_destroy(&BaseMutex);

    return (MIXFOK);

}


/*
 * When this function is invoked, it checks current time against the next
 * Base and Aggr Dump Times. If they coincide, it writes a row in the
 * corresponding scalar and vector files. In order for counters to be
 * regularly dumped to files, this function shall be called at regular
 * intervals during code execution.
 * It returns MIXFKO if counters have not been defined/started,
 * MIXFNOACCESS if it is not able to write counters to file, MIXFOK in any
 * other case.
 */
 Error check_and_dump_ctr(void)
{
    /* Local variables */
    ShortString CurrentDate, TimeStamp;
    Error       result;
    bool        DumpBase = false,
                DumpAggr = false;
    char        Time[5];
    int         i,j;

    /* Check if counters are actually running */
    if ( (BaseCtrActive==false) || (BaseCtrDir[0]=='\0') )  /* Either counters not started yet or define_base_dump() not called */
        return (MIXFKO);

    /* First check if there is something to dump */
    retrieve_time_date(TimeStamp,"%d/%m/%Y,%H:%M");

    /* Check first Base counters                       */
    /* Retrieve minutes (mm) from Timestamp (dd/mm/yyyy,hh:mm) */
    Time[0] = TimeStamp[14];
    Time[1] = TimeStamp[15];
    Time[2] = '\0';
    if (BaseNextDump)
    {   /* BaseNextDump is not NULL, i.e. it points to the next
           dump time (minutes) within BaseDumpTimes string */
        if (strncmp(Time,BaseNextDump,2)==0)
        {   /* Current value of minutes is equal to the next base dump time */
            /* (i.e. it is time to dump base counters) */
            DumpBase = true;
            BaseNextDump += 3;
            if ( (int)(BaseNextDump-BaseDumpTimes)>=strlen(BaseDumpTimes))
                BaseNextDump = BaseDumpTimes;
        }   /* if (strncmp(Time,BaseNextDump)==0) */
    }   /* if (BaseNextDump) */
    else
    {   /* BaseNextDump is NULL, i.e. this is the first row of counters
           in the base dump files */
        if ( (BaseNextDump=strstr(BaseDumpTimes,Time)) )
        {   /* Current value of minutes is contained in the list of base dump times */
            /* (i.e. it is time to dump base counters) */
            DumpBase = true;
            BaseNextDump += 3;
            if ( (int)(BaseNextDump-BaseDumpTimes)>=strlen(BaseDumpTimes))
                BaseNextDump = BaseDumpTimes;
        }   /* if ( BaseNextDump=strstr(BaseDumpTimes,Time) ) */
    }   /* else if (BaseNextDump) */

    /* If Aggregation has been defined, check also Aggregate Counters */
    if (AggrCtrActive)
    {   /* Retrieve hours and minutes (hhmm) from Timestamp (dd/mm/yyyy,hh:mm)*/
        Time[0] = TimeStamp[11];
        Time[1] = TimeStamp[12];
        Time[2] = TimeStamp[14];
        Time[3] = TimeStamp[15];
        Time[4] = '\0';

        if (AggrNextDump)
        {   /* AggrNextDump is not NULL, i.e. it points to the next
            dump time (hours/minutes) within AggrDumpTimes string */
            if (strncmp(Time,AggrNextDump,4)==0)
            {   /* Current value of hours/minutes is equal to the next aggr dump time */
                /* (i.e. it is time to dump aggr counters) */
                DumpAggr = true;
                AggrNextDump += 5;
                if ( (int)(AggrNextDump-AggrDumpTimes)>=strlen(AggrDumpTimes))
                    AggrNextDump = AggrDumpTimes;
            }   /* if (strncmp(Time,AggrNextDump)==0) */
        }   /* if (AggrNextDump) */
        else
        {   /* AggrNextDump is NULL, i.e. this is the first row of counters
            in the aggr dump files */
            if ( (AggrNextDump=strstr(AggrDumpTimes,Time)) )
            {   /* Current value of hours/minutes is contained in the list of aggr dump times */
                /* (i.e. it is time to dump aggr counters) */
                DumpAggr = true;
                AggrNextDump += 5;
                if ( (int)(AggrNextDump-AggrDumpTimes)>=strlen(AggrDumpTimes))
                    AggrNextDump = AggrDumpTimes;
            }   /* if ( AggrNextDump=strstr(AggrDumpTimes,Time) ) */
        }   /* else if (AggrNextDump) */
    }   /* if (AggrCtrActive) */

    retrieve_time_date(CurrentDate, "%d%m%Y");

    /* Dump Base counters if needed */
    if (DumpBase)
    {
        /* Enter the critical section for base counters */
        pthread_mutex_lock(&BaseMutex);

        /* Check if base counters shall be rotated */
        if ( strcmp(CurrentDate,BaseDumpOpenDate) )
            if ( (result=CloseReopenBaseCounters()) != MIXFOK)
            {
                pthread_mutex_unlock(&BaseMutex);
                return (result);
            }

        /* Dump Scalar Counters (PEG and ROLLER) */
        fprintf(BaseCtr_fd,"%s,",TimeStamp);
        for (i = 0; i < (numScalarCtr - 1); i++)
            fprintf(BaseCtr_fd, "%u,", scalarCtr[i].BaseVal);
        fprintf(BaseCtr_fd, "%u\n", scalarCtr[numScalarCtr - 1].BaseVal);

        /* Dump Vector Counter (PEG and ROLLER) */
        for (i = 0; i<numVectorCtr; i++)
        {
            fprintf(vectorCtr[i].BaseCtr_fd,"%s,",TimeStamp);
            for (j = 0; j < (vectorCtr[i].NumInstances - 1); j++)
                fprintf(vectorCtr[i].BaseCtr_fd, "%u,", vectorCtr[i].BaseVal[j]);
            fprintf(vectorCtr[i].BaseCtr_fd, "%u\n", vectorCtr[i].BaseVal[j]);
        }

        /* Reset PEG Scalar Counters */
        for (i = 0; i < numScalarCtr; i++)
            if (scalarCtr[i].Type == PEGCTR)
                scalarCtr[i].BaseVal = 0;

        /* Reset PEG Vector Counters */
        for (i = 0; i < numVectorCtr; i++)
            if (vectorCtr[i].Type == PEGCTR)
                for (j = 0; j < vectorCtr[i].NumInstances; j++)
                    vectorCtr[i].BaseVal[j] = 0;

        fflush (NULL);
        /* Exit from the critical section for base counters */
        pthread_mutex_unlock(&BaseMutex);
    }   /* if (DumpBase) */

        /* Dump Aggr counters if needed */
    if (DumpAggr)
    {
        /* Enter the critical section for aggr counters */
        pthread_mutex_lock(&AggrMutex);

        /* Check if aggr counters shall be rotated */
        if ( strcmp(CurrentDate,AggrDumpOpenDate) )
            if ( (result=CloseReopenAggrCounters()) != MIXFOK)
            {
                pthread_mutex_unlock(&AggrMutex);
                return (result);
        }

        /* Dump Scalar Counters (PEG and ROLLER) */
        fprintf(AggrCtr_fd,"%s,",TimeStamp);
        for (i = 0; i < (numScalarCtr - 1); i++)
            fprintf(AggrCtr_fd, "%u,", scalarCtr[i].AggrVal);
        fprintf(AggrCtr_fd, "%u\n", scalarCtr[numScalarCtr - 1].AggrVal);

        /* Dump Vector Counter (PEG and ROLLER) */
        for (i = 0; i<numVectorCtr; i++)
        {
            fprintf(vectorCtr[i].AggrCtr_fd,"%s,",TimeStamp);
            for (j = 0; j < (vectorCtr[i].NumInstances - 1); j++)
                fprintf(vectorCtr[i].AggrCtr_fd, "%u,", vectorCtr[i].AggrVal[j]);
            fprintf(vectorCtr[i].AggrCtr_fd, "%u\n", vectorCtr[i].AggrVal[j]);
        }

        /* Reset PEG Scalar Counters */
        for (i = 0; i < numScalarCtr; i++)
            if (scalarCtr[i].Type == PEGCTR)
                scalarCtr[i].AggrVal = 0;

        /* Reset PEG Vector Counters */
        for (i = 0; i < numVectorCtr; i++)
            if (vectorCtr[i].Type == PEGCTR)
                for (j = 0; j < vectorCtr[i].NumInstances; j++)
                    vectorCtr[i].AggrVal[j] = 0;

        fflush (NULL);
        /* Exit from the critical section for base counters */
        pthread_mutex_unlock(&AggrMutex);
    }   /* if (DumpAggr) */

    return (MIXFOK);
}
