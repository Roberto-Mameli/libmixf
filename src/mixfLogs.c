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
 * FILE:        mixfLogs.c                                                  *
 * VERSION:     3.0.0                                                       *
 * AUTHOR(S):   Roberto Mameli                                              *
 * PRODUCT:     Library libmixf - general purpose library                   *
 * DESCRIPTION: This source file contains the implementation of a subset of *
 *              the functions provided by libmixf library, specifically:    *
 *              - Log Handling                                              *
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
/* Private variables for Log Handling functions */
static FILE             *Log_fd;                              /* File descriptor of the Log Handling functions, local to the library */
static MediumString     LogFileBaseName = "";                 /* Base Name of the Log File, local to the library */
static MicroString      LogFileTimeStampFormat = "";          /* Format for the time stamp used for the log file name */
static bool             LogOpenFlag=false;                    /* Flag used to understand whether the log is open or not, local to the library */
static bool             LogOpenRotate=false;                  /* Flag that specifies if the log file shall be closed and re-opened every day */
static MicroString      LogOpenDate="";                       /* Date when the current log file was opened, local to the library */
static MicroString      LogLevelList[MAXLOGLEVELS];           /* Strings corresponding to available Log Levels (from 1 to 8) */
static EventInfo        Events[EVENTARRAYSIZE];               /* Array of events (contains textual description, number of parameters and severity level */
static uint8_t          numevents=0,                          /* Number of registered events */
                        numlevels=1,                          /* Number of log levels (1 is the default if not initialized) */
                        LogLevel=0;                           /* Log level, local to the library (0 is the default if not initialized) */
static pthread_mutex_t  LogMutex;                             /* Mutex used to handle cuncurrent access to log file (in case of multiple threads) */


/*******************************
 *                             *
 *      Static Functions       *
 * (only visible in this file) *
 *                             *
 *******************************/
/*
 * This in an internal function that closes and reopens an existing log file without
 * handling locks. It cannot be invoked from outside the library, it is part of the
 * implementation of the register_event() function, which invokes it when it needs
 * to rotate a log at 00:00.
 * BE AWARE that it doesn't set/clear locks, which are managed within the calling
 * function. In this way, there is no possibility that a cuncurrent instance of
 * register_event() tries to access the og file between close_log() and open_log()
 * invocation.
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if the log is not open,
 * MIXFNOACCESS if the filename cannot be reopened.
 */
static Error CloseReopenLog (void)
{
    /* Local variables */
    LongString   LogFileName;
    ShortString  TimeStamp;

    /* Do not manage MUTEX. They are handled by calling function, i.e. register_event() */
    if ( (LogOpenFlag==false) || (Log_fd==NULL) )   /* Log is not open - return MIXFKO error */
        return (MIXFKO);

    /* Close the existing log */
    fclose (Log_fd);

    /* Evaluates and keeps the date of log reopening in ddmmyyyy format */
    /* This is stored for Log Rotation */
    retrieve_time_date(LogOpenDate,"%d%m%Y");

    /* Evaluate log file name */
    if ( (LogFileTimeStampFormat != NULL) && (LogFileTimeStampFormat[0] != '\0') )
    {
        retrieve_time_date(TimeStamp,LogFileTimeStampFormat);
        sprintf (LogFileName,"%s_%s.log",LogFileBaseName,TimeStamp);
    }
    else
        sprintf (LogFileName,"%s.log",LogFileBaseName);

    /* Open new Log File in append mode */
    if ( (Log_fd=fopen(LogFileName,"a")) == NULL)
    {
        LogOpenFlag = false;
        return (MIXFNOACCESS);
    }
    LogOpenFlag = true;     /* Strictly speaking this is not needed, but in any case it does not harm */

    return (MIXFOK);
}


/***********************************
 *                                 *
 *        Visible Functions        *
 * (can be used outside this file  *
 *  and are part of the library)   *
 *                                 *
 ***********************************/
/* ------------
 * Log Handling
 * ------------*/
