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
/*  Copyright (C) 1992-2022 by Dianne Skoll                    */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Polish"

/* Day names */
#  define L_SUNDAY "Niedziela"
#  define L_MONDAY "Poniedziałek"
#  define L_TUESDAY "Wtorek"
#  define L_WEDNESDAY "Środa"
#  define L_THURSDAY "Czwartek"
#  define L_FRIDAY "Piątek"
#  define L_SATURDAY "Sobota"

/* Month names */
#  define L_JAN "Styczeń"
#  define L_FEB "Luty"
#  define L_MAR "Marzec"
#  define L_APR "Kwiecień"
#  define L_MAY "Maj"
#  define L_JUN "Czerwiec"
#  define L_JUL "Lipiec"
#  define L_AUG "Sierpień"
#  define L_SEP "Wrzesień"
#  define L_OCT "Październik"
#  define L_NOV "Listopad"
#  define L_DEC "Grudzień"

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
#  define L_IS "będzie"
#  define L_WAS "było"
#define L_AND "i"
/* What to add to make "hour" or "minute" plural */
#define L_NPLU( N ) ((N == 1) ? "ę" : ((N==12) || (N==13) || (N==14)) ? "" : \
     ((N%10==2) || (N%10==3) || (N%10==4)) ? "y" : "" )
