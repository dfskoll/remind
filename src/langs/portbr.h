/***************************************************************/
/*                                                             */
/*  PORTBR.H                                                   */
/*                                                             */
/*  Support for the Brazilian Portuguese Language.             */
/*                                                             */
/*  Contributed by Marco Paganini (paganini@ism.com.br).       */
/*                                                             */
/*  This file is part of REMIND.                               */
/*                                                             */
/*  REMIND is Copyright (C) 1992-1998 by David F. Skoll        */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*  This file is Copyright (C) 1996 by Marco Paganini and      */
/*  David F. Skoll.                                            */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Brazilian Portuguese"

/* Day names */
#define L_SUNDAY "domingo"
#define L_MONDAY "segunda"
#define L_TUESDAY "terca"
#define L_WEDNESDAY "quarta"
#define L_THURSDAY "quinta"
#define L_FRIDAY "sexta"
#define L_SATURDAY "sabado"

/* Month names */
#define L_JAN "janeiro"
#define L_FEB "fevereiro"
#define L_MAR "marco"
#define L_APR "abril"
#define L_MAY "maio"
#define L_JUN "junho"
#define L_JUL "julho"
#define L_AUG "agosto"
#define L_SEP "setembro"
#define L_OCT "outubro"
#define L_NOV "novembro"
#define L_DEC "dezembro"

/* Today and tomorrow */
#define L_TODAY "hoje"
#define L_TOMORROW "amanha"

/* The default banner */
#define L_BANNER "Avisos para %w, %d de %m de %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "atras"
#define L_FROMNOW "adiante"

/* "in %d days' time" */
#define L_INXDAYS "em %d dias"

/* "on" as in "on date..." */
#define L_ON "em"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
#define L_PLURAL "s"

/* Minutes, hours, at, etc */
#define L_NOW "agora"
#define L_AT "as"
#define L_MINUTE "minuto"
#define L_HOUR "hora"
#define L_IS "sao"
#define L_WAS "eram"
#define L_AND "e"

/* What to add to make "hour" plural */
#define L_HPLU "s"  
/* What to add to make "minute" plural */
#define L_MPLU "s"

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */

#define L_ORDINAL_OVERRIDE plu = "";

/* Portuguese weekdays must be treated separately */
#define _ON_WEEKDAY(x) ((x % 7) < 2) ? "no" : "na"

#define L_A_OVER \
    sprintf(s, "%s %s, %d de %s de %d", _ON_WEEKDAY(jul), DayName[jul%7], d, MonthName[m], y);

#define L_C_OVER \
	sprintf(s, "%s %s", _ON_WEEKDAY(jul), DayName[jul%7]);

#define L_G_OVER \
	sprintf(s, "%s %s, %d %s", _ON_WEEKDAY(jul), DayName[jul%7], d, MonthName[m]);

#define L_J_OVER \
    sprintf(s, "%s %s, %d de %s de %d", _ON_WEEKDAY(jul), DayName[jul%7], d, MonthName[m], y);
                    
#define L_K_OVER \
    sprintf(s, "%s %s, %d de %s", _ON_WEEKDAY(jul), DayName[jul%7], d, MonthName[m]);

/* Portuguese does not use some suffixes, some some %u and %j are the same */
#define L_U_OVER \
    sprintf(s, "%s %s, %d de %s de %d", _ON_WEEKDAY(jul), DayName[jul%7], d, MonthName[m], y);

#define L_V_OVER \
    sprintf(s, "%s %s, %d de %s", _ON_WEEKDAY(jul), DayName[jul%7], d, MonthName[m]);

