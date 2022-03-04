/***************************************************************/
/*                                                             */
/*  FINNISH.H                                                  */
/*                                                             */
/*  Support for the Finnish language.                          */
/*                                                             */
/*  Author: Mikko Silvonen <silvonen@iki.fi>                   */
/*                                                             */
/*  See http://www.iki.fi/silvonen/remind/ for a list of       */
/*  Finnish holidays.                                          */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  This file is Copyright (C) 1993-1998 by Mikko Silvonen.    */
/*  REMIND is Copyright (C) 1992-2021 by Dianne Skoll          */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Finnish"

/* Day names */
#define L_SUNDAY "sunnuntai"
#define L_MONDAY "maanantai"
#define L_TUESDAY "tiistai"
#define L_WEDNESDAY "keskiviikko"
#define L_THURSDAY "torstai"
#define L_FRIDAY "perjantai"
#define L_SATURDAY "lauantai"

/* Month names */
#define L_JAN "tammikuu"
#define L_FEB "helmikuu"
#define L_MAR "maaliskuu"
#define L_APR "huhtikuu"
#define L_MAY "toukokuu"
#define L_JUN "kesäkuu"
#define L_JUL "heinäkuu"
#define L_AUG "elokuu"
#define L_SEP "syyskuu"
#define L_OCT "lokakuu"
#define L_NOV "marraskuu"
#define L_DEC "joulukuu"

/* Today and tomorrow */
#define L_TODAY "tänään"
#define L_TOMORROW "huomenna"

/* The default banner */
#define L_BANNER "Viestit %wna %d. %mta %y%o:"

/* "am" and "pm" */
#define L_AM " ap."
#define L_PM " ip."

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "sitten"
#define L_FROMNOW "kuluttua"

/* "in %d days' time" */
#define L_INXDAYS "%d päivän kuluttua"

/* "on" as in "on date...", but in Finnish it is a case ending;
   L_PARTIT is the partitive ending appended to -kuu and -tai */
#define L_ON "na"
#define L_PARTIT "ta"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
/* The partitive ending of "day" */
#define L_PLURAL "ä"

/* Minutes, hours, at, etc */
#define L_NOW "nyt"
#define L_AT "klo"
#define L_MINUTE "minuutti"
#define L_HOUR "tunti"
#define L_IS "on"
#define L_WAS "oli"
#define L_AND "ja"

/* What to add to make "hour" plural (or actually partitive) */
#define L_HPLU "a"
/* What to add to make "minute" plural (or actually partitive) */
#define L_MPLU "a"

/* Genitive form of "hour" */
#define L_HGEN "tunnin"
/* Genitive form of "minute" */
#define L_MGEN "minuutin"

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */

#define L_ORDINAL_OVERRIDE switch(d) { \
    case 1:  plu = ":senä"; break; \
    case 2:  plu = ":sena"; break; \
    default: \
     switch(d%10) { \
       case 2: \
       case 3: \
       case 6: \
       case 8:  plu = ":ntena"; break; \
       default: plu = ":ntenä"; break; \
     } \
}
#define L_A_OVER if (altmode == '*') { sprintf(s, "%s %d. %s %d", DayName[jul%7], d, MonthName[m], y); } else { sprintf(s, "%s%s %d. %s%s %d", DayName[jul%7], L_ON, d, MonthName[m], L_PARTIT, y); }
#define L_C_OVER if (altmode == '*') { sprintf(s, "%s", DayName[jul%7]); } else { sprintf(s, "%s%s", DayName[jul%7], L_ON); }
#define L_E_OVER sprintf(s, "%02d%c%02d%c%04d", d, DateSep, m+1, DateSep, y);
#define L_F_OVER sprintf(s, "%02d%c%02d%c%04d", m+1, DateSep, d, DateSep, y);
#define L_G_OVER if (altmode == '*') { sprintf(s, "%s %d. %s", DayName[jul%7], d, MonthName[m]); } else { sprintf(s, "%s%s %d. %s%s", DayName[jul%7], L_ON, d, MonthName[m], L_PARTIT); }
#define L_H_OVER sprintf(s, "%02d%c%02d", d, DateSep, m+1);
#define L_I_OVER sprintf(s, "%02d%c%02d", m+1, DateSep, d);
#define L_J_OVER if (altmode == '*') { sprintf(s, "%s %sn %d%s %d", DayName[jul%7], MonthName[m], d, plu, y); } else { sprintf(s, "%s%s %sn %d%s %d", DayName[jul%7], L_ON, MonthName[m], d, plu, y); }
#define L_K_OVER if (altmode == '*') { sprintf(s, "%s %sn %d%s", DayName[jul%7], MonthName[m], d, plu); } else { sprintf(s, "%s%s %sn %d%s", DayName[jul%7], L_ON, MonthName[m], d, plu); }
#define L_L_OVER sprintf(s, "%04d%c%02d%c%02d", y, DateSep, m+1, DateSep, d);
#define L_Q_OVER sprintf(s, "n");
#define L_U_OVER if (altmode == '*') { sprintf(s, "%s %d%s %s %d", DayName[jul%7], d, plu, MonthName[m], y); } else { sprintf(s, "%s%s %d%s %s%s %d", DayName[jul%7], L_ON, d, plu, MonthName[m], L_PARTIT, y); }
#define L_V_OVER if (altmode == '*') { sprintf(s, "%s %d%s %s", DayName[jul%7], d, plu, MonthName[m]); } else { sprintf(s, "%s%s %d%s %s%s", DayName[jul%7], L_ON, d, plu, MonthName[m], L_PARTIT); }
#define L_1_OVER \
if (tdiff == 0) \
  sprintf(s, "%s", L_NOW); \
