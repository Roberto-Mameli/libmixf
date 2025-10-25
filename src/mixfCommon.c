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
 * FILE:        mixfCommon.c                                                *
 * VERSION:     3.0.0                                                       *
 * AUTHOR(S):   Roberto Mameli                                              *
 * PRODUCT:     Library libmixf - general purpose library                   *
 * DESCRIPTION: This source file contains the implementation of a subset of *
 *              the functions provided by libmixf library, specifically:    *
 *              - File and File System Handling                             *
 *              - Time and Date Handling                                    *
 *              - String Handling                                           *
 *              - License Handling                                          *
 *              - Lock Handling                                             *
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
static pthread_mutex_t  LockMutex=PTHREAD_MUTEX_INITIALIZER;  /* Mutex used to handle cuncurrent access to lock file  */


/*******************************
 *                             *
 *      Static Functions       *
 * (only visible in this file) *
 *                             *
 *******************************/


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
Error check_file_name_validity (char *filename)
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
Error retrieve_path (char *Path)
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
Error read_files_input_dir (char *inputdir, DirContent ** list_of_files)
{
    /* Local variables */
    DIR             *currPath;
    struct dirent   *des;
    struct stat     buf;
    char            name[MAXFILENAMELEN],
                    fullName[MAXFILENAMELEN];

    DirContent   *first = NULL,
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
                if ( (p=malloc (sizeof(DirContent)) ) == NULL)
                {

                    clear_input_file_list (&first);
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
 * This function releases memory allocated by read_files_input_dir()
 * for the Input File List.
 */
void clear_input_file_list(DirContent ** ptr)
{
    /* Local variables */
    DirContent *p, *q;

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
void retrieve_time_date (char * TimeDateString, char * format)
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
time_t get_time_stamp (char * LogLine, char * format)
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
char * filter_and_extract (char * Line, const char * Filter, const char * StartWith)
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
 * The function returns false if the strings match,
 * true otherwise.
 * The third parameter is a boolean flag that is true if
 * string1 contains a wildcard, false otherwise.
 * The fourth parameter is a boolean flag that is true if
 * the two strings match exactly, false in case the match is
 * due to the presence of wildcards in the second string.
 */
bool strcmp_wildcards  (char * string1,
                        char * string2,
                        bool * WildcardInStr1,
                        bool * ExactMatch)
{
    char     *p1, *p2, *p3, *p4,
              p3char;

    if (strpbrk(string1,"*?!") != NULL)
        (*WildcardInStr1) = true;
    else
        (*WildcardInStr1) = false;

    *ExactMatch = false;

    /* If strings are equal char-by-char, return
     * directly false (MATCH) and set ExactMatch flag to true */
    if ( strcmp(string1,string2) == 0)
    {
        *ExactMatch = true;
        return (false);
    }


    /* If strings are not equal char-by-char, but first string
     * contains a wildcard, return directly true (NO MATCH)*/
    if ( (*WildcardInStr1)==true )
        return (true);

    /* If the first string (i.e. the one
     * without wildcards) is shorter, then
     * they cannot be equal
     * The only exception is when the two strings
     * are of the type "1234" and "1234*" or
     * "" and "*"
     * This is the reason for strlen(string2)-1
     * below */
    if (strlen(string1) < (strlen(string2)-1) )
        return (true);

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
                    return (false);
                for (p3=p2;(*p3!='\0')&&(*p3!='*')&&(*p3!='?')&&(*p3!='!') ; p3++);
                p3char = *p3;
                *p3 = '\0';
                if ( (p4=strstr(p1,p2)) == NULL)
                    return (true);
                p1 = p4 + (p3-p2);
                *p3 = p3char;
                return (strcmp_wildcards(p1,p3,WildcardInStr1,ExactMatch));
                break;
            }
            case '?':
            case '!':
            {
                if (*p1 == '\0')
                    return (true);
                p1++;
                p2++;
                break;
            }
            default:
            {
                if (*p1 != *p2)
                    return (true);
                p1++;
                p2++;
                break;
            }
        }
    }

    if (*p1 == '\0')
        return (false);
    else
        return (true);

}


