/***************************************************************/
/*                                                             */
/*  FRENCH.H                                                   */
/*                                                             */
/*  Support for the French language.                           */
/*                                                             */
/*  Contributed by Laurent Duperval.                           */
/*                                                             */
/*  This file is part of REMIND.                               */
/*                                                             */
/*  REMIND is Copyright (C) 1992-2022 by Dianne Skoll          */
/*  This file is Copyright (C) 1993 by Laurent Duperval and    */
/*  Dianne Skoll.                                              */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "French"

/* Day names */
#define L_SUNDAY "dimanche"
#define L_MONDAY "lundi"
#define L_TUESDAY "mardi"
#define L_WEDNESDAY "mercredi"
#define L_THURSDAY "jeudi"
#define L_FRIDAY "vendredi"
#define L_SATURDAY "samedi"

/* Month names */
#define L_JAN "janvier"
#define L_FEB "février"
#define L_MAR "mars"
#define L_APR "avril"
#define L_MAY "mai"
#define L_JUN "juin"
#define L_JUL "juillet"
#define L_AUG "août"
#define L_SEP "septembre"
#define L_OCT "octobre"
#define L_NOV "novembre"
#define L_DEC "décembre"

/* Today and tomorrow */
#define L_TODAY "aujourd'hui"
#define L_TOMORROW "demain"

/* The default banner */
#define L_BANNER "Rappels pour %w, %d%s %m, %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "il y a"
#define L_FROMNOW "dans"

/* "in %d days' time" */
#define L_INXDAYS "dans %d jours"

/* "on" as in "on date..." */
#define L_ON "le"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
#define L_PLURAL "s"

/* Minutes, hours, at, etc */
#define L_NOW "maintenant"
#define L_AT "à"
#define L_MINUTE "minute"
#define L_HOUR "heure"
#define L_IS "est"
#define L_WAS "était"
#define L_AND "et"
/* What to add to make "hour" plural */
#define L_HPLU "s"
/* What to add to make "minute" plural */
#define L_MPLU "s"

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */

#define L_ORDINAL_OVERRIDE \
switch(d) { \
   case 1: plu = "er"; break; \
   default: plu = ""; break; \
}

#define L_1_OVER \
if (tdiff == 0) \
sprintf(s, L_NOW); \
else if (tdiff < 0) { \
  if (mdiff == 0) \
      sprintf(s, "il y a %d heure%s", hdiff, hplu); \
  else if (hdiff == 0) \
      sprintf(s, "il y a %d minute%s", mdiff, mplu); \
  else \
      sprintf(s, "il y a %d heure%s et %d minute%s", hdiff, hplu, mdiff, mplu); \
} else { \
   if (mdiff == 0) \
      sprintf(s, "dans %d heure%s", hdiff, hplu); \
   else if (hdiff == 0) \
      sprintf(s, "dans %d minute%s", mdiff, mplu); \
   else \
      sprintf(s, "dans %d heure%s et %d minute%s", hdiff, hplu, mdiff, mplu); \
}

#define L_J_OVER if (altmode == '*') { sprintf(s, "%s, %d%s %s, %d", DayName[jul%7], d, plu, MonthName[m], y); } else { sprintf(s, "%s %s, %d%s %s, %d", L_ON, DayName[jul%7], d, plu, MonthName[m], y); }

#define L_K_OVER if (altmode == '*') { sprintf(s, "%s, %d%s %s", DayName[jul%7], d, plu, MonthName[m]); } else { sprintf(s, "%s %s, %d%s %s", L_ON, DayName[jul%7], d, plu, MonthName[m]); }

#endif /* L_IN_DOSUBST */

