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
 * FILE:        mixfApi.c                                                   *
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
static Boolean          LogOpenFlag=FALSE;                    /* Flag used to understand whether the log is open or not, local to the library */
static Boolean          LogOpenRotate=FALSE;                  /* Flag that specifies if the log file shall be closed and re-opened every day */
static MicroString      LogOpenDate="";                       /* Date when the current log file was opened, local to the library */
static MicroString      LogLevelList[MAXLOGLEVELS];           /* Strings corresponding to available Log Levels (from 1 to 8) */
static EventInfo        Events[EVENTARRAYSIZE];               /* Array of events (contains textual description, number of parameters and severity level */
static uint8_t          numevents=0,                          /* Number of registered events */
                        numlevels=1,                          /* Number of log levels (1 is the default if not initialized) */
                        LogLevel=0;                           /* Log level, local to the library (0 is the default if not initialized) */
static pthread_mutex_t  LogMutex;                             /* Mutex used to handle cuncurrent access to log file (in case of multiple threads) */
static pthread_mutex_t  LockMutex=PTHREAD_MUTEX_INITIALIZER;  /* Mutex used to handle cuncurrent access to lock file  */

/* Private variables for Configuration File Handling functions */
static Param            *ParamVect=NULL;                      /* Pointer to a dynamically allocated array of Configuration Parameters  */
static uint8_t          numparams=0,                          /* Actual Number of Configuration Parameters */
                        maxparams=DEFPARAMARRAYSIZE;          /* Max Number of Configuration Parameters */
static Boolean          ParsedFlag=FALSE;                     /* Set to TRUE when the Configuration File is actually parsed */

/* Private variables for Counters Handling (v.2.0.0) */
static uint16_t         numScalarCtr = 0,                     /* Number of Scalar Counters, between 0 and MAXSCALARCTRNUM */
                        numVectorCtr = 0;                     /* Number of Vector Counters, between 0 and MAXVECTORCTRNUM */
static uint32_t         cumVectorInst = 0;                    /* Cumulative Number of Instances for Vector Counters (<=MAXVECTORCTRINST) */
static scalarCtrInfo    scalarCtr[MAXSCALARCTRNUM];           /* Array of Scalar Counters */
static vectorCtrInfo    vectorCtr[MAXVECTORCTRNUM];           /* Array of Vector Counters */
static MediumString     BaseCtrDir = "",                      /* Path to the directory in which Base counters are collected */
                        AggrCtrDir = "";                      /* Path to the directory in which Aggr counters are collected */
static Medium2String    BaseDumpTimes = "";                   /* String containing base dump times (third parameter of DefineBaseDump) */
static LongString       AggrDumpTimes = "";                   /* String containing aggr dump times (third parameter of DefineAggrDump) */
static MicroString      BaseCtrTimeStampFormat = "%d%m%Y",    /* String containing base dump time stamp format (second parameter of DefineBaseDump) */
                        AggrCtrTimeStampFormat = "%d%m%Y",    /* String containing aggr dump time stamp format (second parameter of DefineAggrDump) */
                        BaseDumpOpenDate = "",                /* Date when the current base ctr file was opened, local to the library */
                        AggrDumpOpenDate = "";                /* Date when the current aggr ctr file was opened, local to the library */
char                    *BaseNextDump = NULL,                 /* Pointer within BaseDumpTimes to the next dump time */
                        *AggrNextDump = NULL;                 /* Pointer within AggrDumpTimes to the next dump time */
static Boolean          BaseCtrActive = FALSE,                /* Flag used to understand whether the base ctr file is open or not */
                        AggrCtrActive = FALSE;                /* Flag used to understand whether the aggr ctr file is open or not */
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
 * This function is used within ParseCfgParamFile(). It
 * adds a new event along with the corresponding line
 * to the list of events given back by the routine.
 * Please observe that it returns immediately without
 * affecting the list if the event is UNDEFINED
 */
static void AddEventInList (EventCode evn, uint16_t line, EventList **list)
{
    EventList *ptr1, *ptr2;

    /* Do not add an UNDEFINED event to the list */
    if (evn==UNDEFINED)
        return;

    ptr2 = malloc (sizeof (EventList));
    ptr2->event = evn;
    ptr2->line = line;
    ptr2->next = NULL;

    ptr1 = *list;
    if (ptr1==NULL)
    {
        *list = ptr2;
        return;
    }

    while (ptr1->next != NULL)
        ptr1 = ptr1->next;
    ptr1->next = ptr2;

    return;

}


/*
 * This in an internal function that closes and reopens an existing log file without
 * handling locks. It cannot be invoked from outside the library, it is part of the
 * implementation of the RegisterEvent() function, which invokes it when it needs
 * to rotate a log at 00:00.
 * BE AWARE that it doesn't set/clear locks, which are managed within the calling
 * function. In this way, there is no possibility that a cuncurrent instance of
 * RegisterEvent() tries to access the og file between CloseLog() and OpenLog()
 * invocation.
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if the log is not open,
 * MIXFNOACCESS if the filename cannot be reopened.
 */
static Error CloseReopenLog (void)
{
    /* Local variables */
    Medium2String   LogFileName;
    ShortString     TimeStamp;

    /* Do not manage MUTEX. They are handled by calling function, i.e. RegisterEvent() */
    if ( (LogOpenFlag==FALSE) || (Log_fd==NULL) )   /* Log is not open - return MIXFKO error */
        return (MIXFKO);

    /* Close the existing log */
    fclose (Log_fd);

    /* Evaluates and keeps the date of log reopening in ddmmyyyy format */
    /* This is stored for Log Rotation */
    RetrieveTimeDate(LogOpenDate,"%d%m%Y");

    /* Evaluate log file name */
    if ( (LogFileTimeStampFormat != NULL) && (LogFileTimeStampFormat[0] != '\0') )
    {
        RetrieveTimeDate(TimeStamp,LogFileTimeStampFormat);
        sprintf (LogFileName,"%s_%s.log",LogFileBaseName,TimeStamp);
    }
    else
        sprintf (LogFileName,"%s.log",LogFileBaseName);

    /* Open new Log File in append mode */
    if ( (Log_fd=fopen(LogFileName,"a")) == NULL)
    {
        LogOpenFlag = FALSE;
        return (MIXFNOACCESS);
    }
    LogOpenFlag = TRUE;     /* Strictly speaking this is not needed, but in any case it does not harm */

    return (MIXFOK);
}


/*
 * This in an internal function that closes and reopens all existing base counters file
 * without handling locks. It cannot be invoked from outside the library, it is part of the
 * implementation of the CheckAndDumpCtr() function, which invokes it when it needs
 * to rotate counters file at 00:00.
 * BE AWARE that it doesn't set/clear locks, which are managed within the calling
 * function.
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if base counters are not open,
 * MIXFNOACCESS if the filenames cannot be reopened.
 */
