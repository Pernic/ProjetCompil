#include <stdlib.h>

/* deux macros pratiques, utilisees dans les allocations */
#define NEW(howmany, type) (type *) calloc((unsigned) howmany, sizeof(type))
#define NIL(type) (type *) 0

#define TRUE 1
#define FALSE 0 

typedef int bool;

/* la structure d'un arbre (noeud ou feuille) */
typedef struct _Tree {
  short op;         /* etiquette de l'operateur courant */
  short nbChildren; /* nombre de sous-arbres */
  union {
    char *str;      /* valeur de la feuille si op = ID ou STR */
    int val;        /* valeur de la feuille si op = CST */
    struct _Tree **children; /* tableau des sous-arbres dans les autres cas*/
  } u;
} Tree, *TreeP;


/* la structure ci-dessous permet de memoriser des listes variable/valeur */
typedef struct _Decl
{ char *name;
  int val;
  struct _Decl *next;
} VarDecl, *VarDeclP;


/* Etiquettes additionnelles pour les arbres de syntaxe abstraite.
 * Les tokens tels que ADD, SUB, etc. servent directement d'etiquette.
 * Attention a ne pas donner des valeurs identiques a celles des tokens
 * produits par Bison dans le fichier tp_y.h
 */
#define NE 1
#define EQ 2
#define LT 3
#define LE 4
#define GT 5
#define GE 6
#define LIST 7

/* Codes d'erreurs */
#define NO_ERROR	0
#define USAGE_ERROR	1
#define LEXICAL_ERROR	2
#define SYNTAX_ERROR    3
#define CONTEXT_ERROR	4
#define EVAL_ERROR	5
#define UNEXPECTED	10


/* Type pour la valeur de retour de Flex et les actions de Bison
 * le premier champ est necessaire pour flex
 * les autres correspondent aux variantes utilisees dans les actions
 * associees aux productions de la grammaire. 
*/
typedef union
{ char C;
  char *S;
  int I; 
  TreeP T;
} YYSTYPE;

#define YYSTYPE YYSTYPE

/* construction des declarations */
VarDeclP makeVar(char *name);
VarDeclP declVar(char *name, TreeP tree, VarDeclP currentScope);

/* construction pour les arbres */
TreeP makeLeafStr(short etiquette, char *str);
TreeP makeLeafInt(short etiquette, int val);
TreeP makeTree(short etiquette, int nbChildren, ...);

/* evaluateur d'expressions */
int evalMain(TreeP tree, VarDeclP lvar);
VarDeclP evalDecls (TreeP tree);

/* ecriture formatee */
void pprintVar(VarDeclP decl, TreeP tree);
void pprintValueVar(VarDeclP decl);
void pprint(TreeP tree);
void pprintMain(TreeP);
