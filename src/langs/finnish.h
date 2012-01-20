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
/*  REMIND is Copyright (C) 1992-1998 by David F. Skoll        */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
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
#if ISOLATIN1
#define L_JUN "kes\xE4kuu"
#define L_JUL "hein\xE4kuu"
#elif IBMEXTENDED
#define L_JUN "kes\x84kuu"
#define L_JUL "hein\x84kuu"
#else
#define L_JUN "kes{kuu"
#define L_JUL "hein{kuu"
#endif
#define L_AUG "elokuu"
#define L_SEP "syyskuu"
#define L_OCT "lokakuu"
#define L_NOV "marraskuu"
#define L_DEC "joulukuu"

/* Today and tomorrow */
#if ISOLATIN1
#define L_TODAY "t\xE4n\xE4\xE4n"
#elif IBMEXTENDED
#define L_TODAY "t\x84n\x84\x84n"
#else
#define L_TODAY "t{n{{n"
#endif
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
#if ISOLATIN1
#define L_INXDAYS "%d p\xE4iv\xE4n kuluttua"
#elif IBMEXTENDED
#define L_INXDAYS "%d p\x84iv\x84n kuluttua"
#else
#define L_INXDAYS "%d p{iv{n kuluttua"
#endif

/* "on" as in "on date...", but in Finnish it is a case ending;
   L_PARTIT is the partitive ending appended to -kuu and -tai */
#define L_ON "na"
#define L_PARTIT "ta"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
/* The partitive ending of "day" */
#if ISOLATIN1
#define L_PLURAL "\xE4"
#elif IBMEXTENDED
#define L_PLURAL "\x84"
#else
#define L_PLURAL "{"
#endif

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