static Error CloseReopenBaseCounters(void)
{
    /* Local variables */
    Medium2String   DumpFile;
    ShortString     TimeStamp;
    int             i,j;

    /* Do not manage MUTEX. They are handled by calling function */
    if (BaseCtrActive==FALSE)   /* Base counters not running - return MIXFKO error */
        return (MIXFKO);

    /* Evaluates and keeps the date of counters reopening in ddmmyyyy format */
    /* This is stored for Rotation */
    RetrieveTimeDate(BaseDumpOpenDate,"%d%m%Y");

    /* Close the existing base counters files */
    fclose (BaseCtr_fd);
    BaseCtr_fd = NULL;
    for (i = 0; i<numVectorCtr; i++)
    {
        fclose (vectorCtr[i].BaseCtr_fd);
        vectorCtr[i].BaseCtr_fd = NULL;
    }

    /* Evaluate Time Stamp */
    RetrieveTimeDate(TimeStamp, BaseCtrTimeStampFormat);

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
 * implementation of the CheckAndDumpCtr() function, which invokes it when it needs
 * to rotate counters file at 00:00.
 * BE AWARE that it doesn't set/clear locks, which are managed within the calling
 * function.
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if aggr counters are not open,
 * MIXFNOACCESS if the filenames cannot be reopened.
 */
static Error CloseReopenAggrCounters(void)
{
    /* Local variables */
    Medium2String   DumpFile;
    ShortString     TimeStamp;
    int             i,j;

    /* Do not manage MUTEX. They are handled by calling function */
    if (AggrCtrActive==FALSE)   /* Aggr counters not running - return MIXFKO error */
        return (MIXFKO);

    /* Evaluates and keeps the date of counters reopening in ddmmyyyy format */
    /* This is stored for Rotation */
    RetrieveTimeDate(AggrDumpOpenDate,"%d%m%Y");

    /* Close the existing base counters files */
    fclose (AggrCtr_fd);
    AggrCtr_fd = NULL;
    for (i = 0; i<numVectorCtr; i++)
    {
        fclose (vectorCtr[i].AggrCtr_fd);
        vectorCtr[i].AggrCtr_fd = NULL;
    }

    /* Evaluate Time Stamp */
    RetrieveTimeDate(TimeStamp, AggrCtrTimeStampFormat);

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
/* -----------------------------
 * File and File System Handling
 * -----------------------------*/
/*
 * Check filename validity
 * A valid input file starts with a letter, a digit, a dot, a slash or an
 * underscore and does not contain any of the following characters:
 * |!"£$%()=?'^\[]*+@#;:,<>&
 * Returns MIXFOK if filename satisfies these constraints, MIXFKO otherwise
 */
Error CheckFileNameValidity (char *filename)
{

    if ( (strlen(filename)<1) || (strlen(filename)>MAXFILENAMELEN) || (strpbrk(filename,"|!\"£$%()=?\'^\\[]*+@#;:,<>&") != NULL) )
        return (MIXFKO);
    if (  ((filename[0] >= 'a') && (filename[0] <= 'z')) ||
          ((filename[0] >= 'A') && (filename[0] <= 'Z'))  ||
          ((filename[0] >= '0') && (filename[0] <= '9'))  ||
          (filename[0] == '.') || (filename[0] == '/') || (filename[0] == '_') )
    {
        return (MIXFOK);
    }
    return (MIXFKO);
}


/*
 * This function provides a string containing the
 * path of the current working directory in the file system.
 * It does not malloc memory for the string.
 * Returns MIXFKO in case of errors (e.g. permission to read or
 * search a component of the filename was denied), MIXFOK
 * in any other case.
 */
Error RetrievePath (char *Path)
{
    char *cwd;

    if ((cwd = getcwd(NULL, MAXFILENAMELEN)) == NULL)
        return (MIXFKO);

    strcpy (Path,cwd);
    free(cwd);  /* free memory allocated by getcwd() */

    return (MIXFOK);

}


/*
 * This function reads all the files in the inputdir Directory and
 * provides a list of all filenames (filenames are reported relative
 * to inputdir, i.e. without absolute path). Only files are considered,
 * directories are discarded.
 * Returns MIXFKO in case of errors (e.g. permission to read or
 * search a component of the input dir was denied), MIXFOK
 * in any other case.
 */
Error ReadFilesInputDir (char *inputdir, dir_content ** list_of_files)
{
    /* Local variables */
    DIR             *currPath;
    struct dirent   *des;
    struct stat     buf;
    char            name[MAXFILENAMELEN],
                    fullName[MAXFILENAMELEN];

    dir_content   *first = NULL,
                  *p     = NULL;

    if ( (currPath=opendir(inputdir)) != NULL )
    {
        /* In case input directory does not end with trailing '/', add it */
        if (inputdir[strlen(inputdir)-1]!='/')
            strcat (inputdir,"/");

        while ( (des = readdir(currPath)) != NULL )
        {   /* Loop within input dir and evaluate name
               of the current object within inputdir   */
            strcpy (name,des->d_name);
            strcpy (fullName,inputdir);
            strcat (fullName, name);

            /* Collect info about this object */
            lstat (fullName, &buf);

            /* If this is a file allocate a new element in the list */
            if (S_ISREG(buf.st_mode))
            {
                if ( (p=malloc (sizeof(dir_content)) ) == NULL)
                {

                    ClearInputFileList (&first);
                    return (MIXFKO);
                }
                strcpy (p->filename,name);
                p->next = first;
                first = p;
            }
        }   /* while ( (des = */

        /* Done, exit and provide back pointer to the newly created list */
        *list_of_files = first;
        closedir (currPath);
        return (MIXFOK);

    }   /* if ( (currPath */
    else
        return (MIXFKO);

}


/*
 * This function releases memory allocated by ReadFilesInputDir()
 * for the Input File List.
 */
void ClearInputFileList(dir_content ** ptr)
{
    /* Local variables */
    dir_content *p, *q;

    q = *ptr;
    while (q!=NULL)
    {
        p=q->next;
        free (q);
        q = p;
    }
    *ptr = NULL;
}


/* ----------------------
 * Time and Date Handling
 * ----------------------*/
/*
 * This function provides a string containing current
 * time and date. The string is provided back into
 * TimeDateString, while format represents the format
 * as specified by strftime() man page (e.g. "%c" for
 * full date and time, "%T" for time in HH:MM:YY format,
 * "%F" for date in the format YYYY-MM-DD, etc.).
 * The routine does not allocate memory for the date
 * and time string.
 */
void RetrieveTimeDate (char * TimeDateString, char * format)
{
    /* Local Variables */
    time_t      currenttime;

    /* Evaluate current time and date */
    time (&currenttime);
    strftime(TimeDateString, MAXDATETIMELEN, format, localtime(&currenttime));

}


/*
 *  This function takes a string that begins with a Time stamp (e.g.
 *  a Log Line) and decodes it according to the format specified in
 *  the format parameter (see man strptime for details about the format).
 *  It converts this time stamp into a time_t value (i.e.
 *  number of seconds from epoch) and provides it back as a result
 *  (0 if it detects a mismatch in the format).
 */
time_t GetTimeStamp (char * LogLine, char * format)
{
    /* Local variables */
    struct tm   TimeFromLog;

    if (strptime (LogLine, format, &TimeFromLog) != NULL)
        return (mktime (&TimeFromLog));
    else
        return (0);
}


/* ---------------
 * String Handling
 * ---------------*/
/*
 * This function takes an input string (Line parameter) and
 * if both the following conditions apply:
 *   (1) the Line contains the string specified in the
 *       Filter parameter
 *   (2) the Line contains also the string that begins
 *       with the StartWith parameter
 * it provides back a char pointer to the first character
 * in the Line of the StartWith parameter. If any of the
 * previous conditions doesn't apply, it provides NULL
 */
char * FilterAndExtract (char * Line, const char * Filter, const char * StartWith)
{
    if ( strstr(Line,Filter)!= NULL )
        return (strstr(Line,StartWith));
    else
        return (NULL);
}


/*
 * This function is an enhanced version of strcmp(). It takes
 * two input strings and compare them eventually taking
 * wildcards into account. Specifically, if the second string
 * contains a '*', this represents any combination of chars
 * (from 0 up to any possible number n). If the second string
 * contains a '?' or a '!', this represents exactly one character.
 * Such wildcards are allowed only within the second string.
 * If they are contained in the first string, they are not
 * considered as wildcards, but exactly as any other character
 * (i.e. if first string is "1234*", it can only match with
 * "1234*" and anything else).
 * The function returns FALSE if the strings match,
 * TRUE otherwise.
 * The third parameter is a boolean flag that is TRUE if
 * string1 contains a wildcard, FALSE otherwise.
 * The fourth parameter is a boolean flag that is TRUE if
 * the two strings match exactly, FALSE in case the match is
 * due to the presence of wildcards in the second string.
 */
Boolean StrCmpWildcards  (char    * string1,
                          char    * string2,
                          Boolean * WildcardInStr1,
                          Boolean * ExactMatch)
{
    char     *p1, *p2, *p3, *p4,
              p3char;

    if (strpbrk(string1,"*?!") != NULL)
        (*WildcardInStr1) = TRUE;
    else
        (*WildcardInStr1) = FALSE;

    *ExactMatch = FALSE;

    /* If strings are equal char-by-char, return
     * directly FALSE (MATCH) and set ExactMatch flag to TRUE */
    if ( strcmp(string1,string2) == 0)
    {
        *ExactMatch = TRUE;
        return (FALSE);
    }


    /* If strings are not equal char-by-char, but first string
     * contains a wildcard, return directly TRUE (NO MATCH)*/
    if ( (*WildcardInStr1)==TRUE )
        return (TRUE);

    /* If the first string (i.e. the one
     * without wildcards) is shorter, then
     * they cannot be equal
     * The only exception is when the two strings
     * are of the type "1234" and "1234*" or
     * "" and "*"
     * This is the reason for strlen(string2)-1
     * below */
    if (strlen(string1) < (strlen(string2)-1) )
        return (TRUE);

    /* Compare strings char by char, handling
     * properly wildcards in the second one */
    p1 = string1;
    p2 = string2;
    while (*p2 != '\0')
    {
        switch (*p2)
        {
            case '*':
            {
                p2++;
                if (*p2 == '\0')
                    return (FALSE);
                for (p3=p2;(*p3!='\0')&&(*p3!='*')&&(*p3!='?')&&(*p3!='!') ; p3++);
                p3char = *p3;
                *p3 = '\0';
                if ( (p4=strstr(p1,p2)) == NULL)
                    return (TRUE);
                p1 = p4 + (p3-p2);
                *p3 = p3char;
                return (StrCmpWildcards(p1,p3,WildcardInStr1,ExactMatch));
                break;
            }
            case '?':
            case '!':
            {
                if (*p1 == '\0')
                    return (TRUE);
                p1++;
                p2++;
                break;
            }
            default:
            {
                if (*p1 != *p2)
                    return (TRUE);
                p1++;
                p2++;
                break;
            }
        }
    }

    if (*p1 == '\0')
        return (FALSE);
    else
        return (TRUE);

}


/*
 * This function takes a NULL terminated string and
 * modifies it by removing all blanks, tabs and new lines
 * The behavior is undefined if the string
 * is not NULL terminated.
 * Please consider that the input string is modified
 * by this function.
 */
void RemoveBlanks (char * buf)
{
    /* Local pointers at the beginning of the input string */
    char  * p = buf,
          * q = buf;

    while ( (*p != '\0') && (*q != '\0') )
    {
        while ( (*q == ' ') || (*q == '\t') || (*q == '\n') )
            q += 1;
        if (*q == '\0')
            break;
        *p = *q;
        p += 1;
        q += 1;
    }
    *p = '\0';

}


/*
 * This function takes a NULL terminated string (src) and
 * copies it onto another string (dst), but removing all
 * blanks, tabs and new lines. It differs from
 * RemoveBlanks() since it preserves the original
 * string (i.e. it does not modify the input string).
 * The behaviour is undefined if the string
 * is not NULL terminated
 */
void CopyAndRemoveBlanks (char * dst, char * src)
{
    /* Local pointers at the beginning of the input string */
    char  * p = src,
          * q = dst;

    while (*p != '\0')
    {
        while ( (*p == ' ') || (*p == '\t') || (*p == '\n') )
            p += 1;
        if (*p == '\0')
            break;
        *q = *p;
        p += 1;
        q += 1;
    }
    *q = '\0';

}


/*
 * This function provides back TRUE if the input string consists only
 * of digits, FALSE otherwise
 */
Boolean OnlyDigits (char * inputstr)
{
    /* Local pointer at the beginning of the input string */
    char  * p;

    if ( (inputstr == NULL) || (*inputstr == '\0') )
        return (FALSE);

    for (p=inputstr; *p != '\0'; p++)
        if ( (*p < '0') || (*p > '9') )
            return (FALSE);

    return (TRUE);

}


/*
 * This function takes a string as input parameter and provides TRUE
 * if it represents an e-mail syntactically correct, FALSE otherwise.
 * It is assumed that the input string is shorter than 128 characters,
 * otherwise it is considered not valid (there is no specific rule
 * that states so, but it seems a reasonable assumption in almost all
 * practical situations).
 */
Boolean CheckMailValidity (char * email)
{
    /* Local Variables */
    char      *p, *q;
    int        len;
    MediumString emailcopy;

    /* Check preliminarly string length */
    if ( ((len=strlen(email)) >= MEDIUMSTRINGMAXLEN) || (len<1) )
        return (FALSE);     /* the input string is empty or longer than 127 characters */

    /* Take a local copy of the email, to avoid affecting input params */
    /* The copy is limited to MEDIUMSTRINGMAXLEN-1 = 127 characters    */
    strncpy (emailcopy,email,MEDIUMSTRINGMAXLEN-1);
    emailcopy[MEDIUMSTRINGMAXLEN-1] = '\0';

    /* Determines the position of @ */
    if ( (p=strchr(emailcopy,'@')) == NULL)
        return (FALSE);     /* No @ in the input string -> Not a valid e-mail */

    /* Found (at least) a @, separate substrings at the left and at the right */
    /* Let's consider the following example format: name@domain.tld (tld = top level domain) */

    /* First, work on the left substring, i.e. name */
    q = emailcopy;
    *p = '\0';
    len = strlen(q);    /* length of the left substring, since @ has been replaced by null character */

    if ( (len==0) || (q[0]=='.') || (q[len-1]=='.') )
        return (FALSE);     /* the left substring is empty, or it begins or ends with a dot -> Not a valid e-mail */

    for (q=emailcopy; *q!='\0'; q++)
    {
        if ( strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.",*q) == NULL)
            return (FALSE); /* in the left substring there is an invalid character */
        if ( (*q=='.') && ( (*(q-1)=='.') || (*(q+1)=='.') ) )
            return (FALSE); /* in the left substring there are two consecutive dots */
    }

    /* Now work on the right substring, i.e. domain.tld */
    q = ++p;
    len = strlen(q);    /* length of the right substring */

    if ( (len==0) || (q[0]=='.') || (q[len-1]=='.') )
        return (FALSE);     /* the right substring is empty, or it begins or ends with a dot -> Not a valid e-mail */

    if ( (strchr(p,'.')) == NULL)
        return (FALSE);     /* No . in the right string -> Not a valid e-mail */

    for (q=p; *q!='\0'; q++)
    {
        if ( strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.",*q) == NULL)
            return (FALSE); /* in the right substring there is an invalid character */
        if ( (*q=='.') && ( (*(q-1)=='.') || (*(q+1)=='.') ) )
            return (FALSE); /* in the right substring there are two consecutive dots */
    }

    /* In the following, we check that top level domain has length between 2 and 3 */
    /* Not sure this check shall be kept. For example, in CheckUrlValidity() it has been removed */
    /* Comment the following 4 lines if the check shall be removed  */
//    for (q=p+len-1; *q!='.'; q--);  /* go back from the end to the last dot in the right part */
//    q++;
//    if ( ((len=strlen(q)) <2 ) || (len>3) )
//        return (FALSE);     /* top level domain shall be 2 or 3 charaters */
    /* At last I decided to remove it */

    return (TRUE);

}


/*
 * This function provides TRUE if the input parameter is a valid IPv4 Address (i.e. a
 * string formatted as a.b.c.d, where a, b, c, d are integers between 0 and 255), FALSE
 * otherwise. If TRUE the second parameter provides the IP addr converted into a uint32_t
 */
Boolean CheckIPv4AddValidity (char *ipAddrStr, uint32_t *ipAddr)
{
    /* Local Variables */
    char        *p,
                *q;
    int         i=0;
    int         n[4];
    ShortString ip;

    /* Takes a local copy of ip address, to avoid affecting it */
    strcpy (ip,ipAddrStr);

    p = ip;

    while (i<=2)
    {
        /* Extract first three numbers by looking for dots - return FALSE if any error*/
        if ( (q=strstr(p,"."))==NULL)
            return (FALSE);
        *q='\0';
        if ( !OnlyDigits(p) )
            return (FALSE);
        n[i] = atoi (p);
        if ( (n[i]<0) || (n[i]>255) )
            return (FALSE);
        i++;
        p = q+1;
    }   /* while (i<2) */

    /* Now extract last number and return FALSE if any error */
    if ( !OnlyDigits(p) )
        return (FALSE);
    n[3] = atoi (p);
    if ( (n[3]<0) || (n[3]>255) )
        return (FALSE);

    *ipAddr = (n[0]<<24) | (n[1]<<16) | (n[2]<<8) | n[3];

    return (TRUE);
}


/*
 * This function takes a string as input parameter and provides TRUE
 * if it represents an URL syntactically correct, FALSE otherwise.
 * It is assumed that the input string is shorter than 256 characters,
 * otherwise it is considered not valid (there is no specific rule
 * that states so, but it seems a reasonable assumption in almost all
 * practical situations).
 * Observe that the general URL format is the following:
 *   [protocol://][username[:password]@]host[:port][</path>][?querystring][#fragment]
 * This version does not implement full URL verification, since it has
 * some minor limitations:
 *   - it does not support URL authentication, i.e. in case of
 *     [username[:password]@] the URL is not recognized as valid;
 *   - [?querystring] and [#fragment] are not completely verified (both
 *     should be formatted as concatenation of parameter/value pairs, i.e.
 *     param1=value1&param2=value2... This is not controlled, the routine
 *     just checks that those sections contain only allowed characters
 */
Boolean CheckUrlValidity (char * url)
{
    /* Local Variables */
    char            *p, *q, *r, *s;
    int             len;
    uint32_t        tmp;
    Medium2String   urlcopy;

    /* Check preliminarly string length */
    if ( ((len=strlen(url)) >= MEDIUM2STRINGMAXLEN) || (len<1) )
        return (FALSE);     /* the input string is empty or longer than 255 characters */

    /* Take a local copy of the url, to avoid affecting input param  */
    /* The copy is limited to MEDIUM2STRINGMAXLEN-1 = 255 characters */
    strncpy (urlcopy,url,MEDIUM2STRINGMAXLEN-1);
    urlcopy[MEDIUM2STRINGMAXLEN-1] = '\0';

    /* Determines the position of :// substring, if present */
    if ( (p=strstr(urlcopy,"://")) != NULL )
    {
        /* the protocol is present, check it */
        *p = '\0';
        p += 3;
        if ( strcasecmp(urlcopy,"mailto")==0 )
            return (CheckMailValidity(p));

        if ( strcasecmp(urlcopy,"http")  &&
             strcasecmp(urlcopy,"https") &&
             strcasecmp(urlcopy,"ftp")   &&
             strcasecmp(urlcopy,"ftps")  &&
             strcasecmp(urlcopy,"sftp")  &&
             strcasecmp(urlcopy,"gopher")&&
             strcasecmp(urlcopy,"news")  &&
             strcasecmp(urlcopy,"telnet")&&
             strcasecmp(urlcopy,"aim") )
            return (FALSE);
    }   /* if ( p=strcasestr(urlcopy,"://") ) */
    else
        p = urlcopy;

    /* p points to the beginning of the host[:port][</path>][?querystring][#fragment] section  */
    /* now set q at the beginning of host and p at the the end (first slash or NULL character) */
    /* r points to the [</path>][?querystring][#fragment] section (NULL if not present) */
    q = p;
    if ( (p=strchr(q,'/')) == NULL)
    {   /* No slash --> only host[:port] --> r set to NULL */
        len = strlen(q);    /* length of the host[:port] section */
        p = q+len;
        r = NULL;
    }
    else
    {   /* Slash is present --> [</path>][?querystring][#fragment] is pointed by r */
        *p = '\0';
        r = p+1;
    }

    if ( (p=strchr(q,':')) != NULL)
    {   /* : is present --> both host (pointed by q) and port (pointed by p) */
        *p = '\0';
        p++;
    }
    if (CheckIPv4AddValidity(q,&tmp)==FALSE)
    {   /* host (pointed by q) is not a valid IPv4 address, check it has the correct syntax */
        len = strlen(q);    /* length of the host */
        if ( (len==0) || (q[0]=='.') || (q[len-1]=='.') )
            return (FALSE);     /* The host is empty, or it begins or ends with a dot -> Not valid */
        if ( (strchr(q,'.')) == NULL)
            return (FALSE);     /* No . in the host -> Not valid */
        for (s=q; *s!='\0'; s++)
        {
            if ( strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.",*s) == NULL)
                return (FALSE); /* in the host there is an invalid character */
            if ( (*s=='.') && ( (*(s-1)=='.') || (*(s+1)=='.') ) )
                return (FALSE); /* in the host there are two consecutive dots */
        }

        /* Differently from CheckMailValidity(), there is no check on top level domain length */
//      for (s=q+len-1; *s!='.'; s--);  /* go back from the end to the last dot in the host */
//      s++;
//      if ( ((len=strlen(s)) <2 ) || (len>3) )
//          return (FALSE);     /* top level domain shall be 2 or 3 charaters */

    }   /* if (CheckIPv4AddValidity(q,&tmp)==FALSE) */

    /* If we are here, host is present and correct */
    /* If present, check the port (i.e. if p is not NULL) */
    if ( (p!= NULL) && (*p!='\0') && ((OnlyDigits(p)==FALSE) || (atoi(p)<0) || (atoi(p)>65535)) )
        return (FALSE);

    /* If r is null or points to an empty string, then the URL is terminated and is valid */
    if ( (r==NULL) || (*r=='\0') )
        return (TRUE);

    /* r points to the beginning of [</path>][?querystring][#fragment] section */
    if ( ((p=strchr(r,'?')) != NULL) || ((p=strchr(r,'#')) != NULL) )
    {   /* [?querystring][#fragment] is not empty and is pointed by q */
        *p = '\0';
        q = p+1;
    }
    else
        q = NULL;

    if ( (CheckFileNameValidity(r)!=MIXFOK) || (*r=='.') || (*r=='/') )
        return (FALSE);

    if (q==NULL)    /* No [?querystring][#fragment] --> URL is valid */
        return (TRUE);

    /* q points to the beginning of [?querystring][#fragment] section */
    if ( ((p=strchr(q,'?')) != NULL) || ((p=strchr(q,'#')) != NULL) )
    {   /* there are both [?querystring] and [#fragment] sections */
        /* the first is pointed by q, the other one by s */
        *p = '\0';
        s = p+1;
    }
    else
        s = NULL;

    if ( (strlen(q)<1) || (strpbrk(q,"|!\"£$()?\'^\\[]*+@#;:,<>") != NULL) )
        return (FALSE);
    if ( (s!=NULL) && ((strlen(s)<1) || (strpbrk(s,"|!\"£$()?\'^\\[]*+@#;:,<>") != NULL)) )
        return (FALSE);

    return (TRUE);
}


/* This function takes two input parameters, specifically a string composed
 * by a set of characters (charset, in the second parameter) and an integer
 * (length, in the third parameter). It provides back in the first parameter
 * a random token composed by length characters extracted from the charset.
 * There is no specific length limitation in the generated token, length can
 * be set arbitrarily high, given that the string in the first parameter is
 * allocated correspondingly. The function behaviour is unpredictable in
 * case the first string has not enough space.
 * The function returns MIXFOK in case of token successfully generated,
 * MIXFKO in case of errors (e.g. empty charset or length <=0).
 */
Error GenerateToken (char *token, char *charset, int length)
{
    /* Local Variables */
    char *p, *q;
    int   i, len;
    time_t   currenttime;
    static Boolean RandSeedInitialized = FALSE;

    /* Preliminary checks */
    if ( (token==NULL) || (charset==NULL) || (charset[0]=='\0') || (length<=0) )
        return (MIXFKO);
    len = strlen (charset);

    /* Initialize Random Seed only once */
    if (RandSeedInitialized == FALSE)
    {   /* First Invocation - Initialize Random Seed */
        /* Evaluate current time to be used as a seed for random numbers generator */
        time (&currenttime);
        srand ( (unsigned int) currenttime);
        RandSeedInitialized = TRUE;
    }

    /* Loop and build token step by step extracting random chars from charset */
    p=token;
    for (i=0; i<length;i++)
    {
        q = charset + rand()%len;
        *p = *q;
        p++;
    }

    /* Insert trailing 0 */
    *p = '\0';

    return (MIXFOK);
}


/* ----------------------------
 * Configuration Files Handling
 * ----------------------------*/
/*
 * Reset the internal list of allowed parameters
 * (call this before initializing the list)
 */
void ResetParamList (void)
{
    numparams=0;
    maxparams=DEFPARAMARRAYSIZE;
    if (ParamVect)
        free (ParamVect);
    ParsedFlag=FALSE;
    return ;
}


/* This function is used to define the maximum number of parameters managed by the
 * Configuration Files Handling functions. Allowed values are comprised in the range
 * between 1 and 255. This function is not mandatory, i.e. it can also be skipped;
 * in this case, the default value for the maximum number of parameters is set to 8.
 * This function provides MIXFOK if execution is successful, MIXFKO in case it is
 * called twice without invoking ResetParamList() first, MIXFOVFL if the parameter
 * is outside the allowed range 1-255 or the system runs out of memory
 */
Error InitParamList (int maxp)
{
    /* If the function has already been called or at least a parameter */
    /* has already been defined, then provide MIXFKO error             */
    if ( (ParamVect) || (numparams) )
        return (MIXFKO);

    /* If the parameter is outside the allowed range, return MIXFOVFL */
    if ( (maxp < 1) || (maxp > MAXPARAMARRAYSIZE) )
        return (MIXFOVFL);

    /* If the system is unable to allocate the memoru, return MIXFOVFL*/
    if  ( (ParamVect = calloc (maxp, sizeof(Param)))==NULL )
        return (MIXFOVFL);

    /* Everything OK, initialize parameters and return MIXFOK */
    numparams = 0;
    maxparams = maxp;
    return (MIXFOK);

}

/*
 * This function adds a numerical parameter to the list of parameters allowed in the
 * configuration file. The first argument is the parameter name, the second is the Mandatory
 * flag; it must be set to TRUE if the parameter is mandatory, FALSE if it is optional in the
 * configuration file. Third, fourth and fifth parameter are 3 integers that represent respectively
 * the minimum, maximum and default value for the parameter (please observe that default is
 * actually meaningful only if the Mandatory flag is FALSE). The last four parameters are event
 * codes that are associated respectively to the following events: (1) Event corresponding to a
 * Mandatory parameter non provisioned; (2) Event corresponding to an Optional parameter not
 * provisioned (default value used instead); (3) Event corresponding to a parameter that is redefined
 * (at least twice); (4) Event corresponding to a parameter value out of range or malformed (e.g. not
 * a number). Use UNDEFINED for those events that are not meaningful (e.g. event (1) does not make
 * sense if Mandatory flag is FALSE).
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in InitParamList),
 * MIXFWRONGDEF if the condition min <= default <= max is not satisfied and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error AddNumericalParam (char *name, Boolean mand, int min, int max, int def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def < min) || (def > max) || (min > max) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = FALSE;
    ParamVect[numparams].Type = numerical;
    ParamVect[numparams].Values.Num.Min = min;
    ParamVect[numparams].Values.Num.Max = max;
    ParamVect[numparams].Values.Num.Def = def;
    ParamVect[numparams].Values.Num.Val = def;
    ParamVect[numparams].MandNotProv = evn1;
    ParamVect[numparams].OptNotProv = evn2;
    ParamVect[numparams].Redefined = evn3;
    ParamVect[numparams].MalfOrOOR = evn4;

    /* Increase the total number of parameters and exit without errors */
    numparams+=1;
    return (MIXFOK);

}


/*
 * This function adds a literal parameter to the list of parameters allowed in the
 * configuration file. The description is exactly the same as AddNumericalParam(), with the
 * only difference that there is no minimum or maximum value, but only a default (which is
 * defined in the third argument). There is no special check on the possible values of
 * literal parameters, therefore the MIXFWRONGDEF error code cannot be returned by this
 * function
 */
Error AddLiteralParam (char *name, Boolean mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = FALSE;
    ParamVect[numparams].Type = literal;
    if (def != NULL)
    {
        strcpy (ParamVect[numparams].Values.Lit.Def,def);
        strcpy (ParamVect[numparams].Values.Lit.Val,def);
    }
    else
    {   /* if the default is NULL set it to the empty string */
        ParamVect[numparams].Values.Lit.Def[0] = '\0';
        ParamVect[numparams].Values.Lit.Val[0] = '\0';
    }
    ParamVect[numparams].MandNotProv = evn1;
    ParamVect[numparams].OptNotProv = evn2;
    ParamVect[numparams].Redefined = evn3;
    ParamVect[numparams].MalfOrOOR = evn4;

    /* Increase the total number of parameters and exit without errors */
    numparams+=1;
    return (MIXFOK);

}


/*
 * This function adds a filename parameter to the list of parameters allowed in the
 * configuration file. The description is exactly the same as AddLiteralParam(); however,
 * there is a minor difference between literal and filename parameters, since the
 * filename parameter can only get values that are valid filenames (see also
 * CheckFileNameValidity() above. This routine can provide back all error codes as the
 * previous ones, including MIXFWRONGDEF (which is given when the default value in the
 * third argument is not a valid file name)
 */
Error AddFilenameParam (char *name, Boolean mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (CheckFileNameValidity(def) != MIXFOK) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = FALSE;
    ParamVect[numparams].Type = filename;
    if (def != NULL)
    {
        strcpy (ParamVect[numparams].Values.Lit.Def,def);
        strcpy (ParamVect[numparams].Values.Lit.Val,def);
    }
    else
    {   /* if the default is NULL set it to the empty string */
        ParamVect[numparams].Values.Lit.Def[0] = '\0';
        ParamVect[numparams].Values.Lit.Val[0] = '\0';
    }
    ParamVect[numparams].MandNotProv = evn1;
    ParamVect[numparams].OptNotProv = evn2;
    ParamVect[numparams].Redefined = evn3;
    ParamVect[numparams].MalfOrOOR = evn4;

    /* Increase the total number of parameters and exit without errors */
    numparams+=1;
    return (MIXFOK);

}


/* This function adds a char parameter to the list of parameters allowed in the
 * configuration file. A CHAR Parameter is a single character string and shall
 * be enclosed within apices in the configuration file (e.g. "a") otherwise it is
 * considered malformed. Apices are needed to ensure that also the blank character
 * can be easily identified (" ").
 * The first argument is the parameter name, the second is the Mandatory flag;
 * it must be set to TRUE if the parameter is mandatory, FALSE if it is optional in the
 * configuration file. Third, fourth and fifth parameter are 3 chars that represent respectively
 * the minimum, maximum and default value for the parameter (please observe that default is
 * actually meaningful only if the Mandatory flag is FALSE). The last four parameters are event
 * codes that are associated respectively to the following events: (1) Event corresponding to a
 * Mandatory parameter non provisioned; (2) Event corresponding to an Optional parameter not
 * provisioned (default value used instead); (3) Event corresponding to a parameter that is redefined
 * (at least twice); (4) Event corresponding to a parameter value out of range or malformed (e.g. not
 * a char). Use UNDEFINED for those events that are not meaningful (e.g. event (1) does not make
 * sense if Mandatory flag is FALSE).
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in InitParamList),
 * MIXFWRONGDEF if the condition min <= default <= max is not satisfied and finally
 * MIXFKO for any other error (e.g. parameter name empty) */
Error AddCharParam (char *name, Boolean mand, char min, char max, char def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def < min) || (def > max) || (min > max) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = FALSE;
    ParamVect[numparams].Type = character;
    ParamVect[numparams].Values.Car.Min = min;
    ParamVect[numparams].Values.Car.Max = max;
    ParamVect[numparams].Values.Car.Def = def;
    ParamVect[numparams].Values.Car.Val = def;
    ParamVect[numparams].MandNotProv = evn1;
    ParamVect[numparams].OptNotProv = evn2;
    ParamVect[numparams].Redefined = evn3;
    ParamVect[numparams].MalfOrOOR = evn4;

    /* Increase the total number of parameters and exit without errors */
    numparams+=1;
    return (MIXFOK);

}


/*
 * This function adds a mail parameter to the list of parameters allowed in the
 * configuration file. The description is exactly the same as AddLiteralParam(); however,
 * there is a minor difference between literal and mail parameters, since the
 * mail parameter can only get values that are valid e-mail addresses.
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in InitParamList),
 * MIXFWRONGDEF if the default value in the third argument is not a valid e-mail and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error AddMailParam (char *name, Boolean mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (CheckMailValidity(def)==FALSE) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = FALSE;
    ParamVect[numparams].Type = email;
    if (def != NULL)
    {
        strcpy (ParamVect[numparams].Values.Lit.Def,def);
        strcpy (ParamVect[numparams].Values.Lit.Val,def);
    }
    else
    {   /* if the default is NULL set it to the empty string */
        ParamVect[numparams].Values.Lit.Def[0] = '\0';
        ParamVect[numparams].Values.Lit.Val[0] = '\0';
    }
    ParamVect[numparams].MandNotProv = evn1;
    ParamVect[numparams].OptNotProv = evn2;
    ParamVect[numparams].Redefined = evn3;
    ParamVect[numparams].MalfOrOOR = evn4;

    /* Increase the total number of parameters and exit without errors */
    numparams+=1;
    return (MIXFOK);

}


/*
 * This function adds an ipv4 parameter to the list of parameters allowed in the
 * configuration file. The description is exactly the same as AddLiteralParam(); however,
 * there is a minor difference between literal and ipv4 parameters, since the
 * ipv4 parameter can only get values that are valid IPv4 addresses.
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in InitParamList),
 * MIXFWRONGDEF if the default value in the third argument is not a valid IPv4 address and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error AddIPv4Param (char *name, Boolean mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    uint32_t    tmp;

    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (CheckIPv4AddValidity(def,&tmp)==FALSE) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = FALSE;
    ParamVect[numparams].Type = ipv4;
    if (def != NULL)
    {
        strcpy (ParamVect[numparams].Values.ipv4.Def,def);
        strcpy (ParamVect[numparams].Values.ipv4.Val,def);
    }
    else
    {   /* if the default is NULL set it to the empty string */
        ParamVect[numparams].Values.ipv4.Def[0] = '\0';
        ParamVect[numparams].Values.ipv4.Val[0] = '\0';
    }
    ParamVect[numparams].MandNotProv = evn1;
    ParamVect[numparams].OptNotProv = evn2;
    ParamVect[numparams].Redefined = evn3;
    ParamVect[numparams].MalfOrOOR = evn4;

    /* Increase the total number of parameters and exit without errors */
    numparams+=1;
    return (MIXFOK);

}


