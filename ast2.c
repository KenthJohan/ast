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


enum pm_tok
{
	PM_TOK_MUL = '*',
	PM_TOK_POW = '^',
	PM_TOK_PLUS = '+',
	PM_TOK_ID = 256,
	PM_TOK_LIT = 257,
};

struct pm_tokinfo
{
	int lin;
	int col;
	int tok;
	int padding;
	char const * a;
	char const * b;
};


void tok_next (struct pm_tokinfo * tok)
{
again:
	tok->a = tok->b;
	switch (*tok->b)
	{
	case '\0':
		tok->tok = 0;
		return;
	case '\t':
	case ' ':
		tok->col ++;
		tok->b ++;
		goto again;
	case '\r':
		tok->b ++;
		goto again;
	case '\n':
		tok->col = 0;
		tok->lin ++;
		tok->b ++;
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
		tok->tok = *tok->b;
		tok->b ++;
		tok->col ++;
		return;
	}
	if (0) {}
	else if (csc_next_literal (&tok->b, &tok->col))
	{
		tok->tok = PM_TOK_LIT;
	}
	else if (csc_next_indentifer (&tok->b, &tok->col))
	{
		tok->tok = PM_TOK_ID;
	}
	return;
}

void print_code (char const * code, char const * a, char const * b)
{
	ASSERT (a >= code);
	ASSERT (b >= a);
	ASSERT (b >= a);
	fwrite (code, 1, (unsigned long long)(a-code), stdout);
	fputs (TCOL(TCOL_UNDERSCORE,TCOL_DEFAULT,TCOL_RED), stdout);
	fwrite (a, 1, (unsigned long long)(b-a), stdout);
	fputs (TCOL_RST, stdout);
	fputs (b, stdout);
	fputs ("\n\n", stdout);
}


enum pm_act
{
	PM_ACT_ADD_SIBLING,
	PM_ACT_ADD_PARENT,
	PM_ACT_ADD_CHILD
};

struct pm_rule
{
	enum pm_act act;
};

struct pm_node
{
	struct csc_tree4 tree;
	struct pm_tokinfo tokinfo;
};


void parse (struct pm_rule * rule, struct pm_tokinfo * tokinfo, struct pm_node * node)
{
	struct pm_node * newnode;
	switch (rule [tokinfo->tok].act)
	{
	case PM_ACT_ADD_PARENT:
		newnode = calloc (1, sizeof (struct pm_node));
		node->tokinfo = (*tokinfo);
		csc_tree4_addparent (&(node->tree), &(newnode->tree));
		break;
	case PM_ACT_ADD_CHILD:
		newnode = calloc (1, sizeof (struct pm_node));
		node->tokinfo = (*tokinfo);
		csc_tree4_addchild (&(node->tree), &(newnode->tree));
		break;
	case PM_ACT_ADD_SIBLING:
		newnode = calloc (1, sizeof (struct pm_node));
		node->tokinfo = (*tokinfo);
		csc_tree4_addsibling (&(node->tree), &(newnode->tree));
		break;
	}
}



int main (int argc, char * argv [])
{
	ASSERT (argc);
	ASSERT (argv);
	setbuf (stdout, NULL);
	setlocale (LC_CTYPE, "");

	struct pm_rule rule[] =
	{
	[PM_TOK_ID] = {.act = PM_ACT_ADD_SIBLING},
	[PM_TOK_LIT] = {.act = PM_ACT_ADD_SIBLING},
	[PM_TOK_PLUS] = {.act = PM_ACT_ADD_PARENT},
	};

	char * code = "123 ^ abc123 * abc";
	struct pm_tokinfo tok;
	tok.b = code;

	while (1)
	{
		tok_next (&tok);
		ASSERT (tok.b >= tok.a);
		print_code (code, tok.a, tok.b);
		getc (stdin);
	}

	return EXIT_SUCCESS;
}
