/**********************************************************************************
 * -----------------------------------------                                      *
 * C/C++ Mixed Functions Library (libmixf)                                        *
 * -----------------------------------------                                      *
 * Copyright 2019-2021 Roberto Mameli                                             *
 *                                                                                *
 * Licensed under the Apache License, Version 2.0 (the "License");                *
 * you may not use this file except in compliance with the License.               *
 * You may obtain a copy of the License at                                        *
 *                                                                                *
 *     http://www.apache.org/licenses/LICENSE-2.0                                 *
 *                                                                                *
 * Unless required by applicable law or agreed to in writing, software            *
 * distributed under the License is distributed on an "AS IS" BASIS,              *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.       *
 * See the License for the specific language governing permissions and            *
 * limitations under the License.                                                 *
 * ------------------------------------------------------------------------       *
 *                                                                                *
 * FILE:        Example2.c                                                        *
 *                                                                                *
 * DESCRIPTION: This source code provides an example of usage of some             *
 *              libmixf functions related to lock handling and counters           *
 *              handling.                                                         *
 *                                                                                *
 * NOTE WELL:   THIS EXAMPLE USES THE FOLLOWING libmixf FUNCTIONS:                *
 *              - CheckLockPresent()    // Lock handling functions                *
 *              - SetLock()                                                       *
 *              - ResetLock()                                                     *
 *              - DefineScalarCtrNum()  // Counters handling                      *
 *              - DefineScalarCtr()                                               *
 *              - DefineVectorCtrNum()                                            *
 *              - DefineVectorCtr()                                               *
 *              - SetVectorCtrInstName()                                          *
 *              - DefineBaseDump()                                                *
 *              - DefineAggrDump()                                                *
 *              - StartCounters()                                                 *
 *              - StopCounters()                                                  *
 *              - IncrPegScalarCtr()                                              *
 *              - IncrPegVectorCtr()                                              *
 *              - CheckAndDumpCtr()                                               *
 *                                                                                *
 *  ----------------------------------------------------------------------------  *
 *  DISCLAIMER                                                                    *
 *  ----------------------------------------------------------------------------  *
 *  Example files are provided only as an example of development of a working     *
 *  software program using libmixf libraries. The source code provided is not     *
 *  written as an example of a released, production level application, it is      *
 *  intended only to demonstrate usage of the API functions used herein.          *
 *  The Author provides the source code examples "AS IS" without warranty of      *
 *  any kind, either expressed or implied, including, but not limited to the      *
 *  implied warranties of merchantability and fitness for a particular purpose.   *
 *  The entire risk as to the quality and performance of the source code          *
 *  examples is with you. Should any part of the source code examples prove       *
 *  defective you (and not the Author) assume the entire cost of all necessary    *
 *  servicing, repair or correction. In no event shall the Author be liable for   *
 *  damages of any kind, including direct, indirect, incidental, consequential,   *
 *  special, exemplary or punitive, even if it has been advised of the            *
 *  possibility of such damage.                                                   *
 *  The Author does not warrant that the contents of the source code examples,    *
 *  whether will meet your requirements or that the source code examples are      *
 *  error free.                                                                   *
 **********************************************************************************/


/**********************
 * Linux system files *
 **********************/
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>


/******************
 * libmixf header *
 ******************/
#include "mixf.h"


/***************
 * Definitions *
 ***************/
#define STRINGLEN           32

/* Scalar Counters */
#define LOWERCASECTR        0
#define UPPERCASECTR        1
#define DIGITCTR            2
#define OTHERCHARCTR        3
#define PLUSMINUSROLLERCTR  4
#define NUMSCALARCTR        5

/* Vector Counters */
#define LOWERVECTORID       0
#define LOWERVECTORINST     26
#define UPPERVECTORID       1
#define UPPERVECTORINST     26
#define DIGITVECTORID       2
#define DIGITVECTORINST     10
#define NUMVECTORCTR        3

#define WAKEUPTIMER     60                                      /* used to wake up every minute to check if a dump is needed */

/* Base Dump Params */
#define BASEDUMPDIR     "../stats/base"
#define BASEDUMPTIMES   "00,05,10,15,20,25,30,35,40,45,50,55"   /* every 5 minutes */

/* Aggr Dump Params */
#define AGGRDUMPDIR     "../stats/aggr"
#define AGGRDUMPTIMES   "0000,0030,0100,0130,0200,0230,0300,0330,0400,0430,0500,0530,0600,0630,0700,0730,0800,0830,0900,0930,1000,1030,1100,1130,1200,1230,1300,1330,1400,1430,1500,1530,1600,1630,1700,1730,1800,1830,1900,1930,2000,2030,2100,2130,2200,2230,2300,2330"   /* every 30 minutes */

/* Lock File Name */
#define LOCKFILENAME    "./.lock.lck"