/*
 * This function adds an URL parameter to the list of parameters allowed in the
 * configuration file. The description is exactly the same as AddLiteralParam(); however,
 * there is a minor difference between literal and URL parameters, since the
 * URL parameter can only get values that are valid URLs.
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in InitParamList),
 * MIXFWRONGDEF if the default value in the third argument is not a valid URL and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error AddUrlParam (char *name, Boolean mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (CheckUrlValidity(def)==FALSE) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = FALSE;
    ParamVect[numparams].Type = url;
    if (def != NULL)
    {
        strcpy (ParamVect[numparams].Values.Url.Def,def);
        strcpy (ParamVect[numparams].Values.Url.Val,def);
    }
    else
    {   /* if the default is NULL set it to the empty string */
        ParamVect[numparams].Values.Url.Def[0] = '\0';
        ParamVect[numparams].Values.Url.Val[0] = '\0';
    }
    ParamVect[numparams].MandNotProv = evn1;
    ParamVect[numparams].OptNotProv = evn2;
    ParamVect[numparams].Redefined = evn3;
    ParamVect[numparams].MalfOrOOR = evn4;

    /* Increase the total number of parameters and exit without errors */
    numparams+=1;
    return (MIXFOK);

}


/*
 * This routine provides through the second argument the value of the numerical parameter
 * whose name is given by the first argument. It provides return code MIXFOK if the parameter
 * is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
 * configuration file (i.e. ParseCfgParamFile() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is TRUE if the parameter was
 * actually provisioned in the configuration file, while is FALSE if it wasn't
 */
