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
/*  REMIND is Copyright (C) 1992-1998 by David F. Skoll        */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*  This file is Copyright (C) 1993 by Laurent Duperval and    */
/*  David F. Skoll.                                            */
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
#if ISOLATIN1
#define L_FEB "f\351vrier"
#else
#define L_FEB "fevrier"
#endif
#define L_MAR "mars"
#define L_APR "avril"
#define L_MAY "mai"
#define L_JUN "juin"
#define L_JUL "juillet"
#if ISOLATIN1
#define L_AUG "ao\373t"
#else
#define L_AUG "aout"
#endif
#define L_SEP "septembre"
#define L_OCT "octobre"
#define L_NOV "novembre"
#if ISOLATIN1
#define L_DEC "d\351cembre"
#else
#define L_DEC "decembre"
#endif
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
#if ISOLATIN1
#define L_AT "\340"
#else
#define L_AT "a"
#endif
#define L_MINUTE "minute"
#define L_HOUR "heure"
#define L_IS "est"
#if ISOLATIN1
#define L_WAS "\351tait"
#else
#define L_WAS "etait"
#endif
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
#if ISOLATIN1
    "Ok",
    "']' manquant",
    "Apostrophe manquant",
    "Expression trop complexe - trop d'op\351rateurs",
    "Expression trop complexe - trop d'op\351randes",
    "')' manquante",
    "Fonction non-d\351finie",
    "Caract\350re ill\351gal",
    "Op\351rateur binaire attendu",
    "Manque de m\351moire",
    "Nombre mal form\351",
    "Erreur interne - 'underflow' de la pile d'op\351rateurs",
    "Erreur interne - 'underflow' de la pile de variables",
    "Impossible de convertir",
    "Types non-\351quivalents",
    "D\351bordement de date",
    "Erreur interne - erreur de pile",
    "Division par z\351ro",
    "Variable non d\351finie",
    "Fin de ligne non attendue",
    "Fin de fichier non attendue",
    "Erreur I/O",
    "Ligne trop longue",
    "Erreur interne",
    "Mauvaise date sp\351cifi\351e",
    "Pas assez d'arguments",
    "Trop d'arguments",
    "Heure mal form\351e",
    "Nombre trop \351lev\351",
    "Nombre trop bas",
    "Impossible d'ouvrir le fichier",
    "Trop d'INCLUDE imbriqu\351s",
    "Erreur d'analyse",
    "Impossible de calculer le d\351clenchement",
    "Trop de IF imbriqu\351s",
    "ELSE sans IF correspondant",
    "ENDIF sans IF correspondant",
    "Impossible d'omettre (OMIT) tous les jours",
    "El\351ment(s) \351tranger(s) sur la ligne",
    "POP-OMIT-CONTEXT sans PUSH-OMIT-CONTEXT correspondant",
    "RUN d\351activ\351",
    "Erreur de domaine",
    "Identificateur invalide",
    "Appel r\351cursif d\351tect\351",
    "",
    "Impossible de modifier une variable syst\350me",
    "Fonction de la librairie C ne peut repr\351senter la date/l'heure",
    "Tentative de red\351finition d'une fonction intrins\350que",
    "Impossible d'imbriquer une d\351finition de fonction dans une expression",
    "Pour utiliser le facteur de r\351p\351tition la date doit \352tre sp\351cifi\351e au complet",
    "Ann\351e sp\351cifi\351e deux fois",
    "Mois sp\351cifi\351 deux fois",
    "Jour sp\351cifi\351 deux fois",
    "El\351ment inconnu",
    "Mois et jour doivent \352tre sp\351cifi\351s dans commande OMIT",
    "Trop de OMITs partiels",
    "Trop de OMITs complets",
    "Attention: PUSH-OMIT-CONTEXT sans POP-OMIT-CONTEXT correspondant",
    "Erreur \340 la lecture du fichier",
    "Fin de ligne attendue",
    "Date h\351breuse invalide",
    "IIF demande nombre d'arguments impair",
    "Attention: ENDIF manquant",
    "Virgule attendue",
    "Jour de la semaine sp\351cifi\351 deux fois",
    "Utiliser un seul parmi BEFORE, AFTER ou SKIP",
    "Impossible d'imbriquer MSG, MSF, RUN, etc. dans une expression",
    "Valeur de r\351p\351tition sp\351cifi\351e deux fois",
    "Valeur delta sp\351cifi\351e deux fois",
    "Valeur de retour sp\351cifi\351e deux fois",
    "Mot-cl\351 ONCE utilis\351 deux fois. (Hah.)",
    "Heure attendue apr\350s AT",
    "Mot-cl\351 THROUGH/UNTIL utilis\351 deux fois",
    "Sp\351cification de date incompl\350te",
    "Mot-cl\351 FROM/SCANFROM utilis\351 deux fois",
    "Variable",
    "Valeur",
    "*NON-DEFINI*",
    "Entr\351e dans UserFN",
    "Sortie de UserFN",
    "Expir\351",
    "fork() \351chou\351 - impossible de faire les appels en queue",
    "Impossible d'acc\351der au fichier",
    "Date syst\350me ill\351gale: Ann\351e est inf\351rieure \340 %d\n",
    "Option de d\351verminage inconnue '%c'\n",
    "Option inconnue '%c'\n",
    "Usager inconnu '%s'\n",
    "Impossible de changer gid pour %d\n",
    "Impossible de changer uid pour %d\n",
    "Manque de m\351moire pour environnement\n",
    "Signe '=' manquant",
    "Nom de variable absent",
    "Expression absente",
    "Impossible de changer la date d'acc\350s de %s\n",
    "Remind: '-i' option: %s\n",
    "Pas de rappels.",
    "%d rappel(s) en file pour aujourd'hui.\n",
    "Nombre attendu",
    "Fonction ill\351gale apr\350s WARN",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"