#if ISOLATIN1
#define L_ORDINAL_OVERRIDE switch(d) { \
    case 1:  plu = ":sen\xE4"; break; \
    case 2:  plu = ":sena"; break; \
    default: \
     switch(d%10) { \
       case 2: \
       case 3: \
       case 6: \
       case 8:  plu = ":ntena"; break; \
       default: plu = ":nten\xE4"; break; \
     } \
}
#elif IBMEXTENDED
#define L_ORDINAL_OVERRIDE switch(d) { \
    case 1:  plu = ":sen\x84"; break; \
    case 2:  plu = ":sena"; break; \
    default: \
     switch(d%10) { \
       case 2: \
       case 3: \
       case 6: \
       case 8:  plu = ":ntena"; break; \
       default: plu = ":nten\x84"; break; \
     } \
}
#else
#define L_ORDINAL_OVERRIDE switch(d) { \
    case 1:  plu = ":sen{"; break; \
    case 2:  plu = ":sena"; break; \
    default: \
       switch(d%10) { \
	  case 2: \
	  case 3: \
	  case 6: \
	  case 8:  plu = ":ntena"; break; \
	  default: plu = ":nten{"; break; \
       } \
}
#endif
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
#if ISOLATIN1
    "Ok",
    "Puuttuva ']'",
    "Puuttuva lainausmerkki",
    "Liian monimutkainen lauseke - liikaa operaattoreita",
    "Liian monimutkainen lauseke - liikaa operandeja",
    "Puuttuva ')'",
    "M\xE4\xE4rittelem\xE4t\xF6n funktio",
    "Virheellinen merkki",
    "Kaksipaikkainen operaattori puuttuu",
    "Muisti loppui",
    "Virheellinen luku",
    "Operaattoripino tyhj\xE4 - sis\xE4inen virhe",
    "Muuttujapino tyhj\xE4 - sis\xE4inen virhe",
    "Tyyppimuunnos ei onnistu",
    "Virheellinen tyyppi",
    "Liian suuri p\xE4iv\xE4ys",
    "Pinovirhe - sis\xE4inen virhe",
    "Jako nollalla",
    "M\xE4\xE4rittelem\xE4t\xF6n funktio",
    "Odottamaton rivin loppu",
    "Odottamaton tiedoston loppu",
    "Sy\xF6tt\xF6- tai tulostusvirhe",
    "Liian pitk\xE4 rivi",
    "Sis\xE4inen virhe",
    "Virheellinen p\xE4iv\xE4ys",
    "Liian v\xE4h\xE4n argumentteja",
    "Liian paljon argumentteja",
    "Virheellinen aika",
    "Liian suuri luku",
    "Liian pieni luku",
    "Tiedoston avaus ei onnistu",
    "Liian monta sis\xE4kk\xE4ist\xE4 INCLUDEa",
    "J\xE4sennysvirhe",
    "Laukaisuhetken laskenta ei onnistu",
    "Liian monta sis\xE4kk\xE4ist\xE4 IF-lausetta",
    "ELSE ilman IF-lausetta",
    "ENDIF ilman IF-lausetta",
    "Kaikkia viikonp\xE4ivi\xE4 ei voi j\xE4tt\xE4\xE4 pois",
    "Ylim\xE4\xE4r\xE4isi\xE4 merkkej\xE4 rivill\xE4",
    "POP-OMIT-CONTEXT ilman PUSH-OMIT-CONTEXTia",
    "RUN-lauseen k\xE4ytt\xF6 estetty",
    "Arvoaluevirhe",
    "Virheellinen tunniste",
    "Rekursiivinen funktiokutsu havaittu",
    "",
    "J\xE4rjestelm\xE4muuttujan muuttaminen ei onnistu",
    "C-kirjastofunktio ei pysty esitt\xE4m\xE4\xE4n p\xE4iv\xE4yst\xE4 tai aikaa",
    "Sis\xE4isen funktion m\xE4\xE4ritelm\xE4\xE4 yritettiin muuttaa",
    "Lausekkeessa ei voi olla sis\xE4kk\xE4isi\xE4 funktiom\xE4\xE4ritelmi\xE4",
    "P\xE4iv\xE4yksen t\xE4ytyy olla t\xE4ydellinen toistokertoimessa",
    "Vuosi annettu kahdesti",
    "Kuukausi annettu kahdesti",
    "P\xE4iv\xE4 annettu kahdesti",
    "Tuntematon sana tai merkki",
    "OMIT-komennossa on annettava kuukausi ja p\xE4iv\xE4",
    "Liian monta osittaista OMIT-komentoa",
    "Liian monta t\xE4ydellist\xE4 OMIT-komentoa",
    "Varoitus: PUSH-OMIT-CONTEXT ilman POP-OMIT-CONTEXTia",
    "Virhe tiedoston luvussa",
    "Pilkku puuttuu",
    "Virheellinen juutalainen p\xE4iv\xE4ys",
    "IIF vaatii parittoman m\xE4\xE4r\xE4n argumentteja",
    "Varoitus: puuttuva ENDIF",
    "Pilkku puuttuu",
    "Viikonp\xE4iv\xE4 annettu kahdesti",
    "K\xE4yt\xE4 vain yht\xE4 komennoista BEFORE, AFTER ja SKIP",
    "Sis\xE4kk\xE4isi\xE4 MSG-, MSF- ja RUN-lauseita ei voi k\xE4ytt\xE4\xE4 lausekkeessa",
    "Toistokerroin annettu kahdesti",
    "Delta-arvo annettu kahdesti",
    "Peruutusarvo annettu kahdesti",
    "ONCE-avainsanaa k\xE4ytetty kahdesti. (Hah.)",
    "AT-sanan per\xE4st\xE4 puuttuu aika",
    "THROUGH/UNTIL-sanaa k\xE4ytetty kahdesti",
    "Ep\xE4t\xE4ydellinen p\xE4iv\xE4ys",
    "FROM/SCANFROM-sanaa k\xE4ytetty kahdesti",
    "Muuttuja",
    "Arvo",
    "*M\xC4\xC4RITTELEM\xC4T\xD6N*",
    "Siirryt\xE4\xE4n funktioon",
    "Poistutaan funktiosta",
    "Vanhentunut",
    "fork() ep\xE4onnistui - jonomuistutukset eiv\xE4t toimi",
    "Tiedoston avaus ei onnistu",
    "Virheellinen j\xE4rjestelm\xE4p\xE4iv\xE4ys: vuosi on v\xE4hemm\xE4n kuin %d\n",
    "Tuntematon virheenetsint\xE4tarkenne '%c'\n",
    "Tuntematon tarkenne '%c'\n",
    "Tuntematon k\xE4ytt\xE4j\xE4 '%s'\n",
    "Ryhm\xE4numeron vaihto %d:ksi ei onnistunut\n",
    "K\xE4ytt\xE4j\xE4numeron vaihto %d:ksi ei onnistunut\n",
    "Muisti ei riit\xE4 ymp\xE4rist\xF6lle\n",
    "Puuttuva '='-merkki",
    "Puuttuva muuttujanimi",
    "Puuttuva lauseke",
    "P\xE4iv\xE4n asetus %s:ksi ei onnitus\n",
    "Remind: tarkenne '-i': %s\n",
    "Ei viestej\xE4.",
    "%d viesti(\xE4) t\xE4m\xE4n p\xE4iv\xE4n jonossa.\n",
    "Numero puuttuu",
    "Virheellinen funktio WARN-lausekkeessa",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"