/*
 * This function defines the maximum number of log levels (first parameter)
 * and the default value for the log level (second parameter). The maximum number
 * of levels shall be between 1 and 8, with 1 being the default value. The default
 * value for the log level (second parameter) shall range between 0 and (first
 * parameter-1). Please remember that the higher is the log level, the lower
 * is the severity (i.e. level 0 events represents very very critical issues,
 * while level 7 is for very minor events).
 * This function provides MIXFKO in case of wrong parameters (e.g. outside allowed
 * ranges), MIXFOK if everything is ok
 */
Error define_log_levels (uint8_t numloglevels, uint8_t defloglevel)
{
    if ((numloglevels<1) || (numloglevels>MAXLOGLEVELS))
        return (MIXFKO);
    numlevels = numloglevels;
    if ((defloglevel<0) || (defloglevel>=numlevels))
        return (MIXFKO);
    LogLevel = defloglevel;

    return (MIXFOK);

}


/*
 * This function associates a textual description (second parameter) to each log
 * level (first parameter). The latter shall be included in the interval 0-(N-1),
 * where N is the max number of log levels defined through define_log_levels().
 * This function provides MIXFKO in case of wrong parameters (e.g. outside allowed
 * ranges), MIXFOK if everything is ok.
 */
Error define_level_descr (uint8_t level, char * textdescr)
{
    if ((level<0) || (level>=numlevels))
        return (MIXFKO);
    strcpy (LogLevelList[level], textdescr);
    return (MIXFOK);
}


/*
 * Sets the current Log Level to the value specified by the parameter
 * Before invocation, the Log Level is by default set to 0 (i.e. the highest severity)
 * It provides MIXFOK in case of success, MIXFKO if the value is outside the maximum
 * log level, i.e. N-1, where N is the max number of log levels defined
 * through define_log_levels()
 */
Error set_log_level (uint8_t level)
{
    if ((level<0) || (level>=numlevels))
        return (MIXFKO);
    LogLevel = level;
    return (MIXFOK);
}


/*
 * Get the current Log Level
 */
uint8_t get_log_level (void)
{
    return (LogLevel);
}


/*
 * Define the maximum number of events for the system. This value shall be within
 * 1 and 255. This function is mandatory and shall be necessarily called before
 * the first occurrence of define_event(). It provides MIXFOK in case of success,
 * MIXFKO otherwise
 */
Error define_num_events (uint8_t maxevents)
{
    if ((maxevents<1) || (maxevents>EVENTARRAYSIZE))
        return (MIXFKO);
    numevents = maxevents;

    return (MIXFOK);
}


/*
 * This function defines attributes for each event. Specifically, the event code
 * specified by the first parameter (which shall be defined in the interval between
 * 0 and M-1, where M is the num of events defined by define_num_events() ) is associated
 * with a severity level specified by the second parameter (which shall be in the
 * interval [0,N-1], being N the max number of log levels defined through
 * define_log_levels()). The third parameter defines the textual description for the
 * event. This string may contain up to 3 placeholders defined as "%1", "%2" and
 * "%3", which will be used to identify the position within the string of up
 * to 3 parameters (see register_event() below).
 * If the function is called multiple times with the same event code, each invoation
 * overwrites previous data (only the last definition applies).
 * This function provides MIXFOK in case of success, MIXFOVFL if the first or the second
 * parameter are out of allowed ranges, MIXFFORMATERROR if the string does not respect
 * the format specified above (e.g. it contains an invalid placeholder, such as "%4"
 * or a valid placeholder repeated twice)
 */
