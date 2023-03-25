#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <memory.h>

#define SIGNED_16_MAX 32767
#define SIGNED_16_MIN -32768

#define InstrTypesNum 37
enum token_type
{
	instruction,
	immediate,
	label,
	reg
};
char tokentypes[4][20] = {"instruction", "immediate", "label", "register"};
typedef enum token_type token_type;
enum argument_list
{
	R,
	RR,
	RRR,
	RRI5,
	RRI32se,
	RRI32ze,
	RI16,
	RI16R,
	RL,
	L,
	RRL,
	NotInstrButLabel
};
typedef enum argument_list arg_list_type;
char regs[32][10] = {"0", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
					 "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"

};
int INSTR_NUM = 57;
int REG_NUMBER = 32;

char InstrTypes[InstrTypesNum][10] = {"srl", "sll", "sra", "jr", "jalr", "add", "sub", "and", "or", "xor", "nor", "slt",
									  "addi", "slti", "andi", "ori", "xori", "lui",
									  "lb", "lh", "lw", "sb", "sh", "sw",
									  "bltz", "j", "jal", "beq", "blez",
									  "sllv", "srlv", "srav",
									  "mfhi", "mthi", "mflo", "mtlo", "mult"};
arg_list_type InstrTypesArgs[InstrTypesNum] = {RRI5, RRI5, RRI5, R, R, RRR, RRR, RRR, RRR, RRR, RRR, RRR,
											   RRI32se, RRI32se, RRI32ze, RRI32ze, RRI32ze, RI16,
											   RI16R, RI16R, RI16R, RI16R, RI16R, RI16R,
											   RL, L, L, RRL, RL,
											   RRR, RRR, RRR,
											   R, R, R, R, RR};
int opcodes[InstrTypesNum] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							  8, 10, 12, 13, 14, 15,
							  32, 33, 35, 40, 41, 43,
							  1, 2, 3, 4, 6,
							  0, 0, 0,
							  0, 0, 0, 0, 0};
int functs[InstrTypesNum] = {2, 0, 3, 8, 9, 32, 34, 36, 37, 38, 39, 42,
							 -1, -1, -1, -1, -1, -1,
							 -1, -1, -1, -1, -1, -1,
							 -1, -1, -1, -1, -1,
							 4, 6, 7,
							 16, 17, 18, 19, 24};
// int shamts[18] = {};

enum extend_type
{
	None,
	SignExtend,
	ZeroExtend,
	LowerPartZeroExtend
};
typedef enum extend_type extend_type;

char charputback;
int line = 1, col = 0;

struct token
{
	token_type type;
	// int reg;
	// int immediate;
	int int_value;
	// char instrction[10];
	// char label[10];
	char str[10];
	int line, col;
	int labeldefine;
};
typedef struct token token;

struct node
{
	token *tokenp;
	struct node *next;
};
typedef struct node node;
typedef token element_type;

struct statement
{
	char label[10];
	int opcode, rd, rs, rt, shamt, funct, immediate_value;
	int linenum, label_target_num;
	arg_list_type arg_lst;
	extend_type extend;
	// int labeldefine;
};
typedef struct statement statement;
struct node_statement
{
	statement *state;
	struct node_statement *next;
};
typedef struct node_statement node_statement;

struct hex_code
{
	int hex;
	struct hex_code *next;
};
typedef struct hex_code hex_code;

node *read_mips(char *filename);
char next(FILE *fp);
char skip(FILE *fp);
void putback(char ch);
int chrpos(char *s, char c);
int scan_imm(char ch, FILE *fp);

node_statement *parse_mips(node *root);

hex_code *build_mips(node_statement *root);
// char * bin2hex_with_len(int bin, int len);

void write_mips(hex_code *root, char *filename);

int write_linklist(node *head, element_type *element);
void display_linklist(node *root);
void display_token(token *tkp);
void display_statement(node_statement *root);
void display_single_statement(node_statement *root);

int main(void)
{
	char filename[20];
	scanf("%s", filename);
	// printf("dbg>>>kkmd1\n");
	node *rt = read_mips(filename);
	// printf("dbg>>>kkmd2\n");
	// display_linklist(rt);
	// printf("\n\n\n\n\n");
	node_statement *srt = parse_mips(rt);
	// display_statement(srt);
	// puts("Ko ko ma de.\n");
	hex_code *hr = build_mips(srt);
	write_mips(hr, filename);
	return 0;
}

char next(FILE *fp)
{
	char ch;
	col++;
	if (charputback)
	{
		ch = charputback;
		charputback = 0;
		return ch;
	}
	else
	{
		ch = fgetc(fp);
		if (ch == '\n')
		{
			line++;
			col = 0;
		}

		return ch;
	}
}