/* What to add to make "hour" plural */
#define L_HPLU L_NPLU( hdiff )
/* What to add to make "minute" plural */
#define L_MPLU L_NPLU( mdiff )

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */
#define L_AMPM_OVERRIDE(ampm, hour) \
ampm = (hour<12) ? \
     (hour<5) ? " w nocy" \
     : (hour<10) ? " rano" \
     : " przed południem" \
     : (hour<18) ? " po południu" \
     : (hour<22) ? " wieczorem" \
     : " w nocy";
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
    "OK",
    "Brakujący ']'",
    "Brakujący nawias",
    "Zbyt skomplikowane wyrażenie - za dużo operatorów",
    "Zbyt skomplikowane wyrażenie - za dużo argumentów",
    "Brakujący ')'",
    "Nie zdefiniowana funkcja",
    "Nielegalny znak",
    "Spodziewany operator binarny",
    "Brak pamięci",
    "Niepoprawny numer",
    "Pusty stos operatorów - błąd wewnętrzny",
    "Pusty stos zmiennych - błąd wewnętrzny",
    "Niemożliwa konwersja",
    "Błąd typu",
    "Nadmiar daty",
    "Błąd stosu - błąd wewnętrzny",
    "Dzielenie przez zero",
    "Niezdefiniowana zmienna",
    "Niespodziewany koniec linii",
    "Niespodziewany koniec pliku",
    "Błąd wejscia/wyjscia",
    "Za długa linia",
    "Błąd wewnętrzny",
    "Zła specyfikacja daty",
    "Za mało argumentów",
    "Za dużo argumentów",
    "Nieprawidłowy czas",
    "Liczba za duża",
    "Liczba za mała",
    "Nie mogę otworzyć pliku",
    "Zbyt zagnieżdżone INCLUDE",
    "Błąd składniowy",
    "Nie mogę obliczyć przypomnienia",
    "Zbyt zagnieżdżone IF",
    "ELSE bez IF do pary",
    "ENDIF bez IF do pary",
    "Nie mogę ominąć (OMIT) wszystkich dni",
    "Niespodziewany wyraz w lini",
    "POP-OMIT-CONTEXT bez PUSH-OMIT-CONTEXT",
    "Komenda RUN zablokowana",
    "Błąd dziedziny",
    "Niepoprawny identyfikator",
    "Wykryto rekursywne wywołanie funkcji",
    "",
    "Nie mogę zmienić zmiennej systemowej",
    "Funkcja biblioteki C nie może reprezentowac daty/czasu",
    "Próba redefinicji funkcji wbudowanej",
    "Nie wolno zagnieżdżać definicji funkcji w wyrażeniu",
    "Aby użyc powtórzenia trzeba w pełni wyspecyfikować datę",
    "Rok podany dwókrotnie",
    "Miesiąc podany dwókrotnie",
    "Dzień podany dwókrotnie",
    "Nieznane słowo",
    "W komendzie OMIT trzeba podać miesiąc i dzień",
    "Za dużo częściowych komend OMIT",
    "Za dużo pełnych komend OMIT",
    "Ostrzeżenie: PUSH-OMIT-CONTEXT bez POP-OMIT-CONTEXT",
    "Błąd odczytu pliku",
    "Oczekiwany koniec linii",
    "Błędna data hebrajska",
    "IIF wymaga nieparzystej liczby argumentów",
    "Ostrzeżenie: Brakujacy ENDIF",
    "Oczekiwany przecinek",
    "Dzień tygodnia podany dwókrotnie",
    "Dozwolone tylko jedno z:  BEFORE, AFTER i SKIP",
    "Nie można zagnieżdżać MSG, MSF, RUN, itp. w wyrażeniu",
    "Wartość powtorzenia podana dwókrotnie",
    "Wartość różnicy podana dwókrotnie",
    "Wartość cofnięcia podana dwókrotnie",
    "Słowo ONCE użyte dwókrotnie.",
    "Po AT oczekiwany jest czas",
    "Słowo THROUGH/UNTIL użyte dwókrotnie",
    "Niekompletna specyfikacja daty",
    "Słowo FROM/SCANFROM użyte dwókrotnie",
    "Zmienna",
    "Wartość",
    "*NIE ZDEFINIOWANE*",
    "Początek UserFN",
    "Koniec UserFN",
    "Przemineło",
    "Niepowodzenie w funkcji fork() - nie mogę kolejkować przypomnień",
    "Nie ma dostępu do pliku",
    "Błędna data systemowa: Rok mniejszy niż %d\n",
    "Nieznana flaga odpluskwiania '%c'\n",
    "Nieznana opcja '%c'\n",
    "Nieznany użytkownik '%s'\n",
    "Nie mogę zmienić gid na %d\n",
    "Nie mogę zmienić uid na %d\n",
    "Brak pamięci na zmienne środowiska\n",
    "Brak znaku '='",
    "Brak nazwy zmiennej",
    "Brak wyrażenia",
    "Nie mogę zmienić daty dostępu pliku %s\n",
    "Remind: '-i' option: %s\n",
    "Brak przypomnień.",
    "%d Przypomnienia zakolejkowane na później.\n",
    "Spodziewana liczba",
    "Illegal function in WARN clause (NEEDS TRANSLATION TO POLISH)",
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
    fprintf(ErrFp, "\nSposób użycia: remind [opcje] plik [data] [czas] [*powtórzenie]\n");
    fprintf(ErrFp, "Opcje:\n");
    fprintf(ErrFp, " -n     Wypisz następne przypomnienia w prostym formacie\n");
    fprintf(ErrFp, " -r     Zablokuj dyrektywy RUN\n");
    fprintf(ErrFp, " -c[n]  Wypisz kalendarz na n (domyślnie 1) miesięcy\n");
    fprintf(ErrFp, " -c+[n] Wypisz kalendarz na n (domyślnie 1) tygodni\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Ustaw szerokość, wypełnienie i odstępy w kalendarzu\n");
    fprintf(ErrFp, " -s[+][n] Wypisz uproszczony kalendarz na n (1) miesięcy (tygodni)\n");
    fprintf(ErrFp, " -p[n]  To samo co -s, ale kompatybilne z rem2ps\n");
    fprintf(ErrFp, " -v     Obszerniejsze komentarze\n");
    fprintf(ErrFp, " -o     Ignoruj instrukcje ONCE\n");
    fprintf(ErrFp, " -t     Odpal wszystkie przyszłe przypomnienia niezależnie od delty\n");
    fprintf(ErrFp, " -h     Praca bezszmerowa\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Nie odpalaj przyponień czasowych - kolejkuj je\n");
    fprintf(ErrFp, " -q     Nie kolejkuj przyponień czasowych\n");
    fprintf(ErrFp, " -f     Nie przechodź do pracy w tle\n");
    fprintf(ErrFp, " -z[n]  Pracuj jako demon, budząc się co n (5) minut\n");
#endif
    fprintf(ErrFp, " -d...  Odpluskwianie: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Komunikaty o błędach skieruj na stdout\n");
    fprintf(ErrFp, " -b[n]  Format czasu: 0=am/pm, 1=24godz., 2=żaden\n");
    fprintf(ErrFp, " -x[n]  Limit powtórzeń klauzuli SATISFY (domyślnie=1000)\n");
    fprintf(ErrFp, " -kcmd  Wywołaj 'cmd' dla przypomnień typu MSG\n");
    fprintf(ErrFp, " -g[ddd] Sortuj przypomnienia według daty, czasu i priorytetu\n");
    fprintf(ErrFp, " -ivar=val Zainicjuj zmienną var wartościa val i zachowaj ja\n");
    fprintf(ErrFp, " -m     Rozpocznij kalendarz od poniedziałku zamiast od niedzieli\n");
    exit(1);
}
#endif /* L_IN_INIT */