#else /* ISOLATIN1 */
    "Ok",
    "']' manquant",
    "Apostrophe manquant",
    "Expression trop complexe - trop d'operateurs",
    "Expression trop complexe - trop d'operandes",
    "')' manquante",
    "Fonction non-definie",
    "Caractere illegal",
    "Operateur binaire attendu",
    "Manque de memoire",
    "Nombre mal forme",
    "Erreur interne - 'underflow' de la pile d'operateurs",
    "Erreur interne - 'underflow' de la pile de variables",
    "Impossible de convertir",
    "Types non-equivalents",
    "Debordement de date",
    "Erreur interne - erreur de pile",
    "Division par zero",
    "Variable non definie",
    "Fin de ligne non attendue",
    "Fin de fichier non attendue",
    "Erreur I/O",
    "Ligne trop longue",
    "Erreur interne",
    "Mauvaise date specifiee",
    "Pas assez d'arguments",
    "Trop d'arguments",
    "Heure mal formee",
    "Nombre trop eleve",
    "Nombre trop bas",
    "Impossible d'ouvrir le fichier",
    "Trop d'INCLUDE imbriques",
    "erreur d'analyse",
    "Impossible de calculer le declenchement",
    "Trop de IF imbriques",
    "ELSE sans IF correspondant",
    "ENDIF sans IF correspondant",
    "Impossible d'omettre (OMIT) tous les jours",
    "Element(s) etranger(s) sur la ligne",
    "POP-OMIT-CONTEXT sans PUSH-OMIT-CONTEXT correspondant",
    "RUN desactive",
    "Erreur de domaine",
    "Identificateur invalide",
    "Appel recursif detecte",
    "",
    "Impossible de modifier une variable systeme",
    "Fonction de la librairie C ne peut representer la date/l'heure",
    "Tentative de redefinition d'une fonction intrinseque",
    "Impossible d'imbriquer une definition de fonction dans une expression",
    "Pour utiliser le facteur de repetition la date doit etre specifiee au complet",
    "Annee specifiee deux fois",
    "Mois specifie deux fois",
    "Jour specifie deux fois",
    "Element inconnu",
    "Mois et jour doivent etre specifies dans commande OMIT",
    "Trop de OMITs partiels",
    "Trop de OMITs complets",
    "Attention: PUSH-OMIT-CONTEXT sans POP-OMIT-CONTEXT correspondant",
    "Erreur a la lecture du fichier",
    "Fin de ligne attendue",
    "Date hebreuse invalide",
    "IIF demande nombre d'arguments impair",
    "Attention: ENDIF manquant",
    "Virgule attendue",
    "Jour de la semaine specifie deux fois",
    "Utiliser un seul parmi BEFORE, AFTER ou SKIP",
    "Impossible d'imbriquer MSG, MSF, RUN, etc. dans une expression",
    "Valeur de repetition specifiee deux fois",
    "Valeur delta specifiee deux fois",
    "Valeur de retour specifiee deux fois",
    "Mot-cle ONCE utilise deux fois. (Hah.)",
    "Heure attendue apres AT",
    "Mot-cle THROUGH/UNTIL utilise deux fois",
    "Specification de date incomplete",
    "Mot-cle FROM/SCANFROM utilise deux fois",
    "Variable",
    "Valeur",
    "*NON-DEFINI*",
    "Entree dans UserFN",
    "Sortie de UserFN",
    "Expire",
    "fork() echoue - impossible de faire les appels en queue",
    "Impossible d'acceder au fichier",
    "Date systeme illegale: Annee est inferieure a %d\n",
    "Option de deverminage inconnue '%c'\n",
    "Option inconnue '%c'\n",
    "Usager inconnu '%s'\n",
    "Impossible de changer gid pour %d\n",
    "Impossible de changer uid pour %d\n",
    "Manque de memoire pour environnement\n",
    "Signe '=' manquant",
    "Nom de variable absent",
    "Expression absente",
    "Impossible de changer la date d'acces de %s\n",
    "Remind: '-i' option: %s\n",
    "Pas de rappels.",
    "%d rappel(s) en file pour aujourd'hui.\n",
    "Nombre attendu",
    "Fonction illegale apres WARN",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"
