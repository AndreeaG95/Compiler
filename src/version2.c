/*
 * version2.c
 *
 *  Created on: 16.05.2017
 *      Author: Andreea
 */

/*
int consume(int code){
	printf("expected:%d got:%d \n", code, crtTk->code);
	if(crtTk->code == code){
		consumedTk=crtTk;
		crtTk=crtTk->next;
		return 1;
	}

	return 0;
}

int typeBase(){
	Token_t startTk=crtTk;
	if(consume(INT)){
		return 1;
	}
	if(consume(DOUBLE)){
		return 1;
	}
	if(consume(CHAR)){
		return 1;
	}
	if(consume(STRUCT)){
		if(consume(ID))
			return 1;
		crtTk=startTk;
	}
	return 0;
}

int exprPrimary(){
	Token_t startTk=crtTk;

	if(consume(CT_INT)) return 1;
	if(consume(CT_REAL)) return 1;
	if(consume(CT_CHAR)) return 1;
	if(consume(CT_STRING)) return 1;

	if(consume(LPAR)){
		if(expr()){
			if(consume(RPAR)){
				return 1;
			}else tkerr(crtTk,"missing )");
		}else tkerr(crtTk,"invalid expression after (");
		crtTk=startTk;
	}

	if(consume(ID)){
		if(consume(LPAR)){
			if(expr()){
				while(1){
					if(consume(COMMA)){
						if(expr()){

						}else{
							tkerr(crtTk,"missing expression");
						}
					}else
						break;
				}
			}
			if(!consume(RPAR)) tkerr(crtTk,"missing )");
		}
		//crtTk=startTk;
		return 1;
	}
	return 0;
}

int exprPostFix(){
	Token_t startTk=crtTk;
	if(!exprPrimary()){
		crtTk = startTk;
		return 0;
	}

	while(1){
		startTk = crtTk;
		if(consume(LBRACKET) && expr() && consume(RBRACKET))
			continue;
		crtTk = startTk;
		if(consume(DOT) && consume(ID))
			continue;
		break;
	}
	return 1;
}

//exprUnary: ( SUB | NOT ) exprUnary | exprPostfix ;
int exprUnary(){
	if(exprPostFix()) return 1;

	if((consume(SUB))){
		if(exprUnary()){
			return 1;
		}else
			tkerr(crtTk, "invalid expression");
	}else if(consume(NOT)){
		if(exprUnary()){
			return 1;
		}else
			tkerr(crtTk, "invalid expression");
	}

	return 0;
}

//typeName: typeBase arrayDecl?
int typeName(){
	if(!typeBase()) return 0;
	arrayDecl();
	return 1;
}

//exprCast: LPAR typeName RPAR exprCast | exprUnary ;
int exprCast(){
	if(exprUnary()){
		return 1;
	}

	if(!consume(LPAR)) tkerr(crtTk,"missing ( after cast");
	if(!typeName()) tkerr(crtTk,"invalid expression after (");
	if(!consume(RPAR)) tkerr(crtTk,"missing )");
	if(!exprCast()) tkerr(crtTk,"invalid cast");
	return 1;
}

//exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
int exprMul(){
	Token_t startTk=crtTk;
	if(exprCast()){
		return 1;
	}

	if(exprMul()){
		if(consume(MUL) || consume(DIV)){
			if(exprCast()){
				return 1;
			}
		}
		crtTk=startTk;
	}
	return 0;
}

//exprAdd: exprUnary ( ADD | SUB ) exprMul | exprMul
int exprAdd(){
	Token_t startTk=crtTk;

	if(exprUnary()){
		if(consume(ADD) || consume(SUB)){
			if(exprMul()){
				return 1;
			}
		}
		crtTk=startTk;
	}

	if(exprMul()){
		return 1;
	}

	return 0;
}

//exprRel: exprUnary( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd ;
int exprRel(){
	Token_t startTk=crtTk;

	if(exprUnary()){
		if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
			if(exprAdd()){
				return 1;
			}
		}
		crtTk=startTk;
	}

	if(exprAdd()){
		return 1;
	}

	return 0;
}

//exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel ;
int exprEq(){

    if(!exprRel())
        return 0;

    while(consume(EQUAL) || consume(NOTEQ)){
        if(!exprRel())
            tkerr(crtTk, "invalid and expression\n");
    }

    return 1;
}

//exprAnd: exprAnd AND exprEq | exprEq ;
int exprAnd(){

	if(!exprEq()) return 0;

    while(consume(AND))
    {
        if(!exprEq())
            tkerr(crtTk, "invalid expression equal");
    }

    return 1;
}

//exprOr OR exprAnd | exprAnd
int exprOr(){

	if(!exprAnd()) return 0;

	while(consume(OR)){
		if(!exprAnd())
			tkerr(crtTk,"invalid expression");
	}

	return 1;
}

//exprUnary ASSIGN exprAssign | exprOr ;
int exprAssign(){
	Token_t startTk=crtTk;

	if(exprUnary() && consume(ASSIGN) && exprAssign())
		return 1;

	crtTk=startTk; // restore crtTk to the entry value

	return exprOr();
}

int expr(){
	return exprAssign();
}

//arrayDecl: LBRACKET expr? RBRACKET
int arrayDecl(){
	if(!consume(LBRACKET))return 0;
	expr();
	if(!consume(RBRACKET))tkerr(crtTk,"missing }");
	return 1;
}

//declVar:  typeBase ID arrayDecl? ( COMMA ID arrayDecl? )* SEMICOLON ;
int declVar(){
	if(!typeBase()) return 0;
	if(!consume(ID)) return 0;
	arrayDecl();
	while(consume(COMMA)){
		consume(ID);
		arrayDecl();
	}
	if(!consume(SEMICOLON)) tkerr(crtTk,"missign ;");
	return 1;
}

//declStruct: STRUCT ID LACC declVar* RACC SEMICOLON ;
int declStruct(){
	printf("Token %d\n", crtTk->code);
	if(!consume(STRUCT)) return 0;
	if(!consume(ID)) tkerr(crtTk,"syntax error");
	if(!consume(LACC))tkerr(crtTk,"missing{ after struct declaration");
	while(declVar()){

	}
	if(!consume(RACC))tkerr(crtTk,"missing } or syntax error");
	if(!consume(SEMICOLON)) tkerr(crtTk,"missing ;");
	return 1;
}

//funcArg: typeBase ID arrayDecl? ;
int funcArg(){

	if(!typeBase()) tkerr(crtTk,"invalid syntax");
	if(!consume(ID)) tkerr(crtTk,"invalid syntax");
	arrayDecl();
	return 1;
}

//stm
int stm(){
	Token_t startTk=crtTk;

	if(consume(IF)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(!stm())
								return 0;
						}else
							return 1;
					}tkerr(crtTk,"invalid syntax");
				}tkerr(crtTk,"missing )");
			}tkerr(crtTk,"invalid expression");
		}tkerr(crtTk,"missing (");
	}
	crtTk=startTk;

	if(consume(BREAK)){
		if(consume(SEMICOLON)){
			return 1;
		}tkerr(crtTk,"missing ;");
	}
	crtTk=startTk;

	//RETURN expr? SEMICOLON
	if(consume(RETURN)){
		expr();
		if(consume(SEMICOLON)){
			return 1;
		}tkerr(crtTk,"missing ;");
	}
	crtTk=startTk;
	//WHILE LPAR expr RPAR stm

	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						return 1;
					}else tkerr(crtTk,"missing while statement");
				}else tkerr(crtTk,"missing )");
			}else tkerr(crtTk,"invalid expression after (");
		}else tkerr(crtTk,"missing ( after while");
	}
	crtTk=startTk;

	//FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
	if(consume(FOR)){
		if(consume(LPAR)){
			expr();
			if(consume(SEMICOLON)){
				expr();
				if(consume(SEMICOLON)){
					expr();
					if(consume(RPAR)){
						if(stm()){
							return 1;
						}else
							tkerr(crtTk,"missing for statement");
					}else tkerr(crtTk,"missing )");
				}else tkerr(crtTk,"missing;");
			}else tkerr(crtTk,"missing ;");
		}else tkerr(crtTk,"missing ) in for");
	}
	crtTk=startTk;

	if(stmCompound()) return 1;

	if(expr()){
		if(consume(SEMICOLON))
			return 1;
	}

	return 0;
}

//stmCompound: LACC ( declVar | stm )* RACC ;
int stmCompound(){
	if(!consume(LACC)) return 0;
	while(declVar() || stm());
	if(!consume(RACC)) tkerr(crtTk,"missing } ");
	return 1;
}

int declFunc(){

	if(typeBase()){
		consume(MUL);
	}else if(!consume(VOID)){
		tkerr(crtTk,"invalid function declaration");
	}else
		tkerr(crtTk,"invalid function declaration");

	if(!consume(LPAR)) tkerr(crtTk,"missing ( after function declaration");

	if(funcArg()){
		while(1){
			if(consume(COMMA)){
				if(!funcArg())
					tkerr(crtTk,"invalid function");
			}else
				break;
		}
	}
	if(!consume(RPAR)) tkerr(crtTk,"missing )");
	if(!stmCompound()) tkerr(crtTk,"invalid function declaration");

	if(stmCompound()) return 1;

	return 1;
}

//unit: ( declStruct | declFunc | declVar )* END ;
int unit(){
	while(1){
		if(declVar()){
		}
		else if(declStruct()){
			}else if(declFunc()){
				}else break;
	}

	if(!consume(END)) return 0;
	return 1;
}
*/