#elif IBMEXTENDED
    "Ok",
    "Puuttuva ']'",
    "Puuttuva lainausmerkki",
    "Liian monimutkainen lauseke - liikaa operaattoreita",
    "Liian monimutkainen lauseke - liikaa operandeja",
    "Puuttuva ')'",
    "M\x84\x84rittelem\x84t\x94n funktio",
    "Virheellinen merkki",
    "Kaksipaikkainen operaattori puuttuu",
    "Muisti loppui",
    "Virheellinen luku",
    "Operaattoripino tyhj\x84 - sis\x84inen virhe",
    "Muuttujapino tyhj\x84 - sis\x84inen virhe",
    "Tyyppimuunnos ei onnistu",
    "Virheellinen tyyppi",
    "Liian suuri p\x84iv\x84ys",
    "Pinovirhe - sis\x84inen virhe",
    "Jako nollalla",
    "M\x84\x84rittelem\x84t\x94n funktio",
    "Odottamaton rivin loppu",
    "Odottamaton tiedoston loppu",
    "Sy\x94tt\x94- tai tulostusvirhe",
    "Liian pitk\x84 rivi",
    "Sis\x84inen virhe",
    "Virheellinen p\x84iv\x84ys",
    "Liian v\x84h\x84n argumentteja",
    "Liian paljon argumentteja",
    "Virheellinen aika",
    "Liian suuri luku",
    "Liian pieni luku",
    "Tiedoston avaus ei onnistu",
    "Liian monta sis\x84kk\x84ist\x84 INCLUDEa",
    "J\x84sennysvirhe",
    "Laukaisuhetken laskenta ei onnistu",
    "Liian monta sis\x84kk\x84ist\x84 IF-lausetta",
    "ELSE ilman IF-lausetta",
    "ENDIF ilman IF-lausetta",
    "Kaikkia viikonp\x84ivi\x84 ei voi j\x84tt\x84\x84 pois",
    "Ylim\x84\x84r\x84isi\x84 merkkej\x84 rivill\x84",
    "POP-OMIT-CONTEXT ilman PUSH-OMIT-CONTEXTia",
    "RUN-lauseen k\x84ytt\x94 estetty",
    "Arvoaluevirhe",
    "Virheellinen tunniste",
    "Rekursiivinen funktiokutsu havaittu",
    "",
    "J\x84rjestelm\x84muuttujan muuttaminen ei onnistu",
    "C-kirjastofunktio ei pysty esitt\x84m\x84\x84n p\x84iv\x84yst\x84 tai aikaa",
    "Sis\x84isen funktion m\x84\x84ritelm\x84\x84 yritettiin muuttaa",
    "Lausekkeessa ei voi olla sis\x84kk\x84isi\x84 funktiom\x84\x84ritelmi\x84",
    "P\x84iv\x84yksen t\x84ytyy olla t\x84ydellinen toistokertoimessa",
    "Vuosi annettu kahdesti",
    "Kuukausi annettu kahdesti",
    "P\x84iv\x84 annettu kahdesti",
    "Tuntematon sana tai merkki",
    "OMIT-komennossa on annettava kuukausi ja p\x84iv\x84",
    "Liian monta osittaista OMIT-komentoa",
    "Liian monta t\x84ydellist\x84 OMIT-komentoa",
    "Varoitus: PUSH-OMIT-CONTEXT ilman POP-OMIT-CONTEXTia",
    "Virhe tiedoston luvussa",
    "Pilkku puuttuu",
    "Virheellinen juutalainen p\x84iv\x84ys",
    "IIF vaatii parittoman m\x84\x84r\x84n argumentteja",
    "Varoitus: puuttuva ENDIF",
    "Pilkku puuttuu",
    "Viikonp\x84iv\x84 annettu kahdesti",
    "K\x84yt\x84 vain yht\x84 komennoista BEFORE, AFTER ja SKIP",
    "Sis\x84kk\x84isi\x84 MSG-, MSF- ja RUN-lauseita ei voi k\x84ytt\x84\x84 lausekkeessa",
    "Toistokerroin annettu kahdesti",
    "Delta-arvo annettu kahdesti",
    "Peruutusarvo annettu kahdesti",
    "ONCE-avainsanaa k\x84ytetty kahdesti. (Hah.)",
    "AT-sanan per\x84st\x84 puuttuu aika",
    "THROUGH/UNTIL-sanaa k\x84ytetty kahdesti",
    "Ep\x84t\x84ydellinen p\x84iv\x84ys",
    "FROM/SCANFROM-sanaa k\x84ytetty kahdesti",
    "Muuttuja",
    "Arvo",
    "*M\x8E\x8ERITTELEM\x8ET\x99N*",
    "Siirryt\x84\x84n funktioon",
    "Poistutaan funktiosta",
    "Vanhentunut",
    "fork() ep\x84onnistui - jonomuistutukset eiv\x84t toimi",
    "Tiedoston avaus ei onnistu",
    "Virheellinen j\x84rjestelm\x84p\x84iv\x84ys: vuosi on v\x84hemm\x84n kuin %d\n",
    "Tuntematon virheenetsint\x84tarkenne '%c'\n",
    "Tuntematon tarkenne '%c'\n",
    "Tuntematon k\x84ytt\x84j\x84 '%s'\n",
    "Ryhm\x84numeron vaihto %d:ksi ei onnistunut\n",
    "K\x84ytt\x84j\x84numeron vaihto %d:ksi ei onnistunut\n",
    "Muisti ei riit\x84 ymp\x84rist\x94lle\n",
    "Puuttuva '='-merkki",
    "Puuttuva muuttujanimi",
    "Puuttuva lauseke",
    "P\x84iv\x84n asetus %s:ksi ei onnitus\n",
    "Remind: tarkenne '-i': %s\n",
    "Ei viestej\x84.",
    "%d viesti(\x84) t\x84m\x84n p\x84iv\x84n jonossa.\n",
    "Numero puuttuu"
    "Virheellinen funktio WARN-lausekkeessa",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"

