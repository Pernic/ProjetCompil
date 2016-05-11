#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "tp.h"
#include "tp_y.h"

#ifdef DEBUG
 #define DEBUG(x) x
#else
  #define DEBUG(x) if(0) 0;
#endif
/*
 * Toute cette premiere partie n'a (normalement) pas besoin d'etre modifiee
 */

extern int yyparse();
extern int yylineno;

int eval(TreeP tree, VarDeclP decls);
TreeP getChild(TreeP tree, int rank);

/* Niveau de 'verbosite'.
 * Par defaut, n'imprime que le resultat et les messages d'erreur
 */
bool verbose = FALSE;

/* Evaluation ou pas. Par defaut, on evalue les expressions */
bool noEval = FALSE;

/* code d'erreur a retourner */
int errorCode = NO_ERROR;

FILE *fd = NIL(FILE);

/* Appel:
 *   tp [-option]* programme.txt
 * Les options doivent apparaitre avant le nom du fichier du programme.
 * Options: -[eE] -[vV] -[hH?]
 */
int main(int argc, char **argv) {

  DEBUG(printf("Debug enabled\n");)
  int fi;
  int i, res;

  if (argc == 1) {
    fprintf(stderr, "Syntax: tp -e -v program.txt\n");
    exit(USAGE_ERROR);
  }
  for(i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'v': case 'V':
	verbose = TRUE; continue;
      case 'e': case 'E':
	noEval = TRUE; continue;
      case '?': case 'h': case 'H':
	fprintf(stderr, "Syntax: tp -e -v program.txt\n");
	exit(USAGE_ERROR);
      default:
	fprintf(stderr, "Error: Unknown Option: %c\n", argv[i][1]);
	exit(USAGE_ERROR);
      }
    } else break;
  }

  if (i == argc) {
    fprintf(stderr, "Error: Program file is missing\n");
    exit(USAGE_ERROR);
  }

  if ((fi = open(argv[i++], O_RDONLY)) == -1) {
    fprintf(stderr, "Error: Cannot open %s\n", argv[i-1]);
    exit(USAGE_ERROR);
  }

  /* redirige l'entree standard sur le fichier... */
  close(0); dup(fi); close(fi);

  /* Lance l'analyse syntaxique de tout le source, en appelant yylex au fur
   * et a mesure. Execute les actions semantiques en parallele avec les
   * reductions.
   * yyparse renvoie 0 si le source est syntaxiquement correct, une valeur
   * differente de 0 en cas d'erreur syntaxique (eventuellement dues a des
   * erreurs lexicales).
   * Comme l'interpretation globale est automatiquement lancee par les actions
   * associees aux reductions, une fois que yyparse a termine il n'y
   * a plus rien a faire (sauf fermer les fichiers)
   * Si le code du programme contient une erreur, on bloque l'evaluation.
   * S'il n'y a que des erreurs contextuelles on essaye de ne pas s'arreter
   * a la premiere mais de continuer l'analyse pour en trovuer d'autres, quand
   * c'est possible.
   */
  res = yyparse();
  if (fd != NIL(FILE)) fclose(fd);
  if (res == 0 && errorCode == NO_ERROR) return 0;
  else {
    int res2 = res ? SYNTAX_ERROR : errorCode;
    printf("Error in file. Kind of error: %d\n", res2); 
    return res2;
  }
}


void setError(int code) {
  errorCode = code;
  if (code != NO_ERROR) { noEval = TRUE; }
}


/* yyerror:  fonction importee par Bison et a fournir explicitement. Elle
 * est appelee quand Bison detecte une erreur syntaxique.
 * Ici on se contente d'un message minimal.
 */
void yyerror(char *ignore) {
  fprintf(stderr, "Syntax error on line: %d\n", yylineno);
}


/* Fonction auxiliaire pour la construction d'arbre */
TreeP makeNode(int nbChildren, short etiquette) {
  TreeP tree = NEW(1, Tree);
  tree->op = etiquette;
  tree->nbChildren = nbChildren;
  tree->u.children = nbChildren > 0 ? NEW(nbChildren, TreeP) : NIL(TreeP);
  return(tree);
}


/* Construction d'un arbre a nbChildren branches, passees en parametres
 * La vraie fonction a appeler.
 */
TreeP makeTree(short etiquette, int nbChildren, ...) {
  va_list args;
  int i;
  TreeP tree = makeNode(nbChildren, etiquette); 
  va_start(args, nbChildren);
  for (i = 0; i < nbChildren; i++) { 
    tree->u.children[i] = va_arg(args, TreeP);
  }
  va_end(args);
  return(tree);
}


/* Retourne le nieme fils d'un arbre (de 0 a n-1) */
TreeP getChild(TreeP tree, int n) {
  return tree->u.children[n];
}


/* Constructeur de feuille dont la valeur est une chaine de caracteres
 * (un identificateur de variable).
 */
TreeP makeLeafStr(short op, char *str) {
  TreeP tree = makeNode(0, op);
  tree->u.str = str;
  return(tree);
}


/* Constructeur de feuille dont la valeur est un entier */
TreeP makeLeafInt(short op, int val) {
  TreeP tree = makeNode(0, op); 
  tree->u.val = val;
  return(tree);
}



/*
 * Seconde partie a modifier/completer
 */