/*
 * This function takes a NULL terminated string and
 * modifies it by removing all blanks, tabs and new lines
 * The behavior is undefined if the string
 * is not NULL terminated.
 * Please consider that the input string is modified
 * by this function.
 */
void remove_blanks (char * buf)
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
 * remove_blanks() since it preserves the original
 * string (i.e. it does not modify the input string).
 * The behaviour is undefined if the string
 * is not NULL terminated
 */
void copy_remove_blanks (char * dst, char * src)
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
 * This function provides back true if the input string consists only
 * of digits, false otherwise
 */
bool only_digits (char * inputstr)
{
    /* Local pointer at the beginning of the input string */
    char  * p;

    if ( (inputstr == NULL) || (*inputstr == '\0') )
        return (false);

    for (p=inputstr; *p != '\0'; p++)
        if ( (*p < '0') || (*p > '9') )
            return (false);

    return (true);

}


/*
 * This function takes a string as input parameter and provides true
 * if it represents an e-mail syntactically correct, false otherwise.
 * It is assumed that the input string contains up to 128 characters,
 * otherwise it is considered not valid (there is no specific rule
 * that states so, but it seems a reasonable assumption in almost all
 * practical situations).
 */
bool check_mail_validity (char * email)
{
    /* Local Variables */
    char      *p, *q;
    int        len;
    MediumString emailcopy;

    /* Check preliminarly string length */
    if ( ((len=strlen(email)) > MEDIUMSTRINGMAXLEN) || (len<1) )
        return (false);     /* the input string is empty or longer than 128 characters */

    /* Take a local copy of the email, to avoid affecting input params */
    /* The copy is limited to MEDIUMSTRINGMAXLEN = 128 characters    */
    strncpy (emailcopy,email,MEDIUMSTRINGMAXLEN);
    emailcopy[MEDIUMSTRINGMAXLEN] = '\0';

    /* Determines the position of @ */
    if ( (p=strchr(emailcopy,'@')) == NULL)
        return (false);     /* No @ in the input string -> Not a valid e-mail */

    /* Found (at least) a @, separate substrings at the left and at the right */
    /* Let's consider the following example format: name@domain.tld (tld = top level domain) */

    /* First, work on the left substring, i.e. name */
    q = emailcopy;
    *p = '\0';
    len = strlen(q);    /* length of the left substring, since @ has been replaced by null character */

    if ( (len==0) || (q[0]=='.') || (q[len-1]=='.') )
        return (false);     /* the left substring is empty, or it begins or ends with a dot -> Not a valid e-mail */

    for (q=emailcopy; *q!='\0'; q++)
    {
        if ( strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.",*q) == NULL)
            return (false); /* in the left substring there is an invalid character */
        if ( (*q=='.') && ( (*(q-1)=='.') || (*(q+1)=='.') ) )
            return (false); /* in the left substring there are two consecutive dots */
    }

    /* Now work on the right substring, i.e. domain.tld */
    q = ++p;
    len = strlen(q);    /* length of the right substring */

    if ( (len==0) || (q[0]=='.') || (q[len-1]=='.') )
        return (false);     /* the right substring is empty, or it begins or ends with a dot -> Not a valid e-mail */

    if ( (strchr(p,'.')) == NULL)
        return (false);     /* No . in the right string -> Not a valid e-mail */

    for (q=p; *q!='\0'; q++)
    {
        if ( strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.",*q) == NULL)
            return (false); /* in the right substring there is an invalid character */
        if ( (*q=='.') && ( (*(q-1)=='.') || (*(q+1)=='.') ) )
            return (false); /* in the right substring there are two consecutive dots */
    }

    /* In the following, we check that top level domain has length between 2 and 3 */
    /* Not sure this check shall be kept. For example, in check_url_validity() it has been removed */
    /* Comment the following 4 lines if the check shall be removed  */
//    for (q=p+len-1; *q!='.'; q--);  /* go back from the end to the last dot in the right part */
//    q++;
//    if ( ((len=strlen(q)) <2 ) || (len>3) )
//        return (false);     /* top level domain shall be 2 or 3 charaters */
    /* At last I decided to remove it */

    return (true);

}


/*
 * This function provides true if the input parameter is a valid IPv4 Address (i.e. a
 * string formatted as a.b.c.d, where a, b, c, d are integers between 0 and 255), false
 * otherwise. If true the second parameter provides the IP addr converted into a uint32_t
 */
bool check_ipv4_add_validity (char *ipAddrStr, uint32_t *ipAddr)
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
        /* Extract first three numbers by looking for dots - return false if any error*/
        if ( (q=strstr(p,"."))==NULL)
            return (false);
        *q='\0';
        if ( !only_digits(p) )
            return (false);
        n[i] = atoi (p);
        if ( (n[i]<0) || (n[i]>255) )
            return (false);
        i++;
        p = q+1;
    }   /* while (i<2) */

    /* Now extract last number and return false if any error */
    if ( !only_digits(p) )
        return (false);
    n[3] = atoi (p);
    if ( (n[3]<0) || (n[3]>255) )
        return (false);

    *ipAddr = (n[0]<<24) | (n[1]<<16) | (n[2]<<8) | n[3];

    return (true);
}