#else
    "Ok",
    "Puuttuva ']'",
    "Puuttuva lainausmerkki",
    "Liian monimutkainen lauseke - liikaa operaattoreita",
    "Liian monimutkainen lauseke - liikaa operandeja",
    "Puuttuva ')'",
    "M{{rittelem{t|n funktio",
    "Virheellinen merkki",
    "Kaksipaikkainen operaattori puuttuu",
    "Muisti loppui",
    "Virheellinen luku",
    "Operaattoripino tyhj{ - sis{inen virhe",
    "Muuttujapino tyhj{ - sis{inen virhe",
    "Tyyppimuunnos ei onnistu",
    "Virheellinen tyyppi",
    "Liian suuri p{iv{ys",
    "Pinovirhe - sis{inen virhe",
    "Jako nollalla",
    "M{{rittelem{t|n funktio",
    "Odottamaton rivin loppu",
    "Odottamaton tiedoston loppu",
    "Sy|tt|- tai tulostusvirhe",
    "Liian pitk{ rivi",
    "Sis{inen virhe",
    "Virheellinen p{iv{ys",
    "Liian v{h{n argumentteja",
    "Liian paljon argumentteja",
    "Virheellinen aika",
    "Liian suuri luku",
    "Liian pieni luku",
    "Tiedoston avaus ei onnistu",
    "Liian monta sis{kk{ist{ INCLUDEa",
    "J{sennysvirhe",
    "Laukaisuhetken laskenta ei onnistu",
    "Liian monta sis{kk{ist{ IF-lausetta",
    "ELSE ilman IF-lausetta",
    "ENDIF ilman IF-lausetta",
    "Kaikkia viikonp{ivi{ ei voi j{tt{{ pois",
    "Ylim{{r{isi{ merkkej{ rivill{",
    "POP-OMIT-CONTEXT ilman PUSH-OMIT-CONTEXTia",
    "RUN-lauseen k{ytt| estetty",
    "Arvoaluevirhe",
    "Virheellinen tunniste",
    "Rekursiivinen funktiokutsu havaittu",
    "",
    "J{rjestelm{muuttujan muuttaminen ei onnistu",
    "C-kirjastofunktio ei pysty esitt{m{{n p{iv{yst{ tai aikaa",
    "Sis{isen funktion m{{ritelm{{ yritettiin muuttaa",
    "Lausekkeessa ei voi olla sis{kk{isi{ funktiom{{ritelmi{",
    "P{iv{yksen t{ytyy olla t{ydellinen toistokertoimessa",
    "Vuosi annettu kahdesti",
    "Kuukausi annettu kahdesti",
    "P{iv{ annettu kahdesti",
    "Tuntematon sana tai merkki",
    "OMIT-komennossa on annettava kuukausi ja p{iv{",
    "Liian monta osittaista OMIT-komentoa",
    "Liian monta t{ydellist{ OMIT-komentoa",
    "Varoitus: PUSH-OMIT-CONTEXT ilman POP-OMIT-CONTEXTia",
    "Virhe tiedoston luvussa",
    "Pilkku puuttuu",
    "Virheellinen juutalainen p{iv{ys",
    "IIF vaatii parittoman m{{r{n argumentteja",
    "Varoitus: puuttuva ENDIF",
    "Pilkku puuttuu",
    "Viikonp{iv{ annettu kahdesti",
    "K{yt{ vain yht{ komennoista BEFORE, AFTER ja SKIP",
    "Sis{kk{isi{ MSG-, MSF- ja RUN-lauseita ei voi k{ytt{{ lausekkeessa",
    "Toistokerroin annettu kahdesti",
    "Delta-arvo annettu kahdesti",
    "Peruutusarvo annettu kahdesti",
    "ONCE-avainsanaa k{ytetty kahdesti. (Hah.)",
    "AT-sanan per{st{ puuttuu aika",
    "THROUGH/UNTIL-sanaa k{ytetty kahdesti",
    "Ep{t{ydellinen p{iv{ys",
    "FROM/SCANFROM-sanaa k{ytetty kahdesti",
    "Muuttuja",
    "Arvo",
    "*M[[RITTELEM[T\\N*",
    "Siirryt{{n funktioon",
    "Poistutaan funktiosta",
    "Vanhentunut",
    "fork() ep{onnistui - jonomuistutukset eiv{t toimi",
    "Tiedoston avaus ei onnistu",
    "Virheellinen j{rjestelm{p{iv{ys: vuosi on v{hemm{n kuin %d\n",
    "Tuntematon virheenetsint{tarkenne '%c'\n",
    "Tuntematon tarkenne '%c'\n",
    "Tuntematon k{ytt{j{ '%s'\n",
    "Ryhm{numeron vaihto %d:ksi ei onnistunut\n",
    "K{ytt{j{numeron vaihto %d:ksi ei onnistunut\n",
    "Muisti ei riit{ ymp{rist|lle\n",
    "Puuttuva '='-merkki",
    "Puuttuva muuttujanimi",
    "Puuttuva lauseke",
    "P{iv{n asetus %s:ksi ei onnitus\n",
    "Remind: tarkenne '-i': %s\n",
    "Ei viestej{.",
    "%d viesti({) t{m{n p{iv{n jonossa.\n",
    "Numero puuttuu",
    "Virheellinen funktio WARN-lausekkeessa",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"


