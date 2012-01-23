/***************************************************************/
/*                                                             */
/*  POLISH.H                                                   */
/*                                                             */
/*  Support for the Polish language.                           */
/*                                                             */
/*  This file was submitted by Jerzy Sobczyk.  I don't         */
/*  guarantee that there are no mistakes - I don't speak       */
/*  Polish.                                                    */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Polish"

/* Day names */
#if ISOLATIN1
#  define L_SUNDAY "Niedziela"
#  define L_MONDAY "Poniedzia\263ek"
#  define L_TUESDAY "Wtorek"
#  define L_WEDNESDAY "\246roda"
#  define L_THURSDAY "Czwartek"
#  define L_FRIDAY "Pi\261tek"
#  define L_SATURDAY "Sobota"
#else
#  define L_SUNDAY "Niedziela"
#  define L_MONDAY "Poniedzialek"
#  define L_TUESDAY "Wtorek"
#  define L_WEDNESDAY "Sroda"
#  define L_THURSDAY "Czwartek"
#  define L_FRIDAY "Piatek"
#  define L_SATURDAY "Sobota"
#endif

/* Month names */
#if ISOLATIN1
#  define L_JAN "Stycze\361"
#  define L_FEB "Luty"
#  define L_MAR "Marzec"
#  define L_APR "Kwiecie\361"
#  define L_MAY "Maj"
#  define L_JUN "Czerwiec"
#  define L_JUL "Lipiec"
#  define L_AUG "Sierpie\361"
#  define L_SEP "Wrzesie\361"
#  define L_OCT "Pa\274dziernik"
#  define L_NOV "Listopad"
#  define L_DEC "Grudzie\361"
#else
#  define L_JAN "Styczen"
#  define L_FEB "Luty"
#  define L_MAR "Marzec"
#  define L_APR "Kwiecien"
#  define L_MAY "Maj"
#  define L_JUN "Czerwiec"
#  define L_JUL "Lipiec"
#  define L_AUG "Sierpien"
#  define L_SEP "Wrzesien"
#  define L_OCT "Pazdziernik"
#  define L_NOV "Listopad"
#  define L_DEC "Grudzien"
#endif

/* Today and tomorrow */
#define L_TODAY "dzisiaj"
#define L_TOMORROW "jutro"

/* The default banner */
#define L_BANNER "Terminarz na %w, %d. %m %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "temu"
#define L_FROMNOW "od teraz"

/* "in %d days' time" */
#define L_INXDAYS "za %d dni"

/* "on" as in "on date..." */
#define L_ON "-"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
#define L_PLURAL ""

/* Minutes, hours, at, etc */
#define L_NOW "teraz"
#define L_AT "o"
#define L_MINUTE "minut"
#define L_HOUR "godzin"
#if ISOLATIN1
#  define L_IS "b\352dzie"
#  define L_WAS "by\263o"
#else
#  define L_IS "bedzie"
#  define L_WAS "bylo"
#endif
#define L_AND "i"
/* What to add to make "hour" or "minute" plural */
#if ISOLATIN1
#define L_NPLU( N ) ((N == 1) ? "\352" : ((N==12) || (N==13) || (N==14)) ? "" : \
     ((N%10==2) || (N%10==3) || (N%10==4)) ? "y" : "" )
#else
#define L_NPLU( N ) ((N == 1) ? "e" : ((N==12) || (N==13) || (N==14)) ? "" : \
     ((N%10==2) || (N%10==3) || (N%10==4)) ? "y" : "" )
#endif
/* What to add to make "hour" plural */
#define L_HPLU L_NPLU( hdiff )
/* What to add to make "minute" plural */
#define L_MPLU L_NPLU( mdiff )

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */
#if ISOLATIN1
#define L_AMPM_OVERRIDE(ampm, hour) \
ampm = (hour<12) ? \
     (hour<5) ? " w nocy" \
     : (hour<10) ? " rano" \
     : " przed po\263udniem" \
     : (hour<18) ? " po po\263udniu" \
     : (hour<22) ? " wieczorem" \
     : " w nocy";
