#include "lex.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

const char* identities[] = {
		  "END",
		  "ID",
		  "BREAK",
		  "CHAR",
		  "DOUBLE",
		  "ELSE",
		  "FOR",
		  "IF",
		  "INT",
		  "RETURN",
		  "STRUCT",
		  "VOID",
		  "WHILE",
		  "CT_INT",
		  "EXP",
		  "CT_REAL",
		  "ESC",
		  "CT_CHAR",
		  "CT_STRING",
		  "COMMA",
		  "SEMICOLON",
		  "LPAR",
		  "RPAR",
		  "LBRACKET",
		  "RBRACKET",
		  "LACC",
		  "RACC",
		  "ADD",
		  "SUB",
		  "MUL",
		  "DIV",
		  "DOT",
		  "AND",
		  "OR",
		  "NOT",
		  "ASSIGN",
		  "EQUAL",
		  "NOTEQ",
		  "LESS",
		  "LESSEQ",
		  "GREATER",
		  "GREATEREQ",
		  "SPACE",
		  "LINECOMMENT",
		  "COMMENT"
};

double my_x;
int is_e;

//structure to keep token
Token_t addTk(int code) {

	Token_t tk;
	tk = malloc(sizeof(struct Token));
	if (tk == NULL)
		err("Could not allocate memory for Token");
	tk->code = code;
	tk->line = line;
	tk->next = NULL;

	if (lastToken) {
		lastToken->next = tk;
	} else
		firstToken = tk;

	lastToken = tk;
	return tk;
}

void err(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "error:");
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
	exit(-1);
}

//print error at specific lines
void tkerr(const Token_t tk, const char *fmt, ...) {

	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "error on line %d:", tk->line);
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
	exit(-1);
}

char* createString(const char *pStartCh, char* pCrt) {
	int size = pCrt - pStartCh;
	char* text = malloc(sizeof(char) * size +1);
	int i = 0;
	char*p;
	for (p = (char *) pStartCh; p < pCrt; p++) {
		text[i] = *p;
		i++;
	}

	text[i] = '\0';
	//printf("%s\n", text);
	return text;
}

void printTokens(){
	Token_t t;
	for(t=firstToken; t != lastToken; t=t->next){
		if(t->code == 1)
			printf("token :  %s | text: %s | line: %d\n", identities[t->code], t->text, t->line);
		else if(t->code == 13)
			printf("token :  %s | value: %ld | line: %d\n", identities[t->code], t->i, t->line);
		else if(t->code == 15)
			printf("token :  %s | value: %f | line: %d\n", identities[t->code], t->r, t->line);
		else
			printf("token :  %s | line: %d\n", identities[t->code], t->line);
	}
}