char skip(FILE *fp)
{
	char ch = next(fp);
	while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
		ch = next(fp);
	return ch;
}

int chrpos(char *s, char ch)
{
	char *p = strchr(s, ch);
	return (p ? p - s : -1);
}

int scan_imm(char ch, FILE *fp)
{
	int res = 0, curr_tok = col, neg = 0;
	if (ch == '-')
	{
		ch = next(fp);
		neg = 1;
	}
	int k, base = 10;
	while ((k = chrpos("0123456789abcde", ch)) >= 0 || ch == 'x')
	{
		if (ch == 'x' && res == 0)
		{
			ch = next(fp);
			base = 16;
		}
		else if (ch == 'x' && res)
		{
			// error
			printf("Error(%d,%d): Not a valid number.\n", line, curr_tok);
			exit(-1);
			// show error part here
			// TODO
		}
		else
		{
			res = res * base + k;
			ch = next(fp);
		}
	}
	putback(ch);
	return neg ? -res : res;
}

node *read_mips(char *filename)
{
	node *root = NULL, *curr = NULL;
	FILE *fp = fopen(filename, "r");
	if (!fp)
	{
		printf("Error:Cannot open file %s.\n", filename);
		exit(-1);
	}
	char ch = skip(fp), buffer[10] = {0};
	int index = 0;
	while (ch != EOF)
	{
		if (isalpha(ch))
		{
			int curr_col = col, curr_line = line;
			buffer[index++] = ch;
			while (isalpha(ch = next(fp)))
				buffer[index++] = ch;
			token *tkp = (token *)malloc(sizeof(token));
			strcpy(tkp->str, buffer);
			putback(ch);
			ch = skip(fp);
			if ((ch == ':' || ch == '#' || line != curr_line || ch == EOF) && strcmp(buffer, "nop"))
			{
				tkp->type = label;
				if (ch == ':')
					tkp->labeldefine = 1;
				else
					tkp->labeldefine = 0;
			}
			else
				tkp->type = instruction;
			putback(ch);
			tkp->col = curr_col;
			tkp->line = curr_line;
			if (!curr)
			{
				root = (node *)malloc(sizeof(node));
				root->tokenp = tkp;
				root->next = NULL;
				curr = root;
			}
			else
			{
				curr->next = (node *)malloc(sizeof(node));
				curr = curr->next;
				curr->tokenp = tkp;
				curr->next = NULL;
			}
			memset(buffer, 0, sizeof(buffer));
			index = 0;
		}

		else if (isdigit(ch) || ch == '-')
		{
			int curr_col = col, curr_line = line;
			token *tkp = (token *)malloc(sizeof(token));
			tkp->type = immediate;
			tkp->int_value = scan_imm(ch, fp);
			tkp->col = curr_col;
			tkp->line = curr_line;
			if (!curr)
			{
				root = (node *)malloc(sizeof(node));
				root->tokenp = tkp;
				root->next = NULL;
				curr = root;
			}
			else
			{
				curr->next = (node *)malloc(sizeof(node));
				curr = curr->next;
				curr->tokenp = tkp;
				curr->next = NULL;
			}
		}

		else if (ch == '$')
		{
			int curr_col = col, curr_line = line;
			while (isalnum(ch = next(fp)))
				buffer[index++] = ch;
			int flag = 0, i = 0;
			for (i = 0; i < REG_NUMBER; i++)
				if (!strcmp(buffer, regs[i]))
				{
					flag = 1;
					break;
				}
				else
				{
					continue;
				}
			if (flag)
			{
				token *tkp = (token *)malloc(sizeof(token));
				tkp->type = reg;
				tkp->int_value = i;
				tkp->col = curr_col;
				tkp->line = curr_line;
				if (!curr)
				{
					root = (node *)malloc(sizeof(node));
					root->tokenp = tkp;
					root->next = NULL;
					curr = root;
				}
				else
				{
					curr->next = (node *)malloc(sizeof(node));
					curr = curr->next;
					curr->tokenp = tkp;
					curr->next = NULL;
				}
			}
			else
			{
				printf("Error(%d,%d): No register named $%s.\n", line, curr_col, buffer);
				exit(-1);
			}
			memset(buffer, 0, sizeof(buffer));
			index = 0;
		}
		else if (ch == '#')
		{
			ch = next(fp);
			while (ch != '\n' && ch != EOF)
				ch = next(fp);
			putback(ch);
		}
		ch = next(fp);
	}
	return root;
}

