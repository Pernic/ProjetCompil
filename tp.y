/* les tokens ici sont ceux supposes etre renvoyes par l'analyseur lexical
 * A adapter par chacun en fonction de ce qu'il a ecrit dans tp.l
 * Bison ecrase le contenu de tp_y.h a partir de la description de la ligne
 * suivante. C'est donc cette ligne qu'il faut adapter si besoin, pas tp_y.h !
 */
%token IF THEN ELSE BEG END ADD SUB DECL_LIST AFFECT MUL
%token <S> ID	/* voir %type ci-dessous pour le sens de <S> et Cie */
%token <I> CST RELOP EQ2 NE2 LE2 GE2 GT2 LT2

/* indications de precedence d'associativite. Les operateurs sur une meme
 * ligne (separes par un espace) ont la meme priorite. Les ligns sont donnees
 * par precedence croissante d'operateurs.
 */
%left ADD SUB

/* voir la definition de YYSTYPE dans main.h 
 * Les indications ci-dessous servent a indiquer a Bison que les "valeurs" $i
 * ou $$ associees a ces non-terminaux doivent utiliser la variante indiquee
 * de l'union YYSTYPE (par exemple la variante D ou S, etc.)
 * La "valeur" associee a un terminal utilise toujours la meme variante
 */
%type <T> expr bexpr decl declL

%{
#include "tp.h"
#include <stdio.h>     /* les definition des types et les etiquettes des noeuds */

extern int yylex();	/* fournie par Flex */
extern void yyerror();  /* definie dans tp.c */

Tree t;

%}

%% 
 /*
 * Attention: on est dans un analyseur ascendant donc on s'occupe des composants
 * d'une construction avant de traiter la construction elle-meme.
 *
 * Les macros d'allocation NEW et de nullite NIL sont definies dans tp.h.
 * Leur usage n'est bien sur pas obligatoire, juste conseille (elles typent
 * explicitement les pointeurs, ce qui permet de reveler certaines fautes de conception).
 *
 * Les definition des types YYSTYPE, VarDecl, VarDeclP, Tree, TreeP et autres
 * sont dans tp.h
 */

 /* "programme" est l'axiome de la grammaire */
programme : declL BEG expr END {printf("Prog !!!!!!!!!!! \n");evalMain($3,$1);}
;

/* Une liste eventuellement vide de declarations de variables */
declL : decl { $$ = $1;}
| decl declL { $$ = addToScope($1,$2);}
| declL declL { $$ = addToScope($1,$2);}
| declL decl { $$ = addToScope($1,$2);}
;


/* une declaration de variable ou de fonction, terminee par un ';'. */
decl : ID ';' { $$ = makeVar($1);}
| ID AFFECT expr ';' { $$ = evalDecls($3);}
;


/* les appels ci-dessous creent un arbre de syntaxe abstraite pour l'expression
 * arithmetique. On rappelle que la methode est ascendante, donc les arbres
 * des operandes sont deja construits au moment de rajouter le noeud courant.
 * Dans la premiere regle, par exemple, $2, $4 et $6 representent donc
 * les arbres qui sont les composants d'un if-then-else.
 * la fonction makeTree est definie dans tp.c et prend un nombre variables
 * d'arguments (au moins 2). Le premier est l'etiquette du noeud a construire,
 * le second est le nombre de fils.
 */
expr : IF bexpr THEN expr ELSE expr
    { $$ = makeTree(IF, 3, $2, $4, $6); }
|IF bexpr THEN expr
    { $$ = makeTree(IF,2,$2,$4);}
| expr ADD expr
    { $$ = makeTree(ADD, 2, $1, $3); }
| expr SUB expr
    { $$ = makeTree(SUB, 2, $1, $3); }
| expr MUL expr
    { $$ = makeTree(MUL, 2, $1, $3); }
|expr AFFECT expr
    { $$ = makeTree(AFFECT,2,$1,$3);}
| CST
    { $$ = makeLeafInt(CST, $1); }
| ID
    { $$ = makeLeafStr(ID, $1); }
| '(' expr ')'
    { $$ = $2; }
;

/* Expression booleenne: il n'y a pas de booleen dans le langage. Ces expressions
 * ne peuvent apparaitre que comme conditions dans un if-teh-else.
 */ 

bexpr : expr EQ2 expr 
    { $$ = makeTree(EQ2, 2, $1, $3); }
| expr NE2 expr 
    { $$ = makeTree(NE2, 2, $1, $3); }
| expr LE2 expr 
    { $$ = makeTree(LE2, 2, $1, $3); }
| expr GE2 expr 
    { $$ = makeTree(GE2, 2, $1, $3); }
| expr GT2 expr 
    { $$ = makeTree(GT2, 2, $1, $3); }
| expr LT2 expr 
    { $$ = makeTree(LT2, 2, $1, $3); }
| '(' bexpr ')'
    { $$ = $2; }
;