int getNextToken() {

	int state = 0, nCh;
	char ch, prev;
	const char *pStartCh;
	Token_t tk;

	while (1) {
		ch = *pCrCh;
		switch (state) {
		case 0:
			if (isalpha(ch) || ch == '_') {
				//start of identifier(ID) case
				pStartCh = pCrCh;
				pCrCh++;
				state = 1;
			} else if (ch == '=') {
				pCrCh++;
				state = 3;
			} else if (ch == ' ' || ch == '\r' || ch == '\t') {
				pCrCh++;
			} else if (ch == '\n') {
				line++;
				pCrCh++;
			} else if (ch == '\0') { //end of input string
				addTk(END);
				return END;
			} else if(ch == '+'){
				pCrCh++;
				state = 6;
			} else if(ch == '-'){
				pCrCh++;
				state = 7;
			} else if(ch == '!'){
				pCrCh++;
				state = 8;
			}
			else if(ch == '<'){
				pCrCh++;
				state = 11;
			}else if(ch == '>'){
				pCrCh++;
				state = 14;
			}else if (ch == '*'){
				pCrCh++;
				state = 17;
			}else if (ch == '/'){
				pCrCh++;
				if(*pCrCh == '/')
					state = 36;
				else if(*pCrCh == '*'){
					state = 37;
					pCrCh++;
				}else{
					state = 18;
				}
			}else if (ch == '.'){
				pCrCh++;
				state = 19;
			}else if(ch == '&'){
				pCrCh++;
				if(ch == '&'){
					pCrCh++;
					state = 20;
				}else
					tkerr(addTk(END), "invalid character");

			}else if(ch == '|'){
				pCrCh++;
				if(ch == '|'){
					pCrCh++;
					state = 21;
				}else
					tkerr(addTk(END), "invalid character");
			}else if(ch == ',') {
				pCrCh++;
				state = 22;
			}else if(ch == ';'){
				pCrCh++;
				state = 23;
			}else if(ch == '('){
				pCrCh++;
				state = 24;
			}else if(ch == ')'){
				pCrCh++;
				state = 25;
			}else if(ch == '['){
				pCrCh++;
				state = 26;
			}else if(ch == ']'){
				pCrCh++;
				state = 27;
			}else if(ch == '{'){
				pCrCh++;
				state = 28;
			}else if(ch == '}'){
				pCrCh++;
				state = 29;
			}else if (ch == '0'){
				pStartCh = pCrCh;
				pCrCh++;
				my_x = 0;
				if(*pCrCh =='.'){
					state = 39;
					pCrCh++;
				}else if(*pCrCh == 'e'){
					state = 40;
					pCrCh++;
				}else
					state = 30;
			}else if (isdigit(ch) && ch != 0){
				//start of decimal constant case
				pStartCh = pCrCh;
				pCrCh++;
				state = 35;
			}else if (ch == '"'){
				//start of constant string
				pCrCh++;
				state = 38;
			}else if(ch == '\''){
				tk = addTk(CT_CHAR);
				pCrCh++;
				tk->i = *pCrCh;
				pCrCh++;
				if(*pCrCh != '\'')
					tkerr(addTk(END), "invalid character");
				pCrCh++;
			}else
				tkerr(addTk(END), "invalid character");
			break;
		case 1:
			if (isalnum(ch) || ch == '_') {
				pCrCh++;
			} else
				state = 2;
			break;
		case 2:
			nCh = pCrCh - pStartCh; //the id length
			//keywords test
			if (nCh == 5 && !memcmp(pStartCh, "break", 5))
				tk = addTk(BREAK);
			else if (nCh == 4 && !memcmp(pStartCh, "char", 4))
				tk = addTk(CHAR);
			else if (nCh == 6 && !memcmp(pStartCh, "double", 6))
				tk = addTk(DOUBLE);
			else if (nCh == 4 && !memcmp(pStartCh, "else", 4))
				tk = addTk(ELSE);
			else if (nCh == 3 && !memcmp(pStartCh, "for", 3))
				tk = addTk(FOR);
			else if (nCh == 2 && !memcmp(pStartCh, "if", 2))
				tk = addTk(IF);
			else if (nCh == 3 && !memcmp(pStartCh, "int", 3)) {
				tk = addTk(INT);
			} else if (nCh == 6 && !memcmp(pStartCh, "return", 6))
				tk = addTk(RETURN);
			else if (nCh == 6 && !memcmp(pStartCh, "struct", 6))
				tk = addTk(STRUCT);
			else if (nCh == 4 && !memcmp(pStartCh, "void", 4))
				tk = addTk(VOID);
			else if (nCh == 5 && !memcmp(pStartCh, "while", 5))
				tk = addTk(WHILE);
			else { //if no then it is ID
				tk = addTk(ID);
				tk->text = createString(pStartCh, pCrCh);
			}
			return tk->code;
		case 3:
			if (ch == '=') {
				pCrCh++;
				state = 4;
			} else
				state = 5;
			break;
		case 4:
			addTk(EQUAL);
			return EQUAL;
		case 5:
			addTk(ASSIGN);
			return ASSIGN;
		case 6:
			addTk(ADD);
			return ADD;
		case 7:
			addTk(SUB);
			return SUB;
		case 8:
			if(ch == '='){
				pCrCh++;
				state = 9;
			}else
				state = 10;
			break;
		case 9:
			addTk(NOTEQ);
			return NOTEQ;
		case 10:
			addTk(NOT);
			return NOT;
		case 11:
			if(ch == '='){
				pCrCh++;
				state = 12;
			}else
				state = 13;
			break;
		case 12:
			addTk(LESSEQ);
			return LESSEQ;
		case 13:
			addTk(LESS);
			return LESS;
		case 14:
			if(ch == '='){
				pCrCh++;
				state = 15;
			}else
				state = 16;
			break;
		case 15:
			addTk(GREATEREQ);
			return GREATEREQ;
		case 16:
			addTk(GREATER);
			return GREATER;
		case 17:
			addTk(MUL);
			return MUL;
		case 18:
			addTk(DIV);
			return DIV;
		case 19:
			addTk(DOT);
			return DOT;
		case 20:
			addTk(AND);
			return AND;
		case 21:
			addTk(OR);
			return OR;
		case 22:
			addTk(COMMA);
			return COMMA;
		case 23:
			addTk(SEMICOLON);
			return SEMICOLON;
		case 24:
			addTk(LPAR);
			return LPAR;
		case 25:
			addTk(RPAR);
			return RPAR;
		case 26:
			addTk(LBRACKET);
			return LBRACKET;
		case 27:
			addTk(RBRACKET);
			return RBRACKET;
		case 28:
			addTk(LACC);
			return LACC;
		case 29:
			addTk(RACC);
			return RACC;
		case 30:
			if(ch == 'x'){
				pCrCh++;
				pStartCh = pCrCh;
				state = 31;
			}else{
				pStartCh = pCrCh;
				state = 32;
			}
			break;
		case 31://hex case
			if (isdigit(ch) || (ch >= 'a' && 'f' <= ch) || (ch >= 'a' && 'f' <= ch)) {
				pCrCh++;
			} else
				state = 33;
			break;
		case 32://octal case
			if(ch >='0' && ch <= '7')
				pCrCh++;
			else
				state = 34;
			break;
		case 33: //final value of constant int in hex case
			nCh = pCrCh - pStartCh;
			char *val = createString(pStartCh, pCrCh);
			tk = addTk(CT_INT);
			tk->i = (int)strtol(val, NULL, 16);
			return tk->i;
		case 34:
			nCh = pCrCh - pStartCh;
			char *val1 = createString(pStartCh, pCrCh);
			tk = addTk(CT_INT);
			tk->i = (int)strtol(val1, NULL, 8);
			return tk->i;
		case 35:
			if(isdigit(ch)){
				pCrCh++;
			}else{
				if(ch == '.'){
					is_e = 0;
					state = 39;
					pCrCh++;
					break;
				}else if(ch == 'e' || ch == 'E'){
					state = 40;
					pCrCh++;
					break;
				}

				nCh = pCrCh - pStartCh;
				char *val2 = createString(pStartCh, pCrCh);
				tk = addTk(CT_INT);
				tk->i = (int)strtol(val2, NULL, 10);
				return tk->i;
			}
			break;
		case 36: //line comment
			while(*pCrCh != '\n'){
				pCrCh++;
			}
			line++;
			state = 0;
			break;
		case 37: //multiline comment
			while(*pCrCh != '/'){
				prev = *pCrCh;
				pCrCh++;
				if(*pCrCh == '/' && prev == '*'){
					pCrCh++;
					break;
				}
			}
			state = 0;
			break;
		case 38:
			pStartCh = pCrCh;
			char prev = *pCrCh;
			if(*pCrCh == '"'){
				tk = addTk(CT_STRING);
				tk->text = "";
				pCrCh++;
				return tk->code;
			}else if(prev == '\''){
				tk = addTk(CT_CHAR);
				pCrCh++;
				val = createString(pStartCh, pCrCh);
				tk->text = "";
				pCrCh++;
				return tk->code;
			}

			while( (*pCrCh != '"' && prev != " ") && *pCrCh != '\''){
				pCrCh++;
			}
			nCh = pCrCh - pStartCh;
			val = createString(pStartCh, pCrCh);
			if(*pCrCh == '"')
				tk = addTk(CT_STRING);
			else
				tk = addTk(CT_CHAR);
			tk->text = val;
			pCrCh++;
			return tk->code;
		case 39:
			if(isdigit(ch) || ch == '.'){
				pCrCh++;
			}else if(ch == 'e' && !is_e){
				pCrCh++;
				is_e = 1;
				if(*pCrCh == '-' || *pCrCh == '+')
					pCrCh++;
			}else{
				nCh = pCrCh - pStartCh;
				char *val2 = createString(pStartCh, pCrCh);
				tk = addTk(CT_REAL);
				//printf("val2: %f\n", (double)strtod(val2, NULL));
				tk->r = (double)strtod(val2, NULL);
				return tk->code;
			}
			break;
		case 40:
			if(ch == 'e' && !is_e){
				pCrCh++;
				is_e = 1;
				if(*pCrCh == '-' || *pCrCh == '+')
					pCrCh++;
			}else if(isdigit(ch))
				pCrCh++;
				else{
					nCh = pCrCh - pStartCh;
					char *val2 = createString(pStartCh, pCrCh);
					tk = addTk(CT_REAL);
					tk->r = my_x*(double)strtod(val2, NULL);
					return tk->code;
				}
			break;
		}

	}

	tkerr(crtTk, "This should not happen !!");
	return NULL;
}






	  
	
  
