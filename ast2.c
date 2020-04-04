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

#include <csc_debug.h>
#include <csc_tcol.h>
#include <csc_basic.h>
#include <csc_str.h>
#include <csc_tok_c.h>
#include <csc_tree4.h>
#include <csc_tree4_print.h>
#include <csc_malloc_file.h>


#define PM_NEXT_REPLACES (1 << 0)
#define PM_MOVEUP (1 << 1)
#define PM_PRECEDENCE (1 << 2)


enum pm_tok
{
	PM_TOK_TERMINATOR = '\0',
	PM_TOK_MUL = '*',
	PM_TOK_POW = '^',
	PM_TOK_PLUS = '+',
	PM_TOK_COMMA = ',',
	PM_TOK_PARR = '(',
	PM_TOK_PARL = ')',
	PM_TOK_BRAR = '{',
	PM_TOK_BRAL = '}',
	PM_TOK_SQRR = '[',
	PM_TOK_SQRL = ']',
	PM_TOK_START = 256,
	PM_TOK_ID,
	PM_TOK_LITERAL,
};


enum pm_act
{
	PM_ACT_SIBLING,
	PM_ACT_PARENT_PRECEDENCE,
	PM_ACT_CHILD
};


struct pm_tokinfo
{
	int lin;
	int col;
	enum pm_tok tok;
	int padding;
	char const * a;
	char const * b;
};


struct pm_node
{
	struct csc_tree4 tree;
	struct pm_tokinfo tokinfo;
};


char const * pm_tok_tostr (enum pm_tok tok)
{
	switch (tok)
	{
	case PM_TOK_START:return "START";
	case PM_TOK_MUL:return "MUL";
	case PM_TOK_POW:return "POW";
	case PM_TOK_PLUS:return "PLUS";
	case PM_TOK_ID:return "ID";
	case PM_TOK_LITERAL:return "LITERAL";
	default:return "TOK?";
	}
}


int pm_tok_section_index (enum pm_tok tok)
{
	switch (tok)
	{
	case '(': return 0;
	case ')': return 0;
	case '{': return 1;
	case '}': return 1;
	case '[': return 2;
	case ']': return 2;
	default:ASSERT(0);
	}
}


int pm_tok_precedence (enum pm_tok tok)
{
	switch (tok)
	{
	case '*': return 3;
	case '+': return 2;
	case '^': return 9;
	case ',': return 15;
	case '(': return 29;
	case ')': return 29;
	default:return 0;
		ASSERT(0);
	}
}


void pm_tokinfo_next (struct pm_tokinfo * tokinfo)
{
	ASSERT_PARAM_NOTNULL (tokinfo);
again:
	ASSERT (tokinfo->b >= tokinfo->a);
	tokinfo->a = tokinfo->b;
	switch (*tokinfo->b)
	{
	case '\0':
		tokinfo->tok = 0;
		return;
	case '\t':
	case ' ':
		tokinfo->col ++;
		tokinfo->b ++;
		goto again;
	case '\r':
		tokinfo->b ++;
		goto again;
	case '\n':
		tokinfo->col = 0;
		tokinfo->lin ++;
		tokinfo->b ++;
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
	case ';':
		tokinfo->tok = (enum pm_tok) (*tokinfo->b);
		tokinfo->b ++;
		tokinfo->col ++;
		return;
	}
	if (0) {}
	else if (csc_next_literal (&tokinfo->b, &tokinfo->col))
	{
		tokinfo->tok = PM_TOK_LITERAL;
	}
	else if (csc_next_indentifer (&tokinfo->b, &tokinfo->col))
	{
		tokinfo->tok = PM_TOK_ID;
	}
	return;
}


/**
 * @brief Print the code and highlight code section a to b.
 * @param code
 * @param a
 * @param b
 */
void print_code (char const * code, char const * a, char const * b)
{
	ASSERT (a >= code);
	ASSERT (b >= a);
	fwrite (code, 1, (unsigned long long)(a-code), stdout);
	fputs (TCOL(TCOL_UNDERSCORE,TCOL_DEFAULT,TCOL_RED), stdout);
	fwrite (a, 1, (unsigned long long)(b-a), stdout);
	fputs (TCOL_RST, stdout);
	fputs (b, stdout);
	fputs ("\n\n", stdout);
}


