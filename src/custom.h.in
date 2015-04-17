/***************************************************************/
/*                                                             */
/*  CUSTOM.H.IN                                                */
/*                                                             */
/*  Contains various configuration parameters for Remind       */
/*  which you can customize.                                   */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

/*---------------------------------------------------------------------*/
/* LAT_DEG, LAT_MIN and LAT_SEC: Latitude of your location             */
/* LON_DEG, LON_MIN and LON_SEC: Longitude of your location            */
/* LOCATION: A string identifying your location.                       */
/* All latitude and longitude numbers should be positive for the       */
/* northern and western hemisphere.  If you live in the southern       */
/* hemisphere, ALL latitude values should be negative.  If you live    */
/* in the eastern hemisphere, ALL longitude values should be negative. */
/*                                                                     */
/* The default values are initially set to Ottawa, Ontario, Canada.    */
/*---------------------------------------------------------------------*/
#define LAT_DEG 45
#define LAT_MIN 24
#define LAT_SEC  0
#define LON_DEG 75
#define LON_MIN 39
#define LON_SEC  0
#define LOCATION "Ottawa"

/*---------------------------------------------------------------------*/
/* DEFAULT_PAGE:  The default page size to use for Rem2PS.             */
/* The Letter version is appropriate for North America; the A4 version */
/* is appropriate for Europe.                                          */
/*---------------------------------------------------------------------*/
#define DEFAULT_PAGE {"Letter", 612, 792}
/* #define DEFAULT_PAGE {"A4", 595, 842} */
  
/*---------------------------------------------------------------------*/
/* DATESEP:  The default date separator.  Standard usage is '-';       */
/* others may prefer '/'.                                              */
/*---------------------------------------------------------------------*/
#define DATESEP '-'
/* #define DATESEP '/' */

/*---------------------------------------------------------------------*/
/* TIMESEP:  The default time separator.  North American usage is ':'; */
/* others may prefer '.'.                                              */
/*---------------------------------------------------------------------*/
#define TIMESEP ':'
/* #define TIMESEP '.' */

/*---------------------------------------------------------------------*/
/* ISOLATIN1: define it to 1 if you use the                            */
/* ISO 8859-1 character set instead of ASCII.                          */
/*---------------------------------------------------------------------*/
#define ISOLATIN1 1

/*---------------------------------------------------------------------*/
/* ISOLATIN2: define it to 1 if you use the                            */
/* ISO 8859-2 character set instead of ASCII.                          */
/* NOT ALL LANGUAGE MODULES SUPPORT THIS.  Note that at most one of    */
/* ISOLATIN1, ISOLATIN2, IBMEXTENDED and IBM852 should be 1; if more   */
/* then one are defined as 1, the results are unspecified.             */
/*---------------------------------------------------------------------*/
#define ISOLATIN2 0

/*---------------------------------------------------------------------*/
/* IBMEXTENDED: define as 1 if your system uses the  IBM extended      */
/* character set.                                                      */
/* NOT ALL LANGUAGE MODULES SUPPORT THIS.  Note that at most one of    */
/* ISOLATIN1, ISOLATIN2, IBMEXTENDED and IBM852 should be 1; if more   */
/* then one are defined as 1, the results are unspecified.             */
/*---------------------------------------------------------------------*/
#define IBMEXTENDED 0

/*---------------------------------------------------------------------*/
/* IBM852: define as 1 if your system uses the IBM CPI-852 extended    */
/* character set.                                                      */
/* NOT ALL LANGUAGE MODULES SUPPORT THIS.  Note that at most one of    */
/* ISOLATIN1, ISOLATIN2, IBMEXTENDED and IBM852 should be 1; if more   */
/* then one are defined as 1, the results are unspecified.             */
/*---------------------------------------------------------------------*/
#define IBM852 0

/*---------------------------------------------------------------------*/
/* WANT_U_OPTION: Comment out the next define to permanently disable   */
/* the -u option.                                                      */
/*---------------------------------------------------------------------*/
#define WANT_U_OPTION 1

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* You most likely do NOT have to tweak anything after this!          */
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

/*---------------------------------------------------------------------*/
/* WANT_SHELL_ESCAPING:  Define this if you want special shell         */
/* characters to be escaped with a backslash for the -k option.        */
/*---------------------------------------------------------------------*/
#if defined(UNIX)
#define WANT_SHELL_ESCAPING 1
#endif

