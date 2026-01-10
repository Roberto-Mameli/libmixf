/**********************************************************************************
 * -----------------------------------------                                      *
 * C/C++ Mixed Functions Library (libmixf)                                        *
 * -----------------------------------------                                      *
 * Copyright 2019-2026 Roberto Mameli                                             *
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
 * FILE:        Example1.c                                                        *
 *                                                                                *
 * DESCRIPTION: This example takes one argument, that is the name of              *
 *              a configuration file. This file shall have the following          *
 *              format:                                                           *
 *                  $STRINGTOCONVERT = <value of the string without apices>       *
 *                  $LICENSEFILE = <file name to save encrypted string>           *
 *              i.e. it shall be composed of two mandatory parameters             *
 *              (respectively a literal parameter constituted by a clear text     *
 *              string and a filename parameter). The program parses the          *
 *              configuration file and, if everything is OK, it provides          *
 *              two different choices. The first allows to encrypt                *
 *              $STRINGTOCONVERT writing the result into $LICENSEFILE.            *
 *              The second allows to read $LICENSEFILE, decrypt it and            *
 *              check that the content is equal to $STRINGTOCONVERT.              *
 *                                                                                *
 * NOTE WELL:   THIS EXAMPLE USES THE FOLLOWING libmixf FUNCTIONS:                *
 *              - check_file_name_validity()  // File and File System Handling    *
 *              - reset_param_list()          // Configuration Files handling     *
 *              - init_param_list()                                               *
 *              - add_literal_param()                                             *
 *              - add_filename_param()                                            *
 *              - parse_cfg_param_file()                                          *
 *              - get_list_param_value()                                          *
 *              - get_fname_param_value()                                         *
 *              - clear_event_list()                                              *
 *              - create_license()            // License Handling                 *
 *              - check_license()                                                 *
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


/******************
 * libmixf header *
 ******************/
#include "mixf.h"


/***************************************************
 * Events associated to configuration file parsing *
 ***************************************************/
#define STRINGNOTPROV       1
#define STRINGREDEF         2
#define FILENOTPROV         3
#define FILEREDEF           4
#define FILENOTVALID        5


/*********************
 * Other definitions *
 *********************/
#define STRINGLEN           512


/***********************
 * Print Command Usage *
 ***********************/
void PrintUsage (char *command)
{
    printf ("Usage: %s <configuration file>\n\n",command);
    printf ("The configuration file shall contain two mandatory parameters:\n\n");
    printf ("   $STRINGTOCONVERT - clear text string that shall be converted\n");
    printf ("                      and written into license file;\n");
    printf ("   $LICENSEFILE     - name of the license file produced.\n\n");
    printf ("If the configuration file is parsed correctly, three choices\n");
    printf ("are available: the first allows to encrypt $STRINGTOCONVERT\n");
    printf ("writing the result into $LICENSEFILE; the second checks the\n");
    printf ("content of $LICENSEFILE and verifies that it coincides with\n");
    printf ("$STRINGTOCONVERT. Finally, the third forces configuration\n");
    printf ("file reload.\n\n");

    return;
}


/****************************************
 * Print possible error codes on stderr *
 ****************************************/
void PrintError (Error error)
{
    switch (error)
    {
        case MIXFKO:
        {
            fprintf (stderr,"Error - the libmixf call was not successful\n\n");
            break;
        }
        case MIXFNOACCESS:
        {
            fprintf (stderr,"Error - Unable to access file or parameter\n\n");
            break;
        }
        case MIXFFORMATERROR:
        {
            fprintf (stderr,"Error - File wrongly formatted\n\n");
            break;
        }
        case MIXFPARAMUNKNOWN:
        {
            fprintf (stderr,"Error - Parameter not recognized\n\n");
            break;
        }
        case MIXFWRONGDEF:
        {
            fprintf (stderr,"Error - Invalid parameter definition\n\n");
            break;
        }
        case MIXFOVFL:
        {
            fprintf (stderr,"Error - Maximum number of parameters exceeded\n\n");
            break;
        }
        default:
        {
            fprintf (stderr,"Unrecognized error in the libmixf library\n\n");
            break;
        }
    }
    return;
}


/*************************
 * Print possible events *
 *************************/