void display_token(token *tkp)
{
	switch (tkp->type)
	{
	case immediate:
	case reg:
		printf("%d(%d:%d:%s)", tkp->int_value, tkp->line, tkp->col, tokentypes[tkp->type]);
		break;
	case instruction:
	case label:
		printf("%s(%d:%d:%s)", tkp->str, tkp->line, tkp->col, tokentypes[tkp->type]);
		break;
	}
	// printf("<dbg %p>", )
	return;
}

void display_linklist(node *root)
{
	display_token(root->tokenp);
	// printf("<dbg %p>", root);
	root = root->next;
	while (root)
	{
		printf(" -> ");
		display_token(root->tokenp);
		// printf("<dbg %p>", root);
		root = root->next;
	}
	putchar('\n');
}

void putback(char ch)
{
	charputback = ch;
	col--;
}

node_statement *parse_mips(node *root)
{
	node *pre = root;
	int curr_parse_line_num = 1;
	token_type expect = instruction;
	node_statement *root_statement = NULL;
	node_statement *node_stat_p = root_statement;
	// root_statement->next = NULL;
	while (root)
	{
		if (root->tokenp->type != expect)
		{
			if ((root->tokenp->type == label) && (root->tokenp->labeldefine))
			{
				if (!root_statement)
				{
					root_statement = (node_statement *)malloc(sizeof(node_statement));
					root_statement->state = (statement *)malloc(sizeof(statement));
					// memset(root_statement->state, 0, sizeof(statement));
					// memcpy(root_statement->state->label, root->tokenp->str, sizeof(root->tokenp->str));
					root_statement->state->label_target_num = curr_parse_line_num;
					node_stat_p = root_statement;
					node_stat_p->next = NULL;
					// node_stat_p->state = (statement *)malloc(sizeof(statement));
					strcpy(root_statement->state->label, root->tokenp->str);
					node_stat_p->state->arg_lst = NotInstrButLabel;
					root = root->next;
					expect = instruction;
				}
				else
				{
					node_stat_p->next = (node_statement *)malloc(sizeof(node_statement));
					node_stat_p = node_stat_p->next;
					node_stat_p->next = NULL;
					node_stat_p->state = (statement *)malloc(sizeof(statement));
					// memset(node_stat_p->state, 0, sizeof(statement));
					memcpy(node_stat_p->state->label, root->tokenp->str, sizeof(root->tokenp->str));
					// strcpy(root_statement->state->label, root->tokenp->str);
					node_stat_p->state->label_target_num = curr_parse_line_num;
					node_stat_p->state->arg_lst = NotInstrButLabel;
					root = root->next;
					expect = instruction;
				}
				continue;
			}
			else
			{
				printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line,
					   root->tokenp->col, tokentypes[expect], tokentypes[root->tokenp->type]);
				exit(-1);
			}
		}
		else
		{
			int index = 0, argindex = 0;

			if (!strcmp(root->tokenp->str, "mv"))
			{
				// mv $a, $b
				// addi $a, $b, 0
				node *tmp = root->next;
				node *replacer = (node *)malloc(sizeof(node)), *newcomer = (node *)malloc(sizeof(node));
				replacer->tokenp = (token *)malloc(sizeof(token));
				newcomer->tokenp = (token *)malloc(sizeof(token));
				replacer->tokenp->col = root->tokenp->col;
				replacer->tokenp->line = root->tokenp->line;
				replacer->tokenp->type = instruction;
				strcpy(replacer->tokenp->str, "addi");
				replacer->next = root->next;

				newcomer->tokenp->type = immediate;
				newcomer->tokenp->col = root->tokenp->col;
				newcomer->tokenp->line = root->tokenp->line;
				newcomer->tokenp->int_value = 0;

				newcomer->next = root->next->next->next;
				replacer->next->next->next = newcomer;
				root = replacer;
				// display_linklist(root);
			}
			else if (!strcmp(root->tokenp->str, "nop"))
			{
				// nop
				// addi $0, $0, 0
				node *tmp = root->next;
				node *replacer = (node *)malloc(sizeof(node));
				node *newcomers[3];
				replacer->tokenp = (token *)malloc(sizeof(token));
				for (int i = 0; i < 3; i++)
				{
					newcomers[i] = (node *)malloc(sizeof(node));
					newcomers[i]->tokenp = (token *)malloc(sizeof(token));
				}
				replacer->tokenp->col = root->tokenp->col;
				replacer->tokenp->line = root->tokenp->line;
				replacer->tokenp->type = instruction;
				strcpy(replacer->tokenp->str, "addi");
				// replacer->next = root->next;

				for (int i = 0; i < 2; i++)
				{
					newcomers[i]->tokenp->type = reg;
					newcomers[i]->tokenp->col = root->tokenp->col;
					newcomers[i]->tokenp->line = root->tokenp->line;
					newcomers[i]->tokenp->int_value = 0;
				}
				newcomers[2]->tokenp->type = immediate;
				newcomers[2]->tokenp->col = root->tokenp->col;
				newcomers[2]->tokenp->line = root->tokenp->line;
				newcomers[2]->tokenp->int_value = 0;

				// replacer->next->next->next = newcomer;
				// newcomer->next = root->next->next->next;
				replacer->next = newcomers[0];
				replacer->next->next = newcomers[1];
				replacer->next->next->next = newcomers[2];
				replacer->next->next->next->next = root->next;
				root = replacer;
			}
			else if (!strcmp(root->tokenp->str, "not"))
			{
				// not $a, $b
				// xori $a, $b, -1
				node *tmp = root->next;
				node *replacer = (node *)malloc(sizeof(node)), *newcomer = (node *)malloc(sizeof(node));
				replacer->tokenp = (token *)malloc(sizeof(token));
				newcomer->tokenp = (token *)malloc(sizeof(token));
				replacer->tokenp->col = root->tokenp->col;
				replacer->tokenp->line = root->tokenp->line;
				replacer->tokenp->type = instruction;
				strcpy(replacer->tokenp->str, "xori");
				replacer->next = root->next;

				newcomer->tokenp->type = immediate;
				newcomer->tokenp->col = root->tokenp->col;
				newcomer->tokenp->line = root->tokenp->line;
				newcomer->tokenp->int_value = -1;

				newcomer->next = root->next->next->next;
				replacer->next->next->next = newcomer;
				root = replacer;
			}
			else if (!strcmp(root->tokenp->str, "neg"))
			{
				// neg $a, $b
				// sub $a, $0, $b
				node *tmp = root->next;
				node *replacer = (node *)malloc(sizeof(node)), *newcomer = (node *)malloc(sizeof(node));
				replacer->tokenp = (token *)malloc(sizeof(token));
				newcomer->tokenp = (token *)malloc(sizeof(token));
				replacer->tokenp->col = root->tokenp->col;
				replacer->tokenp->line = root->tokenp->line;
				replacer->tokenp->type = instruction;
				strcpy(replacer->tokenp->str, "sub");
				replacer->next = root->next;

				newcomer->tokenp->type = reg;
				newcomer->tokenp->col = root->tokenp->col;
				newcomer->tokenp->line = root->tokenp->line;
				newcomer->tokenp->int_value = 0;

				newcomer->next = root->next->next;
				replacer->next->next = newcomer;
				root = replacer;
			}
			else if (!strcmp(root->tokenp->str, "li"))
			{
				// li $t0, 0xabcdef00
				// lui $1, oxabcd
				// ori $t0, $1, 0xef00
				node *tmp = root->next;
				node *newcomers[7];
				for (int i = 0; i < 6; i++)
				{
					newcomers[i] = (node *)malloc(sizeof(node));
					newcomers[i]->tokenp = (token *)malloc(sizeof(token));
					newcomers[i]->tokenp->line = root->tokenp->line;
					newcomers[i]->tokenp->col = root->tokenp->col;
				}

				newcomers[0]->tokenp->type = instruction;
				newcomers[1]->tokenp->type = instruction;
				strcpy(newcomers[0]->tokenp->str, "lui");
				strcpy(newcomers[1]->tokenp->str, "ori");

				newcomers[2]->tokenp->type = reg;
				newcomers[3]->tokenp->type = reg;
				newcomers[2]->tokenp->int_value = 1;
				newcomers[3]->tokenp->int_value = 1;

				int t = root->next->next->tokenp->int_value;
				newcomers[4]->tokenp->type = immediate;
				newcomers[5]->tokenp->type = immediate;
				newcomers[4]->tokenp->int_value = (t >> 16) & 0xffff; // higher half
				newcomers[5]->tokenp->int_value = t & 0xffff;

				newcomers[0]->next = newcomers[2];
				newcomers[2]->next = newcomers[4];
				newcomers[4]->next = newcomers[1];
				newcomers[1]->next = root->next;
				newcomers[5]->next = root->next->next->next;
				root->next->next = newcomers[3];
				newcomers[3]->next = newcomers[5];
				root = newcomers[0];
			}

			for (index = 0; index < InstrTypesNum; index++)
			{
				if (!strcmp(root->tokenp->str, InstrTypes[index]))
					break;
			}

			if (!root_statement)
			{
				root_statement = (node_statement *)malloc(sizeof(node_statement));
				// memset(root_statement->state, 0, sizeof(statement));
				node_stat_p = root_statement;
				node_stat_p->next = NULL;
				node_stat_p->state = (statement *)malloc(sizeof(statement));
			}
			else
			{
				node_stat_p->next = (node_statement *)malloc(sizeof(node_statement));
				node_stat_p = node_stat_p->next;
				node_stat_p->next = NULL;
				node_stat_p->state = (statement *)malloc(sizeof(statement));
				// memset(node_stat_p->state, 0, sizeof(statement));
			}

			if (index == InstrTypesNum)
			{
				printf("Error(%d,%d): No instruction named %s.\n",
					   root->tokenp->line, root->tokenp->col, root->tokenp->str);
				exit(-1);
			}

			node_stat_p->state->arg_lst = InstrTypesArgs[index];
			node_stat_p->state->label_target_num = 0;
			switch (InstrTypesArgs[index])
			{
			case RRR:
			{
				int reg_mul[3];
				for (int i = 0; i < 3; i++)
				{
					root = root->next;
					if (root && root->tokenp->type == reg)
						reg_mul[i] = root->tokenp->int_value;
					else
					{
						printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
							   tokentypes[reg], tokentypes[root->tokenp->type]);
						exit(-1);
					}
				}
				node_stat_p->state->opcode = opcodes[index];
				node_stat_p->state->funct = functs[index];
				node_stat_p->state->shamt = 0;
				if (node_stat_p->state->funct == 4 || node_stat_p->state->funct == 6 || node_stat_p->state->funct == 7)
				{
					node_stat_p->state->rd = reg_mul[0];
					node_stat_p->state->rt = reg_mul[1];
					node_stat_p->state->rs = reg_mul[2];
				}
				else
				{
					node_stat_p->state->rd = reg_mul[0];
					node_stat_p->state->rs = reg_mul[1];
					node_stat_p->state->rt = reg_mul[2];
				}
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->extend = None;
				break;
			}
			case RR:
			{
				int reg_mul[3];
				for (int i = 0; i < 2; i++)
				{
					root = root->next;
					if (root && root->tokenp->type == reg)
						reg_mul[i] = root->tokenp->int_value;
					else
					{
						printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
							   tokentypes[reg], tokentypes[root->tokenp->type]);
						exit(-1);
					}
				}
				node_stat_p->state->opcode = opcodes[index];
				node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = shamts[index];
				if (0)
				{
					node_stat_p->state->rs = reg_mul[0];
					node_stat_p->state->rt = reg_mul[1];
				}
				else
				{
					node_stat_p->state->rs = reg_mul[0];
					node_stat_p->state->rt = reg_mul[1];
				}
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->extend = None;
				break;
			}
			case R:
			{
				int reg_single;
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == reg)
				{
					reg_single = root->tokenp->int_value;
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[reg], tokentypes[root->tokenp->type]);
					exit(-1);
				}
				node_stat_p->state->opcode = opcodes[index];
				node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = shamts[index];
				if (functs[index] == 16 || functs[index] == 18)
				{
					node_stat_p->state->rd = reg_single;
				}
				else
				{
					node_stat_p->state->rs = reg_single;
				}
				if (functs[index] == 9)
					node_stat_p->state->rd = 31;
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->extend = None;
				break;
			}
			case L:
			{
				int reg_single;
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == label)
				{
					// reg_single = root->tokenp->int_value;
					strcpy(node_stat_p->state->label, root->tokenp->str);
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[label], tokentypes[root->tokenp->type]);
					exit(-1);
				}
				node_stat_p->state->opcode = opcodes[index];
				node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = shamts[index];
				node_stat_p->state->rd = reg_single;
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->extend = None;
				break;
			}
			case RL:
			{
				int reg_single;
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == reg)
				{
					reg_single = root->tokenp->int_value;
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[reg], tokentypes[root->tokenp->type]);
					exit(-1);
				}

				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == label)
				{
					// reg_single = root->tokenp->int_value;
					strcpy(node_stat_p->state->label, root->tokenp->str);
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[label], tokentypes[root->tokenp->type]);
					exit(-1);
				}

				node_stat_p->state->opcode = opcodes[index];
				node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = shamts[index];
				node_stat_p->state->rs = reg_single;
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->extend = None;
				break;
			}
			case RRL:
			{
				int imm = 0, reg_mul[3];
				for (int i = 0; i < 2; i++)
				{
					if (!(root->next))
					{
						printf("Error(%d): Lacking argument.\n", root->tokenp->line);
						exit(-1);
					}
					root = root->next;
					if (root && (root->tokenp->type == reg))
						reg_mul[i] = root->tokenp->int_value;
					else
					{
						printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
							   tokentypes[reg], tokentypes[root->tokenp->type]);
						exit(-1);
					}
				}

				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == label)
				{
					// reg_single = root->tokenp->int_value;
					strcpy(node_stat_p->state->label, root->tokenp->str);
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[label], tokentypes[root->tokenp->type]);
					exit(-1);
				}

				node_stat_p->state->opcode = opcodes[index];
				node_stat_p->state->funct = functs[index];
				node_stat_p->state->shamt = imm;
				node_stat_p->state->rs = reg_mul[0];
				node_stat_p->state->rt = reg_mul[1];
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->extend = None;
				break;
			}
			case RRI5:
			{
				int imm, reg_mul[3];
				for (int i = 0; i < 2; i++)
				{
					if (!(root->next))
					{
						printf("Error(%d): Lacking argument.\n", root->tokenp->line);
						exit(-1);
					}
					root = root->next;
					if (root && (root->tokenp->type == reg))
						reg_mul[i] = root->tokenp->int_value;
					else
					{
						printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
							   tokentypes[reg], tokentypes[root->tokenp->type]);
						exit(-1);
					}
				}
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == immediate)
				{
					if (root->tokenp->int_value > (pow(2, 5) - 1) || root->tokenp->int_value < 0)
					{
						printf("Error(%d,%d): Immediate %d overflow.\n", root->tokenp->line, root->tokenp->col, root->tokenp->int_value);
						exit(-1);
					}
					else
					{
						imm = root->tokenp->int_value;
					}
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[immediate], tokentypes[root->tokenp->type]);
					exit(-1);
				}
				node_stat_p->state->opcode = opcodes[index];
				node_stat_p->state->funct = functs[index];
				node_stat_p->state->shamt = imm;
				node_stat_p->state->rd = reg_mul[0];
				node_stat_p->state->rt = reg_mul[1];
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->extend = None;
				break;
			}
			case RRI32se:
			{
				int imm, reg_mul[3];
				for (int i = 0; i < 2; i++)
				{
					if (!(root->next))
					{
						printf("Error(%d): Lacking argument.\n", root->tokenp->line);
						exit(-1);
					}
					root = root->next;
					if (root && (root->tokenp->type == reg))
						reg_mul[i] = root->tokenp->int_value;
					else
					{
						printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
							   tokentypes[reg], tokentypes[root->tokenp->type]);
						exit(-1);
					}
				}
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == immediate)
				{
					if (root->tokenp->int_value > SIGNED_16_MAX || root->tokenp->int_value < SIGNED_16_MIN)
					{
						printf("Error(%d,%d): Immediate %d overflow.\n", root->tokenp->line, root->tokenp->col, root->tokenp->int_value);
						exit(-1);
					}
					else
					{
						imm = root->tokenp->int_value;
					}
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[immediate], tokentypes[root->tokenp->type]);
					exit(-1);
				}
				node_stat_p->state->opcode = opcodes[index];
				// node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = imm;
				node_stat_p->state->rt = reg_mul[0];
				node_stat_p->state->rs = reg_mul[1];
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->immediate_value = imm;
				node_stat_p->state->extend = SignExtend;
				break;
			}
			case RRI32ze:
			{
				int imm, reg_mul[3];
				for (int i = 0; i < 2; i++)
				{
					if (!(root->next))
					{
						printf("Error(%d): Lacking argument.\n", root->tokenp->line);
						exit(-1);
					}
					root = root->next;
					if (root && (root->tokenp->type == reg))
						reg_mul[i] = root->tokenp->int_value;
					else
					{
						printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
							   tokentypes[reg], tokentypes[root->tokenp->type]);
						exit(-1);
					}
				}
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == immediate)
				{
					if (root->tokenp->int_value > SIGNED_16_MAX || root->tokenp->int_value < SIGNED_16_MIN)
					{
						printf("Error(%d,%d): Immediate %d overflow.\n", root->tokenp->line, root->tokenp->col, root->tokenp->int_value);
						exit(-1);
					}
					else
					{
						imm = root->tokenp->int_value;
					}
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[immediate], tokentypes[root->tokenp->type]);
					exit(-1);
				}
				node_stat_p->state->opcode = opcodes[index];
				// node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = imm;
				node_stat_p->state->rt = reg_mul[0];
				node_stat_p->state->rs = reg_mul[1];
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->immediate_value = imm;
				node_stat_p->state->extend = ZeroExtend;
				break;
			}
			case RI16:
			{
				int imm, reg_mul[3];
				for (int i = 0; i < 1; i++)
				{
					if (!(root->next))
					{
						printf("Error(%d): Lacking argument.\n", root->tokenp->line);
						exit(-1);
					}
					root = root->next;
					if (root && (root->tokenp->type == reg))
						reg_mul[i] = root->tokenp->int_value;
					else
					{
						printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
							   tokentypes[reg], tokentypes[root->tokenp->type]);
						exit(-1);
					}
				}
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == immediate)
				{
					if (root->tokenp->int_value > SIGNED_16_MAX || root->tokenp->int_value < SIGNED_16_MIN)
					{
						printf("Error(%d,%d): Immediate %d overflow.\n", root->tokenp->line, root->tokenp->col, root->tokenp->int_value);
						exit(-1);
					}
					else
					{
						imm = root->tokenp->int_value;
					}
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[immediate], tokentypes[root->tokenp->type]);
					exit(-1);
				}
				node_stat_p->state->opcode = opcodes[index];
				// node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = imm;
				node_stat_p->state->rt = reg_mul[0];
				// node_stat_p->state->rs = reg_mul[0];
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->immediate_value = imm;
				node_stat_p->state->extend = LowerPartZeroExtend;
				break;
			}
			case RI16R:
			{
				int imm, reg_mul[3];
				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root && (root->tokenp->type == reg))
					reg_mul[0] = root->tokenp->int_value;
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[reg], tokentypes[root->tokenp->type]);
					exit(-1);
				}

				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root->tokenp->type == immediate)
				{
					if (root->tokenp->int_value > SIGNED_16_MAX || root->tokenp->int_value < SIGNED_16_MIN)
					{
						printf("Error(%d,%d): Immediate %d overflow.\n", root->tokenp->line, root->tokenp->col, root->tokenp->int_value);
						exit(-1);
					}
					else
					{
						imm = root->tokenp->int_value;
					}
				}
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[immediate], tokentypes[root->tokenp->type]);
					exit(-1);
				}

				if (!(root->next))
				{
					printf("Error(%d): Lacking argument.\n", root->tokenp->line);
					exit(-1);
				}
				root = root->next;
				if (root && (root->tokenp->type == reg))
					reg_mul[1] = root->tokenp->int_value;
				else
				{
					printf("Error(%d,%d): Expected %s but got %s.\n", root->tokenp->line, root->tokenp->col,
						   tokentypes[reg], tokentypes[root->tokenp->type]);
					exit(-1);
				}

				node_stat_p->state->opcode = opcodes[index];
				// node_stat_p->state->funct = functs[index];
				// node_stat_p->state->shamt = imm;
				node_stat_p->state->rt = reg_mul[0];
				node_stat_p->state->rs = reg_mul[1];
				node_stat_p->state->linenum = curr_parse_line_num;
				node_stat_p->state->immediate_value = imm;
				node_stat_p->state->extend = None;
				break;
			}
			}
			pre = root;
			root = root->next;
			curr_parse_line_num++;
		}
	}
	return root_statement;
}

