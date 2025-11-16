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
 * FILE:        mixfConf.c                                                  *
 * VERSION:     3.0.0                                                       *
 * AUTHOR(S):   Roberto Mameli                                              *
 * PRODUCT:     Library libmixf - general purpose library                   *
 * DESCRIPTION: This source file contains the implementation of a subset of *
 *              the functions provided by libmixf library, specifically:    *
 *              - Configuration Files Handling                              *
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
/* Private variables for Configuration File Handling functions */
static Param            *ParamVect=NULL;                      /* Pointer to a dynamically allocated array of Configuration Parameters  */
static uint8_t          numparams=0,                          /* Actual Number of Configuration Parameters */
                        maxparams=DEFPARAMARRAYSIZE;          /* Max Number of Configuration Parameters */
static bool             ParsedFlag=false;                     /* Set to true when the Configuration File is actually parsed */


/*******************************
 *                             *
 *      Static Functions       *
 * (only visible in this file) *
 *                             *
 *******************************/
/*
 * This function is used within parse_cfg_param_file(). It
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


/***********************************
 *                                 *
 *        Visible Functions        *
 * (can be used outside this file  *
 *  and are part of the library)   *
 *                                 *
 ***********************************/
/* ----------------------------
 * Configuration Files Handling
 * ----------------------------*/
/*
 * Reset the internal list of allowed parameters
 * (call this before initializing the list)
 */
void reset_param_list (void)
{
    numparams=0;
    maxparams=DEFPARAMARRAYSIZE;
    if (ParamVect)
        free (ParamVect);
    ParsedFlag=false;
    return ;
}


/* This function is used to define the maximum number of parameters managed by the
 * Configuration Files Handling functions. Allowed values are comprised in the range
 * between 1 and 255. This function is not mandatory, i.e. it can also be skipped;
 * in this case, the default value for the maximum number of parameters is set to 8.
 * This function provides MIXFOK if execution is successful, MIXFKO in case it is
 * called twice without invoking reset_param_list() first, MIXFOVFL if the parameter
 * is outside the allowed range 1-255 or the system runs out of memory
 */
