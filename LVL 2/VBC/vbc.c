// Assignment name:		vbc
// Expected files:		*.c *h
// Allowed functions:	malloc, calloc, realloc, free, printf, isdigit, write

// Write a program that prints the result of a mathematical expression given as argument.
// It must handle addition, multiplication and parenthesis. All values are between 0 and 9 included.
// In case of an unexpected symbol, you must print "Unexpected token '%c'\n".
// If the expression ends unexpectedly, you must print "Unexpected end of input\n".
// The same rule applies if finding an unexpected '(' or ')'.
// In case of a syscall failure, you must exit with 1.

// Examples:
// $> ./vbc '1' | cat -e
// 1$
// $> ./vbc '2+3' | cat -e
// 5$
// $> ./vbc '3*4+5' | cat -e
// 17$
// $> ./vbc '3+4*5' | cat -e
// 23$
// $> ./vbc '(3+4)*5' | cat -e
// 35$
// $> ./vbc '(((((2+2)*2+2)*2+2)*2+2)*2+2)*2' | cat -e
// 188$
// $> ./vbc '1+'
// Unexpected end of input
// $> ./vbc '1+2)' | cat -e
// Unexpected token ')'$

// File provided: vbc.c, see below.



/* vbc.c — version gardant la structure fournie à l’examen,
 * avec commentaires indiquant les fonctions modifiées ou non.
 */

#include <stdio.h>
#include <malloc.h>   /* pour calloc/free */
#include <ctype.h>    /* pour isdigit */

/* --------------------------------------------------------------------------
 * Structure de l’AST
 * -------------------------------------------------------------------------- */
typedef struct node {
    enum {
        ADD,
        MULTI,
        VAL
    } type;
    int           val;
    struct node  *l;
    struct node  *r;
} node;

/* --------------------------------------------------------------------------
 * new_node : MODIFIÉE
 *   Alloue et initialise un nouveau nœud en copiant la structure passée.
 *   Dans la version d’origine, sizeof(n) était incorrect :
 *   on utilise sizeof *ret pour allouer la bonne taille.
 * -------------------------------------------------------------------------- */
node *new_node(node n)
{
    node *ret = calloc(1, sizeof *ret);
    if (!ret)
        return NULL;
    *ret = n;
    return ret;
}

/* --------------------------------------------------------------------------
 * destroy_tree : NON MODIFIÉE
 *   Libère récursivement tous les nœuds de l’arbre.
 * -------------------------------------------------------------------------- */
void destroy_tree(node *n)
{
    if (!n) return;
    if (n->type != VAL) {
        destroy_tree(n->l);
        destroy_tree(n->r);
    }
    free(n);
}

/* --------------------------------------------------------------------------
 * unexpected : MODIFIÉE
 *   Affiche l’erreur correspondant :
 *     - "Unexpected token '%c'\n" si c != 0
 *     - "Unexpected end of input\n" si c == 0
 *   Le message "end of input" remplace l’original "end of file".
 * -------------------------------------------------------------------------- */
void unexpected(char c)
{
    if (c)
        printf("Unexpected token '%c'\n", c);
    else
        printf("Unexpected end of input\n");
}

/* --------------------------------------------------------------------------
 * accept : NON MODIFIÉE
 *   Si **s == c, avance *s et renvoie 1 ; sinon renvoie 0.
 * -------------------------------------------------------------------------- */