#endif /* ISOLATIN1 */
};
#endif /* MK_GLOBALS */

/* The following is only used in init.c */
#ifdef L_IN_INIT
#define L_USAGE_OVERRIDE 1
void Usage(void)
{
    fprintf(ErrFp, "\nREMIND %s (%s version) Copyright 1992-1998 David F. Skoll\n", VERSION, L_LANGNAME);
    fprintf(ErrFp, "Copyright 1999-2000 Roaring Penguin Software Inc.\n");
#ifdef BETA
    fprintf(ErrFp, ">>>> BETA VERSION <<<<\n");
#endif
#if ISOLATIN1
    fprintf(ErrFp, "\nUtilisation: remind [options] fichier [date] [heure] [*r\351p\351tition]\n");
    fprintf(ErrFp, "Options:\n");
    fprintf(ErrFp, " -n     Afficher la prochaine occurence des rappels en format simple\n");
    fprintf(ErrFp, " -r     D\351sactiver les instructions RUN\n");
    fprintf(ErrFp, " -c[n]  Produire un calendrier pour n (d\351faut 1) mois\n");
    fprintf(ErrFp, " -c+[n] Produire un calendrier pour n (d\351faut 1) semaines\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Sp\351cifier largeur, remplissage et espacement du calendrier\n");
    fprintf(ErrFp, " -s[+][n] Produire un 'calendrier simple' pour n (1) mois (semaines)\n");
    fprintf(ErrFp, " -p[n]  Comme -s, mais avec entr\351e compatible avec rem2ps\n");
    fprintf(ErrFp, " -v     Mode verbeux\n");
    fprintf(ErrFp, " -o     Ignorer instructions ONCE\n");
    fprintf(ErrFp, " -t     D\351clencher tous les rappels peu importe le delta\n");
    fprintf(ErrFp, " -h     Mode silencieux\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Ne pas d\351clencher les rappels minut\351s imm\351diatement - les mettre en file\n");
    fprintf(ErrFp, " -q     Ne pas mettre les rappels minut\351s en file\n");
    fprintf(ErrFp, " -f     D\351clencher les rappels minut\351s imm\351diatement en restant en avant-plan\n");
    fprintf(ErrFp, " -z[n]  Entrer en mode 'daemon', r\351veil chaque n (5) minutes\n");
#endif
    fprintf(ErrFp, " -d...  Debug: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Envoyer les messages de stderr \340 stdout\n");
    fprintf(ErrFp, " -b[n]  Formats de l'heure pour le calendrier: 0=am/pm, 1=24hr, 2=aucun\n");
    fprintf(ErrFp, " -x[n]  Limite d'it\351rations pour la clause SATISFY (def=150)\n");
    fprintf(ErrFp, " -kcmd  Ex\351cuter 'cmd' pour les rappels de type MSG\n");
    fprintf(ErrFp, " -g[ddd] Trier les rappels par date, heure et priorit\351 avant d'\351mettre\n");
    fprintf(ErrFp, " -ivar=val Initialiser var \340 val et conserver var\n");
    fprintf(ErrFp, " -m     Commencer le calendrier avec lundi plut\364t que dimanche\n");
#else /* ISOLATIN1 */
    fprintf(ErrFp, "\nUtilisation: remind [options] fichier [date] [heure] [*repetition]\n");
    fprintf(ErrFp, "Options:\n");
    fprintf(ErrFp, " -n     Afficher la prochaine occurence des rappels en format simple\n");
    fprintf(ErrFp, " -r     Desactiver les instructions RUN\n");
    fprintf(ErrFp, " -c[n]  Produire un calendrier pour n (defaut 1) mois\n");
    fprintf(ErrFp, " -c+[n] Produire un calendrier pour n (defaut 1) semaines\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Specifier largeur, remplissage et espacement du calendrier\n");
    fprintf(ErrFp, " -s[+][n] Produire un 'calendrier simple' pour n (1) mois (semaines)\n");
    fprintf(ErrFp, " -p[n]  Comme -s, mais avec entree compatible avec rem2ps\n");
    fprintf(ErrFp, " -v     Mode verbeux\n");
    fprintf(ErrFp, " -o     Ignorer instructions ONCE\n");
    fprintf(ErrFp, " -t     Declencher tous les rappels peu importe le delta\n");
    fprintf(ErrFp, " -h     Mode silencieux\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Ne pas declencher les rappels minutes immediatement - les mettre en file\n");
    fprintf(ErrFp, " -q     Ne pas mettre les rappels minutes en file\n");
    fprintf(ErrFp, " -f     Declencher les rappels minutes immediatement en restant en avant-plan\n");
    fprintf(ErrFp, " -z[n]  Entrer en mode 'daemon', reveil chaque n (5) minutes\n");
#endif
    fprintf(ErrFp, " -d...  Debug: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Envoyer les messages de stderr a stdout\n");
    fprintf(ErrFp, " -b[n]  Formats de l'heure pour le calendrier: 0=am/pm, 1=24hr, 2=aucun\n");
    fprintf(ErrFp, " -x[n]  Limite d'iterations pour la clause SATISFY (def=150)\n");
    fprintf(ErrFp, " -kcmd  Executer 'cmd' pour les rappels de type MSG\n");
    fprintf(ErrFp, " -g[ddd] Trier les rappels par date, heure et priorite avant d'emettre\n");
    fprintf(ErrFp, " -ivar=val Initialiser var a val et conserver var\n");
    fprintf(ErrFp, " -m     Commencer le calendrier avec lundi plutot que dimanche\n");
#endif /* ISOLATIN1 */
    exit(1);
}
#endif /* L_IN_INIT */