/* Character corresponding to Carriage Return */
#define CR              '\n'


/********************
 * Global variables *
 ********************/
Boolean Exit=FALSE;

char    ScalarCounters[NUMSCALARCTR][STRINGLEN] =
        {
            /* LOWERCASECTR */          "TotalNumberLowerCaseLetters",
            /* UPPERCASECTR */          "TotalNumberUpperCaseLetters",
            /* DIGITCTR  */             "TotalNumberDigits",
            /* OTHERCHARCTR */          "TotalNumberOtherChars",
            /* PLUSMINUSROLLERCTR */    "Plus-MinusHits"
        };

char    VectorCounters[NUMVECTORCTR][STRINGLEN] =
        {
            /* LOWERVECTORID */         "NumberLowerCaseLetters",
            /* UPPERVECTORID */         "NumberUpperCaseLetters",
            /* DIGITVECTORID */         "NumberDigits"
        };

char    VectorCounterInst[NUMVECTORCTR][STRINGLEN] =
        {
            /* LOWERVECORID  */         "LowerCaseLetter",
            /* UPPERVECTORID */         "UpperCaseLetter",
            /* DIGITVECTORID */         "Digit"
        };

int VectorCounterInstNo[NUMVECTORCTR]={LOWERVECTORINST, UPPERVECTORINST, DIGITVECTORINST};


/**********************
 * Internal functions *
 **********************/
static Error InitCounters(void)
{
    /* Local variables */
    Error   res;
    int     i;
    char    Inst[2];

    /* First, initialize scalar counters */
    if ( (res=DefineScalarCtrNum(NUMSCALARCTR)) != MIXFOK )
        return (res);
    for (i=0;i<NUMSCALARCTR;i++)
        if ( (res=DefineScalarCtr(i,PEGCTR,0,ScalarCounters[i])) != MIXFOK )
            return (res);
    /* The last scalar counter is defined as a roller counter with initial value set to 0 */
    if ( (res=DefineScalarCtr(PLUSMINUSROLLERCTR,ROLLERCTR,0,ScalarCounters[PLUSMINUSROLLERCTR])) != MIXFOK )
        return (res);

    /* Second, initialize vector counters */
    if ( (res=DefineVectorCtrNum(NUMVECTORCTR)) != MIXFOK )
        return (res);
    for (i=0;i<NUMVECTORCTR;i++)
        if ( (res=DefineVectorCtr(i,VectorCounterInstNo[i],PEGCTR,0,VectorCounters[i],VectorCounterInst[i])) != MIXFOK )
            return (res);

    /* Now, give a name to all instances of all vector counters */
    Inst[1]='\0';
    for (i=0; i < LOWERVECTORINST; i++)
    {
        Inst[0] = 'a' + (char) i;
        if ( (res=SetVectorCtrInstName(LOWERVECTORID,i,Inst) )!= MIXFOK )
            return (res);
    }
    for (i=0; i < UPPERVECTORINST; i++)
    {
        Inst[0] = 'A' + (char) i;
        if ( (res=SetVectorCtrInstName(UPPERVECTORID,i,Inst) )!= MIXFOK )
            return (res);
    }
    for (i=0; i < DIGITVECTORINST; i++)
    {
        Inst[0] = '0' + (char) i;
        if ( (res=SetVectorCtrInstName(DIGITVECTORID,i,Inst) )!= MIXFOK )
            return (res);
    }

    /* Define Base and Aggregate Dump parameters */
    if ( (res=DefineBaseDump(BASEDUMPDIR,NULL,BASEDUMPTIMES)) != MIXFOK)
        return (res);

    res=DefineAggrDump(AGGRDUMPDIR,NULL,AGGRDUMPTIMES);

    return (res);
}


/* Signal Handler function */
static void signalHandler(int sigType)
{   /* Signal Handler */
    switch (sigType)
    {
        case SIGALRM:
        {   /* SIGALRM - This is used just to wake up every minute from blocking
               getchar in the main loop - Reschedule SIGALRM */
            alarm (WAKEUPTIMER);
            break;
        }
        case SIGINT:
        case SIGTERM:
        {   /* Set global variable EXIT to TRUE in order to force clean exit */
            Exit = TRUE;
            break;
        }
        default:
        {
            break;
        }
    }

    return;
}


/***********************************************
 * This is the main() function.                *
 ***********************************************/