#endif
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
    fprintf(ErrFp, ">>>> BETAVERSIO <<<<\n");
#endif
#if ISOLATIN1
    fprintf(ErrFp, "K\xE4ytt\xF6: remind [tarkenteet] tiedosto [p\xE4iv\xE4ys] [aika] [*toisto]\n");
    fprintf(ErrFp, "Tarkenteet:\n");
    fprintf(ErrFp, " -n     Tulosta viestien seuraavat esiintymiskerrat yksink. muodossa\n");
    fprintf(ErrFp, " -r     Est\xE4 RUN-lauseiden k\xE4ytt\xF6\n");
    fprintf(ErrFp, " -c[n]  Tulosta n:n kuukauden kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -c+[n] Tulosta n:n viikon kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -w[n[,p[,s]]] Aseta kalenterin leveys, tasaus ja v\xE4lit\n");
    fprintf(ErrFp, " -s[+][n] Tulosta n:n kuukauden (viikon) 'yksink. kalenteri' (oletus 1)\n");
    fprintf(ErrFp, " -p[n]  Kuten -s, mutta tulosta rem2ps:lle sopivassa muodossa\n");
    fprintf(ErrFp, " -v     Laveat tulostukset\n");
    fprintf(ErrFp, " -o     \xC4l\xE4 noudata ONCE-lauseita\n");
    fprintf(ErrFp, " -t     Laukaise kaikki viestit deltan arvosta v\xE4litt\xE4m\xE4tt\xE4\n");
    fprintf(ErrFp, " -h     Suppeat tulostukset\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     \xC4l\xE4 laukaise viestej\xE4 heti - lis\xE4\xE4 ne jonoon\n");
    fprintf(ErrFp, " -q     \xC4l\xE4 lis\xE4\xE4 viestej\xE4 jonoon\n");
    fprintf(ErrFp, " -f     Laukaise viestit, pysy etualalla\n");
    fprintf(ErrFp, " -z[n]  K\xE4ynnisty demonina, her\xE4tys n:n (5:n) minuutin v\xE4lein\n");