/* The next ones are used only when MK_GLOBALS is set */
#ifdef MK_GLOBALS
#define L_ERR_OVERRIDE 1
EXTERN char *ErrMsg[] =
{
    "Ok",
    "']' manquant",
    "Apostrophe manquant",
    "Expression trop complexe - trop d'opérateurs",
    "Expression trop complexe - trop d'opérandes",
    "')' manquante",
    "Fonction non-définie",
    "Caractère illégal",
    "Opérateur binaire attendu",
    "Manque de mémoire",
    "Nombre mal formé",
    "Erreur interne - 'underflow' de la pile d'opérateurs",
    "Erreur interne - 'underflow' de la pile de variables",
    "Impossible de convertir",
    "Types non-équivalents",
    "Débordement de date",
    "Erreur interne - erreur de pile",
    "Division par zéro",
    "Variable non définie",
    "Fin de ligne non attendue",
    "Fin de fichier non attendue",
    "Erreur I/O",
    "Ligne trop longue",
    "Erreur interne",
    "Mauvaise date spécifiée",
    "Pas assez d'arguments",
    "Trop d'arguments",
    "Heure mal formée",
    "Nombre trop élevé",
    "Nombre trop bas",
    "Impossible d'ouvrir le fichier",
    "Trop d'INCLUDE imbriqués",
    "Erreur d'analyse",
    "Impossible de calculer le déclenchement",
    "Trop de IF imbriqués",
    "ELSE sans IF correspondant",
    "ENDIF sans IF correspondant",
    "Impossible d'omettre (OMIT) tous les jours",
    "Elément(s) étranger(s) sur la ligne",
    "POP-OMIT-CONTEXT sans PUSH-OMIT-CONTEXT correspondant",
    "RUN déactivé",
    "Erreur de domaine",
    "Identificateur invalide",
    "Appel récursif détecté",
    "",
    "Impossible de modifier une variable système",
    "Fonction de la librairie C ne peut représenter la date/l'heure",
    "Tentative de redéfinition d'une fonction intrinsèque",
    "Impossible d'imbriquer une définition de fonction dans une expression",
    "Pour utiliser le facteur de répétition la date doit être spécifiée au complet",
    "Année spécifiée deux fois",
    "Mois spécifié deux fois",
    "Jour spécifié deux fois",
    "Elément inconnu",
    "Mois et jour doivent être spécifiés dans commande OMIT",
    "Trop de OMITs partiels",
    "Trop de OMITs complets",
    "Attention: PUSH-OMIT-CONTEXT sans POP-OMIT-CONTEXT correspondant",
    "Erreur à la lecture du fichier",
    "Fin de ligne attendue",
    "Date hébreuse invalide",
    "IIF demande nombre d'arguments impair",
    "Attention: ENDIF manquant",
    "Virgule attendue",
    "Jour de la semaine spécifié deux fois",
    "Utiliser un seul parmi BEFORE, AFTER ou SKIP",
    "Impossible d'imbriquer MSG, MSF, RUN, etc. dans une expression",
    "Valeur de répétition spécifiée deux fois",
    "Valeur delta spécifiée deux fois",
    "Valeur de retour spécifiée deux fois",
    "Mot-clé ONCE utilisé deux fois. (Hah.)",
    "Heure attendue après AT",
    "Mot-clé THROUGH/UNTIL utilisé deux fois",
    "Spécification de date incomplète",
    "Mot-clé FROM/SCANFROM utilisé deux fois",
    "Variable",
    "Valeur",
    "*NON-DEFINI*",
    "Entrée dans UserFN",
    "Sortie de UserFN",
    "Expiré",
    "fork() échoué - impossible de faire les appels en queue",
    "Impossible d'accéder au fichier",
    "Date système illégale: Année est inférieure à %d\n",
    "Option de déverminage inconnue '%c'\n",
    "Option inconnue '%c'\n",
    "Usager inconnu '%s'\n",
    "Impossible de changer gid pour %d\n",
    "Impossible de changer uid pour %d\n",
    "Manque de mémoire pour environnement\n",
    "Signe '=' manquant",
    "Nom de variable absent",
    "Expression absente",
    "Impossible de changer la date d'accès de %s\n",
    "Remind: '-i' option: %s\n",
    "Pas de rappels.",
    "%d rappel(s) en file pour aujourd'hui.\n",
    "Nombre attendu",
    "Fonction illégale après WARN",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long",
    "Time specified twice",
    "Cannot specify DURATION without specifying AT"
};
#endif /* MK_GLOBALS */

/* The following is only used in init.c */
#ifdef L_IN_INIT
#define L_USAGE_OVERRIDE 1
void Usage(void)
{
    fprintf(ErrFp, "\nREMIND %s (%s version) Copyright 1992-2022 Dianne Skoll\n", VERSION, L_LANGNAME);
#ifdef BETA
    fprintf(ErrFp, ">>>> BETA VERSION <<<<\n");
#endif
    fprintf(ErrFp, "\nUtilisation: remind [options] fichier [date] [heure] [*répétition]\n");
    fprintf(ErrFp, "Options:\n");
    fprintf(ErrFp, " -n     Afficher la prochaine occurence des rappels en format simple\n");
    fprintf(ErrFp, " -r     Désactiver les instructions RUN\n");
    fprintf(ErrFp, " -c[n]  Produire un calendrier pour n (défaut 1) mois\n");
    fprintf(ErrFp, " -c+[n] Produire un calendrier pour n (défaut 1) semaines\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Spécifier largeur, remplissage et espacement du calendrier\n");
    fprintf(ErrFp, " -s[+][n] Produire un 'calendrier simple' pour n (1) mois (semaines)\n");
    fprintf(ErrFp, " -p[n]  Comme -s, mais avec entrée compatible avec rem2ps\n");
    fprintf(ErrFp, " -v     Mode verbeux\n");
    fprintf(ErrFp, " -o     Ignorer instructions ONCE\n");
    fprintf(ErrFp, " -t     Déclencher tous les rappels peu importe le delta\n");
    fprintf(ErrFp, " -h     Mode silencieux\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Ne pas déclencher les rappels minutés immédiatement - les mettre en file\n");
    fprintf(ErrFp, " -q     Ne pas mettre les rappels minutés en file\n");
    fprintf(ErrFp, " -f     Déclencher les rappels minutés immédiatement en restant en avant-plan\n");
    fprintf(ErrFp, " -z[n]  Entrer en mode 'daemon', réveil chaque n (5) minutes\n");
#endif
    fprintf(ErrFp, " -d...  Debug: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Envoyer les messages de stderr à stdout\n");
    fprintf(ErrFp, " -b[n]  Formats de l'heure pour le calendrier: 0=am/pm, 1=24hr, 2=aucun\n");
    fprintf(ErrFp, " -x[n]  Limite d'itérations pour la clause SATISFY (def=1000)\n");
    fprintf(ErrFp, " -kcmd  Exécuter 'cmd' pour les rappels de type MSG\n");
    fprintf(ErrFp, " -g[ddd] Trier les rappels par date, heure et priorité avant d'émettre\n");
    fprintf(ErrFp, " -ivar=val Initialiser var à val et conserver var\n");
    fprintf(ErrFp, " -m     Commencer le calendrier avec lundi plutôt que dimanche\n");
    exit(1);
}
#endif /* L_IN_INIT */