enum pm_act pm_act_fromtok (enum pm_tok tok)
{
	switch (tok)
	{
	case PM_TOK_MUL: return PM_ACT_PARENT_PRECEDENCE;
	case PM_TOK_POW: return PM_ACT_PARENT_PRECEDENCE;
	case PM_TOK_PLUS: return PM_ACT_PARENT_PRECEDENCE;
	case PM_TOK_PARR: return PM_ACT_CHILD;
	case PM_TOK_PARL: return PM_ACT_SIBLING;
	case PM_TOK_ID: return PM_ACT_SIBLING;
	case PM_TOK_LITERAL: return PM_ACT_SIBLING;
	default:ASSERT (0);
	}
}


void pm_parse (struct pm_tokinfo * tokinfo, struct pm_node * node, struct pm_node ** nextnode)
{
	ASSERT_PARAM_NOTNULL (tokinfo);
	ASSERT_PARAM_NOTNULL (node);
	ASSERT_PARAM_NOTNULL (nextnode);
	ASSERT_PARAM_NOTNULL (*nextnode);
	struct pm_node * newnode = NULL;
tryagain:
	switch (pm_act_fromtok (tokinfo->tok))
	{
	case PM_ACT_PARENT_PRECEDENCE:
		if (node->tree.parent)
		{
			struct pm_node * parent = container_of (node->tree.parent, struct pm_node, tree);
			int p0 = pm_tok_precedence (tokinfo->tok);
			int p1 = pm_tok_precedence (parent->tokinfo.tok);
			if (p0 < p1)
			{
				node = parent;
				goto tryagain;
			}
		}
		(*nextnode) = node;
		newnode = calloc (1, sizeof (struct pm_node));
		newnode->tokinfo = (*tokinfo);
		csc_tree4_addparent (&(node->tree), &(newnode->tree));
		break;
	case PM_ACT_CHILD:
		newnode = calloc (1, sizeof (struct pm_node));
		newnode->tokinfo = (*tokinfo);
		csc_tree4_addchild (&(node->tree), &(newnode->tree));
		(*nextnode) = newnode;
		break;
	case PM_ACT_SIBLING:
		newnode = calloc (1, sizeof (struct pm_node));
		newnode->tokinfo = (*tokinfo);
		csc_tree4_addsibling (&(node->tree), &(newnode->tree));
		(*nextnode) = newnode;
		break;
	}
}


void pm_print_traverse_cb (struct csc_tree4 const * nodepos, void *ptr)
{
	ASSERT_PARAM_NOTNULL (nodepos);
	ASSERT_PARAM_NOTNULL (ptr);
	struct pm_node const * node = container_of_const (nodepos, struct pm_node const, tree);
	char const * color = NULL;
	char const * a = node->tokinfo.a;
	char const * b = node->tokinfo.b;
	int ab_length = (int)(b - a);
	enum pm_tok tok = node->tokinfo.tok;
	//The userpointer ptr is the position of the parser:
	//Highlight the position of parser in the tree with a different color:
	if (node == ptr)
	{
		color = TCOL (TCOL_UNDERSCORE, TCOL_DEFAULT, TCOL_RED);
	}
	else
	{
		color = TCOL (TCOL_BOLD, TCOL_DEFAULT, TCOL_DEFAULT);
	}
	printf ("%s" "%s" TCOL_RST, color, pm_tok_tostr (tok));
	printf (" %.*s\n", ab_length, a);
}


int main (int argc, char * argv [])
{
	ASSERT (argc);
	ASSERT (argv);
	setbuf (stdout, NULL);
	setlocale (LC_CTYPE, "");

	//Having a starting node will avoid edge cases, i.e. a empty tree will have one node:
	struct pm_node * root = calloc (1, sizeof (struct pm_node));
	struct pm_node * nodepos = root;
	nodepos->tokinfo.tok = PM_TOK_START;

	//Start the tokinfo at beginning of the code:
	char * code = "a ^ b(x+3^2+y)";
	struct pm_tokinfo tokinfo = {0};
	tokinfo.b = code;

	while (1)
	{
		pm_tokinfo_next (&tokinfo);
		ASSERT (tokinfo.b >= tokinfo.a);
		print_code (code, tokinfo.a, tokinfo.b);
		pm_parse (&tokinfo, nodepos, &nodepos);
		csc_tree4_print_traverse (&(root->tree), &(nodepos->tree), pm_print_traverse_cb);
		getc (stdin);
	}

	return EXIT_SUCCESS;
}