Error init_param_list (int maxp)
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
 * flag; it must be set to true if the parameter is mandatory, false if it is optional in the
 * configuration file. Third, fourth and fifth parameter are 3 integers that represent respectively
 * the minimum, maximum and default value for the parameter (please observe that default is
 * actually meaningful only if the Mandatory flag is false). The last four parameters are event
 * codes that are associated respectively to the following events: (1) Event corresponding to a
 * Mandatory parameter non provisioned; (2) Event corresponding to an Optional parameter not
 * provisioned (default value used instead); (3) Event corresponding to a parameter that is redefined
 * (at least twice); (4) Event corresponding to a parameter value out of range or malformed (e.g. not
 * a number). Use UNDEFINED for those events that are not meaningful (e.g. event (1) does not make
 * sense if Mandatory flag is false).
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
 * MIXFWRONGDEF if the condition min <= default <= max is not satisfied and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error add_numerical_param (char *name, bool mand, int min, int max, int def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
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
    ParamVect[numparams].Provisioned = false;
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
 * configuration file. The description is exactly the same as add_numerical_param(), with the
 * only difference that there is no minimum or maximum value, but only a default (which is
 * defined in the third argument). There is no special check on the possible values of
 * literal parameters, therefore the MIXFWRONGDEF error code cannot be returned by this
 * function
 */
Error add_literal_param (char *name, bool mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
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
    ParamVect[numparams].Provisioned = false;
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
 * configuration file. The description is exactly the same as add_literal_param(); however,
 * there is a minor difference between literal and filename parameters, since the
 * filename parameter can only get values that are valid filenames (see also
 * check_file_name_validity() above. This routine can provide back all error codes as the
 * previous ones, including MIXFWRONGDEF (which is given when the default value in the
 * third argument is not a valid file name)
 */
Error add_filename_param (char *name, bool mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (check_file_name_validity(def) != MIXFOK) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = false;
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
 * it must be set to true if the parameter is mandatory, false if it is optional in the
 * configuration file. Third, fourth and fifth parameter are 3 chars that represent respectively
 * the minimum, maximum and default value for the parameter (please observe that default is
 * actually meaningful only if the Mandatory flag is false). The last four parameters are event
 * codes that are associated respectively to the following events: (1) Event corresponding to a
 * Mandatory parameter non provisioned; (2) Event corresponding to an Optional parameter not
 * provisioned (default value used instead); (3) Event corresponding to a parameter that is redefined
 * (at least twice); (4) Event corresponding to a parameter value out of range or malformed (e.g. not
 * a char). Use UNDEFINED for those events that are not meaningful (e.g. event (1) does not make
 * sense if Mandatory flag is false).
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
 * MIXFWRONGDEF if the condition min <= default <= max is not satisfied and finally
 * MIXFKO for any other error (e.g. parameter name empty) */
Error add_char_param (char *name, bool mand, char min, char max, char def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
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
    ParamVect[numparams].Provisioned = false;
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
 * configuration file. The description is exactly the same as add_literal_param(); however,
 * there is a minor difference between literal and mail parameters, since the
 * mail parameter can only get values that are valid e-mail addresses.
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
 * MIXFWRONGDEF if the default value in the third argument is not a valid e-mail and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error add_mail_param (char *name, bool mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (check_mail_validity(def)==false) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = false;
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
 * configuration file. The description is exactly the same as add_literal_param(); however,
 * there is a minor difference between literal and ipv4 parameters, since the
 * ipv4 parameter can only get values that are valid IPv4 addresses.
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
 * MIXFWRONGDEF if the default value in the third argument is not a valid IPv4 address and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error add_ipv4_param (char *name, bool mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    uint32_t    tmp;

    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (check_ipv4_add_validity(def,&tmp)==false) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = false;
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
 * configuration file. The description is exactly the same as add_literal_param(); however,
 * there is a minor difference between literal and URL parameters, since the
 * URL parameter can only get values that are valid URLs.
 * This function provides MIXFOK if parameter is successfully added to the list, MIXFOVFL if
 * the maximum number of allowed parameters is exceeded (max value defined in init_param_list),
 * MIXFWRONGDEF if the default value in the third argument is not a valid URL and finally
 * MIXFKO for any other error (e.g. parameter name empty)
 */
Error add_url_param (char *name, bool mand, char *def, EventCode evn1, EventCode evn2, EventCode evn3, EventCode evn4)
{
    /* Preliminary Checks */
    if (numparams >= maxparams)
        return (MIXFOVFL);
    if ( (name == NULL) || (name[0]=='\0') )
        return (MIXFKO);
    if ( (def != NULL) && (def[0] != '\0') && (check_url_validity(def)==false) )
        return (MIXFWRONGDEF);

    /* If the vector has not been allocated yet, allocate it before going on */
    if  (ParamVect==NULL)
        if ( (ParamVect = calloc (maxparams, sizeof(Param)))==NULL )
            return (MIXFOVFL);

    /* MIXFOK, now add the parameter */
    strcpy (ParamVect[numparams].Name, name);
    ParamVect[numparams].Mandatory = mand;
    ParamVect[numparams].Provisioned = false;
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
 * configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is true if the parameter was
 * actually provisioned in the configuration file, while is false if it wasn't
 */
Error get_num_param_value (char *param, int *value, bool *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==false)
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
 * configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is true if the parameter was
 * actually provisioned in the configuration file, while is false if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error get_list_param_value (char *param, char *value, bool *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==false)
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
 * configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is true if the parameter was
 * actually provisioned in the configuration file, while is false if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error get_fname_param_value (char *param, char *value, bool *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==false)
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
 * configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is true if the parameter was
 * actually provisioned in the configuration file, while is false if it wasn't */
Error get_char_param_value (char *param, char *value, bool *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==false)
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
 * configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is true if the parameter was
 * actually provisioned in the configuration file, while is false if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error get_mail_param_value (char *param, char *value, bool *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==false)
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
 * configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is true if the parameter was
 * actually provisioned in the configuration file, while is false if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error get_ipv4_param_value (char *param, char *value, bool *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==false)
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
 * configuration file (i.e. parse_cfg_param_file() was not invoked), MIXFPARAMUNKNOWN if the
 * parameter is not known. The third argument is a flag that is true if the parameter was
 * actually provisioned in the configuration file, while is false if it wasn't.
 * The string in which the parameter value is copied (second parameter) is not allocated
 * by the routine, rather shall be allocated by the caller.
 */
Error get_url_param_value (char *param, char *value, bool *prov)
{
    int     i;
    Error   res = MIXFPARAMUNKNOWN;

    if (ParsedFlag==false)
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
 * was actually defined in the file (true) or if the
 * default value was provisioned instead (false). The latter
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
Error parse_cfg_param_file (char *CfgFileName, uint16_t *totalline, EventList **events)
{
    /* Local variables */
    ExtendedString stringbuffer,
                   inputstringbuffer;
    char           *p, *q;
    uint8_t        i;
    int            val;
    bool           found;
    EventList      *evnlst = NULL;
    FILE           *CfgFile_fd;

    /* Set initial values */
    *totalline = 0;
    *events = NULL;

    /* Assign default values to all parameters and set Provisioned flag to false */
    for (i=0; i<numparams; i+=1)
    {
        ParamVect[i].Provisioned = false;
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
    while (fgets (inputstringbuffer,EXTENDEDSTRINGMAXLEN,CfgFile_fd))
    {
        /* Valid string into inputstringbuffer */
        copy_remove_blanks (stringbuffer, inputstringbuffer);
        (*totalline) += 1;
        if ( (stringbuffer[0] == '\0') || (stringbuffer[0] == '#') )
            continue ;  /* This is either an empty line or a comment - skip this line */
        /* the line is neither a comment nor an empty line - therefore it must begin with a valid param */
        found = false;
        i=0;
        p = stringbuffer;
        while ( (found == false) && (i<numparams) )
        {
            if ( (strncmp(p,ParamVect[i].Name,strlen(ParamVect[i].Name)) == 0) && (*(p+strlen(ParamVect[i].Name))=='=') )
            {   /* The parameter defined in this line is the one in i-th position in ParamVect array */
                found = true;

                /* Now use inputstringbuffer to extract the value after '=' in order to keep spaces */
                for (p = inputstringbuffer; (*p!='\0')&&(*p!='#')&&(*p!='=');p+=1);
                if (*p != '=')
                {
                    clear_event_list (&evnlst);
                    return (MIXFFORMATERROR);
                }
                for (p += 1 ; (*p==' ')||(*p=='\t') ; p += 1);  /* Skip all spaces and tabs after '=' */
                if ( (*p == '\0') || (*p == '#') )
                {   /* Either the line is terminated after '=' or a comment begins */
                    clear_event_list (&evnlst);
                    return (MIXFFORMATERROR);
                }
                for (q=p; (*q!='\0')&&(*q!='#')&&(*q!='\n'); q+=1); /* stop at EOL or at # */
                *q = '\0';                              /* p and q now points to the beginning and to the end of the parameter */

                /* Extract the parameter depending on its type */
                switch (ParamVect[i].Type)
                {
                    case numerical:
                    {   /* This is a numerical parameter - convert it and check against limits */
                        remove_blanks (p);
                        val = atoi(p);
                        if ( (only_digits(p)==false) || (val<ParamVect[i].Values.Num.Min) || (val>ParamVect[i].Values.Num.Max) )
                        {   /* Malformed or Out of Range - Add an event in the event list and restore default */
                            ParamVect[i].Values.Num.Val = ParamVect[i].Values.Num.Def;
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            ParamVect[i].Values.Num.Val = val;
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = true;
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
                                ParamVect[i].Provisioned = true;
                            }
                        }
                        break;
                    }
                    case literal:
                    {   /* This is a literal parameter - no specific checks are done */
                        strcpy (ParamVect[i].Values.Lit.Val,p);
                        if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                            AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                        ParamVect[i].Provisioned = true;
                        break;
                    }
                    case filename:
                    {   /* This is a filename parameter */
                        if (check_file_name_validity(p) != MIXFOK)
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.Lit.Val, ParamVect[i].Values.Lit.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.Lit.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = true;
                        }
                        break;
                    }
                    case email:
                    {   /* This is an email parameter */
                        if ( check_mail_validity(p) == false)
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.Lit.Val, ParamVect[i].Values.Lit.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.Lit.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = true;
                        }
                        break;
                    }
                    case ipv4:
                    {   /* This is an ipv4 parameter */
                        uint32_t tmp;
                        if ( check_ipv4_add_validity(p,&tmp) == false)
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.ipv4.Val, ParamVect[i].Values.ipv4.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.ipv4.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = true;
                        }
                        break;
                    }
                   case url:
                    {   /* This is an url parameter */
                        if ( check_url_validity(p) == false )
                        {   /* Malformed - Add an event in the event list and restore default */
                            strcpy (ParamVect[i].Values.Url.Val, ParamVect[i].Values.Url.Def);
                            AddEventInList (ParamVect[i].MalfOrOOR, *totalline, &evnlst);
                        }
                        else
                        {
                            strcpy (ParamVect[i].Values.Url.Val,p);
                            if (ParamVect[i].Provisioned)   /* Already defined - Add an event in the event list */
                                AddEventInList (ParamVect[i].Redefined, *totalline, &evnlst);
                            ParamVect[i].Provisioned = true;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                continue;   /* this jumps to the next iteration of while ( (found == false) && (i<numparams) ) */
            }   /* if ( strncmp(stringbuffer,ParamVect[i].Name, ... */
            i += 1;     /* next parameter in ParamVect[i] */
        }   /* while ( (found == false) && .. */
        if (found == false)
        {   /* There is an unrecognized parameter - Clear event list and exit */
            clear_event_list (&evnlst);
            return (MIXFPARAMUNKNOWN);
        }
    }   /* while (fgets (inputstringbuffer)... */

    /* We reach this point when fgets() returns null, i.e. the Task Definition file is finished */
    fclose (CfgFile_fd);

    /* Now perform an additional check on missing mandatory and optional parameters */
    for (i=0; i<numparams; i+=1)
    {   /* Check if there are parameters not provisioned and add the corresponding */
        /* event to the event list (line number is 0 since not applicable in this case) */
        if (ParamVect[i].Provisioned == false)
        {
            if (ParamVect[i].Mandatory)
                AddEventInList (ParamVect[i].MandNotProv, 0, &evnlst);
            else
                AddEventInList (ParamVect[i].OptNotProv, 0, &evnlst);
        }
    }

    /* Everything seems MIXFOK - set the ParsedFlag to true and exit without errors */
    *events = evnlst;
    ParsedFlag = true;
    return (MIXFOK);

}


/*
 * This function releases all the memory allocated for the event list
 * by parse_cfg_param_file().
 *
 */
void clear_event_list(EventList **ptr)
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