Error GetNumParamValue (char *param, int *value, Boolean *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==FALSE)
        return (MIXFNOACCESS);

    for (i=0; (i<numparams)&&(res!=MIXFOK) ; i+=1)
        if ( (ParamVect[i].Type == numerical) && (strcmp (param, ParamVect[i].Name)==0) )
        {
            res = MIXFOK;
            *value = ParamVect[i].Values.Num.Val;
            *prov = ParamVect[i].Provisioned;
        }

    return (res);
}


/*
 * This routine provides through the second argument the value of the literal parameter
 * whose name is given by the first argument. It provides return code MIXFOK if the parameter
 * is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
 * configuration file (i.e. ParseCfgParamFile() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is TRUE if the parameter was
 * actually provisioned in the configuration file, while is FALSE if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error GetLitParamValue (char *param, char *value, Boolean *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==FALSE)
        return (MIXFNOACCESS);

    for (i=0; (i<numparams)&&(res!=MIXFOK) ; i+=1)
        if ( (ParamVect[i].Type == literal) && (strcmp (param, ParamVect[i].Name)==0) )
        {
            res = MIXFOK;
            strcpy (value,ParamVect[i].Values.Lit.Val);
            *prov = ParamVect[i].Provisioned;
        }

    return (res);
}


/*
 * This routine provides through the second argument the value of the filename parameter
 * whose name is given by the first argument. It provides return code MIXFOK if the parameter
 * is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
 * configuration file (i.e. ParseCfgParamFile() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is TRUE if the parameter was
 * actually provisioned in the configuration file, while is FALSE if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error GetFNameParamValue (char *param, char *value, Boolean *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==FALSE)
        return (MIXFNOACCESS);

    for (i=0; (i<numparams)&&(res!=MIXFOK) ; i+=1)
        if ( (ParamVect[i].Type == filename) && (strcmp (param, ParamVect[i].Name)==0) )
        {
            res = MIXFOK;
            strcpy (value,ParamVect[i].Values.Lit.Val);
            *prov = ParamVect[i].Provisioned;
        }

    return (res);
}


/* This routine provides through the second argument the value of the char parameter
 * whose name is given by the first argument. It provides return code MIXFOK if the parameter
 * is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
 * configuration file (i.e. ParseCfgParamFile() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is TRUE if the parameter was
 * actually provisioned in the configuration file, while is FALSE if it wasn't */
