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
 * FILE:        Example3.c                                                        *
 *                                                                                *
 * DESCRIPTION: This example reads either an e-mail, an URL or an IPv4            *
 *              address and validates it; parameters can either be read from      *
 *              the terminal or from a configuration file. In the latter case     *
 *              parameters in the file will be named respectively: email, url     *
 *              and ip                                                            *
 *                                                                                *
 * NOTE WELL:   THIS EXAMPLE USES THE FOLLOWING libmixf functions:                *
 *              - check_file_name_validity()  // File and File System Handling    *
 *              - check_mail_validity()       // String Handling                  *
 *              - check_ipv4_add_validity()                                       *
 *              - check_url_validity()                                            *
 *              - reset_param_list()          // Configuration Files Handling     *
 *              - add_mail_param()                                                *
 *              - add_ipv4_param()                                                *
 *              - add_url_param()                                                 *
 *              - parse_cfg_param_file()                                          *
 *              - get_mail_param_value()                                          *
 *              - get_ipv4_param_value()                                          *
 *              - get_url_param_value()                                           *
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
#define MAILNOTPROV       1
#define MAILREDEF         2
#define MAILNOTVALID      3
#define IPV4NOTPROV       4
#define IPV4REDEF         5
#define IPV4NOTVALID      6
#define URLNOTPROV        7
#define URLREDEF          8
#define URLNOTVALID       9

#define EMAILPARAM          "email"
#define IPV4PARAM           "ip"
#define URLPARAM            "url"


/*********************
 * Other definitions *
 *********************/
#define STRINGLEN           256


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
void PrintEventList (EventList *evnptr)
{
    if (evnptr == NULL)
        return;

    switch (evnptr->event)
    {
        case MAILNOTPROV:
        {
            fprintf (stderr,"Parameter "EMAILPARAM" not provisioned\n\n");
            break;
        }
        case MAILREDEF:
        {
            fprintf (stderr,"Parameter "EMAILPARAM" redefined at line %d\n\n",evnptr->line);
            break;
        }
        case MAILNOTVALID:
        {
            fprintf (stderr,"Parameter "EMAILPARAM" defined at line %d is not valid\n\n",evnptr->line);
            break;
        }
        case IPV4NOTPROV:
        {
            fprintf (stderr,"Parameter "IPV4PARAM" not provisioned\n\n");
            break;
        }
        case IPV4REDEF:
        {
            fprintf (stderr,"Parameter "IPV4PARAM" redefined at line %d\n\n",evnptr->line);
            break;
        }
        case IPV4NOTVALID:
        {
            fprintf (stderr,"Parameter "IPV4PARAM" defined at line %d is not valid\n\n",evnptr->line);
            break;
        }
        case URLNOTPROV:
        {
            fprintf (stderr,"Parameter "URLPARAM" not provisioned\n\n");
            break;
        }
        case URLREDEF:
        {
            fprintf (stderr,"Parameter "URLPARAM" redefined at line %d\n\n",evnptr->line);
            break;
        }
        case URLNOTVALID:
        {
            fprintf (stderr,"Parameter "URLPARAM" defined at line %d is not valid\n\n",evnptr->line);
            break;
        }
        default:
        {
            fprintf (stderr,"Unrecognized event after parsing configuration file\n\n");
            break;
        }
    }

    PrintEventList (evnptr->next);

    return;
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
    printf ("\t(1) - Read and validate email\n");
    printf ("\t(2) - Read and validate IPv4 address\n");
    printf ("\t(3) - Read and validate URL\n");
    printf ("\t(4) - Read above parameters from configuration file\n");
    printf ("\t(0) - Exit\n\n");

    printf ("\tEnter the selected choice: ");
    while ( (c = getchar())!= EOF )
    {
        if ( (c<'0') || (c>'4') )
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
    int         choice;
    char        String[STRINGLEN],
                File [STRINGLEN];
    Error       err;
    EventList   *listofevents=NULL;
    uint16_t    line;
    bool     prov;

    /* Initialize parameters */
    reset_param_list();
    if ( (err=add_mail_param (EMAILPARAM,false,"test@mail.com",UNDEFINED,MAILNOTPROV,MAILREDEF,MAILNOTVALID)) != MIXFOK)
    {
        PrintError (err);
        exit (-1);
    }
    if ( (err=add_ipv4_param (IPV4PARAM,false,"127.0.0.1",UNDEFINED,IPV4NOTPROV,IPV4REDEF,IPV4NOTVALID)) != MIXFOK)
    {
        PrintError (err);
        exit (-1);
    }
    if ( (err=add_url_param (URLPARAM,false,"https://127.0.0.1:8443",UNDEFINED,URLNOTPROV,URLREDEF,URLNOTVALID)) != MIXFOK)
    {
        PrintError (err);
        exit (-1);
    }

    /* Menu */
    while ( (choice=PrintMenu()) != 0)
    {
        switch (choice)
        {
            case 1:
            {   /* Read and validate email */
                system ("clear");
                printf ("Please insert a valid e-mail: ");
                scanf ("%s",String);
                if (check_mail_validity(String))
                    printf ("OK, this is a valid e-mail...");
                else
                    printf ("Mmmh, I asked you a valid email...");
                WaitEnterKey();
                break;
            }
            case 2:
            {   /* Read and validate IPv4 address */
                uint32_t ipaddr;

                system ("clear");
                printf ("Please insert a valid IPv4 address (in the form a.b.c.d): ");
                scanf ("%s",String);
                if (check_ipv4_add_validity(String, &ipaddr))
                    printf ("OK, this is a valid IPv4 address... (%#0x)",ipaddr);
                else
                    printf ("Mmmh, I asked you a valid IPv4 address...");
                WaitEnterKey();
                break;
            }
            case 3:
            {   /* Read and validate URL */
                system ("clear");
                printf ("Please insert a valid URL: ");
                scanf ("%s",String);
                if (check_url_validity(String))
                    printf ("OK, this is a valid URL...");
                else
                    printf ("Mmmh, I asked you a valid URL...");
                WaitEnterKey();
                break;
            }
            case 4:
            {   /* Read Parameters from configuration file */
                system ("clear");
                printf ("Please insert configuration file name: ");
                scanf ("%s",File);
                if ( check_file_name_validity(File)!=MIXFOK )
                    printf ("This is not a valid File Name");
                else
                {
                    if ( (err=parse_cfg_param_file(File,&line,&listofevents)) != MIXFOK)
                    {
                        PrintError(err);
                        if ( (err==MIXFFORMATERROR) || (err==MIXFPARAMUNKNOWN) )
                        fprintf (stderr,"Error occurred at line %d\n",line);
                    }
                    else
                    {
                        printf ("The configuration file was read successfully (total lines %d)\n",line);
                        PrintEventList (listofevents);
                        if (listofevents != NULL)
                            clear_event_list(&listofevents);
                        if ( (err=get_mail_param_value(EMAILPARAM,String,&prov)) != MIXFOK)
                            PrintError(err);
                        else
                            printf ("\n"EMAILPARAM": %s\n",String);
                        if ( (err=get_ipv4_param_value(IPV4PARAM,String,&prov)) != MIXFOK)
                            PrintError(err);
                        else
                            printf ("\n"IPV4PARAM": %s\n",String);
                        if ( (err=get_url_param_value(URLPARAM,String,&prov)) != MIXFOK)
                            PrintError(err);
                        else
                            printf ("\n"URLPARAM": %s\n",String);
                    }
                }
                WaitEnterKey();
                break;
            }
            default:
            {
                WaitEnterKey();
                break;
            }

        }
    }

    exit (0);
}