/* Avant evaluation, verifie si tout id qui apparait dans tree a bien ete declare
 * et est donc dans lvar.
 * On impose que ca soit le cas y compris si on n'a pas besoin a l'evaluation de
 * la valeur de cet id.
 */
bool checkScope(TreeP tree, VarDeclP lvar) {
  return FALSE;
}

/* Verifie si besoin que nouv n'apparait pas deja dans list. l'ajoute en
 * tete et renvoie la nouvelle liste
 */
VarDeclP addToScope(VarDeclP list, VarDeclP nouv) {
  if(list == NIL(VarDeclP))
    return nouv;
  nouv->next = list;
  return nouv;
}


/* Construit le squelette d'un couple (variable, valeur), sans la valeur. */
VarDeclP makeVar(char *name) {
  VarDeclP res = NEW(1, VarDecl);
  res->name = name; 
  res->next = NIL(VarDecl);
  return(res);
}


/* Associe une variable a l'expression qui definit sa valeur, et procede a 
 * l'evaluation de cette expression, sauf si on est en mode noEval
 */
VarDeclP declVar(char *name, TreeP tree, VarDeclP currentScope) {

  VarDeclP res = currentScope;
  VarDeclP last = currentScope;
  while(res != NIL(VarDecl))
  {
    if(!strcmp(res->name, name)) 
    {
      return currentScope;
    }
    last = res;
    res = res->next;
  }
  VarDeclP nouv = malloc(sizeof(VarDecl));
  nouv->name = name;
  nouv->val = evalVar(tree, nouv);
  last->next = nouv;
  return currentScope;
}


/* Evaluation d'une variable */
int evalVar(TreeP tree, VarDeclP decls) {
  
  int resultat = 0 ;
  
  char *name = (char*) tree->u.str;
  VarDeclP res = decls;
  while(res->next != NIL(VarDeclP)){
    if(!strcmp(res->name,name))
      return res->val;
    res = res->next;
  }
  
  return resultat;
}


/* Evaluation d'un if then else. Attention a n'evaluer que la partie necessaire ! */
int evalIf(TreeP tree, VarDeclP decls) {
  int resultat = 0;

  TreeP tree_cond = getChild(tree, 0); 
  TreeP tree_vrai = getChild(tree, 1); 

  if(tree->nbChildren == 2) 
  {
	if(eval(tree_cond,decls))
	{
		return eval(tree_vrai,decls);
	}
	return 0;
  }else if(tree->nbChildren == 3) 
  {
	if(eval(tree_cond,decls))
	{
		return eval(tree_vrai,decls);
	}
	else
	{
		TreeP tree_faux = getChild(tree, 2); 
		return eval(tree_faux,decls);
	} 
  } 
}



VarDeclP evalAff (TreeP tree, VarDeclP decls) {
  char *name = (char*) tree->u.str;
  int valeur = evalVar(tree,decls);
  
  int ispresent = 0;
  VarDeclP res = decls;
  
  while(res->next != NIL(VarDeclP)){
    if(strcmp(res->name,name))
    {
		ispresent = 1;
       res->val = valeur;
    }
    res = res->next;
  }
  
  /* la var n'existe pas */
  if(!ispresent) 
  {
	  VarDeclP nouvelle_var = malloc(sizeof(VarDecl));
	  nouvelle_var->name = name;
	  nouvelle_var->val = valeur;
	  res->next = nouvelle_var;
  }
  
  return decls;
}


/* Ici decls correspond au sous-arbre qui est la partie declarative */
VarDeclP evalDecls (TreeP tree) {


  return NIL(VarDecl);
}


/* Evaluation par parcours recursif de l'arbre representant une expression. 
 * Les valeurs des identificateurs situes aux feuilles de l'arbre sont a
 * rechercher dans la liste decls
 * Attention a n'evaluer que ce qui doit l'etre et au bon moment
 * selon la semantique de l'operateur (cas du IF, etc.)
 */
int eval(TreeP tree, VarDeclP decls) {
  if (tree == NIL(Tree)) { exit(UNEXPECTED); }
  DEBUG(pprint(tree);)
  switch (tree->op) {
  case ID:
    return evalVar(tree, decls);
  case CST:
    return(tree->u.val);
  case EQ:
    return (eval(getChild(tree, 0), decls) == eval(getChild(tree, 1), decls));
  case NE:
    return (eval(getChild(tree, 0), decls) != eval(getChild(tree, 1), decls));
  case ADD:
    return (eval(getChild(tree, 0), decls) + eval(getChild(tree, 1), decls));
  case SUB:
    return (eval(getChild(tree, 0), decls) - eval(getChild(tree, 1), decls));
  case LT2:
    return (eval(getChild(tree, 0), decls) < eval(getChild(tree, 1), decls));
  case IF:
   case MUL : 
    return (eval(getChild(tree, 0), decls) * eval(getChild(tree, 1), decls));
  default: 
    fprintf(stderr, "Erreur! etiquette indefinie: %d\n", tree->op);
    exit(UNEXPECTED);
  }
}

int evalMain(TreeP tree, VarDeclP lvar) {
  int res;
  if (noEval) {
    fprintf(stderr, "\nSkipping evaluation step.\n");
  } else {
      res = eval(tree, lvar);
      printf("\n/*Result: %d*/\n", res);
  }
  return errorCode;
}