#endif
    fprintf(ErrFp, " -d...  Virheenetsint\xE4: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Ohjaa virhetulostus stdout-vuohon\n");
    fprintf(ErrFp, " -b[n]  Ajan ilmaisu: 0=ap/ip, 1=24 tuntia, 2=ei aikoja\n");
    fprintf(ErrFp, " -x[n]  SATISFY-lauseen toistoraja (oletus 150)\n");
    fprintf(ErrFp, " -kcmd  Suorita 'cmd' MSG-tyyppisille viesteille\n");
    fprintf(ErrFp, " -g[ddd] Lajittele viestit p\xE4iv\xE4yksen, ajan ja t\xE4rkeyden mukaan\n");
    fprintf(ErrFp, " -ivar=val Alusta muuttuja var arvolla val ja s\xE4ilyt\xE4 var\n");
    fprintf(ErrFp, " -m     Aloita kalenteri maanantaista eik\xE4 sunnuntaista\n");
    exit(1);
#elif IBMEXTENDED
    fprintf(ErrFp, "K\x84ytt\x94: remind [tarkenteet] tiedosto [p\x84iv\x84ys] [aika] [*toisto]\n");
    fprintf(ErrFp, "Tarkenteet:\n");
    fprintf(ErrFp, " -n     Tulosta viestien seuraavat esiintymiskerrat yksink. muodossa\n");
    fprintf(ErrFp, " -r     Est\x84 RUN-lauseiden k\x84ytt\x94\n");
    fprintf(ErrFp, " -c[n]  Tulosta n:n kuukauden kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -c+[n] Tulosta n:n viikon kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -w[n[,p[,s]]] Aseta kalenterin leveys, tasaus ja v\x84lit\n");
    fprintf(ErrFp, " -s[+][n] Tulosta n:n kuukauden (viikon) 'yksink. kalenteri' (oletus 1)\n");
    fprintf(ErrFp, " -p[n]  Kuten -s, mutta tulosta rem2ps:lle sopivassa muodossa\n");
    fprintf(ErrFp, " -v     Laveat tulostukset\n");
    fprintf(ErrFp, " -o     \x8El\x84 noudata ONCE-lauseita\n");
    fprintf(ErrFp, " -t     Laukaise kaikki viestit deltan arvosta v\x84litt\x84m\x84tt\x84\n");
    fprintf(ErrFp, " -h     Suppeat tulostukset\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     \x8El\x84 laukaise viestej\x84 heti - lis\x84\x84 ne jonoon\n");
    fprintf(ErrFp, " -q     \x8El\x84 lis\x84\x84 viestej\x84 jonoon\n");
    fprintf(ErrFp, " -f     Laukaise viestit, pysy etualalla\n");
    fprintf(ErrFp, " -z[n]  K\x84ynnisty demonina, her\x84tys n:n (5:n) minuutin v\x84lein\n");