Error GetCharParamValue (char *param, char *value, Boolean *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==FALSE)
        return (MIXFNOACCESS);

    for (i=0; (i<numparams)&&(res!=MIXFOK) ; i+=1)
        if ( (ParamVect[i].Type == character) && (strcmp (param, ParamVect[i].Name)==0) )
        {
            res = MIXFOK;
            *value = ParamVect[i].Values.Car.Val;
            *prov = ParamVect[i].Provisioned;
        }

    return (res);
}


/*
 * This routine provides through the second argument the value of the email parameter
 * whose name is given by the first argument. It provides return code MIXFOK if the parameter
 * is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
 * configuration file (i.e. ParseCfgParamFile() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is TRUE if the parameter was
 * actually provisioned in the configuration file, while is FALSE if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error GetMailParamValue (char *param, char *value, Boolean *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==FALSE)
        return (MIXFNOACCESS);

    for (i=0; (i<numparams)&&(res!=MIXFOK) ; i+=1)
        if ( (ParamVect[i].Type == email) && (strcmp (param, ParamVect[i].Name)==0) )
        {
            res = MIXFOK;
            strcpy (value,ParamVect[i].Values.Lit.Val);
            *prov = ParamVect[i].Provisioned;
        }

    return (res);
}


/*
 * This routine provides through the second argument the value of the ipv4 parameter
 * whose name is given by the first argument. It provides return code MIXFOK if the parameter
 * is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
 * configuration file (i.e. ParseCfgParamFile() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is TRUE if the parameter was
 * actually provisioned in the configuration file, while is FALSE if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error GetIPv4ParamValue (char *param, char *value, Boolean *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==FALSE)
        return (MIXFNOACCESS);

    for (i=0; (i<numparams)&&(res!=MIXFOK) ; i+=1)
        if ( (ParamVect[i].Type == ipv4) && (strcmp (param, ParamVect[i].Name)==0) )
        {
            res = MIXFOK;
            strcpy (value,ParamVect[i].Values.ipv4.Val);
            *prov = ParamVect[i].Provisioned;
        }

    return (res);
}


/*
 * This routine provides through the second argument the value of the URL parameter
 * whose name is given by the first argument. It provides return code MIXFOK if the parameter
 * is successfully read, MIXFNOACCESS if parameter is defined, but not actually read from the
 * configuration file (i.e. ParseCfgParamFile() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is TRUE if the parameter was
 * actually provisioned in the configuration file, while is FALSE if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error GetUrlParamValue (char *param, char *value, Boolean *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==FALSE)
        return (MIXFNOACCESS);

    for (i=0; (i<numparams)&&(res!=MIXFOK) ; i+=1)
        if ( (ParamVect[i].Type == url) && (strcmp (param, ParamVect[i].Name)==0) )
        {
            res = MIXFOK;
            strcpy (value,ParamVect[i].Values.Url.Val);
            *prov = ParamVect[i].Provisioned;
        }

    return (res);
}


/*
 * This routine opens and parses the configuration file specified as
 * first parameter (CfgFileName). This file shall be composed of
 * lines with the following format:
 *      PARAM = VALUE
 * Also comments are allowed (#), both at the beginning or at the end
 * of a line. If the file does not respect this layout, it is considered
 * wrongly formatted.
 * For each allowed parameter ParamVect[xx] contains a set of information
 * (parameter name, type, default value and range if applicable, as well as
 * a set of flags that are used to specify if the this is a mandatory
 * or optional parameter).
 * The routine opens the file, parses it, checks for errors and,
 * if everything is ok, the actual value of the parameter is written
 * either into ParamVect[xx].Values.Num.Val or
 * ParamVect[xx].Values.Lit.Val (it depends on the
 * parameter type). The flag in ParamVect[xx].Provisioned
 * is used to understand if the corresponding parameter
 * was actually defined in the file (TRUE) or if the
 * default value was provisioned instead (FALSE). The latter
 * case applies only for optional parameters.
 * The routine may provide back:
 *  - MIXFOK
 *    the file was opened successfully, it is correctly formatted
 *    and do not contain unrecognized parameters. In this case
 *    the second parameter (totalline) contains the total number of
 *    lines of the file, while the third is a pointer to a pointer to
 *    a list of events found during parsing
 *  - MIXFNOACCESS
 *    the function is not able to open the configuration file
 *    (it may not exist or the user may not have read permission).
 *    In this case, both the second and the third parameters are
 *    not meaningful
 *  - MIXFFORMATERROR
 *    the file is wrongly formatted (it does not respect the layout
 *    specified above). The second parameter (totalline) contains the
 *    line in which the error occurred, while the third (events) is
 *    null
 *  - MIXFPARAMUNKNOWN
 *    the file contains a parameter that is not recognized, i.e.
 *    which does not belong to the ParamVect array. The second
 *    parameter (totalline) contains the line in which the error
 *    occurred, while the third (events) is null
 *
 * Please observe that relevant events are also provisioned at initialization
 * time in the ParamVect array
 */