Error define_event (EventCode event, uint8_t level, char * descr)
{
    /* Local Variables */
    char           *params[3];     /* Up to 3 parameters may be contained in the description */
    char           *p;
    ExtendedString EvnDescr;

    /* Preliminary checks */
    if ((event<0) || (event>=numevents))
        return (MIXFOVFL);
    if ((level<0) || (level>=numlevels))
        return (MIXFOVFL);

    /* Look for parameters placeholders in the description (i.e. "%1", "%2", "%3") */
    strcpy (EvnDescr,descr);
    p = EvnDescr;
    params[0] = strstr(p,"%1");
    params[1] = strstr(p,"%2");
    params[2] = strstr(p,"%3");

    if (params[0]==NULL)
    {   /* placeholder "%1" is not present */
        /* "%2" and "%3" shall not be present as well */
        if ( (params[1]!=NULL) || (params[2]!=NULL) )
            return (MIXFFORMATERROR);
        Events[event].NumParams = 0;
        Events[event].Level = level;
        strcpy (Events[event].Descr[0], p);
        return (MIXFOK);
    }

    /* placeholder "%1" is present. Look for "%2" */
    if (params[1]==NULL)
    {   /* placeholder "%2" is not present */
        /* "%3" shall not be present as well */
        if ( (params[2]!=NULL) )
            return (MIXFFORMATERROR);
        Events[event].NumParams = 1;
        Events[event].Level = level;
        *(params[0]) = '\0';
        strcpy (Events[event].Descr[0], p);
        p = params[0] + 2;
        strcpy (Events[event].Descr[1], p);
        return (MIXFOK);
    }

    /* placeholders "%1" and "%2" are present. Look for "%3" */
    if (params[2]==NULL)
    {   /* placeholder "%3" is not present */
        Events[event].NumParams = 2;
        Events[event].Level = level;
        *(params[0]) = '\0';
        *(params[1]) = '\0';
        strcpy (Events[event].Descr[0], p);
        p = params[0] +2;
        strcpy (Events[event].Descr[1], p);
        p = params[1] +2;
        strcpy (Events[event].Descr[2], p);
        return (MIXFOK);
    }

    /* All placeholders are present */
    Events[event].NumParams = 3;
    Events[event].Level = level;
    *(params[0]) = '\0';
    *(params[1]) = '\0';
    *(params[2]) = '\0';
    strcpy (Events[event].Descr[0], p);
    p = params[0] +2;
    strcpy (Events[event].Descr[1], p);
    p = params[1] +2;
    strcpy (Events[event].Descr[2], p);
    p = params[2] +2;
    strcpy (Events[event].Descr[3], p);
    return (MIXFOK);
}


/*
 * This function opens a log file named <Basename>_<Timestamp>.log (in append mode)
 * <Basename> and <Timestamp> are respectively the first and the second argument
 * of the function call. <Basename> shall be a valid file name, and is specified with
 * respect to the current working directory (it is strongly advised to use absolute
 * pathnames here). <Timestamp> is a string that shall be defined according to the
 * format described in strftime() man page (e.g. "%F" for date in the format
 * YYYY-MM-DD, etc.). If NULL or empty, the file will be simply opened as
 * <Basename>.log, without a trailing timestamp. The third argument is a bool;   
 * if true, it forces the file to be closed and re-opened daily at 00:00).
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if the log is already open,
 * MIXFFORMATERROR if the first argument is not a valid file name,
 * MIXFNOACCESS if the filename cannot be opened.
 */
Error open_log (char *BaseName, char *format, bool RotateDaily)
{
    /* Local variables */
    LongString  LogFileName;
    ShortString TimeStamp;


    pthread_mutex_init(&LogMutex, NULL);
    pthread_mutex_lock(&LogMutex);

    if (LogOpenFlag)    /* Already open */
    {
        pthread_mutex_unlock(&LogMutex);
        return (MIXFKO);
    }

    if (check_file_name_validity(BaseName) != MIXFOK)
    {
        pthread_mutex_unlock(&LogMutex);
        return (MIXFFORMATERROR);
    }

    LogOpenRotate = RotateDaily;                /* keeps the log open rotation flag into a local variable for future reference */
    strcpy (LogFileBaseName, BaseName);         /* keeps the base name into a local variable for future reference */
    if (format != NULL)                         /* keeps the time stamp format into a local variable for future reference */
        strcpy (LogFileTimeStampFormat, format);
    else
        LogFileTimeStampFormat[0] = '\0';

    /* Evaluates and keeps the date of log opening in ddmmyyyy format */
    /* This is stored for Log Rotation */
    retrieve_time_date(LogOpenDate,"%d%m%Y");

    /* Evaluate log file name */
    if ( (format != NULL) && (format[0] != '\0') )
    {
        retrieve_time_date(TimeStamp,format);
        sprintf (LogFileName,"%s_%s.log",LogFileBaseName,TimeStamp);
    }
    else
        sprintf (LogFileName,"%s.log",LogFileBaseName);

    /* Open Log File in append mode */
    if ( (Log_fd=fopen(LogFileName,"a")) == NULL)
    {
        LogOpenFlag = false;
        pthread_mutex_unlock(&LogMutex);
        return (MIXFNOACCESS);
    }
    LogOpenFlag = true;
    pthread_mutex_unlock(&LogMutex);

    return (MIXFOK);
}