#else
#define L_AMPM_OVERRIDE(ampm, hour) \
ampm = (hour<12) ? \
(hour<5) ? " w nocy" \
: (hour<10) ? " rano" \
: " przed poludniem" \
: (hour<18) ? " po poludniu" \
: (hour<22) ? " wieczorem" \
: " w nocy";
#endif
#define L_ORDINAL_OVERRIDE		plu = "";
#define L_A_OVER			if (altmode == '*') { sprintf(s, "%s, %d. %s %d", DayName[jul%7], d, MonthName[m], y); } else { sprintf(s, "%s %s, %d. %s %d", L_ON, DayName[jul%7], d, MonthName[m], y); }
#define	L_G_OVER			if (altmode == '*') { sprintf(s, "%s, %d. %s", DayName[jul%7], d, MonthName[m]); } else { sprintf(s, "%s %s, %d. %s", L_ON, DayName[jul%7], d, MonthName[m]); }
#define L_U_OVER			L_A_OVER
#define L_V_OVER			L_G_OVER

#endif /* L_IN_DOSUBST */

#define L_0_OVER sprintf(s, L_HPLU);
#define L_9_OVER sprintf(s, L_MPLU);
#define L_1_OVER \
if (tdiff == 0)  \
sprintf(s, L_NOW); \
else if (tdiff > 0) \
{ \
      if (hdiff == 0)  \
	   sprintf(s, "za %d %s%s", mdiff, L_MINUTE, L_MPLU); \
      else if (mdiff == 0) \
           sprintf(s, "za %d %s%s", hdiff, L_HOUR, L_HPLU); \
      else \
	   sprintf(s, "za %d %s%s %s %d %s%s", hdiff, L_HOUR, L_HPLU, \
		   L_AND, mdiff, L_MINUTE, L_MPLU); \
} \
else \
{ \
   if (hdiff == 0)  \
      sprintf(s, "%d %s%s temu", mdiff, L_MINUTE, L_MPLU); \
   else if (mdiff == 0) \
      sprintf(s, "%d %s%s temu", hdiff, L_HOUR, L_HPLU); \
   else \
      sprintf(s, "%d %s%s %s %d %s%s temu", hdiff, L_HOUR, L_HPLU, \
	      L_AND, mdiff, L_MINUTE, L_MPLU); \
} 