int accept(char **s, char c)
{
    if (**s == c) {
        (*s)++;
        return 1;
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * expect : NON MODIFIÉE
 *   Comme accept(), mais en cas d’échec affiche l’erreur via unexpected()
 *   et renvoie 0.
 * -------------------------------------------------------------------------- */
int expect(char **s, char c)
{
    if (accept(s, c))
        return 1;
    unexpected(**s);
    return 0;
}

/* --------------------------------------------------------------------------
 * parse_add : IMPLÉMENTÉE
 *   Gère l’addition et la priorité inférieure :
 *     expr := produit { '+' produit }
 * -------------------------------------------------------------------------- */
static node *parse_add(char **s)
{
    node *left = parse_multi(s);
    if (!left) return NULL;

    while (accept(s, '+')) {
        node *right = parse_multi(s);
        if (!right) {
            destroy_tree(left);
            return NULL;
        }
        node n = { .type = ADD, .val = 0, .l = left, .r = right };
        node *tmp = new_node(n);
        if (!tmp) {
            destroy_tree(left);
            destroy_tree(right);
            return NULL;
        }
        left = tmp;
    }
    return left;
}

/* --------------------------------------------------------------------------
 * parse_multi : IMPLÉMENTÉE
 *   Gère la multiplication et la priorité supérieure :
 *     produit := facteur { '*' facteur }
 * -------------------------------------------------------------------------- */
static node *parse_multi(char **s)
{
    node *left = parse_val(s);
    if (!left) return NULL;

    while (accept(s, '*')) {
        node *right = parse_val(s);
        if (!right) {
            destroy_tree(left);
            return NULL;
        }
        node n = { .type = MULTI, .val = 0, .l = left, .r = right };
        node *tmp = new_node(n);
        if (!tmp) {
            destroy_tree(left);
            destroy_tree(right);
            return NULL;
        }
        left = tmp;
    }
    return left;
}

/* --------------------------------------------------------------------------
 * parse_val : IMPLÉMENTÉE
 *   Gère un chiffre ou une sous‐expression parenthésée :
 *     facteur := digit | '(' expr ')'
 * -------------------------------------------------------------------------- */
static node *parse_val(char **s)
{
    /* cas chiffre */
    if (isdigit((unsigned char)**s)) {
        node n = { .type = VAL, .val = **s - '0', .l = NULL, .r = NULL };
        node *ret = new_node(n);
        if (!ret) return NULL;
        (*s)++;
        return ret;
    }
    /* cas '(' expr ')' */
    if (accept(s, '(')) {
        node *sub = parse_add(s);
        if (!sub) return NULL;
        if (!expect(s, ')')) {
            destroy_tree(sub);
            return NULL;
        }
        return sub;
    }
    /* ni chiffre ni '(', caractère inattendu ou fin prématurée */
    if (**s)
        unexpected(**s);
    else
        unexpected(0);
    return NULL;
}

/* --------------------------------------------------------------------------
 * parse_expr : MODIFIÉE
 *   Point d’entrée : lance parse_add, puis vérifie qu’on est bien
 *   en fin de chaîne (sinon erreur).
 * -------------------------------------------------------------------------- */
node *parse_expr(char *s)
{
    node *ret = parse_add(&s);
    if (!ret)
        return NULL;
    if (*s) {
        unexpected(*s);
        destroy_tree(ret);
        return NULL;
    }
    return ret;
}

/* --------------------------------------------------------------------------
 * eval_tree : NON MODIFIÉE
 *   Évalue l’AST : ADD, MULTI ou VAL.
 * -------------------------------------------------------------------------- */
int eval_tree(node *tree)
{
    switch (tree->type) {
        case ADD:
            return eval_tree(tree->l) + eval_tree(tree->r);
        case MULTI:
            return eval_tree(tree->l) * eval_tree(tree->r);
        case VAL:
            return tree->val;
    }
    return 0; /* jamais atteint */
}

/* --------------------------------------------------------------------------
 * main : MODIFIÉE
 *   - Lit l’argument
 *   - Construit l’AST via parse_expr
 *   - En cas d’erreur retourne 1
 *   - Vérifie échec de printf (exit code 1)
 *   - Retourne 0 en cas de succès
 * -------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;

    node *tree = parse_expr(argv[1]);
    if (!tree)
        return 1;

    if (printf("%d\n", eval_tree(tree)) < 0)
        return 1;

    destroy_tree(tree);
    return 0;
}