/*---------------------------------------------------------------------*/
/* Some implementations have a broken 'putc' and 'putchar'.            */
/*---------------------------------------------------------------------*/
#ifdef __SASC_60
#define BROKEN_PUTC
#endif

/*---------------------------------------------------------------------*/
/* BASE: The base year for date calculation.  NOTE!  January 1 of the  */
/*       base year MUST be a Monday, else Remind will not work!        */
/*       IMPORTANT NOTE:  The Hebrew date routines depend on BASE      */
/*       being set to 1990.  If you change it, you'll have to add the  */
/*       number of days between 1 Jan <NEWBASE> and 1 Jan 1990 to the  */
/*       manifest constant CORRECTION in hbcal.c.  Also, the year      */
/*       folding mechanism in main.c depends on BASE<2001.             */
/*---------------------------------------------------------------------*/
#define BASE 1990

/*---------------------------------------------------------------------*/
/* YR_RANGE: The range of years allowed.  With 32-bit signed integers, */
/* the DATETIME type can store 2^31 minutes or about 4074 years.       */
/*---------------------------------------------------------------------*/
#define YR_RANGE 4000

/*---------------------------------------------------------------------*/
/* VAR_NAME_LEN: The maximum length of variable names.  Don't make it  */
/*               any less than 12.                                     */
/*---------------------------------------------------------------------*/
#define VAR_NAME_LEN 16

/*---------------------------------------------------------------------*/
/* MAX_PRT_LEN: The maximum number of characters to print when         */
/* displaying a string value for debugging purposes.                   */
/*---------------------------------------------------------------------*/
#define MAX_PRT_LEN 40

/*---------------------------------------------------------------------*/
/* MAX_STR_LEN: If non-zero, Remind will limit the maximum length      */
/* of string values to avoid eating up all of memory...                */
/*---------------------------------------------------------------------*/
#define MAX_STR_LEN 65535

/*---------------------------------------------------------------------*/
/* OP_STACK_SIZE: The size of the operator stack for expr. parsing     */
/*---------------------------------------------------------------------*/
#define OP_STACK_SIZE 100

/*---------------------------------------------------------------------*/
/* VAL_STACK_SIZE: The size of the operand stack for expr. parsing     */
/*---------------------------------------------------------------------*/
#define VAL_STACK_SIZE 500

/*---------------------------------------------------------------------*/
/* INCLUDE_NEST: How many nested INCLUDES do we handle?                */
/*---------------------------------------------------------------------*/
#define INCLUDE_NEST 9

/*---------------------------------------------------------------------*/
/* IF_NEST: How many nested IFs do we handle?  Maximum is the number   */
/* of bits in an int, divided by two.  Beware!                         */
/*---------------------------------------------------------------------*/
#define IF_NEST (4*sizeof(unsigned int))

/*---------------------------------------------------------------------*/
/* How many attempts to resolve a weird date spec?                     */
/*---------------------------------------------------------------------*/
#define TRIG_ATTEMPTS 500

/*---------------------------------------------------------------------*/
/* How many global omits of the form YYYY MM DD do we handle?          */
/*---------------------------------------------------------------------*/
#define MAX_FULL_OMITS 500

/*---------------------------------------------------------------------*/
/* How many global omits of the form MM DD do we handle?               */
/*---------------------------------------------------------------------*/
#define MAX_PARTIAL_OMITS 366

/*---------------------------------------------------------------------*/
/* A newline - some systems need "\n\r"                                */
/*---------------------------------------------------------------------*/
#define NL "\n"

/*---------------------------------------------------------------------*/
/* Minimum number of linefeeds in each calendar "box"                  */
/*---------------------------------------------------------------------*/
#define CAL_LINES 5

/*---------------------------------------------------------------------*/
/* Don't change the next definitions                                   */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/* TAG_LEN: The maximum length of tags.  Don't change it               */
/*---------------------------------------------------------------------*/
#define TAG_LEN 48

#define PASSTHRU_LEN 32

#define PSBEGIN "# rem2ps begin"
#define PSEND   "# rem2ps end"

#ifdef BROKEN_PUTC
#define Putc SafePutc
#define PutChar SafePutChar
#else
#define Putc putc
#define PutChar putchar
#endif

#if defined(HAVE_MBSTOWCS) && defined(HAVE_WCTYPE_H)
#define REM_USE_WCHAR 1
#else
#undef REM_USE_WCHAR
#endif
