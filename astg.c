#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <locale.h>
#include <wchar.h>
#include <fcntl.h>
#include <io.h>
#include <stddef.h>

#include <iup.h>
#include <iupcontrols.h>

#include <csc_debug.h>
#include <csc_tcol.h>
#include <csc_basic.h>
#include <csc_str.h>
#include <csc_tok_c.h>
#include <csc_tree4.h>


int pm_tokinfo_next (char const ** p, int * line, int * col, char const ** a)
{
again:
	(*a) = (*p);
	switch (**p)
	{
	case '\0': return 0;
	case ' ':
		(*col) ++;
		(*p) ++;
		goto again;
	case '\r':
		(*p) ++;
		goto again;
	case '\n':
		(*col) = 0;
		(*line) ++;
		(*p) ++;
		goto again;
	case '{':
	case '}':
	case '(':
	case ')':
	case '*':
	case ':':
	case ',':
	case '?':
	case '+':
	case '-':
	case '/':
	case '<':
	case '>':
	case '^':
		(*p) ++;
		(*col) ++;
		return (*p) [-1];
	}
	if (0) {}
	else if (csc_next_literal (p, col))
	{
		return CSC_TOK_C_LITERAL_INTEGER;
	}
	else if (csc_str_next_cmp (p, col, "void"))
	{
		return CSC_TOK_C_CONST;
	}
	else if (csc_str_next_cmp (p, col, "const"))
	{
		return CSC_TOK_C_VOID;
	}
	else if (csc_next_indentifer (p, col))
	{
		return CSC_TOK_C_LITERAL_INTEGER;
	}
	else if (csc_next_indentifer (p, col))
	{
		return CSC_TOK_C_INT;
	}
	return 0;
}

enum ast_nodetype
{
	AST_ROOT,
	AST_DECLERATION,
	AST_FUNCARGUMENTS,
	AST_CALLARGUMENTS,
	AST_IDENTIFIER,
	AST_IDENTIFIER_VARIABLE,
	AST_IDENTIFIER_FUNCTION,
	AST_LITERAL,
	AST_LITERAL_INTEGER,
	AST_LITERAL_STRING,
	AST_ADD,
	AST_SUB,
	AST_MUL,
};


int pm_tok_precedence (int t)
{
	switch (t)
	{
	case '*': return 3;
	case '+': return 2;
	case '^': return 9;
	case ',': return 15;
	//case TOK_IDENTIFIER: return 100;
	//case '(': return 100;
	}
	return 0;
}






struct ast_node;
struct ast_node
{
	enum ast_nodetype kind;
	int token;
	char const * a;
	char const * p;
	struct csc_tree4 tree;
};

/*
TODO:
	fix function and (). does not work correctly.
*/
void ast_add (struct ast_node * node, char const * code)
{
	char const * p = code;
	char const * a;
	int line = 0;
	int col = 0;
	int tok;
	struct ast_node * newnode = NULL;

	tok = pm_tokinfo_next (&p, &line, &col, &a);
	if (tok == 0) {return;}

	while (1)
	{
		//printf ("Token: %20s %4.*s\n", tok_type_tostr (tok), p-a, a);
		switch (tok)
		{
		case CSC_TOK_C_LITERAL_INTEGER:
		case CSC_TOK_C_IDENTIFIER:
			newnode = calloc (1, sizeof (struct ast_node));
			newnode->token = tok;
			newnode->a = a;
			newnode->p = p;
			if (node->kind == AST_IDENTIFIER_FUNCTION)
			{
				csc_tree4_addchild (&(node->tree), &(newnode->tree));
			}
			//Check if node is root, i.e. a node without a parent is considered a root:
			else if (node->tree.parent == NULL)
			{
				csc_tree4_addchild (&(node->tree), &(newnode->tree));
			}
			else
			{
				csc_tree4_addsibling (&(node->tree), &(newnode->tree));
			}
			node = newnode;
			tok = pm_tokinfo_next (&p, &line, &col, &a);
			if (tok == 0) {return;}
			break;

		//Left-right operators:
		case '-':
		case '+':
		case '*':
		case '^':
		case ',':
			//If parent operator has a larger precedence than current operator then
			//we need to walk up the tree until parent does not.
			if (node->tree.parent)
			{
				struct ast_node * parent = container_of (node->tree.parent, struct ast_node, tree);
				int p0 = pm_tok_precedence (parent->token);
				int p1 = pm_tok_precedence (tok);
				if (p0 > p1)
				{
					//bitree2_add_parent (node->tree.parent, &(newnode->tree));
					node = parent;
					break;
				}
			}
			newnode = calloc (1, sizeof (struct ast_node));
			newnode->token = tok;
			newnode->a = a;
			newnode->p = p;
			//There should be a operand in the current node which will be the child of this operator.
			csc_tree4_addparent (&(node->tree), &(newnode->tree));
			tok = pm_tokinfo_next (&p, &line, &col, &a);
			if (tok == 0) {return;}
			break;


		case '(':
			if (node->token == CSC_TOK_C_IDENTIFIER)
			{
				node->kind = AST_IDENTIFIER_FUNCTION;
			}
			break;

		case ')':
			node = container_of (node->tree.parent, struct ast_node, tree);
			break;


		}// END switch
	}// END while
}


