/*
 * Un petit programme de demonstration qui n'utilise que l'analyse lexicale.
 * Permet principalement de tester la correction de l'analyseur lexical et de
 * l'interface entre celui-ci et le programme qui l'appelle.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "tp.h"
#include "tp_y.h"
extern char *strdup(const char *);

/* Couple Ident, numero de ligne de sa premiere apparition */
typedef struct {
  int line;
  char *id;
} IdentS, *IdentP;

/* Table des identificateurs avec leur numero de ligne et son compteur */
IdentP idents = NIL(IdentS);
int nbIdent = 0;


/* Fonction appelee par le programme principal pour obtenir l'unite lexicale
 * suivante. Elle est produite par Flex (fichier tp_l.c)
 */
extern int yylex (void);

/* Chaine de caractere qui est une instance du token courant : definie et
 * mise a jour dans tp_l.c
 */
extern char *yytext;

/* Le numero de ligne courant : defini et mis a jour dans tp_l.c */
extern int yylineno;

/* Variable pour interfacer flex avec le programme qui l'appelle, notamment
 * pour transmettre la "valeur", en plus du type de token reconnu.
 * Le type YYSTYPE est defini dans tp.h.
 */
YYSTYPE yylval;

bool verbose = FALSE;

void setError(int code) {
  /* presente juste pour des raisons de compatibilite */
}

/* Recherche dans la table l'entree correspondant a un identificateur */
IdentP getIdent(char * id) {
  int i;
  for (i = 0; i < nbIdent; i++) {
    if (!strcmp(id, idents[i].id)) return(&idents[i]);
  }
  return NIL(IdentS);
}

/* Cree un identificateur */
IdentP makeIdent(int line, char *id) {
  IdentP ident = getIdent(id);
  if (!ident) {
    ident = &idents[nbIdent++];
    ident->line = line;
    /* Si on a duplique la chaine dans la partie Flex, il n'y a pas de raison
     * de le faire a nouveau ici.
     * Comme ca risque de dependre de ce que les uns et les autres font dans
     * tp.l, je prefere le (re-)faire ici ourb etre sur qu'elle ne soit pas ecrasee.
     */
    ident->id = strdup(id);
  }
  return(ident);
}

/* format d'appel */
void help() {
  fprintf(stderr, "Appel: tp [-h] [-v] programme.txt\n");
}

/* Appel:
 *   tp [-option]* programme.txt
 * Les options doivent apparaitre avant le nom du fichier du programme.
 * Options: -[vV] -[hH?]
 */
int main(int argc, char **argv) {
  idents = NEW(1000, IdentS); /* liste des identificateurs */
  int fi;
  int token;
  int i;

  for(i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'v': case 'V':
	verbose = TRUE; continue;
      case '?': case 'h': case 'H':
	help();
	exit(1);
      default:
	fprintf(stderr, "Option inconnue: %c\n", argv[i][1]);
	exit(1);
      }
    } else break;
  }

  if (i == argc) {
    fprintf(stderr, "Erreur: Fichier programme manquant\n");
    help();
    exit(1);
  }

  if ((fi = open(argv[i++], O_RDONLY)) == -1) {
    fprintf(stderr, "Erreur: fichier inaccessible %s\n", argv[i-1]);
    help();
    exit(1);
  }

  /* redirige l'entree standard sur le fichier... */
  close(0); dup(fi); close(fi);

  while (1) {
    token = yylex();
    switch (token) {
    case 0:
      printf("Fin de l'analyse lexicale\n");
      printf("Liste des identificateurs avec leur premiere occurrence:\n");
      for (i = 0; i < nbIdent; i++) {
        printf("ligne %d : %s\n", idents[i].line, idents[i].id);
      }
      printf("Nombre d'identificateurs: %4d\n", nbIdent);
      return 0;
    case ID:
      makeIdent(yylineno, yylval.S);
      if (verbose) printf("Identificateur:\t\t%s\n", yylval.S);
      break;
    case CST:
      if (verbose) printf("Constante:\t\t%d\n", yylval.I);
      break;
    case BEG: 
    case END:
    case IF:
    case THEN:
    case ELSE:
    case '(':
    case ')':
    case ';':
    case ',':
      if (verbose) printf("Symbole:\t\t%s\n",  yytext);
      break;
    case ADD:
    case SUB:
      if (verbose) printf("Oper arithmetique:\t%s\n", yytext);
      break;
    case RELOP:
      /* inutilement complique ici, mais sert a illustrer la difference
       * entre le token et l'infirmation supplementaire qu'on peut associer
       * via la valeur.
       */
      if (verbose) { 
	printf("Oper de comparaison:\t%s ", yytext);
	switch(yylval.C) {
	case EQ: printf("egalite\n"); break;
	case NE: printf("non egalite\n"); break;
	default: printf("Unexpected code: %d\n", yylval.C);
	}
      }
      break;
    default:
      printf("Token non reconnu:\t\"%d\"\n", token);
    }
  }
}