Error ParseCfgParamFile (char *CfgFileName, uint16_t *totalline, EventList **events)
{
    /* Local variables */
    LongString  stringbuffer,
                inputstringbuffer;
    char        *p, *q;
    uint8_t     i;
    int         val;
    Boolean     found;
    EventList   *evnlst = NULL;
    FILE        *CfgFile_fd;

    /* Set initial values */
    *totalline = 0;
    *events = NULL;

    /* Assign default values to all parameters and set Provisioned flag to FALSE */
    for (i=0; i<numparams; i+=1)
    {
        ParamVect[i].Provisioned = FALSE;
        switch (ParamVect[i].Type)
        {
            case numerical:
            {
                ParamVect[i].Values.Num.Val = ParamVect[i].Values.Num.Def;
                break;
            }
            case character:
            {
                ParamVect[i].Values.Car.Val = ParamVect[i].Values.Car.Def;
                break;
            }
            case url:
            {
                strcpy (ParamVect[i].Values.Url.Val, ParamVect[i].Values.Url.Def);
                break;
            }
            case ipv4:
            {
                strcpy (ParamVect[i].Values.ipv4.Val, ParamVect[i].Values.ipv4.Def);
                break;
            }
            default:
            {   /* valid for literal, filename and mail parameters */
                strcpy (ParamVect[i].Values.Lit.Val, ParamVect[i].Values.Lit.Def);
                break;
            }
        }
    }

    /* Open Configuration File in Read Only mode */
    /* returns an error if not possible */
    if (CfgFileName[0]=='\0')
        return (MIXFNOACCESS);
    if ( (CfgFile_fd=fopen(CfgFileName,"r")) == NULL )
        return (MIXFNOACCESS);

    /* Parse it line-by-line looking for relevant parameters */
    /* Start reading data from the configuration file */
    while (fgets (inputstringbuffer,LONGSTRINGMAXLEN,CfgFile_fd))
    {
        /* Valid string into inputstringbuffer */
        CopyAndRemoveBlanks (stringbuffer, inputstringbuffer);
        (*totalline) += 1;
        if ( (stringbuffer[0] == '\0') || (stringbuffer[0] == '#') )
            continue ;  /* This is either an empty line or a comment - skip this line */
        /* the line is neither a comment nor an empty line - therefore it must begin with a valid param */
        found = FALSE;
        i=0;
        p = stringbuffer;
        while ( (found == FALSE) && (i<numparams) )
        {
            if ( (strncmp(p,ParamVect[i].Name,strlen(ParamVect[i].Name)) == 0) && (*(p+strlen(ParamVect[i].Name))=='=') )
            {   /* The parameter defined in this line is the one in i-th position in ParamVect array */
                found = TRUE;

                /* Now use inputstringbuffer to extract the value after '=' in order to keep spaces */
                for (p = inputstringbuffer; (*p!='\0')&&(*p!='#')&&(*p!='=');p+=1);
                if (*p != '=')
                {
                    ClearEventList (&evnlst);
                    return (MIXFFORMATERROR);
                }
                for (p += 1 ; (*p==' ')||(*p=='\t') ; p += 1);  /* Skip all spaces and tabs after '=' */
                if ( (*p == '\0') || (*p == '#') )
                {   /* Either the line is terminated after '=' or a comment begins */
                    ClearEventList (&evnlst);
                    return (MIXFFORMATERROR);
                }
                for (q=p; (*q!='\0')&&(*q!='#')&&(*q!='\n'); q+=1); /* stop at EOL or at # */
                *q = '\0';                              /* p and q now points to the beginning and to the end of the parameter */

                /* Extract the parameter depending on its type */
                switch (ParamVect[i].Type)
                {
                    case numerical:
                    {   /* This is a numerical parameter - convert it and check against limits */
                        RemoveBlanks (p);
                        val = atoi(p);
                        if ( (OnlyDigits(p)==FALSE) || (val<ParamVect[i].Values.Num.Min) || (val>ParamVect[i].Values.Num.Max) )
                        {   /* Malformed or Out of Range - Add an event in the event list and restore default */
                            ParamVect[i].Values.Num.Val = ParamVect[i].Values.Num.Def;
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            ParamVect[i].Values.Num.Val = val;
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = TRUE;
                        }
                        break;
                    }
                    case character:
                    {   /* This is a character parameter - check it against limits */
                        if ( ( *p != '\"') || ( *(p+2) != '\"' ))
                        {   /* Malformed - Add an event in the event list and restore default */
                            ParamVect[i].Values.Car.Val = ParamVect[i].Values.Car.Def;
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            p +=1;
                            if ( ((*p)<ParamVect[i].Values.Car.Min) || ((*p)>ParamVect[i].Values.Car.Max) )
                            {   /* Out of Range - Add an event in the event list and restore default */
                                ParamVect[i].Values.Car.Val = ParamVect[i].Values.Car.Def;
                                AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                            }
                            else
                            {
                                ParamVect[i].Values.Car.Val = *p;
                                if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                    AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                                ParamVect[i].Provisioned = TRUE;
                            }
                        }
                        break;
                    }
                    case literal:
                    {   /* This is a literal parameter - no specific checks are done */
                        strcpy (ParamVect[i].Values.Lit.Val,p);
                        if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                            AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                        ParamVect[i].Provisioned = TRUE;
                        break;
                    }
                    case filename:
                    {   /* This is a filename parameter */
                        if (CheckFileNameValidity(p) != MIXFOK)
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.Lit.Val, ParamVect[i].Values.Lit.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.Lit.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = TRUE;
                        }
                        break;
                    }
                    case email:
                    {   /* This is an email parameter */
                        if ( CheckMailValidity(p) == FALSE)
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.Lit.Val, ParamVect[i].Values.Lit.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.Lit.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = TRUE;
                        }
                        break;
                    }
                    case ipv4:
                    {   /* This is an ipv4 parameter */
                        uint32_t tmp;
                        if ( CheckIPv4AddValidity(p,&tmp) == FALSE)
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.ipv4.Val, ParamVect[i].Values.ipv4.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.ipv4.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = TRUE;
                        }
                        break;
                    }
                   case url:
                    {   /* This is an url parameter */
                        if ( CheckUrlValidity(p) == FALSE )
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.Url.Val, ParamVect[i].Values.Url.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.Url.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = TRUE;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                continue;   /* this jumps to the next iteration of while ( (found == FALSE) && (i<numparams) ) */
            }   /* if ( strncmp(stringbuffer,ParamVect[i].Name, ... */
            i += 1;     /* next parameter in ParamVect[i] */
        }   /* while ( (found == FALSE) && .. */
        if (found == FALSE)
        {   /* There is an unrecognized parameter - Clear event list and exit */
            ClearEventList (&evnlst);
            return (MIXFPARAMUNKNOWN);
        }
    }   /* while (fgets (inputstringbuffer)... */

    /* We reach this point when fgets() returns null, i.e. the Task Definition file is finished */
    fclose (CfgFile_fd);

    /* Now perform an additional check on missing mandatory and optional parameters */
    for (i=0; i<numparams; i+=1)
    {   /* Check if there are parameters not provisioned and add the corresponding */
        /* event to the event list (line number is 0 since not applicable in this case) */
        if (ParamVect[i].Provisioned == FALSE)
        {
            if (ParamVect[i].Mandatory)
                AddEventInList (ParamVect[i].MandNotProv, 0, &evnlst);
            else
                AddEventInList (ParamVect[i].OptNotProv, 0, &evnlst);
        }
    }

    /* Everything seems MIXFOK - set the ParsedFlag to TRUE and exit without errors */
    *events = evnlst;
    ParsedFlag = TRUE;
    return (MIXFOK);

}


/*
 * This function releases all the memory allocated for the event list
 * by ParseCfgParamFile().
 *
 */
void ClearEventList(EventList **ptr)
{
    /* Local variables */
    EventList *p, *q;

    q = *ptr;
    while (q!=NULL)
    {
        p=q->next;
        free (q);
        q = p;
    }
    *ptr = NULL;
}


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
Error DefineLogLevels (uint8_t numloglevels, uint8_t defloglevel)
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
 * where N is the max number of log levels defined through DefineLogLevels().
 * This function provides MIXFKO in case of wrong parameters (e.g. outside allowed
 * ranges), MIXFOK if everything is ok.
 */
Error DefineLevelDescr (uint8_t level, char * textdescr)
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
 * through DefineLogLevels()
 */
Error SetLogLevel (uint8_t level)
{
    if ((level<0) || (level>=numlevels))
        return (MIXFKO);
    LogLevel = level;
    return (MIXFOK);
}


/*
 * Get the current Log Level
 */
uint8_t GetLogLevel (void)
{
    return (LogLevel);
}


/*
 * Define the maximum number of events for the system. This value shall be within
 * 1 and 255. This function is mandatory and shall be necessarily called before
 * the first occurrence of DefineEvent(). It provides MIXFOK in case of success,
 * MIXFKO otherwise
 */
Error DefineNumEvents (uint8_t maxevents)
{
    if ((maxevents<1) || (maxevents>EVENTARRAYSIZE))
        return (MIXFKO);
    numevents = maxevents;

    return (MIXFOK);
}


/*
 * This function defines attributes for each event. Specifically, the event code
 * specified by the first parameter (which shall be defined in the interval between
 * 0 and M-1, where M is the num of events defined by DefineNumEvents() ) is associated
 * with a severity level specified by the second parameter (which shall be in the
 * interval [0,N-1], being N the max number of log levels defined through
 * DefineLogLevels()). The third parameter defines the textual description for the
 * event. This string may contain up to 3 placeholders defined as "%1", "%2" and
 * "%3", which will be used to identify the position within the string of up
 * to 3 parameters (see RegisterEvent() below).
 * If the function is called multiple times with the same event code, each invoation
 * overwrites previous data (only the last definition applies).
 * This function provides MIXFOK in case of success, MIXFOVFL if the first or the second
 * parameter are out of allowed ranges, MIXFFORMATERROR if the string does not respect
 * the format specified above (e.g. it contains an invalid placeholder, such as "%4"
 * or a valid placeholder repeated twice)
 */
Error DefineEvent (EventCode event, uint8_t level, char * descr)
{
    /* Local Variables */
    char        *params[3];     /* Up to 3 parameters may be contained in the description */
    char        *p;
    LongString  EvnDescr;

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
 * <Basename>.log, without a trailing timestamp. The third argument is a Boolean;
 * if TRUE, it forces the file to be closed and re-opened daily at 00:00).
 * This function provides MIXFOK in case of SUCCESS, MIXFKO if the log is already open,
 * MIXFFORMATERROR if the first argument is not a valid file name,
 * MIXFNOACCESS if the filename cannot be opened.
 */
Error OpenLog (char *BaseName, char *format, Boolean RotateDaily)
{
    /* Local variables */
    Medium2String   LogFileName;
    ShortString     TimeStamp;


    pthread_mutex_init(&LogMutex, NULL);
    pthread_mutex_lock(&LogMutex);

    if (LogOpenFlag)    /* Already open */
    {
        pthread_mutex_unlock(&LogMutex);
        return (MIXFKO);
    }

    if (CheckFileNameValidity(BaseName) != MIXFOK)
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
    RetrieveTimeDate(LogOpenDate,"%d%m%Y");

    /* Evaluate log file name */
    if ( (format != NULL) && (format[0] != '\0') )
    {
        RetrieveTimeDate(TimeStamp,format);
        sprintf (LogFileName,"%s_%s.log",LogFileBaseName,TimeStamp);
    }
    else
        sprintf (LogFileName,"%s.log",LogFileBaseName);

    /* Open Log File in append mode */
    if ( (Log_fd=fopen(LogFileName,"a")) == NULL)
    {
        LogOpenFlag = FALSE;
        pthread_mutex_unlock(&LogMutex);
        return (MIXFNOACCESS);
    }
    LogOpenFlag = TRUE;
    pthread_mutex_unlock(&LogMutex);

    return (MIXFOK);
}


/*
 * This function closes the current Log File
 */