Error PrintEventList (EventList *evnptr)
{
    if (evnptr == NULL)
        return (MIXFOK);

    if (PrintEventList (evnptr->next) != MIXFOK)
        return (MIXFKO);

    switch (evnptr->event)
    {
        case STRINGNOTPROV:
        {
            fprintf (stderr,"Parameter $STRINGTOCONVERT not provisioned\n\n");
            return (MIXFKO);
        }
        case STRINGREDEF:
        {
            fprintf (stderr,"Parameter $STRINGTOCONVERT redefined at line %d\n\n",evnptr->line);
            break;
        }
        case FILENOTPROV:
        {
            fprintf (stderr,"Parameter $LICENSEFILE not provisioned\n\n");
            return (MIXFKO);
        }
        case FILEREDEF:
        {
            fprintf (stderr,"Parameter $LICENSEFILE redefined at line %d\n\n",evnptr->line);
            break;
        }
        case FILENOTVALID:
        {
            fprintf (stderr,"Parameter $LICENSEFILE defined at line %d is not a valid file name\n\n",evnptr->line);
            return (MIXFKO);
        }
        default:
        {
            fprintf (stderr,"Unrecognized event after parsing configuration file\n\n");
            break;
        }
    }
    return (MIXFOK);
}


/**************************************************************************
 * Print available options on the screen and get the choice from the user *
 **************************************************************************/
int PrintMenu (void)
{
    int     c;

    /* Clear the screen and print the menu of available choices */
    system ("clear");
    printf ("*********************\n");
    printf ("* Available choices *\n");
    printf ("*********************\n\n");
    printf ("\tMenu\n\t----\n\n");
    printf ("\t(1) - Encrypt and create license\n");
    printf ("\t(2) - Check license\n");
    printf ("\t(3) - Force configuration file reload\n");
    printf ("\t(0) - Exit\n\n");

    printf ("\tEnter the selected choice: ");
    while ( (c = getchar())!= EOF )
    {
        if ( (c<'0') || (c>'3') )
            continue;
        break;
    }
    return (c-(int)'0');

}


/***************************************
 * Wait until the ENTER key is pressed *
 ***************************************/
void WaitEnterKey (void)
{
    int c;

    printf ("\n\tPress the ENTER key to continue...\n");
    while ((c = getchar()) != '\n' && c != EOF);
    while (getchar() != '\n');
    return;
}

/***********************************************
 * This is the main() function. It parses      *
 * command line arguments and acts accordingly *
 ***********************************************/
