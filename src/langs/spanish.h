/***************************************************************/
/*                                                             */
/*  SPANISH.H                                                  */
/*                                                             */
/*  Support for the Spanish language.                          */
/*                                                             */
/*  Author: Rafa Couto <rafacouto@biogate.com>                 */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*                                                             */
/***************************************************************/

#define L_LANGNAME "Spanish"

/* Nombres de los di'as de la semana */
#define L_SUNDAY "Domingo"
#define L_MONDAY "Lunes"
#define L_TUESDAY "Martes"
#if ISOLATIN1
#define L_WEDNESDAY "Mi\351rcoles"
#else
#define L_WEDNESDAY "Miercoles"
#endif

#define L_THURSDAY "Jueves"
#define L_FRIDAY "Viernes"
#if ISOLATIN1
#define L_SATURDAY "S\341bado"
#else
#define L_SATURDAY "Sabado"
#endif

/* Nombres de los meses */
#define L_JAN "Enero"
#define L_FEB "Febrero"
#define L_MAR "Marzo"
#define L_APR "Abril"
#define L_MAY "Mayo"
#define L_JUN "Junio"
#define L_JUL "Julio"
#define L_AUG "Agosto"
#define L_SEP "Septiembre"
#define L_OCT "Octubre"
#define L_NOV "Noviembre"
#define L_DEC "Diciembre"

/* Hoy y man~ana */
#define L_TODAY "hoy"
#if ISOLATIN1
#define L_TOMORROW "ma\361ana"
#else
#define L_TOMORROW "manana"
#endif

/* El titular habitual */
#define L_BANNER "Agenda para el %w, %d%s %m, %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Hace y desde hoy */
#define L_AGO "hace"
#define L_FROMNOW "desde hoy"

/* "dentro de %d di'as" */
#if ISOLATIN1
#define L_INXDAYS "dentro de %d d\355as"
#define L_ON "el d\355a"
#else
#define L_INXDAYS "dentro de %d di'as"
#define L_ON "el di'a"
#endif

/* "el di'a..." */

/* plurales */
#define L_PLURAL "s"

/* Minutos, horas, a las, etc */
#define L_NOW "ahora"
#define L_AT "a las"
#define L_MINUTE "minuto"
#define L_HOUR "hora"
#define L_IS "es"
#define L_WAS "fue"
#define L_AND "y"
#define L_HPLU "s"  
#define L_MPLU "s"

#endif /* L_IN_DOSUBST */