void display_single_statement(node_statement *root)
{
	printf("%d\t%d\t%d\t%s\n", root->state->opcode, root->state->funct, root->state->linenum, root->state->label);
	return;
}

void display_statement(node_statement *root)
{
	while (root)
	{
		display_single_statement(root);
		root = root->next;
	}
}

hex_code *build_mips(node_statement *root)
{
	// enum argument_list {R, RRR, RRI5, RRI32se, RRI32ze, RI16, RI16R, RL, L, RRL};
	hex_code *hex_root = NULL, *curr_hex_p = NULL;
	node_statement *start = root;
	int tmp;
	while (root)
	{
		if (root->state->arg_lst == NotInstrButLabel)
		{
			root = root->next;
			continue;
		}
		if (!hex_root)
		{
			hex_root = (hex_code *)malloc(sizeof(hex_root));
			hex_root->next = NULL;
			curr_hex_p = hex_root;
		}
		else
		{
			curr_hex_p->next = (hex_code *)malloc(sizeof(hex_code));
			curr_hex_p = curr_hex_p->next;
			curr_hex_p->next = NULL;
		}
		switch (root->state->arg_lst)
		{
		case RRR:
		{
			// ins = (op<<26) | (rs<<21) | (rt<<16) | (rd<<11) | (shamt<<6) | func;
			curr_hex_p->hex = (root->state->opcode << 26) |
							  (root->state->rs << 21) |
							  (root->state->rt << 16) |
							  (root->state->rd << 11) |
							  (root->state->shamt << 6) |
							  (root->state->funct);
			break;
		}
		case R:
		{
			if (root->state->funct == 16 || root->state->funct == 18)
			{
				curr_hex_p->hex = 0 | (root->state->rd << 11) | (root->state->funct);
				// printf("%d\t%d\t%d\t\n", root->state->opcode, root->state->rd, root->state->funct);
			}
			else if (root->state->funct == 9)
			{
				curr_hex_p->hex = (root->state->rs << 21) | (root->state->rd << 11) | (root->state->funct);
			}
			else
			{
				curr_hex_p->hex = (root->state->rs << 21) | (root->state->funct);
			}
			break;
		}
		case RRI5:
		{
			curr_hex_p->hex = (root->state->opcode << 26) |
							  //   (root->state->rs << 21) |
							  (root->state->rt << 16) |
							  (root->state->rd << 11) |
							  (root->state->shamt << 6) |
							  (root->state->funct);
			break;
		}
		case RI16R:
		{
			curr_hex_p->hex = (root->state->opcode << 26) |
							  (root->state->rs << 21) |
							  (root->state->rt << 16) |
							  (root->state->immediate_value);
			break;
		}
		case RRI32ze:
		case RRI32se:
		{
			curr_hex_p->hex = (root->state->opcode << 26) |
							  (root->state->rs << 21) |
							  (root->state->rt << 16) |
							  ((root->state->immediate_value) & 0x0000ffff);
			break;
		}
		case RI16:
		{
			curr_hex_p->hex = (root->state->opcode << 26) |
							  (root->state->rt << 16) |
							  (root->state->immediate_value);
			break;
		}
		case L:
		{
			node_statement *tmp = start;
			int found = 0;
			while (tmp)
			{
				if (tmp->state->label_target_num == 0)
				{
					tmp = tmp->next;
					continue;
				}
				else if (!strcmp(tmp->state->label, root->state->label))
				{
					found = 1;
					break;
				}
				else
				{
					tmp = tmp->next;
					continue;
				}
			}
			if (!found)
			{
				printf("Error(%d): No label named %s is defined.\n",
					   root->state->linenum, root->state->label);
				exit(-1);
			}
			else
			{
				curr_hex_p->hex = (root->state->opcode << 26) |
								  ((0x00100000 + tmp->state->label_target_num - 1));
			}
			break;
		}
		case RL:
		{
			node_statement *tmp = start;
			int found = 0;
			while (tmp)
			{
				if (tmp->state->label_target_num == 0)
				{
					tmp = tmp->next;
					continue;
				}
				else if (!strcmp(tmp->state->label, root->state->label))
				{
					found = 1;
					break;
				}
				else
				{
					tmp = tmp->next;
					continue;
				}
			}
			if (!found)
			{
				printf("Error(%d): No label named %s is defined.\n",
					   root->state->linenum, root->state->label);
				exit(-1);
			}
			else
			{
				int t1 = tmp->state->label_target_num;
				int t2 = root->state->linenum;
				curr_hex_p->hex = (root->state->opcode << 26) |
								  (root->state->rs << 21) |
								  (t1 - t2 - 1);
			}
			break;
		}
		case RRL:
		{
			node_statement *tmp = start;
			int found = 0;
			while (tmp)
			{
				if (tmp->state->label_target_num == 0)
				{
					tmp = tmp->next;
					continue;
				}
				else if (!strcmp(tmp->state->label, root->state->label))
				{
					found = 1;
					break;
				}
				else
				{
					tmp = tmp->next;
					continue;
				}
			}
			if (!found)
			{
				printf("Error(%d): No label named %s is defined.\n",
					   root->state->linenum, root->state->label);
				exit(-1);
			}
			else
			{
				int t1 = tmp->state->label_target_num;
				int t2 = root->state->linenum;
				curr_hex_p->hex = (root->state->opcode << 26) |
								  (root->state->rs << 21) |
								  (root->state->rt << 16) |
								  (t1 - t2 - 1);
			}
			break;
		}
		case RR:
		{
			curr_hex_p->hex = (root->state->opcode << 26) |
							  (root->state->rs << 21) |
							  (root->state->rt << 16) |
							  (root->state->funct);
			break;
		}
		case NotInstrButLabel:
		{
			break;
		}
		}
		root = root->next;
	}
	return hex_root;
}

void write_mips(hex_code *root, char *filename)
{
	int i = 0;
	while (filename[i++] != '.')
		;
	filename[i] = 'c';
	filename[i + 1] = 'o';
	filename[i + 2] = 'e';
	filename[i + 3] = '\0';
	char header[500] = "; This .COE file specifies the contents for a block memory of depth=16, and width=32.\nmemory_initialization_radix=16;\nmemory_initialization_vector=\n";
	FILE *of = fopen(filename, "wt");
	fprintf(of, "%s", header);
	while (root)
	{
		fprintf(of, "%0.8x", root->hex);
		root = root->next;
		if (root)
		{
			fprintf(of, ",\n");
		}
		else
		{
			fprintf(of, ";");
		}
	}
}