/*
 * This function takes a string as input parameter and provides true
 * if it represents an URL syntactically correct, false otherwise.
 * It is assumed that the input string contains up to 256 characters,
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
bool check_url_validity (char * url)
{
    /* Local Variables */
    char       *p, *q, *r, *s;
    int        len;
    uint32_t   tmp;
    LongString urlcopy;

    /* Check preliminarly string length */
    if ( ((len=strlen(url)) > LONGSTRINGMAXLEN) || (len<1) )
        return (false);     /* the input string is empty or longer than 256 characters */

    /* Take a local copy of the url, to avoid affecting input param  */
    /* The copy is limited to LONGSTRINGMAXLEN = 256 characters */
    strncpy (urlcopy,url,LONGSTRINGMAXLEN);
    urlcopy[LONGSTRINGMAXLEN] = '\0';

    /* Determines the position of :// substring, if present */
    if ( (p=strstr(urlcopy,"://")) != NULL )
    {
        /* the protocol is present, check it */
        *p = '\0';
        p += 3;
        if ( strcasecmp(urlcopy,"mailto")==0 )
            return (check_mail_validity(p));

        if ( strcasecmp(urlcopy,"http")  &&
             strcasecmp(urlcopy,"https") &&
             strcasecmp(urlcopy,"ftp")   &&
             strcasecmp(urlcopy,"ftps")  &&
             strcasecmp(urlcopy,"sftp")  &&
             strcasecmp(urlcopy,"gopher")&&
             strcasecmp(urlcopy,"news")  &&
             strcasecmp(urlcopy,"telnet")&&
             strcasecmp(urlcopy,"aim") )
            return (false);
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
    if (check_ipv4_add_validity(q,&tmp)==false)
    {   /* host (pointed by q) is not a valid IPv4 address, check it has the correct syntax */
        len = strlen(q);    /* length of the host */
        if ( (len==0) || (q[0]=='.') || (q[len-1]=='.') )
            return (false);     /* The host is empty, or it begins or ends with a dot -> Not valid */
        if ( (strchr(q,'.')) == NULL)
            return (false);     /* No . in the host -> Not valid */
        for (s=q; *s!='\0'; s++)
        {
            if ( strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.",*s) == NULL)
                return (false); /* in the host there is an invalid character */
            if ( (*s=='.') && ( (*(s-1)=='.') || (*(s+1)=='.') ) )
                return (false); /* in the host there are two consecutive dots */
        }

        /* Differently from check_mail_validity(), there is no check on top level domain length */
//      for (s=q+len-1; *s!='.'; s--);  /* go back from the end to the last dot in the host */
//      s++;
//      if ( ((len=strlen(s)) <2 ) || (len>3) )
//          return (false);     /* top level domain shall be 2 or 3 charaters */

    }   /* if (check_ipv4_add_validity(q,&tmp)==false) */

    /* If we are here, host is present and correct */
    /* If present, check the port (i.e. if p is not NULL) */
    if ( (p!= NULL) && (*p!='\0') && ((only_digits(p)==false) || (atoi(p)<0) || (atoi(p)>65535)) )
        return (false);

    /* If r is null or points to an empty string, then the URL is terminated and is valid */
    if ( (r==NULL) || (*r=='\0') )
        return (true);

    /* r points to the beginning of [</path>][?querystring][#fragment] section */
    if ( ((p=strchr(r,'?')) != NULL) || ((p=strchr(r,'#')) != NULL) )
    {   /* [?querystring][#fragment] is not empty and is pointed by q */
        *p = '\0';
        q = p+1;
    }
    else
        q = NULL;

    if ( (check_file_name_validity(r)!=MIXFOK) || (*r=='.') || (*r=='/') )
        return (false);

    if (q==NULL)    /* No [?querystring][#fragment] --> URL is valid */
        return (true);

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
        return (false);
    if ( (s!=NULL) && ((strlen(s)<1) || (strpbrk(s,"|!\"£$()?\'^\\[]*+@#;:,<>") != NULL)) )
        return (false);

    return (true);
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
Error generate_token (char *token, char *charset, int length)
{
    /* Local Variables */
    char *p, *q;
    int   i, len;
    time_t   currenttime;
    static bool RandSeedInitialized = false;

    /* Preliminary checks */
    if ( (token==NULL) || (charset==NULL) || (charset[0]=='\0') || (length<=0) )
        return (MIXFKO);
    len = strlen (charset);

    /* Initialize Random Seed only once */
    if (RandSeedInitialized == false)
    {   /* First Invocation - Initialize Random Seed */
        /* Evaluate current time to be used as a seed for random numbers generator */
        time (&currenttime);
        srand ( (unsigned int) currenttime);
        RandSeedInitialized = true;
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
Error check_license (char *LicenseFileName, char *DecryptedString)
{
    /* Local variables */
    FILE            *License_fd;
    ShortString     hostname;
    MediumString    Key;
    ExtendedString  LicenseContent;
    unsigned int    keylen, index, keyindex;
    long            hostid;

    /* Check that the license file exists and is not empty */
    if ( (License_fd=fopen(LicenseFileName,"r")) == NULL)
        return (MIXFNOACCESS);

    if (fgets(LicenseContent,EXTENDEDSTRINGMAXLEN,License_fd) == NULL)
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
Error create_license (char *String, char *HostName, char *HostId)
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
 * If the lock is not present, this function returns false, otherwise returns true.
 */
bool check_lock_present(char *Lock_FileName)
{
    FILE *  lock_fd;

    /* Open Lock File - if it does not succeed, the lock is not present */

    pthread_mutex_lock(&LockMutex);
    if ( (lock_fd=fopen(Lock_FileName,"r")) == NULL)
    {
        pthread_mutex_unlock(&LockMutex);
        return (false);
    }

    /* The lock is present */
    fclose (lock_fd);

    pthread_mutex_unlock(&LockMutex);
    return (true);
}


/*
 * The lock is an empty file whose file name is specified as input argument.
 * Set the lock, returns MIXFOK in case of success, MIXFKO in any other case
 */
Error set_lock(char *Lock_FileName)
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
Error reset_lock(char *Lock_FileName)
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