void CloseLog (void)
{
    /* Close log and exit */
    pthread_mutex_lock(&LogMutex);
    fclose (Log_fd);
    LogOpenFlag = FALSE;
    pthread_mutex_unlock(&LogMutex);

    pthread_mutex_destroy (&LogMutex);

    return;
}


/*
 * Register into the log the event whose event code is specified by first argument.
 * This is done only if the event severity is at least equal to the current log level
 * and if the log file is open, otherwise the call returns without effect.
 * For events that contains parameters in the textual description (see DefineEvent()
 * above), they can be specified as strings in the second ("%1" placeholder), third ("%2"
 * placeholder) and fourth ("%3" placeholder) argument). This function provides MIXFOK
 * in case of SUCCESS, MIXFKO if the log is not open, MIXFNOACCESS if the log file
 * cannot be reopened.
 */
Error RegisterEvent (EventCode event, char * param1, char * param2, char * param3)
{
    /* Local variables */
    Error       result = MIXFOK;
    ShortString CurrentDate, Time;

    /* if the severity of the event is higher than current log level, return without logging anything */
    if (Events[event].Level > LogLevel)
        return (result);

    /* enter the critical section */
    pthread_mutex_lock(&LogMutex);

    if ( (LogOpenFlag==FALSE) || (Log_fd==NULL) )   /* Log is not open - return MIXFKO error */
    {
        pthread_mutex_unlock(&LogMutex);
        return (MIXFKO);
    }

    /* Check if the date changed from the last time the log was opened */
    /* If so, and if the LogOpenRotateFlag is TRUE, close old log file and open a new one */
    if (LogOpenRotate)
    {
        RetrieveTimeDate(CurrentDate,"%d%m%Y");
        if ( strcmp(CurrentDate,LogOpenDate) )
            if ( (result=CloseReopenLog()) != MIXFOK)
            {
                pthread_mutex_unlock(&LogMutex);
                return (result);
            }

    }

    RetrieveTimeDate(Time,"%T");
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


/* ----------------
 * License Handling
 * ----------------*/
/*
 * This function takes the license file name as first argument; it takes the first line contained
 * into this file, decrypts it according to a proprietary algorithm, which uses some internal
 * platform dependent parameters (mainly hostname and hostid) and provides the decrypted content
 * of the file into the second string parameter. It returns MIXFOK if the file exists and conversion is
 * successful, MIXFNOACCESS if the file does not exist, is empty or cannot be opened
 */
Error CheckLicense (char *LicenseFileName, char *DecryptedString)
{
    /* Local variables */
    FILE            *License_fd;
    ShortString     hostname;
    MediumString    Key;
    LongString      LicenseContent;
    unsigned int    keylen, index, keyindex;
    long            hostid;

    /* Check that the license file exists and is not empty */
    if ( (License_fd=fopen(LicenseFileName,"r")) == NULL)
        return (MIXFNOACCESS);

    if (fgets(LicenseContent,LONGSTRINGMAXLEN,License_fd) == NULL)
        return (MIXFNOACCESS);

    /* Retrieve host name and host id and concatenate them in order to produce the Key */
    hostid = gethostid();
    gethostname(hostname,SHORTSTRINGMAXLEN);
    sprintf (Key,"%s%#0x",hostname,(unsigned int) hostid);
    keylen = strlen (Key);

    /* decrypt the string by means of bitwise exclusive OR between chars in the
     * encrypted string and chars in the Key */
    index=0;
    keyindex=0;
    while ( (LicenseContent[index]!='\0') && (LicenseContent[index]!='\n') )
    {
        DecryptedString[index] = (LicenseContent[index]-MAGICCHAR) ^ Key[keyindex]  ;
        index += 1;
        keyindex = (keyindex+1) % keylen;
    }
    DecryptedString[index] = '\0';

    fclose (License_fd);
    return (MIXFOK);
}


/*
 * This function takes a clear text string (first parameter) and two other strings, respectively
 * the host name and the hostid, and provides back into the first argument the encrypted string.
 * It returns MIXFOK if conversion is successful, MIXFKO in case of problems (e.g. if the hostname is empty
 * or the hostid is not an 8 digits hex number starting with 0x)
 */
Error CreateLicense (char *String, char *HostName, char *HostId)
{
    /* Local Variables */
    MediumString    Key;
    unsigned int    keylen, index, keyindex;

    /* Preliminary checks */
    if (HostName[0]=='\0')
        return (MIXFKO);
    switch (strlen(HostId))
    {   /* Check that the HostId is a string like 0x-------- or -------- */
        /* where -------- is an eigth digits hex number */
        /* Note: all letters are converted to lowercase */
        case 8:
        {
            for (keyindex=0; keyindex <8; keyindex++)
            {
                if ( (HostId[keyindex]>='0') && (HostId[keyindex]<='9') )
                    continue;
                if ( (HostId[keyindex]>='a') && (HostId[keyindex]<='f') )
                    continue;
                if ( (HostId[keyindex]>='A') && (HostId[keyindex]<='F') )
                {
                    HostId[keyindex] = HostId[keyindex] + 'a' - 'A';
                    continue;
                }
                return (MIXFKO);
            }
            sprintf (Key,"%s0x%s",HostName,HostId);
            break;
        }
        case 10:
        {
            //char    * p;
            if ( (HostId[0] != '0') || ((HostId[1] != 'x') && (HostId[1] != 'X')) )
                return (MIXFKO);
            if (HostId[1] == 'X')
                HostId[1] = 'x';
            for (keyindex=2; keyindex <10; keyindex++)
            {
                if ( (HostId[keyindex]>='0') && (HostId[keyindex]<='9') )
                    continue;
                if ( (HostId[keyindex]>='a') && (HostId[keyindex]<='f') )
                    continue;
                if ( (HostId[keyindex]>='A') && (HostId[keyindex]<='F') )
                {
                    HostId[keyindex] = HostId[keyindex] + 'a' - 'A';
                    continue;
                }
                return (MIXFKO);
            }
            //p = HostId+2;
            sprintf (Key,"%s%s",HostName,HostId);
            break;
        }
        default:
            return (MIXFKO);
    }

    /* Now perform encryption */
    keylen = strlen (Key);
    index=0;
    keyindex=0;
    while ( (String[index]!='\0') && (String[index]!='\n') )
    {
        String[index] = (String[index] ^ Key[keyindex]) + MAGICCHAR;
        index += 1;
        keyindex = (keyindex+1) % keylen;
    }
    String[index] = '\0';
    return (MIXFOK);
}


/* -------------
 * Lock Handling
 * -------------*/
/*
 * The lock is an empty file whose file name is specified as input argument.
 * If the lock is not present, this function returns FALSE, otherwise returns TRUE.
 */
Boolean CheckLockPresent(char *Lock_FileName)
{
    FILE *  lock_fd;

    /* Open Lock File - if it does not succeed, the lock is not present */

    pthread_mutex_lock(&LockMutex);
    if ( (lock_fd=fopen(Lock_FileName,"r")) == NULL)
    {
        pthread_mutex_unlock(&LockMutex);
        return (FALSE);
    }

    /* The lock is present */
    fclose (lock_fd);

    pthread_mutex_unlock(&LockMutex);
    return (TRUE);
}


/*
 * The lock is an empty file whose file name is specified as input argument.
 * Set the lock, returns MIXFOK in case of success, MIXFKO in any other case
 */
Error SetLock(char *Lock_FileName)
{
    FILE *  lock_fd;

    /* Open Lock File - if it does not succeed, the lock cannot be set */
    pthread_mutex_lock(&LockMutex);
    if ( (lock_fd=fopen(Lock_FileName,"w"))==NULL )
    {
        pthread_mutex_unlock(&LockMutex);
        return (MIXFKO);
    }

    fclose (lock_fd);

    pthread_mutex_unlock(&LockMutex);
    return (MIXFOK);
}


/*
 * The lock is an empty file whose file name is specified as input argument.
 * Reset the lock, returns MIXFOK in case of success, MIXFKO in any other case
 */
Error ResetLock(char *Lock_FileName)
{
    /* remove the lock */

    pthread_mutex_lock(&LockMutex);
    if ( (remove(Lock_FileName)) && (errno != ENOENT) )
    {
        pthread_mutex_unlock(&LockMutex);
        return (MIXFKO);
    }

    pthread_mutex_unlock(&LockMutex);
    return (MIXFOK);
}


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
 * Please observe that this function cannot be called after StartCounters()
 */
Error DefineScalarCtrNum(uint16_t numcounters)
{
    int i;

    if (BaseCtrActive == TRUE)      /*if counters have already been started return error */
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
 * Please observe that this function cannot be called after StartCounters()
 */
Error DefineVectorCtrNum(uint16_t numcounters)
{
    int     i;

    if (BaseCtrActive == TRUE)      /*if counters have already been started return error */
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
 * Scalar counters defined through DefineScalarCtrNum(). The second
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
 * The fourth parameter is a string that provides the counter name (up to 31
 * characters, otherwise it is truncated).
 * This function provides MIXFKO either in case of wrong parameters (e.g. outside allowed
 * ranges) or in case of counters already started, MIXFOK if everything is ok */
Error DefineScalarCtr(uint16_t ctrId, uint8_t ctrType, uint32_t ctrInitial, char* ctrName)
{
    if (BaseCtrActive == TRUE)      /*if counters have already been started return error */
        return (MIXFKO);

    if (ctrId >= numScalarCtr)
        return (MIXFKO);

    if ((ctrType != PEGCTR) && (ctrType != ROLLERCTR))
        return (MIXFKO);

    if (strlen(ctrName) >= SHORTSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 31 characters */
        strncpy(scalarCtr[ctrId].Name, ctrName, SHORTSTRINGMAXLEN - 1);
        scalarCtr[ctrId].Name[SHORTSTRINGMAXLEN - 1] = '\0';
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
 * Vector counters defined through DefineVectorCtrNum(). The second
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
 * The fifth parameter is a string that provides the counter name (up to 31
 * characters, otherwise it is truncated).The sixth parameter is another
 * string(up to 31 characters length, otherwise it is truncated) that
 * provides the name of the object associated to instances.
 * Example: referring to the number of bytes sent for different TCP
 * connections, assuming that this is a ROLLER Counter whose ID is 12
 * andthat the maximum number of TCP Connections is 512, the call
 * might be :
 *      DefineVectorCtr(12, 512, ROLLERCTR, 0, "Total Bytes Sent", "TCP Conn. ID")
 * In this case the initial value for the counter has been set to 0 (for
 * all the 512 TCP connections)
 * This function provides MIXFKO either in case of wrong parameters(e.g.outside allowed
 * ranges) or in case of counters already started, MIXFOVFL if the number of cumulative
 * instances of Vector Counters up to function call exceeds 65536, MIXFOK if everything is ok
 */
Error DefineVectorCtr(uint16_t ctrId, uint16_t ctrInst, uint8_t ctrType, uint32_t ctrInitial, char* ctrName, char* instName)
{
    uint32_t    cum;
    int         i;

    if (BaseCtrActive == TRUE)      /*if counters have already been started return error */
        return (MIXFKO);

    if ( (ctrId >= numVectorCtr) || (ctrInst < 1) )
        return (MIXFKO);

    cum = cumVectorInst + ctrInst;

    if (cum > MAXVECTORCTRINST)
        return (MIXFOVFL);

    if ((ctrType != PEGCTR) && (ctrType != ROLLERCTR))
        return (MIXFKO);

    if (strlen(ctrName) >= SHORTSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 31 characters */
        strncpy(vectorCtr[ctrId].Name, ctrName, SHORTSTRINGMAXLEN - 1);
        vectorCtr[ctrId].Name[SHORTSTRINGMAXLEN - 1] = '\0';
    }
    else
        strcpy(vectorCtr[ctrId].Name, ctrName);

    if (strlen(instName) >= SHORTSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 31 characters */
        strncpy(vectorCtr[ctrId].InstName, instName, SHORTSTRINGMAXLEN - 1);
        vectorCtr[ctrId].InstName[SHORTSTRINGMAXLEN - 1] = '\0';
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
 * DefineVectorCtrNum() and DefineVectorCtr(), otherwise MIXFKO is returned.
 * The third parameter is a string that specifies the name of the concerned
 * instance (e.g. if the counter is used to collect Total Bytes Sent for
 * several TCP connections, this name could be used to store the connection ID).
 * If NULL, the call has no effect (i.e. the name is not changed), otherwise it
 * is overwritten. Please observe that up to 15 chars are allowed, if more
 * the name is truncated.
 * Please observe that this function can be called even if counters have been
 * already started
 */
Error SetVectorCtrInstName(uint16_t ctrId, uint16_t ctrInst, char *instIdName)
{
    if ((ctrId >= numVectorCtr) || (ctrInst >= vectorCtr[ctrId].NumInstances) )
        return (MIXFKO);

    if (instIdName == NULL) /* If instIdName is not specified */
        return (MIXFOK);

    if (strlen(instIdName) >= MICROSTRINGMAXLEN)
    {   /* The name is too long, truncate it to 31 characters */
        strncpy(vectorCtr[ctrId].InstIdName[ctrInst], instIdName, MICROSTRINGMAXLEN - 1);
        vectorCtr[ctrId].InstIdName[ctrInst][MICROSTRINGMAXLEN - 1] = '\0';
    }
    else
        strcpy(vectorCtr[ctrId].InstIdName[ctrInst], instIdName);

    return (MIXFOK);
}


/*
 * This function increases a Peg Scalar Counter by one.
 * The only parameter is the Scalar Counter ID and shall be defined
 * in the interval (0,M-1), where M is the the maximum number of
 * Scalar counters defined through DefineScalarCtrNum().
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
Error IncrPegScalarCtr(uint16_t ctrId)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == FALSE)     /* if counters have not been started return error */
        return (MIXFKO);

    if ( (ctrId >= numScalarCtr) || (scalarCtr[ctrId].Type != PEGCTR) )
        return (MIXFKO);

    if (scalarCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through DefineScalarCtr() */
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
 * Vector counters defined through DefineVectorCtrNum().
 * The second parameter is a pointer to a Instance ID. If NULL
 * all the instances related to the concerned Vector counter
 * are increased by 1, otherwise only the specified Instance Id
 * is increased. It must be included in the interval (0,P-1)
 * where P is the number of instances specified in
 * DefineVectorCtr().
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
Error IncrPegVectorCtr(uint16_t ctrId, uint16_t* ctrInst)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == FALSE)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numVectorCtr) || (vectorCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (vectorCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through DefineVectorCtr() */
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
 * DefineScalarCtrNum(). The second and third parameters are
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
Error RetrievePegScalarCtr(uint16_t ctrId, uint32_t* ctrBase, uint32_t* ctrAggr)
{
    if (BaseCtrActive == FALSE)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numScalarCtr) || (scalarCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (scalarCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through DefineScalarCtr() */
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
 * DefineVectorCtrNum(), while the second parameter shall be
 * defined within (0,P-1) where P is the number of instances
 * specified in DefineVectorCtr().
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
Error RetrievePegVectorCtr(uint16_t ctrId, uint16_t ctrInst, uint32_t* ctrBase, uint32_t* ctrAggr)
{
    if (BaseCtrActive == FALSE)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numVectorCtr) || (vectorCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (vectorCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through DefineVectorCtr() */
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
 * DefineScalarCtrNum(). The second parameter represents either
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
Error UpdateRollerScalarCtr(uint16_t ctrId, short delta)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == FALSE)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numScalarCtr) || (scalarCtr[ctrId].Type != ROLLERCTR))
        return (MIXFKO);

    if (scalarCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through DefineScalarCtr() */
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
 * DefineVectorCtrNum(). The second parameter is a pointer to an
 * Instance ID. If NULL all the instances related to the concerned
 * Vector counter are updated, otherwise only the specified Instance Id
 * is affected. It must be included in the interval (0,P-1) where P
 * is the number of instances specified in DefineVectorCtr()
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
Error UpdateRollerVectorCtr(uint16_t ctrId, uint16_t *ctrInst, short delta)
{
    Error   res = MIXFOK;

    if (BaseCtrActive == FALSE)     /* if counters have not been started return error */
        return (MIXFKO);

    if ((ctrId >= numVectorCtr) || (vectorCtr[ctrId].Type != PEGCTR))
        return (MIXFKO);

    if (vectorCtr[ctrId].Name[0] == '\0')   /* the counter has not been defined through DefineVectorCtr() */
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
 *            used if counters collection has been already started through StartCounters()
 *     - MIXFOK: if everything is correct
 * Please note that calling this function is MANDATORY before starting statistic collection
 * Remember also that PEG counters are set to zero at every base interval                    */
Error DefineBaseDump(char* baseDir, char* baseTimeFormat, char* baseTimes)
{
    /* Local variables */
    LongString  Dir;
    char        minutes[3] = { '\0','\0','\0' };
    short       s;
    int         i,len;

    if (BaseCtrActive==TRUE)    /* Base Counters already started, returns MIXFKO */
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
        if (CheckFileNameValidity(baseDir) != MIXFOK)
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
 *                 used if counters collection has been already started through StartCounters()
 *     - MIXFOVFL: if more than 100 dump times have been specified
 *     - MIXFOK:   if everything is correct
 * Please note that calling this function is OPTIONAL before starting statistic collection
 * (if not called, counters will not be aggregated)
 * Remember also that aggregated PEG counters are set to zero at every aggregation interval  */
Error DefineAggrDump(char* aggrDir, char* aggrTimeFormat, char* aggrTimes)
{
    /* Local variables */
    LongString  Dir;
    char        minutes[3] = { '\0','\0','\0' },
                hours[3] = { '\0','\0','\0' };
    short       s, count = 0;
    int         i,len;

    if (BaseCtrActive==TRUE)    /* Base Counters already started, returns MIXFKO */
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
        if (CheckFileNameValidity(aggrDir) != MIXFOK)
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
 *  - MIXFNOACCESS: if any of the base files cannot be opened
 *  - MIXFKO:       if counter collection has alredy been started before through StartCounters()
 *                  or DefineBaseDump() has not been called
 *  - MIXFOK:       if everything is OK
 */
Error StartCounters(void)
{
    /* Local variables */
    Medium2String   DumpFile;
    ShortString     TimeStamp;
    struct stat     FileStat;
    int             i,j;

    if ( (BaseCtrActive==TRUE) || (BaseCtrDir[0]=='\0') )   /* Either counters already started or DefineBaseDump() not called */
        return (MIXFKO);

    /* Initialize global locks */
    pthread_mutex_init(&BaseMutex, NULL);
    pthread_mutex_init(&AggrMutex, NULL);

    /* Retrieve current time stamp according to defined format */
    RetrieveTimeDate(TimeStamp, BaseCtrTimeStampFormat);

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

    /* Now check whether aggregation has been initialized through DefineAggrDump() */
    if (AggrCtrDir[0] == '\0')
    {   /* Aggregation not initialized - Store the BaseDumpOpenDate for file rotation, set BaseCtrActive and exit */
        RetrieveTimeDate(BaseDumpOpenDate, "%d%m%Y");
        BaseCtrActive = TRUE;
        BaseNextDump = NULL;
        return (MIXFOK);
    }

    /* Aggregation has been initialized through DefineAggrDump() */
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
    RetrieveTimeDate(BaseDumpOpenDate, "%d%m%Y");
    strcpy(AggrDumpOpenDate, BaseDumpOpenDate);
    BaseCtrActive = AggrCtrActive = TRUE;
    BaseNextDump = AggrNextDump = NULL;

    return (MIXFOK);

}


/*
 * Stops collecting counters, dumps the last values collected up to that time,
 * closes all files and releases internal resources.
 * Please observe that after this call all internal counters definition are lost
 * It returns MIXFKO if counters have not been started before, MIXFOK in all other cases */
Error StopCounters(void)
{
    int     i;

    if (BaseCtrActive == FALSE)     /* Counters not started */
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
    BaseCtrActive = AggrCtrActive = FALSE;

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
 Error CheckAndDumpCtr(void)
{
    /* Local variables */
    ShortString CurrentDate, TimeStamp;
    Error       result;
    Boolean     DumpBase = FALSE,
                DumpAggr = FALSE;
    char        Time[5];
    int         i,j;

    /* Check if counters are actually running */
    if ( (BaseCtrActive==FALSE) || (BaseCtrDir[0]=='\0') )  /* Either counters not started yet or DefineBaseDump() not called */
        return (MIXFKO);

    /* First check if there is something to dump */
    RetrieveTimeDate(TimeStamp,"%d/%m/%Y,%H:%M");

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
            DumpBase = TRUE;
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
            DumpBase = TRUE;
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
                DumpAggr = TRUE;
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
                DumpAggr = TRUE;
                AggrNextDump += 5;
                if ( (int)(AggrNextDump-AggrDumpTimes)>=strlen(AggrDumpTimes))
                    AggrNextDump = AggrDumpTimes;
            }   /* if ( AggrNextDump=strstr(AggrDumpTimes,Time) ) */
        }   /* else if (AggrNextDump) */
    }   /* if (AggrCtrActive) */

    RetrieveTimeDate(CurrentDate, "%d%m%Y");

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