#define L_1_OVER \
{ \
    if (tdiff == 0) \
	    sprintf(s, L_NOW); \
    else \
    if (hdiff == 0) \
    { \
    	if (mdiff > 0) \
	        sprintf(s, "em %d %s%s", mdiff, L_MINUTE, mplu); \
	    else \
	        sprintf(s, "%d %s%s atras", mdiff, L_MINUTE, mplu); \
	} \
    else if (mdiff == 0) \
    { \
    	if (hdiff > 0) \
        	sprintf(s, "em %d %s%s", hdiff, L_HOUR, hplu); \
        else \
        	sprintf(s, "%d %s%s atras", hdiff, L_HOUR, hplu); \
    } else { \
		if (tdiff > 0) \
        	sprintf(s, "em %d %s%s %s %d %s%s", hdiff, L_HOUR, hplu, L_AND, mdiff, L_MINUTE, mplu);  \
        else \
        	sprintf(s, "%d %s%s %s %d %s%s atras", hdiff, L_HOUR, hplu, L_AND, mdiff, L_MINUTE, mplu); \
    } \
}

#endif /* L_IN_DOSUBST */

/* The next ones are used only when MK_GLOBALS is set */
#ifdef MK_GLOBALS
#define L_ERR_OVERRIDE 1
EXTERN char *ErrMsg[] =
{
    "Ok",
    "Falta um ']'",
    "Falta uma aspa",
    "Expressao muito complexa - muitos operadores",
    "Expressao muito complexa - muitos operandos",
    "Falta um ')'",
    "Funcao nao definida",
    "Caracter ilegal",
    "Esperando operador binario",
    "Sem memoria",
    "Numero mal-formado",
    "Op stack underflow - erro interno",
    "Va stack underflow - erro interno",
    "Nao consigo fazer 'coerce'",
    "Type mismatch",
    "Overflow na data",
    "Erro de stack - erro interno",
    "Divisao por zero",
    "Variavel nao definida",
    "Fim da linha nao esperado",
    "Fim de arquivo nao esperado",
    "Erro de I/O",
    "Linha muito longa",
    "Erro interno",
    "Especificacao de data invalida",
    "Argumentos insuficientes",
    "Argumentos em excesso",
    "Hora mal-formada",
    "Numero muito grande",
    "Numero muito pequeno",
    "Nao consigo abrir o arquivo",
    "Ninho de INCLUDEs muito profundo",
    "Erro de parsing",
    "Nao consigo computar o 'trigger'",
    "Muitos IFs aninhados",
    "ELSE sem o IF correspondente",
    "ENDIF sem o IF correspondente",
    "Nao se pode usar OMIT para todos os dias da semana",
    "Token nao reconhecido na linha",
    "POP-OMIT-CONTEXT sem PUSH-OMIT-CONTEXT correspondente",
    "RUN desabilitado",
    "Erro de dominio",
    "Identificados invalido",
    "Chamada de funcao recursiva detectada",
    "",
    "Nao posso modificar variavel de sistema",
    "Funcao da biblioteca C nao pode representar data/hora",
    "Tentativa de redefinir funcao interna",
    "Nao e' possivel aninhar definicao de funcao em expressao",
    "Data deve ser completamente especificada para usar o fator de REPEAT",
    "Ano especificado duas vezes",
    "Mes especificado duas vezes",
    "Dia especificado duas vezes",
    "Token desconhecido",
    "Mes e dia devem ser especificados no comando OMIT",
    "Muitos OMITs parciais",
    "Muitos OMITs full",
    "Aviso: PUSH-OMIT-CONTEXT sem POP-OMIT-CONTEXT correspondente",
    "Erro na leitura do arquivo",
    "Aguardando fim do arquivo",
    "Data hebraica invalida",
    "IIF necessita de numero impar de argumentos",
    "Warning: ENDIF faltando",
    "Esperando virgula",
    "Dia da semana especificado duas vezes",
    "Use apenas um de BEFORE, AFTER ou SKIP",
    "Nao e possivel aninhar MSG, MSF, RUN, etc. em expressoes",
    "Valor de Repeat especificado duas vezes",
    "Valor de Delta especificado duas vezes",
    "Valor de Back especificado duas vezes",
    "ONCE usado duas vezes (Eheheh)",
    "Esperando hora apos AT",
    "Keyword THROUGH/UNTIL usada duas vezes",
    "Especificacao de data incompleta",
    "Keyword FROM/SCANFROM usada duas vezes",
    "Variavel",
    "Valor",
    "*INDEFINIDO*",
    "Entrando UserFN",
    "Saindo UserFN",
    "Expirou",
    "fork() falhou - Nao posso processar compromissos na fila",
    "Nao consigo acessar o arquivo",
    "Data do sistema ilegal: Ano e menor que %d\n",
    "Flag de debug desconhecido '%c'\n",
    "Opcao desconhecida '%c'\n",
    "Usuario desconhecido '%s'\n",
    "Nao consigo mudar gid para %d\n",
    "Nao consigo mudar uid para %d\n",
    "Sem memoria para o environment\n",
    "Falta o sinal de '='",
    "Falta o nome da variavel",
    "Falta a expressao",
    "Nao consigo resetar a data de acesso de %s\n",
    "Remind: '-i' opcao: %s\n",
    "Sem compromissos.",
    "%d compromisso(s) colocados na fila para mais tarde.\n",
    "Esperando numero",
    "Funcao ilegal na clausula WARN",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"
};
#endif /* MK_GLOBALS */