#endif
    fprintf(ErrFp, " -d...  Virheenetsint\x84: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Ohjaa virhetulostus stdout-vuohon\n");
    fprintf(ErrFp, " -b[n]  Ajan ilmaisu: 0=ap/ip, 1=24 tuntia, 2=ei aikoja\n");
    fprintf(ErrFp, " -x[n]  SATISFY-lauseen toistoraja (oletus 150)\n");
    fprintf(ErrFp, " -kcmd  Suorita 'cmd' MSG-tyyppisille viesteille\n");
    fprintf(ErrFp, " -g[ddd] Lajittele viestit p\x84iv\x84yksen, ajan ja t\x84rkeyden mukaan\n");
    fprintf(ErrFp, " -ivar=val Alusta muuttuja var arvolla val ja s\x84ilyt\x84 var\n");
    fprintf(ErrFp, " -m     Aloita kalenteri maanantaista eik\x84 sunnuntaista\n");
    exit(1);
#else
    fprintf(ErrFp, "K{ytt|: remind [tarkenteet] tiedosto [p{iv{ys] [aika] [*toisto]\n");
    fprintf(ErrFp, "Tarkenteet:\n");
    fprintf(ErrFp, " -n     Tulosta viestien seuraavat esiintymiskerrat yksink. muodossa\n");
    fprintf(ErrFp, " -r     Est{ RUN-lauseiden k{ytt|\n");
    fprintf(ErrFp, " -c[n]  Tulosta n:n kuukauden kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -c+[n] Tulosta n:n viikon kalenteri (oletus 1)\n");
    fprintf(ErrFp, " -w[n[,p[,s]]] Aseta kalenterin leveys, tasaus ja v{lit\n");
    fprintf(ErrFp, " -s[+][n] Tulosta n:n kuukauden (viikon) 'yksink. kalenteri' (oletus 1)\n");
    fprintf(ErrFp, " -p[n]  Kuten -s, mutta tulosta rem2ps:lle sopivassa muodossa\n");
    fprintf(ErrFp, " -v     Laveat tulostukset\n");
    fprintf(ErrFp, " -o     [l{ noudata ONCE-lauseita\n");
    fprintf(ErrFp, " -t     Laukaise kaikki viestit deltan arvosta v{litt{m{tt{\n");
    fprintf(ErrFp, " -h     Suppeat tulostukset\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     [l{ laukaise viestej{ heti - lis{{ ne jonoon\n");
    fprintf(ErrFp, " -q     [l{ lis{{ viestej{ jonoon\n");
    fprintf(ErrFp, " -f     Laukaise viestit, pysy etualalla\n");
    fprintf(ErrFp, " -z[n]  K{ynnisty demonina, her{tys n:n (5:n) minuutin v{lein\n");
#endif
    fprintf(ErrFp, " -d...  Virheenetsint{: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Ohjaa virhetulostus stdout-vuohon\n");
    fprintf(ErrFp, " -b[n]  Ajan ilmaisu: 0=ap/ip, 1=24 tuntia, 2=ei aikoja\n");
    fprintf(ErrFp, " -x[n]  SATISFY-lauseen toistoraja (oletus 150)\n");
    fprintf(ErrFp, " -kcmd  Suorita 'cmd' MSG-tyyppisille viesteille\n");
    fprintf(ErrFp, " -g[ddd] Lajittele viestit p{iv{yksen, ajan ja t{rkeyden mukaan\n");
    fprintf(ErrFp, " -ivar=val Alusta muuttuja var arvolla val ja s{ilyt{ var\n");
    fprintf(ErrFp, " -m     Aloita kalenteri maanantaista eik{ sunnuntaista\n");
    exit(1);
#endif
}
#endif /* L_IN_INIT */