int main(int argc, char *argv[], char *envp[])
{
    /* Local Variables */
    char        c;
    Error       err;
    uint16_t    inst;
    struct sigaction sa;

    /* Install SIGALRM Handler */
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction (SIGALRM,&sa,NULL);
    sigaction (SIGINT,&sa,NULL);
    sigaction (SIGTERM,&sa,NULL);

    /* Check if a lock is present */
    if (CheckLockPresent(LOCKFILENAME))
    {   /* A lock is present, therefore another instance of the same executable is already running */
        printf ("Lock detected... probably another instance is still running\n");
        printf ("Exiting....\n");
        exit (-1);
    }

    /* There is no instance already running - Set a lock */
    if (SetLock(LOCKFILENAME) != MIXFOK)
    {   /* Not able to set a new lock */
        printf ("Not able to set a new lock\n");
        printf ("Exiting....\n");
        exit (-1);
    }

    /* Lock has been correctly set */
    /* Now initialize all counters */
    if ( (InitCounters()) != MIXFOK )
    {   /* Not able to set a init counters */
        printf ("Not able to init counters\n");
        printf ("Exiting....\n");
        ResetLock(LOCKFILENAME);
        exit (-1);
    }

    /* Start Counters */
    if ( (StartCounters()) != MIXFOK )
    {   /* Not able to start counters */
        printf ("Not able to start counters\n");
        printf ("Exiting....\n");
        ResetLock(LOCKFILENAME);
        exit (-1);
    }

    alarm (WAKEUPTIMER);            /* used to wake up every minute in order to check if counters shall be dumped */

    /* Infinite Loop */
    system ("clear");
    while (Exit==FALSE)
    {
        printf ("Please press any key (SPACE to exit)\n");
        if ((c = getchar())!= EOF)
        {
            if (c==' ')
            {
                Exit = TRUE;
                continue;
            }
            if ( (c>='a') && (c<='z') )
            {   /* User pressed a key corresponding to a lowercase character */
                if ( (err=IncrPegScalarCtr(LOWERCASECTR)) != MIXFOK)
                    printf ("Error in %s:%d\n",__FILE__,__LINE__);
                inst = (int) (c-'a');
                if ( (err=IncrPegVectorCtr(LOWERVECTORID,&inst)) != MIXFOK)
                    printf ("Error in %s:%d\n",__FILE__,__LINE__);
                continue;
            }
            if ( (c>='A') && (c<='Z') )
            {   /* User pressed a key corresponding to an uppercase character */
                if ( (err=IncrPegScalarCtr(UPPERCASECTR)) != MIXFOK)
                    printf ("Error in %s:%d\n",__FILE__,__LINE__);
                inst = (int) (c-'A');
                if ( (err=IncrPegVectorCtr(UPPERVECTORID,&inst)) != MIXFOK)
                    printf ("Error in %s:%d\n",__FILE__,__LINE__);
                continue;
            }
            if ( (c>='0') && (c<='9') )
            {   /* User pressed a key corresponding to a digit */
                if ( (err=IncrPegScalarCtr(DIGITCTR)) != MIXFOK)
                    printf ("Error in %s:%d\n",__FILE__,__LINE__);
                inst = (int) (c-'0');
                if ( (err=IncrPegVectorCtr(DIGITVECTORID,&inst)) != MIXFOK)
                    printf ("Error in %s:%d\n",__FILE__,__LINE__);
                continue;
            }
            if (c == '+')
            {   /* User pressed +, increase Roller counter PLUSMINUSROLLERCTR */
                if ( (err=UpdateRollerScalarCtr(PLUSMINUSROLLERCTR,1)) == MIXFOVFL)
                    printf ("Overflow in updating %s (%s:%d)\n",ScalarCounters[PLUSMINUSROLLERCTR],__FILE__,__LINE__);
                continue;
            }
            if (c == '-')
            {   /* User pressed -, decrease Roller counter PLUSMINUSROLLERCTR */
                if ( (err=UpdateRollerScalarCtr(PLUSMINUSROLLERCTR,-1)) == MIXFOVFL)
                    printf ("Underflow in updating %s (%s:%d)\n",ScalarCounters[PLUSMINUSROLLERCTR],__FILE__,__LINE__);
                continue;
            }
            if (c != CR)
            {   /* Any other char, excluding CR, is counted as other */
                if ( (err=IncrPegScalarCtr(OTHERCHARCTR)) != MIXFOK)
                    printf ("Error in %s:%d\n",__FILE__,__LINE__);
                continue;
            }
        }   /* if ((c = getchar())!= EOF)) */

        if (CheckAndDumpCtr() != MIXFOK)
        {   /* Not able to dump counters */
            printf ("Not able to dump counters\n");
            printf ("Exiting....\n");
            ResetLock(LOCKFILENAME);
            exit (-1);
        }

    }   /* Infinite Loop */

    /* SPACE was pressed - close counters and reset lock */
    if (StopCounters() != MIXFOK)
    {   /* Not able to dump counters */
        printf ("Not able to stop counters\n");
        printf ("Exiting....\n");
        ResetLock(LOCKFILENAME);
        exit (-1);
    }

    if (ResetLock(LOCKFILENAME) != MIXFOK)
    {   /* Not able to reset lock */
        printf ("Not able to reset lock\n");
        printf ("Exiting....\n");
        exit (-1);
    }

}