/*
 * This function closes the current Log File
 */
void close_log (void)
{
    /* Close log and exit */
    pthread_mutex_lock(&LogMutex);
    fclose (Log_fd);
    LogOpenFlag = false;
    pthread_mutex_unlock(&LogMutex);

    pthread_mutex_destroy (&LogMutex);

    return;
}


/*
 * Register into the log the event whose event code is specified by first argument.
 * This is done only if the event severity is at least equal to the current log level
 * and if the log file is open, otherwise the call returns without effect.
 * For events that contains parameters in the textual description (see define_event()
 * above), they can be specified as strings in the second ("%1" placeholder), third ("%2"
 * placeholder) and fourth ("%3" placeholder) argument). This function provides MIXFOK
 * in case of SUCCESS, MIXFKO if the log is not open, MIXFNOACCESS if the log file
 * cannot be reopened.
 */
Error register_event (EventCode event, char * param1, char * param2, char * param3)
{
    /* Local variables */
    Error       result = MIXFOK;
    ShortString CurrentDate, Time;

    /* if the severity of the event is higher than current log level, return without logging anything */
    if (Events[event].Level > LogLevel)
        return (result);

    /* enter the critical section */
    pthread_mutex_lock(&LogMutex);

    if ( (LogOpenFlag==false) || (Log_fd==NULL) )   /* Log is not open - return MIXFKO error */
    {
        pthread_mutex_unlock(&LogMutex);
        return (MIXFKO);
    }

    /* Check if the date changed from the last time the log was opened */
    /* If so, and if the LogOpenRotateFlag is true, close old log file and open a new one */
    if (LogOpenRotate)
    {
        retrieve_time_date(CurrentDate,"%d%m%Y");
        if ( strcmp(CurrentDate,LogOpenDate) )
            if ( (result=CloseReopenLog()) != MIXFOK)
            {
                pthread_mutex_unlock(&LogMutex);
                return (result);
            }

    }

    retrieve_time_date(Time,"%T");
    switch (Events[event].NumParams)
    {
        case 0:
        {
            fprintf (Log_fd,"%s - %s(%d) - Event %3d - %s\n",Time,LogLevelList[Events[event].Level],Events[event].Level,event,Events[event].Descr[0]);
            break;
        }
        case 1:
        {
            fprintf (Log_fd,"%s - %s(%d) - Event %3d - %s%s%s\n",Time,LogLevelList[Events[event].Level],Events[event].Level,event,Events[event].Descr[0],param1,Events[event].Descr[1]);
            break;
        }
        case 2:
        {
            fprintf (Log_fd,"%s - %s(%d) - Event %3d - %s%s%s%s%s\n",Time,LogLevelList[Events[event].Level],Events[event].Level,event,Events[event].Descr[0],param1,Events[event].Descr[1],param2,Events[event].Descr[2]);
            break;
        }
        case 3:
        {
            fprintf (Log_fd,"%s - %s(%d) - Event %3d - %s%s%s%s%s%s%s\n",Time,LogLevelList[Events[event].Level],Events[event].Level,event,Events[event].Descr[0],param1,Events[event].Descr[1],param2,Events[event].Descr[2],param3,Events[event].Descr[3]);
            break;
        }
        default:
            break;
    }

    fflush (Log_fd);
    pthread_mutex_unlock(&LogMutex);
    return (MIXFOK);

}