else { \
   s[0] = '\0'; \
   if (hdiff != 0) { \
      if (tdiff < 0) \
	sprintf(s, "%d %s%s ", hdiff, L_HOUR, hplu); \
      else \
        sprintf(s, "%d %s ", hdiff, L_HGEN); \
   } \
   if (mdiff != 0) { \
      if (tdiff < 0) \
         sprintf(s + strlen(s), "%d %s%s ", mdiff, L_MINUTE, mplu); \
      else \
         sprintf(s + strlen(s), "%d %s ", mdiff, L_MGEN); \
   } \
   sprintf(s + strlen(s), when); \
}
#endif /* L_IN_DOSUBST */

/* The next ones are used only when MK_GLOBALS is set */
#ifdef MK_GLOBALS
#define L_ERR_OVERRIDE 1
EXTERN char *ErrMsg[] =
{
    "Ok",
    "Puuttuva ']'",
    "Puuttuva lainausmerkki",
    "Liian monimutkainen lauseke - liikaa operaattoreita",
    "Liian monimutkainen lauseke - liikaa operandeja",
    "Puuttuva ')'",
    "Määrittelemätön funktio",
    "Virheellinen merkki",
    "Kaksipaikkainen operaattori puuttuu",
    "Muisti loppui",
    "Virheellinen luku",
    "Operaattoripino tyhjä - sisäinen virhe",
    "Muuttujapino tyhjä - sisäinen virhe",
    "Tyyppimuunnos ei onnistu",
    "Virheellinen tyyppi",
    "Liian suuri päiväys",
    "Pinovirhe - sisäinen virhe",
    "Jako nollalla",
    "Määrittelemätön funktio",
    "Odottamaton rivin loppu",
    "Odottamaton tiedoston loppu",
    "Syöttö- tai tulostusvirhe",
    "Liian pitkä rivi",
    "Sisäinen virhe",
    "Virheellinen päiväys",
    "Liian vähän argumentteja",
    "Liian paljon argumentteja",
    "Virheellinen aika",
    "Liian suuri luku",
    "Liian pieni luku",
    "Tiedoston avaus ei onnistu",
    "Liian monta sisäkkäistä INCLUDEa",
    "Jäsennysvirhe",
    "Laukaisuhetken laskenta ei onnistu",
    "Liian monta sisäkkäistä IF-lausetta",
    "ELSE ilman IF-lausetta",
    "ENDIF ilman IF-lausetta",
    "Kaikkia viikonpäiviä ei voi jättää pois",
    "Ylimääräisiä merkkejä rivillä",
    "POP-OMIT-CONTEXT ilman PUSH-OMIT-CONTEXTia",
    "RUN-lauseen käyttö estetty",
    "Arvoaluevirhe",
    "Virheellinen tunniste",
    "Rekursiivinen funktiokutsu havaittu",
    "",
    "Järjestelmämuuttujan muuttaminen ei onnistu",
    "C-kirjastofunktio ei pysty esittämään päiväystä tai aikaa",
    "Sisäisen funktion määritelmää yritettiin muuttaa",
    "Lausekkeessa ei voi olla sisäkkäisiä funktiomääritelmiä",
    "Päiväyksen täytyy olla täydellinen toistokertoimessa",
    "Vuosi annettu kahdesti",
    "Kuukausi annettu kahdesti",
    "Päivä annettu kahdesti",
    "Tuntematon sana tai merkki",
    "OMIT-komennossa on annettava kuukausi ja päivä",
    "Liian monta osittaista OMIT-komentoa",
    "Liian monta täydellistä OMIT-komentoa",
    "Varoitus: PUSH-OMIT-CONTEXT ilman POP-OMIT-CONTEXTia",
    "Virhe tiedoston luvussa",
    "Pilkku puuttuu",
    "Virheellinen juutalainen päiväys",
    "IIF vaatii parittoman määrän argumentteja",
    "Varoitus: puuttuva ENDIF",
    "Pilkku puuttuu",
    "Viikonpäivä annettu kahdesti",
    "Käytä vain yhtä komennoista BEFORE, AFTER ja SKIP",
    "Sisäkkäisiä MSG-, MSF- ja RUN-lauseita ei voi käyttää lausekkeessa",
    "Toistokerroin annettu kahdesti",
    "Delta-arvo annettu kahdesti",
    "Peruutusarvo annettu kahdesti",
    "ONCE-avainsanaa käytetty kahdesti. (Hah.)",
    "AT-sanan perästä puuttuu aika",
    "THROUGH/UNTIL-sanaa käytetty kahdesti",
    "Epätäydellinen päiväys",
    "FROM/SCANFROM-sanaa käytetty kahdesti",
    "Muuttuja",
    "Arvo",
    "*MÄÄRITTELEMÄTÖN*",
    "Siirrytään funktioon",
    "Poistutaan funktiosta",
    "Vanhentunut",
    "fork() epäonnistui - jonomuistutukset eivät toimi",
    "Tiedoston avaus ei onnistu",
    "Virheellinen järjestelmäpäiväys: vuosi on vähemmän kuin %d\n",
    "Tuntematon virheenetsintätarkenne '%c'\n",
    "Tuntematon tarkenne '%c'\n",
    "Tuntematon käyttäjä '%s'\n",
    "Ryhmänumeron vaihto %d:ksi ei onnistunut\n",
    "Käyttäjänumeron vaihto %d:ksi ei onnistunut\n",
    "Muisti ei riitä ympäristölle\n",
    "Puuttuva '='-merkki",
    "Puuttuva muuttujanimi",
    "Puuttuva lauseke",
    "Päivän asetus %s:ksi ei onnitus\n",
    "Remind: tarkenne '-i': %s\n",
    "Ei viestejä.",
    "%d viesti(ä) tämän päivän jonossa.\n",
    "Numero puuttuu",
    "Virheellinen funktio WARN-lausekkeessa",
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
    fprintf(ErrFp, "\nREMIND %s (%s version) Copyright 1992-2021 Dianne Skoll\n", VERSION, L_LANGNAME);
#ifdef BETA
    fprintf(ErrFp, ">>>> BETAVERSIO <<<<\n");
#endif
    fprintf(ErrFp, "Käyttö: remind [tarkenteet] tiedosto [päiväys] [aika] [*toisto]\n");
    fprintf(ErrFp, "Tarkenteet:\n");
    fprintf(ErrFp, " -n     Tulosta viestien seuraavat esiintymiskerrat yksink. muodossa\n");
    fprintf(ErrFp, " -r     Estä RUN-lauseiden käyttö\n");
    fprintf(ErrFp, " -c[n]  Tulosta n:n kuukauden kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -c+[n] Tulosta n:n viikon kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -w[n[,p[,s]]] Aseta kalenterin leveys, tasaus ja välit\n");
    fprintf(ErrFp, " -s[+][n] Tulosta n:n kuukauden (viikon) 'yksink. kalenteri' (oletus 1)\n");
    fprintf(ErrFp, " -p[n]  Kuten -s, mutta tulosta rem2ps:lle sopivassa muodossa\n");
    fprintf(ErrFp, " -v     Laveat tulostukset\n");
    fprintf(ErrFp, " -o     Älä noudata ONCE-lauseita\n");
    fprintf(ErrFp, " -t     Laukaise kaikki viestit deltan arvosta välittämättä\n");
    fprintf(ErrFp, " -h     Suppeat tulostukset\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Älä laukaise viestejä heti - lisää ne jonoon\n");
    fprintf(ErrFp, " -q     Älä lisää viestejä jonoon\n");
    fprintf(ErrFp, " -f     Laukaise viestit, pysy etualalla\n");
    fprintf(ErrFp, " -z[n]  Käynnisty demonina, herätys n:n (5:n) minuutin välein\n");
#endif
    fprintf(ErrFp, " -d...  Virheenetsintä: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Ohjaa virhetulostus stdout-vuohon\n");
    fprintf(ErrFp, " -b[n]  Ajan ilmaisu: 0=ap/ip, 1=24 tuntia, 2=ei aikoja\n");
    fprintf(ErrFp, " -x[n]  SATISFY-lauseen toistoraja (oletus 1000)\n");
    fprintf(ErrFp, " -kcmd  Suorita 'cmd' MSG-tyyppisille viesteille\n");
    fprintf(ErrFp, " -g[ddd] Lajittele viestit päiväyksen, ajan ja tärkeyden mukaan\n");
    fprintf(ErrFp, " -ivar=val Alusta muuttuja var arvolla val ja säilytä var\n");
    fprintf(ErrFp, " -m     Aloita kalenteri maanantaista eikä sunnuntaista\n");
    exit(1);
}
#endif /* L_IN_INIT */
