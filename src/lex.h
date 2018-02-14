#ifndef LEX_H
#define LEX_H

#include <stdlib.h>

//------------------Definitions----------------------------
const char* identities[];
enum tk_codes{
  //end of file
  END,
  //identifier
  ID,
  //keyword
  BREAK,
  CHAR,
  DOUBLE, //4
  ELSE,
  FOR,
  IF,
  INT,
  RETURN,
  STRUCT,//10
  VOID,
  WHILE,
  //constant
  CT_INT,
  EXP,
  CT_REAL, //15
  ESC,
  CT_CHAR,
  CT_STRING,
  //delimiters
  COMMA,
  SEMICOLON, //20
  LPAR,
  RPAR,
  LBRACKET,
  RBRACKET,
  LACC, //25
  RACC,
  //operators
  ADD,
  SUB,
  MUL,
  DIV, //30
  DOT,
  AND,
  OR,
  NOT,
  ASSIGN,//35
  EQUAL,
  NOTEQ,
  LESS, //38
  LESSEQ,
  GREATER,
  GREATEREQ,
  //consumed without forming lexical atoms
  SPACE,
  LINECOMMENT,
  COMMENT
};

struct Token {
	int code; //code name
	union {
		char *text; //used for string type varibles
		long int i; //for int/char types
		double r;   //used for double
	};

	int line;     //input file line
	struct Token *next;
};

#define SAFEALLOC(var, type) if((var=(type)malloc(sizeof(Token_t))) == NULL) err("not enough memory")
#define SAFEALLOC2(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

typedef struct Token *Token_t;

//----------------Global variables--------------------------------------
int line;
Token_t lastToken, firstToken, crtTk;
//pointer to current character
char* pCrCh;

//--------------Function Implementation-------------------------------

Token_t addTK(int code);
void err(const char *fmt, ...);
void tkerr(const Token_t tk, const char *fmt, ...);
int getNextToken();
int consume(int code);
void printTokens();
#endif