void ast_iuptree (Ihandle * h, struct csc_tree4 * node, int depth, int leaf)
{
	//Traverse inorder (next, root, child) because it works well with IupTree.
	if (!node) {return;}
	ast_iuptree (h, node->next, depth, leaf + 1);
	char buf [40] = {0};
	struct ast_node * n = container_of (node, struct ast_node, tree);
	snprintf (buf, sizeof (buf), "%02i %02i, %2.*s", depth, leaf, (int)(n->p - n->a), n->a);
	if (node->child)
	{
		IupSetAttributeId (h, "ADDBRANCH", depth, buf);
		printf ("ADDBRANCH%i %s\n", depth, buf);
	}
	else
	{
		IupSetAttributeId (h, "ADDLEAF", depth, buf);
		printf ("ADDLEAF%i %s\n", depth, buf);
	}
	//IupSetAttribute (h, "ADDLEAF", depth);
	ast_iuptree (h, node->child, depth + 1, 0);
}


void ast_print (struct csc_tree4 * node, int depth, int leaf, uint32_t indent)
{
	char vertical[] = TCOL(TCOL_NORMAL,TCOL_GREEN,TCOL_DEFAULT) "\u2502" TCOL_RST;
	char cross[] = TCOL(TCOL_NORMAL,TCOL_GREEN,TCOL_DEFAULT) "\u251C" TCOL_RST;
	char corner[] = TCOL(TCOL_NORMAL,TCOL_GREEN,TCOL_DEFAULT) "\u2514" TCOL_RST;
	char dash[] = TCOL(TCOL_NORMAL,TCOL_GREEN,TCOL_DEFAULT) "\u2500" TCOL_RST;

	//Traverse preorder (root, child, next).
	if (!node) {return;}
	char buf [40] = {0};
	struct ast_node * n = container_of (node, struct ast_node, tree);
	//snprintf (buf, sizeof (buf), "%02i %02i, %2.*s", depth, leaf, (int)(n->p - n->a), n->a);
	snprintf (buf, sizeof (buf), TCOL(TCOL_NORMAL,TCOL_DEFAULT,TCOL_DEFAULT) " %.*s" TCOL_RST, (int)(n->p - n->a), n->a);
	for (int i = 0; i < depth; i ++)
	{
		if (indent & (1 << i))
		{
			printf ("%s", vertical);
		}
		else
		{
			putc (' ', stdout);
		}
		putc (' ', stdout);
		putc (' ', stdout);
	}
	if (node->next)
	{
		printf ("%s", cross);
		indent |= (1 << depth);
	}
	if (node->next == NULL)
	{
		printf ("%s", corner);
		indent &= ~(1 << depth);
	}
	//printf ("%c\n", *n->a);

	//putc (dash, stdout);
	printf ("%s", dash);
	puts (buf);
	ast_print (node->child, depth + 1, 0, indent);
	ast_print (node->next, depth, leaf + 1, indent);
}


struct ast_node * ast_create (char const * name)
{
	struct ast_node * newnode = calloc (1, sizeof (struct ast_node));
	newnode->a = name;
	newnode->p = newnode->a + strlen (name);
	return newnode;
}


int function (Ihandle *ih, int id, int status)
{
	ASSERT (ih);
	printf("%i %i\n", status, id);
	return IUP_DEFAULT;
}


void showast (struct ast_node * node)
{
	Ihandle * tree = IupTree ();
	IupSetAttribute (tree, "SHOWRENAME", "YES");
	IupSetCallback(tree, "SELECTION_CB", (Icallback) function);
	IupSetAttribute (tree, "TITLE", "AST");
	IupSetAttribute (tree, "FONT", "Courier, 14");
	Ihandle * dlg = IupDialog (IupVbox (tree, NULL));
	IupShow (dlg);
	ast_print (&(node->tree), 0, 0, 0);
	ast_iuptree (tree, &(node->tree), 0, 0);
}


int main (int argc, char * argv [])
{
	ASSERT (argc);
	ASSERT (argv);
	setbuf (stdout, NULL);
	setlocale (LC_CTYPE, "");
	IupOpen (&argc, &argv);
	char const code [] =
	"a ^ b * c ^ d + e * f";

	struct ast_node * ast = ast_create (code);
	ast->kind = AST_ROOT;
	//node1 = ast_add_child (node1, ast_create ("E"));
	//node1 = ast_add_child (node1, ast_create ("E"));

	ast_add (ast, code);


	showast (ast);

	IupMainLoop ();
	IupClose ();

	return EXIT_SUCCESS;
}