/* The next ones are used only when MK_GLOBALS is set */
#ifdef MK_GLOBALS
#define L_ERR_OVERRIDE 1
EXTERN char *ErrMsg[] =
{
#if ISOLATIN1
    "OK",
    "Brakuj\261cy ']'",
    "Brakuj\261cy nawias",
    "Zbyt skomplikowane wyra\277enie - za du\277o operator\363w",
    "Zbyt skomplikowane wyra\277enie - za du\277o argument\363w",
    "Brakuj\261cy ')'",
    "Nie zdefiniowana funkcja",
    "Nielegalny znak",
    "Spodziewany operator binarny",
    "Brak pami\352ci",
    "Niepoprawny numer",
    "Pusty stos operator\363w - b\263\261d wewn\352trzny",
    "Pusty stos zmiennych - b\263\261d wewn\352trzny",
    "Niemo\277liwa konwersja",
    "B\263\261d typu",
    "Nadmiar daty",
    "B\263\261d stosu - b\263\261d wewn\352trzny",
    "Dzielenie przez zero",
    "Niezdefiniowana zmienna",
    "Niespodziewany koniec linii",
    "Niespodziewany koniec pliku",
    "B\263\261d wejscia/wyjscia",
    "Za d\263uga linia",
    "B\263\261d wewn\352trzny",
    "Z\263a specyfikacja daty",
    "Za ma\263o argument\363w",
    "Za du\277o argument\363w",
    "Nieprawid\263owy czas",
    "Liczba za du\277a",
    "Liczba za ma\263a",
    "Nie mog\352 otworzy\346 pliku",
    "Zbyt zagnie\277d\277one INCLUDE",
    "B\263\261d sk\263adniowy",
    "Nie mog\352 obliczy\346 przypomnienia",
    "Zbyt zagnie\277d\277one IF",
    "ELSE bez IF do pary",
    "ENDIF bez IF do pary",
    "Nie mog\352 omin\261\346 (OMIT) wszystkich dni",
    "Niespodziewany wyraz w lini",
    "POP-OMIT-CONTEXT bez PUSH-OMIT-CONTEXT",
    "Komenda RUN zablokowana",
    "B\263\261d dziedziny",
    "Niepoprawny identyfikator",
    "Wykryto rekursywne wywo\263anie funkcji",
    "",
    "Nie mog\352 zmieni\346 zmiennej systemowej",
    "Funkcja biblioteki C nie mo\277e reprezentowac daty/czasu",
    "Pr\363ba redefinicji funkcji wbudowanej",
    "Nie wolno zagnie\277d\277a\346 definicji funkcji w wyra\277eniu",
    "Aby u\277yc powt\363rzenia trzeba w pe\263ni wyspecyfikowa\346 dat\352",
    "Rok podany dw\363krotnie",
    "Miesi\261c podany dw\363krotnie",
    "Dzie\361 podany dw\363krotnie",
    "Nieznane s\263owo",
    "W komendzie OMIT trzeba poda\346 miesi\261c i dzie\361",
    "Za du\277o cz\352\266ciowych komend OMIT",
    "Za du\277o pe\263nych komend OMIT",
    "Ostrze\277enie: PUSH-OMIT-CONTEXT bez POP-OMIT-CONTEXT",
    "B\263\261d odczytu pliku",
    "Oczekiwany koniec linii",
    "B\263\352dna data hebrajska",
    "IIF wymaga nieparzystej liczby argument\363w",
    "Ostrze\277enie: Brakujacy ENDIF",
    "Oczekiwany przecinek",
    "Dzie\361 tygodnia podany dw\363krotnie",
    "Dozwolone tylko jedno z:  BEFORE, AFTER i SKIP",
    "Nie mo\277na zagnie\277d\277a\346 MSG, MSF, RUN, itp. w wyra\277eniu",
    "Warto\266\346 powtorzenia podana dw\363krotnie",
    "Warto\266\346 r\363\277nicy podana dw\363krotnie",
    "Warto\266\346 cofni\352cia podana dw\363krotnie",
    "S\263owo ONCE u\277yte dw\363krotnie.",
    "Po AT oczekiwany jest czas",
    "S\263owo THROUGH/UNTIL u\277yte dw\363krotnie",
    "Niekompletna specyfikacja daty",
    "S\263owo FROM/SCANFROM u\277yte dw\363krotnie",
    "Zmienna",
    "Warto\266\346",
    "*NIE ZDEFINIOWANE*",
    "Pocz\261tek UserFN",
    "Koniec UserFN",
    "Przemine\263o",
    "Niepowodzenie w funkcji fork() - nie mog\352 kolejkowa\346 przypomnie\361",
    "Nie ma dost\352pu do pliku",
    "B\263\352dna data systemowa: Rok mniejszy ni\277 %d\n",
    "Nieznana flaga odpluskwiania '%c'\n",
    "Nieznana opcja '%c'\n",
    "Nieznany u\277ytkownik '%s'\n",
    "Nie mog\352 zmieni\346 gid na %d\n",
    "Nie mog\352 zmieni\346 uid na %d\n",
    "Brak pami\352ci na zmienne \266rodowiska\n",
    "Brak znaku '='",
    "Brak nazwy zmiennej",
    "Brak wyra\277enia",
    "Nie mog\352 zmieni\346 daty dost\352pu pliku %s\n",
    "Remind: '-i' option: %s\n",
    "Brak przypomnie\361.",
    "%d Przypomnienia zakolejkowane na p\363\274niej.\n",
    "Spodziewana liczba",
    "Illegal function in WARN clause (NEEDS TRANSLATION TO POLISH)",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"
#else /* ISOLATIN1 */
    "OK",
    "Brakujacy ']'",
    "Brakujacy nawias",
    "Zbyt skomplikowane wyrazenie - za duzo operatorow",
    "Zbyt skomplikowane wyrazenie - za duzo argumentow",
    "Brakujacy ')'",
    "Niezdefiniowana funkcja",
    "Nielegalny znak",
    "Spodziewany operator binarny",
    "Brak pamieci",
    "Niepoprawny numer",
    "Pusty stos operatorow - blad wewnetrzny",
    "Pusty stos zmiennych - blad wewnetrzny",
    "Niemozliwa konwersja",
    "Blad typu",
    "Nadmiar daty",
    "Blad stosu - blad wewnetrzny",
    "Dzielenie przez zero",
    "Niezdefiniowana zmienna",
    "Niespodziewany koniec linii",
    "Niespodziewany koniec pliku",
    "Blad wejscia/wyjscia",
    "Za dluga linia",
    "Blad wewnetrzny",
    "Zla specyfikacja daty",
    "Za malo argumentow",
    "Za duzo argumentow",
    "Nieprawidlowy czas",
    "Liczba za duza",
    "Liczba za mala",
    "Nie moge otworzyc pliku",
    "INCLUDE zbyt zagniezdzone",
    "Blad skladniowy",
    "Nie moge obliczyc przypomnienia",
    "Zbyt zagniezdzone IF",
    "ELSE bez IF do pary",
    "ENDIF bez IF do pary",
    "Nie moge ominac (OMIT) wszystkich dni",
    "Niespodziewany wyraz w lini",
    "POP-OMIT-CONTEXT bez PUSH-OMIT-CONTEXT",
    "Komenda RUN zablokowana",
    "Blad dziedziny",
    "Niepoprawny identyfikator",
    "Wykryto rekursywne wywolanie funkcji",
    "",
    "Nie moga zmienic zmiennej systemowej",
    "Funkcja biblioteki C nie moze reprezentowac daty/czasu",
    "Proba redefinicji funkcji wbudowanej",
    "Nie wolno zagniezdzac definicji funkcji w wyrazeniu",
    "Aby uzyc powtorzenia trzeba w pelni wyspecyfikowac date",
    "Rok podany dwokrotnie",
    "Miesiac podany dwokrotnie",
    "Dzien podany dwokrotnie",
    "Nieznane slowo",
    "W komendzie OMIT trzeba podac miesiac i dzien",
    "Za duzo czesciowych komend OMIT",
    "Za duzo pelnych komend OMIT",
    "ostrzezenie: PUSH-OMIT-CONTEXT bez POP-OMIT-CONTEXT",
    "Blad odczytu pliku",
    "Oczekiwany koniec linii",
    "Bledna data hebrajska",
    "IIF wymaga nieparzystej liczby argumentow",
    "Ostrzezenie: Brakujacy ENDIF",
    "Oczekiwany przecinek",
    "Dzien tygodnia podany dwokrotnie",
    "Dozwolone tylko jedno z:  BEFORE, AFTER i SKIP",
    "Nie mozna zagniezdzac MSG, MSF, RUN, itp. w wyrazeniu",
    "Wartosc powtorzenia podana dwokrotnie",
    "Wartosc roznicy podana dwokrotnie",
    "Wartosc cofniecia podana dwokrotnie",
    "Slowo ONCE uzyte dwokrotnie.",
    "Po AT oczekiwany jest czas",
    "Slowo THROUGH/UNTIL uzyte dwokrotnie",
    "Niekompletna specyfikacja daty",
    "Slowo FROM/SCANFROM uzyte dwokrotnie",
    "Zmienna",
    "Wartosc",
    "*UNDEFINED*",
    "Poczatek UserFN",
    "Koniec UserFN",
    "Przeminelo",
    "niepowodzenie w funkcji fork() - nie moge kolejkowac przypomnien",
    "Nie ma dostepu do pliku",
    "Bledna data systemowa: Rok mniejszy niz %d\n",
    "Nieznana flaga odpluskwiania '%c'\n",
    "Nieznana opcja '%c'\n",
    "Nieznany uzytkownik '%s'\n",
    "Nie moge zmienic gid na %d\n",
    "Nie moge zmienic uid na %d\n",
    "Brak pamieci na zmienne srodowiska\n",
    "Brak znaku '='",
    "Brak nazwy zmiennej",
    "Brak wyrazenia",
    "Nie moge zmienic daty dostepu pliku %s\n",
    "Remind: '-i' option: %s\n",
    "Brak przypomnien.",
    "%d Przypomnienia zakolejkowane na pozniej.\n",
    "Spodziewana liczba",
    "Illegal function in WARN clause (NEEDS TRANSLATION TO POLISH)",
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
    fprintf(ErrFp, "\nSpos\363b u\277ycia: remind [opcje] plik [data] [czas] [*powt\363rzenie]\n");
    fprintf(ErrFp, "Opcje:\n");
    fprintf(ErrFp, " -n     Wypisz nast\352pne przypomnienia w prostym formacie\n");
    fprintf(ErrFp, " -r     Zablokuj dyrektywy RUN\n");
    fprintf(ErrFp, " -c[n]  Wypisz kalendarz na n (domy\266lnie 1) miesi\352cy\n");
    fprintf(ErrFp, " -c+[n] Wypisz kalendarz na n (domy\266lnie 1) tygodni\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Ustaw szeroko\266\346, wype\263nienie i odst\352py w kalendarzu\n");
    fprintf(ErrFp, " -s[+][n] Wypisz uproszczony kalendarz na n (1) miesi\352cy (tygodni)\n");
    fprintf(ErrFp, " -p[n]  To samo co -s, ale kompatybilne z rem2ps\n");
    fprintf(ErrFp, " -v     Obszerniejsze komentarze\n");
    fprintf(ErrFp, " -o     Ignoruj instrukcje ONCE\n");
    fprintf(ErrFp, " -t     Odpal wszystkie przysz\263e przypomnienia niezale\277nie od delty\n");
    fprintf(ErrFp, " -h     Praca bezszmerowa\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Nie odpalaj przyponie\361 czasowych - kolejkuj je\n");
    fprintf(ErrFp, " -q     Nie kolejkuj przyponie\361 czasowych\n");
    fprintf(ErrFp, " -f     Nie przechod\274 do pracy w tle\n");
    fprintf(ErrFp, " -z[n]  Pracuj jako demon, budz\261c si\352 co n (5) minut\n");
#endif
    fprintf(ErrFp, " -d...  Odpluskwianie: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Komunikaty o b\263\352dach skieruj na stdout\n");
    fprintf(ErrFp, " -b[n]  Format czasu: 0=am/pm, 1=24godz., 2=\277aden\n");
    fprintf(ErrFp, " -x[n]  Limit powt\363rze\361 klauzuli SATISFY (domy\266lnie=150)\n");
    fprintf(ErrFp, " -kcmd  Wywo\263aj 'cmd' dla przypomnie\361 typu MSG\n");
    fprintf(ErrFp, " -g[ddd] Sortuj przypomnienia wed\263ug daty, czasu i priorytetu\n");
    fprintf(ErrFp, " -ivar=val Zainicjuj zmienn\261 var warto\266cia val i zachowaj ja\n");
    fprintf(ErrFp, " -m     Rozpocznij kalendarz od poniedzia\263ku zamiast od niedzieli\n");
#else /* ISOLATIN1 */
    fprintf(ErrFp, "\nSposob uzycia: remind [opcje] plik [data] [czas] [*powtorzenie]\n");
    fprintf(ErrFp, "Opcje:\n");
    fprintf(ErrFp, " -n     Wypisz nastepne przypomnienia w prostym formacie\n");
    fprintf(ErrFp, " -r     Zablokuj dyrektywy RUN\n");
    fprintf(ErrFp, " -c[n]  Wypisz kalendarz na n (domyslnie 1) miesiecy\n");
    fprintf(ErrFp, " -c+[n] Wypisz kalendarz na n (domyslnie 1) tygodni\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Ustaw szerokosc, wypelnienie i odstepy w kalendarzu\n");
    fprintf(ErrFp, " -s[+][n] Wypisz uproszczony kalendarz na n (1) miesiecy (tygodni)\n");
    fprintf(ErrFp, " -p[n]  To samo co -s, ale kompatybilne z rem2ps\n");
    fprintf(ErrFp, " -v     Obszerniejsze komentarze\n");
    fprintf(ErrFp, " -o     Ignoruj instrukcje ONCE\n");
    fprintf(ErrFp, " -t     Odpal wszystkie przyszle przypomnienia niezaleznie od delty\n");
    fprintf(ErrFp, " -h     Praca bezszmerowa\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Nie odpalaj przyponien czasowych - kolejkuj je\n");
    fprintf(ErrFp, " -q     Nie kolejkuj przyponien czasowych\n");
    fprintf(ErrFp, " -f     Nie przechodz do pracy w tle\n");
    fprintf(ErrFp, " -z[n]  Pracuj jako demon, budzac sie co n (5) minut\n");
#endif
    fprintf(ErrFp, " -d...  Odpluskwianie: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Komunikaty o bledach skieruj na stdout\n");
    fprintf(ErrFp, " -b[n]  Format czasu: 0=am/pm, 1=24godz., 2=zaden\n");
    fprintf(ErrFp, " -x[n]  Limit powtorzen klauzuli SATISFY (domyslnie=150)\n");
    fprintf(ErrFp, " -kcmd  Wywolaj 'cmd' dla przypomnien typu MSG\n");
    fprintf(ErrFp, " -g[ddd] Sortuj przypomnienia wedlug daty, czasu i priorytetu\n");
    fprintf(ErrFp, " -ivar=val Zainicjuj zmienna var wartoscia val i zachowaj ja\n");
    fprintf(ErrFp, " -m     Rozpocznij kalendarz od poniedzialku zamiast od niedzieli\n");
#endif /* ISOLATIN1 */
    exit(1);
}
#endif /* L_IN_INIT */