/* The following is only used in init.c */
#ifdef L_IN_INIT
#define L_USAGE_OVERRIDE 1
void Usage(void)
{
    fprintf(ErrFp, "\nREMIND %s (versao %s) (C) 1992-1998 David F. Skoll\n", VERSION, L_LANGNAME);
    fprintf(ErrFp, "(C) 1999-2000 Roaring Penguin Software Inc.\n");
#ifdef BETA
    fprintf(ErrFp, ">>>> VERSAO BETA <<<<\n");
#endif
    fprintf(ErrFp, "Uso: remind [opcoes] arquivo [data] [hora] [*rep]\n");
    fprintf(ErrFp, "Opcoes:\n");
    fprintf(ErrFp, " -n     Imprime a proxima ocorrencia em formato simples\n");
    fprintf(ErrFp, " -r     Desabilita a diretiva RUN\n");
    fprintf(ErrFp, " -c[n]  Produz calendario para n (default 1) meses\n");
    fprintf(ErrFp, " -c+[n] Produz calendario para n (default 1) semanas\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Especifica largura, preenchimento e espacejamento do calendario\n");
    fprintf(ErrFp, " -s[+][n] Produz um `calendario simples' para n (1) meses (semanas)\n");
    fprintf(ErrFp, " -p[n]  Identico a -s, porem com saida compativel com rem2ps\n");
    fprintf(ErrFp, " -v     Modo verbose\n");
    fprintf(ErrFp, " -o     Ignora diretivas ONCE\n");
    fprintf(ErrFp, " -t     Aciona todos os compromissos futuros, sem considerar o delta\n");
    fprintf(ErrFp, " -h     Modo `Hush' - quieto\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Nao aciona compromissos com hora imediatamente - apenas coloca na fila\n");
    fprintf(ErrFp, " -q     Nao coloca compromissos com hora na fila\n");
    fprintf(ErrFp, " -f     Aciona compromissos com hora em modo foreground\n");
    fprintf(ErrFp, " -z[n]  Modo `daemon', acordando a cada n (5) minutos.\n");
#endif
    fprintf(ErrFp, " -d...  Debug: e=echo x=expr-eval t=trigger v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Desvia mensagens normalmente enviadas a stderr para stdout\n");
    fprintf(ErrFp, " -b[n]  Formato da hora para o cal: 0=am/pm, 1=24hr, 2=nenhum\n");
    fprintf(ErrFp, " -x[n]  Limite de iteracoes para a clausula SATISFY (default=150)\n");
    fprintf(ErrFp, " -kcmd  Executa `cmd' para os compromissos com MSG\n");
    fprintf(ErrFp, " -g[ddd] Classifica compromissos por data, hora e prioridade antes de exibir\n");
    fprintf(ErrFp, " -ivar=val Inicializa (e preserva) variavel var com val\n");
    fprintf(ErrFp, " -m     Inicia o calendario na segunda, ao inves de domingo\n");
    exit(1);
}
#endif /* L_IN_INIT */
