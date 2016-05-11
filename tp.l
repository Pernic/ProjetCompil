Digit [0-9]
Letter  [a-zA-Z]
LD  ({Letter}|{Digit})
Star \*
NotStar [^*]
NotStarSlash [^*/]
Delim [ \n\t\r]
Symbol [;,]

%{
#include "tp.h"
#include "tp_y.h"
#include <string.h>	/* pour strdup */

extern char *strdup(const char *s);
extern void setError(int code);

extern YYSTYPE yylval;
extern int errorCode;

/* Il est plus economique de ne pas creer une expression reguliere pour chaque
 * mot-clef mais plutot de les reconnaitre dans un premier temps comme des
 * identificateurs et de faire la distinction par un bout de code
 */
int keywordOrId() { 
  /* si ce n'est pas un mot-clef, sauvegarder le texte de l'identificateur
   * et renvoyer le token associe aux identificateurs
   */
  yylval.S = strdup(yytext);
  return(ID);
}


void lexError(char c) {
  fprintf(stderr, "Error: Skipping invalid character '%c' on line %d\n",
          c, yylineno);
  setError(LEXICAL_ERROR);
}


/* les codes symboliques pour les unites lexicales (CST, IF, ADD, ...) sont
 * definis dans tp_y.h et serviront aussi d'etiquettes pour les sommets de
 * l'arbre de syntaxe abstraite qui sera construit par la suite du processus
 * de compilation.
 * 
 * Les "return" qui apparaissent dans les actions associees aux expressions
 * reguleres peuvent renvoyer:
 * - soit le code symbolique d'une U.L.
 * - soit un simple caractere
 * L'eventuelle partie "valeur" de l'U.L. doit etre stockee dans une des
 * variantes de la variable yylval (definie comme etant de type YYSTYPE)
 * afin que les outils compagnons de l'analyseur lexical puisse l'y recuperer.
 * Attention: le contenu de yylval va etre ecrase a chaque appel a l'analyseur
 * lexical et doit donc etre sauvegarde si besoin.
 */
%}

%%
"+"             { return(ADD); }
"-"             { return(SUB); }
":="        	{ return(AFFECT); }
"="             { yylval.C = EQ; return(EQ2); }
"<>"            { yylval.C = NE; return(NE2); }
"<="            { yylval.C = LE; return(LE2); }
">="            { yylval.C = GE; return(GE2); }
">"             { yylval.C = GT; return(GT2); }
"<"             { yylval.C = LT; return(LT2); }
"*"		{ return(MUL);}
"begin"		{ return(BEG); }
"end"		{ return(END); }
"if"		{ return(IF); }
"else"		{ return(ELSE); }
"then"		{ return(THEN); }
{Digit}+        { yylval.I = strtol(yytext, NULL, 0); return(CST); }
{Letter}{LD}*   { yylval.S = yytext;return keywordOrId(); }
{Symbol}        { return(yytext[0]); }
" "		{}
'\n'		{}
. {printf("Erreur lexeur %s\n", yytext);}
%%