int main(int argc, char *argv[], char *envp[])
{

    /* Local variables */
    Error       err=MIXFOK;
    EventList   *listofevents=NULL;
    uint16_t    line;
    int         choice;
    long        hostid;
    char        String[STRINGLEN],
                hostname[STRINGLEN],
                hostidstr[STRINGLEN];
    char        LicenseFileName[MAXFILENAMELEN],
                CfgFileName[MAXFILENAMELEN];
    FILE        *LicenseFd;
    bool     prov;


    /* Retrieve host name and host id */
    hostid = gethostid();
    sprintf (hostidstr,"0x%08x",hostid);
    gethostname(hostname,STRINGLEN);

    /* Initialize parameters */
    reset_param_list();
    if ( (err=init_param_list (2)) != MIXFOK)
    {
        PrintError (err);
        exit (-1);
    }
    if ( (err=add_literal_param ("$STRINGTOCONVERT",true,"",STRINGNOTPROV,UNDEFINED,STRINGREDEF,UNDEFINED)) != MIXFOK)
    {
        PrintError (err);
        exit (-1);
    }
    if ( (err=add_filename_param ("$LICENSEFILE",true,"",FILENOTPROV,UNDEFINED,FILEREDEF,FILENOTVALID)) != MIXFOK)
    {
        PrintError (err);
        exit (-1);
    }

    /* Check command line arguments */
    if ( (argc != 2) || ((check_file_name_validity(argv[1]))!=MIXFOK) )
    {
        PrintUsage(argv[0]);
        exit (-1);
    }

    /* Parse Configuration File */
    strcpy (CfgFileName,argv[1]);
    if ( (err=parse_cfg_param_file(CfgFileName,&line,&listofevents)) != MIXFOK)
    {
        PrintError(err);
        if ( (err==MIXFFORMATERROR) || (err==MIXFPARAMUNKNOWN) )
            fprintf (stderr,"Error occurred at line %d\n",line);
        exit (-1);
    }
    printf ("The configuration file was read successfully (total lines %d)\n",line);

    /* if there is at least an event, print it - the routine returns MIXFKO
     * if one of the events is FATAL (STRINGNOTPROV, FILENOTPROV, FILENOTVALID) */
    if (PrintEventList(listofevents)!=MIXFOK)
    {
        clear_event_list(&listofevents);
        exit (-1);
    }

    /* Clears the list of events */
    if (listofevents != NULL)
    {
        clear_event_list(&listofevents);
        WaitEnterKey();
    }

    /* Get the value of parameters */
    if ( (err=get_list_param_value("$STRINGTOCONVERT",String,&prov)) != MIXFOK )
    {
        PrintError(err);
        exit (-1);
    }
    if ( (err=get_fname_param_value("$LICENSEFILE",LicenseFileName,&prov)) != MIXFOK)
    {
        PrintError(err);
        exit (-1);
    }

    /* Prints out parameters read from the file and hostname and hostid */
    printf ("\n\nThe following parameters have been read:\n");
    printf ("$STRINGTOCONVERT = %s\n",String);
    printf ("$LICENSEFILE     = %s\n",LicenseFileName);
    printf ("\nThis system is characterized by the following hostname and hostid:\n");
    printf ("hostname         = %s\n",hostname);
    printf ("hostid           = %s\n",hostidstr);
    WaitEnterKey();

    /* Menu */
    while ( (choice=PrintMenu()) != 0)
    {
        switch (choice)
        {
            case 1:
            {   /* Encrypt and create license */
                if (create_license (String,hostname,hostidstr))
                {
                    fprintf (stderr, "Found problem when calling create_license()\n");
                    WaitEnterKey();
                    break;
                }
                if ( (LicenseFd=fopen(LicenseFileName,"w")) == NULL)
                {
                    fprintf (stderr, "Cannot write into license file name\n");
                    WaitEnterKey();
                    break;
                }
                fprintf (LicenseFd,"%s",String);
                fclose (LicenseFd);
                WaitEnterKey();
                break;
            }
            case 2:
            {   /* Check license */
                char        Decrypted[STRINGLEN];
                if (check_license(LicenseFileName,Decrypted) != MIXFOK)
                {
                    fprintf (stderr, "Cannot read the license file\n");
                    WaitEnterKey();
                    break;
                }
                get_list_param_value("$STRINGTOCONVERT",String,&prov);
                printf ("Decrypted string is %s\n",Decrypted);
                if (strcmp (Decrypted,String))
                    printf ("License check failed\n");
                else
                    printf ("License check succeeded\n");
                WaitEnterKey();
                break;
            }
            case 3:
            {   /* Force reload */
                if ( (err=parse_cfg_param_file(CfgFileName,&line,&listofevents)) != MIXFOK)
                {
                    PrintError(err);
                    if ( (err==MIXFFORMATERROR) || (err==MIXFPARAMUNKNOWN) )
                        fprintf (stderr,"Error occurred at line %d\n",line);
                    exit (-1);
                }
                printf ("The configuration file was read successfully (total lines %d)\n",line);

                /* if there is at least an event, print it - the routine returns MIXFKO
                 * if one of the events is FATAL (STRINGNOTPROV, FILENOTPROV, FILENOTVALID) */
                if (PrintEventList(listofevents)!=MIXFOK)
                {
                    clear_event_list(&listofevents);
                    exit (-1);
                }

                /* Clears the list of events */
                if (listofevents != NULL)
                {
                    clear_event_list(&listofevents);
                    WaitEnterKey();
                }

                /* Get the value of parameters */
                if ( (err=get_list_param_value("$STRINGTOCONVERT",String,&prov)) != MIXFOK )
                {
                    PrintError(err);
                    exit (-1);
                }
                if ( (err=get_fname_param_value("$LICENSEFILE",LicenseFileName,&prov)) != MIXFOK)
                {
                    PrintError(err);
                    exit (-1);
                }

                /* Prints out parameters read from the file and hostname and hostid */
                printf ("\n\nThe following parameters have been read:\n");
                printf ("$STRINGTOCONVERT = %s\n",String);
                printf ("$LICENSEFILE     = %s\n",LicenseFileName);
                printf ("\nThis system is characterized by the following hostname and hostid:\n");
                printf ("hostname         = %s\n",hostname);
                printf ("hostid           = %s\n",hostidstr);
                WaitEnterKey();
                break;
            }
            default:
            {
                break;
            }

        }
    }

    reset_param_list();
    exit (0);
}
