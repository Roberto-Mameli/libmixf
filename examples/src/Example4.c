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
 * FILE:        Example4.c                                                        *
 *                                                                                *
 * DESCRIPTION: This example performs a loop repeated 20 times. At generic        *
 *              iteration i, it creates a random token of length i, with          *
 *              characters that belong to a charset composed by lowercase         *
 *              letters and digits (0-9)                                          *
 *              After that, it creates 20 random UUID v4 identifiers              *
 *                                                                                *
 * NOTE WELL:   THIS EXAMPLE USES THE FOLLOWING libmixf functions:                *
 *              - generate_token()  // String Handling                            *
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


/******************
 * libmixf header *
 ******************/
#include "mixf.h"

/* Main function */
int main (int argc, char *argv[], char *envp[])
{
    char token[40];
    char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    int i;

    /* First, create 20 token of increasing length (from 1 to 20 characters)*/
    for (i=0; i<20; i++)
    {
        generate_token(token,charset,i+1);
        printf ("Iterazione: %d, Token: %s\n",i,token);
    }

    
    for (i=0; i<20; i++)
    {
        generate_token(token,charset,36);
        token[14] = '4';
        token[8] = token[13] = token[18] =token[23] = '-';
        printf ("Iterazione: %d, UUID v4: %s\n",i,token);
    }
}