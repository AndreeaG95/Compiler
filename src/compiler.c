#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lex.h"
#include "sym.h"
#include "vm.h"

//pentru generare cod
Instr * crtLoopEnd;
int sizeArgs, offset;
extern Instr *instructions,*lastInstruction;
extern int crtDepth; //("adancimea" contextului curent, initial 0)
extern Symbol* crtFunc; //(pointer la simbolul functiei daca in functie, altfel NULL)
extern Symbol* crtStruct;

int expr(RetVal* rv);
int stmCompound(void);
int typeName(Type* ret);
int typeBaseSize(Type *type);

//tipul pe care il poate lua un simbol
char* tip[] = {"TB_INT","TB_DOUBLE","TB_CHAR","TB_STRUCT","TB_VOID"};
char* mem[] ={"MEM_GLOBAL", "MEM_ARG", "MEM_LOCAL"};
char* cls[] = {"CLS_VAR","CLS_FUNC","CLS_EXTFUNC","CLS_STRUCT"};

Token_t consumedTk, neededTk;

/*****************************Type analysis****************************/
Type createType(int typeBase,int nElements){
	Type t;
	t.typeBase=typeBase;
	t.nElements=nElements;
	return t;
}

//needs to be implemented
void cast(Type *dst,Type *src){

	if(src->nElements>-1){
		if(dst->nElements>-1){
			if(src->typeBase!=dst->typeBase)
				tkerr(crtTk,"an array cannot be converted to an array of another type");
		}else{
			tkerr(crtTk,"an array cannot be converted to a non-array");
		}
	}else{
		if(dst->nElements>-1){
			tkerr(crtTk,"a non-array cannot be converted to an array");
		}
	}

	switch(src->typeBase){
	case TB_CHAR:
	case TB_INT:
	case TB_DOUBLE:
		switch(dst->typeBase){
		case TB_CHAR:
		case TB_INT:
		case TB_DOUBLE:
			return;
		}
		break;
	case TB_STRUCT:
		if(dst->typeBase==TB_STRUCT){
			if(src->s!=dst->s)
				tkerr(crtTk,"a structure cannot be converted to another one");
			return;
		}
	}
	tkerr(crtTk,"incompatible types");
}
//incearca sa gaseasca tipul lor rezultat (pe care-l returneaza), conform regulilor aritmetice din AtomC
Type getArithType(Type *s1,Type *s2){

	if(s1->nElements != -1 && s2->nElements)
		tkerr(crtTk, "incompatibile types");

	switch(s1->typeBase){
	case TB_INT:
		if(s2->typeBase != TB_DOUBLE)
			return createType(TB_INT, -1);
		else
			return createType(TB_DOUBLE, -1);
	case TB_DOUBLE:
		return createType(TB_DOUBLE, -1);
	case TB_CHAR:
		return createType(s2->typeBase, -1);
	case TB_STRUCT:
		tkerr(crtTk, "What are you trying to do?!");
	}
	tkerr(crtTk, "Incompatible types");
}

Symbol *addSymbol(Symbols *symbols, const char *name, int cls){
	Symbol *s;
	if(symbols->end==symbols->after){ // create more room
		int count=symbols->after-symbols->begin;
		int n=count*2; // double the room
		if(n==0)n=1; // needed for the initial case

		symbols->begin=(Symbol**)realloc(symbols->begin, n*sizeof(Symbol*));
		if(symbols->begin==NULL)err("not enough memory");
		symbols->end=symbols->begin+count;
		symbols->after=symbols->begin+n;
	}

	SAFEALLOC2(s,Symbol);
	*symbols->end++=s;
	s->name=name;
	s->cls=cls;
	printf("%s --- %d\n",name, crtDepth);
	s->depth=crtDepth;
	return s;
}

Symbol *addExtFunc(const char *name,Type type, void* addr){
	Symbol *s=addSymbol(&symbols,name,CLS_EXTFUNC);
	s->type=type;
	s->addr = addr;
	initSymbols(&s->args);
	return s;
}

Symbol *addFuncArg(Symbol *func,const char *name,Type type){
	Symbol *a=addSymbol(&func->args,name,CLS_VAR);
	a->type=type;
	return a;
}

//cauta simbolul cu numele dat in vectorul specificat si returneaza un pointer catre el daca l-a gasit sau NULL
Symbol *findSymbol(Symbols *symbols,const char *name){

	Symbol **p;
	for(p=symbols->begin; p!=symbols->end;p++){
		if(strncmp((*p)->name, name, strlen(name)) == 0)
				return *p;
	}
	return NULL;
}

Symbol* requireSymbol(Symbols *symbols,const char *name){
	Symbol **p;
	for(p=symbols->begin; p!=symbols->end;p++){
		if(strncmp((*p)->name, name, strlen(name)) == 0)
				return *p;
	}

	tkerr(crtTk, "Symbol could not be found");
	return NULL;
}

/*****************************generare cod *********************************/
int typeFullSize(Type *type){
	return typeBaseSize(type)*(type->nElements>0?type->nElements:1);
}

int typeBaseSize(Type *type){
	int size = 0;
	Symbol **is;

	switch(type->typeBase){
	case TB_INT:
		size = sizeof(long int);
		break;
	case TB_DOUBLE:
		size = sizeof(double);
		break;
	case TB_CHAR:
		size = sizeof(char);
		break;
	case TB_STRUCT:
		for(is = type->s->members.begin; is!=type->s->members.end; is++){
			size += typeFullSize(&(*is)->type);
		}
		break;
	case TB_VOID:
		size = 0;
		break;
	default:
		err("invalid typeBase: %d",type->typeBase);
	}

	return size;
}

int typeArgSize(Type *type){
	if(type->nElements>=0)return sizeof(void*);
	return typeBaseSize(type);
}

Instr *getRVal(RetVal *rv){
	if(rv->isLVal){
		switch(rv->type.typeBase){
		case TB_INT:
		case TB_DOUBLE:
		case TB_CHAR:
		case TB_STRUCT:
			addInstrI(O_LOAD, typeArgSize(&rv->type));
			break;
		default:
			tkerr(crtTk, "unhandeled type: %d", rv->type.typeBase);
		}
	}
	return lastInstruction;
}
//insereaza dupa instructiunea „after” codul necesar pentru conversia
//„actualType->neededType”
void addCastInstr(Instr* after, Type* actualType, Type* neededType){
	if(actualType->nElements >=0 || neededType->nElements>=0)
		return;

	switch(actualType->typeBase){
	case TB_CHAR:
		switch(neededType->typeBase){
		case TB_CHAR: break;
		case TB_INT:
			addInstrAfter(after, O_CAST_C_I);
			break;
		case TB_DOUBLE:
			addInstrAfter(after, O_CAST_C_D);
			break;
		}
		break;
        case TB_INT:
        	switch(neededType->typeBase){
            case TB_CHAR:
            	addInstrAfter(after,O_CAST_I_C);
            	break;
            case TB_INT:break;
            case TB_DOUBLE:
            	addInstrAfter(after,O_CAST_I_D);
            	break;
        	}
            break;
        case TB_DOUBLE:
            switch(neededType->typeBase){
            case TB_CHAR:
            	addInstrAfter(after,O_CAST_D_C);
            	break;
            case TB_INT:
            	addInstrAfter(after,O_CAST_D_I);
            	break;
            case TB_DOUBLE:break;
            }
            break;
	}
}

Instr *createCondJmp(RetVal *rv)
{
	if(rv->type.nElements>=0){  // arrays
		return addInstr(O_JF_A);
	}else{                              // non-arrays
		getRVal(rv);
		switch(rv->type.typeBase){
		case TB_CHAR:return addInstr(O_JF_C);
		case TB_DOUBLE:return addInstr(O_JF_D);
		case TB_INT:return addInstr(O_JF_I);
		default:return NULL;
		}
	}
}

Instr* appendInstr(Instr* i){
	if(lastInstruction)
		lastInstruction ->next = i;
	else
		instructions = i;
	lastInstruction = i;
	i->next = NULL;
	return i;
}

/*****************************domain analysis*******************************/
void addVar(Token_t tkName, Type *t){
	Symbol *s;

	if(crtStruct){
		if(findSymbol(&crtStruct->members,tkName->text))
			tkerr(crtTk,"symbol redefinition: %s",tkName->text);
		s=addSymbol(&crtStruct->members,tkName->text,CLS_VAR);
    }else if(crtFunc){
    	s=findSymbol(&symbols,tkName->text);
    	if(s&&s->depth==crtDepth)
    		tkerr(crtTk,"symbol redefinition: %s",tkName->text);
    	s=addSymbol(&symbols,tkName->text,CLS_VAR);
    	s->mem=MEM_LOCAL;
    }else{
    	if(findSymbol(&symbols,tkName->text))
    		tkerr(crtTk,"symbol redefinition: %s",tkName->text);
    	s=addSymbol(&symbols,tkName->text,CLS_VAR);
    	s->mem=MEM_GLOBAL;
    }

	s->type=*t;

	if(crtStruct||crtFunc){
		s->offset=offset;
	}else{
		s->addr=allocGlobal(typeFullSize(&s->type));
	}
	offset+=typeFullSize(&s->type);
}

void deleteSymbolsAfter(Symbols *symbols, const Symbol *ptr){

    if(symbols->end == symbols->begin)
        return;
    Symbol **p = symbols->begin;
    while((*p) != ptr){
    	p++;
    }

    /* memory needs to be freed here
    Symbol **d = p;
    d++;
    while(d != symbols->end){

    	d++;
    }
    */

    symbols->end = p+1;
}


void showSyms(){
	Symbol **p;

	//printf("%p\n", symbols.end);
	for(p=symbols.begin; p!=symbols.end; p++){
        printf("%s : %s : %s : %s\n",mem[(*p)->mem], (*p)->name, cls[(*p)->cls], tip[((*p)->type).typeBase]);
	}
    printf("\n");
}

/************************************lexy*************************************/

int consume(int code){
	//printf("expected:%d got:%d \n", code, crtTk->code);
	if(crtTk->code == code){
		consumedTk=crtTk;
		crtTk=crtTk->next;
		return 1;
	}

	return 0;
}

int exprPrimary(RetVal* rv){
	Instr *i;

	if(consume(CT_INT)){
		addInstrI(O_PUSHCT_I,consumedTk->i);
		rv->type=createType(TB_INT,-1);
		rv->ctVal.i=consumedTk->i;
		rv->isCtVal=1;rv->isLVal=0;
		return 1;
	}
	if(consume(CT_REAL)){
		i=addInstr(O_PUSHCT_D);
		i->args[0].d=consumedTk->r;
		rv->type=createType(TB_DOUBLE,-1);
		rv->ctVal.d=consumedTk->r;
		rv->isCtVal=1;rv->isLVal=0;
		return 1;
	}
	if(consume(CT_CHAR)){
		addInstrI(O_PUSHCT_C,consumedTk->i);
		rv->type=createType(TB_CHAR,-1);
		rv->ctVal.i=consumedTk->i;
		rv->isCtVal=1;rv->isLVal=0;
		return 1;
	}

	if(consume(CT_STRING)){
		addInstrA(O_PUSHCT_A,consumedTk->text);
		rv->type=createType(TB_CHAR, 0);
		rv->ctVal.str=consumedTk->text;
		rv->isCtVal=1;rv->isLVal=0;
		return 1;
	}

    if(consume(LPAR)){
        if(!expr(rv))
        	 tkerr(crtTk,"invalid expression");
        if(!consume(RPAR))
        	 tkerr(crtTk,"missing )");
        return 1;
    }
    char* tkName;
    RetVal arg;
    if(consume(ID)){
    	tkName= consumedTk->text;
    	Symbol *s=findSymbol(&symbols,tkName);
    	if(!s)tkerr(crtTk,"undefined symbol");
    	rv->type=s->type;
    	rv->isCtVal=0;
    	rv->isLVal=1;
    	if(consume(LPAR)){
            Symbol **crtDefArg=s->args.begin;
            if(s->cls!=CLS_FUNC&&s->cls!=CLS_EXTFUNC)
                tkerr(crtTk,"call of the non-function");

    		if(expr(&arg)){
                if(crtDefArg==s->args.end)tkerr(crtTk,"too many arguments in call");
                cast(&(*crtDefArg)->type,&arg.type);

                if((*crtDefArg)->type.nElements<0){  //only arrays are passed by addr
                	i=getRVal(&arg);
                }else{
                	i=lastInstruction;
                }
                addCastInstr(i,&arg.type,&(*crtDefArg)->type);

                crtDefArg++;
    			while(consume(COMMA)){
    				if(!expr(&arg))
    					tkerr(crtTk,"invalid expression");
    				else{
                        if(crtDefArg==s->args.end)tkerr(crtTk,"too many arguments in call");
                        cast(&(*crtDefArg)->type,&arg.type);

                        if((*crtDefArg)->type.nElements<0){
                        	i=getRVal(&arg);
                        }else{
                        	i=lastInstruction;
                        }
                        addCastInstr(i,&arg.type,&(*crtDefArg)->type);

                        crtDefArg++;
    				}
    			}
    		}
    		if(!consume(RPAR))
    			tkerr(crtTk,"missing )");
    		else{//function call
                if(crtDefArg!=s->args.end)tkerr(crtTk,"too few arguments in call");
                i=addInstr(s->cls==CLS_FUNC?O_CALL:O_CALLEXT);
                i->args[0].addr = s->addr;

                rv->type=s->type;
                rv->isCtVal=rv->isLVal=0;
                return 1;
    		}
    	}

    	// variable
    	printf("%d ---- %p\n", s->depth, s->offset);
    	if(s->depth){
    		addInstrI(O_PUSHFPADDR,s->offset);
    	}else{
    		addInstrA(O_PUSHCT_A,s->addr);
    	}
    	return 1;
    }

    return 0;
}

int exprPostfix(RetVal* rv){
	Token_t startTk=crtTk;
	Instr *startLastInstr = lastInstruction;
	RetVal rve;

    if(!exprPrimary(rv)){
    	deleteInstructionsAfter(startLastInstr);
        crtTk = startTk;
        return 0;
    }

    //deleteInstructionsAfter(startLastInstr);
    startTk=crtTk;
    while(1){
    	//deleteInstructionsAfter(startLastInstr);
        startTk = crtTk;
        if(consume(LBRACKET)){
        	if(expr(&rve) && consume(RBRACKET)){
                if(rv->type.nElements<0)tkerr(crtTk,"only an array can be indexed");
                Type typeInt=createType(TB_INT,-1);
                cast(&typeInt,&rve.type);
                rv->type.nElements=-1;
                rv->isLVal=1;
                rv->isCtVal=0;

                addCastInstr(lastInstruction,&rve.type,&typeInt);
                getRVal(&rve);
                if(typeBaseSize(&rv->type)!=1){
                	addInstrI(O_PUSHCT_I,typeBaseSize(&rv->type));
                	addInstr(O_MUL_I);
                }
                addInstr(O_OFFSET);
        		continue;
        	}
        }

        //deleteInstructionsAfter(startLastInstr);
        crtTk = startTk;
        char* tkName;
        if(consume(DOT) && consume(ID)){
        	tkName = consumedTk->text;
            Symbol *sStruct=rv->type.s;
            Symbol *sMember=findSymbol(&sStruct->members,tkName);
            if(!sMember)
                tkerr(crtTk,"struct %s does not have a member %s",sStruct->name,tkName);
            if(sMember->offset){
            	addInstrI(O_PUSHCT_I,sMember->offset);
            	addInstr(O_OFFSET);
            }
            rv->type=sMember->type;
            rv->isLVal=1;
            rv->isCtVal=0;
            continue;
        }
        break;
    }
    return 1;
}

//exprUnary: ( SUB | NOT ) exprUnary | exprPostfix ;
int exprUnary(RetVal* rv){
	Token_t startTk=crtTk;
	Instr *startLastInstr = lastInstruction;

    if((consume(SUB) || consume(NOT)) && exprUnary(rv)){
        if(consumedTk->code==SUB){
            if(rv->type.nElements>=0)tkerr(crtTk,"unary '-' cannot be applied to an array");
            if(rv->type.typeBase==TB_STRUCT)
                tkerr(crtTk,"unary '-' cannot be applied to a struct");

            getRVal(rv);
            switch(rv->type.typeBase){
            case TB_CHAR:addInstr(O_NEG_C);break;
            case TB_INT:addInstr(O_NEG_I);break;
            case TB_DOUBLE:addInstr(O_NEG_D);break;
            }
        }else{
            if(rv->type.typeBase==TB_STRUCT)tkerr(crtTk,"'!' cannot be applied to a struct");

            if(rv->type.nElements<0){
            	getRVal(rv);
            	switch(rv->type.typeBase){
            	case TB_CHAR:addInstr(O_NOT_C);break;
            	case TB_INT:addInstr(O_NOT_I);break;
            	case TB_DOUBLE:addInstr(O_NOT_D);break;
            	}
            }else{
            	addInstr(O_NOT_A);
            }

            rv->type=createType(TB_INT,-1);
        }
        rv->isCtVal=rv->isLVal=0;
        return 1;
    }else{

    }

    deleteInstructionsAfter(startLastInstr);
    crtTk = startTk;
    return exprPostfix(rv);
}

//arrayDecl: LBRACKET expr? RBRACKET
int arrayDecl(Type* ret){
	Instr *instrBeforeExpr;
	RetVal rv;

    if(consume(LBRACKET)){
    	instrBeforeExpr=lastInstruction;

        if(expr(&rv)){
            if(!rv.isCtVal)
            	tkerr(crtTk,"the array size is not a constant");
            if(rv.type.typeBase != TB_INT)
            	tkerr(crtTk,"the array size is not an integer");
            ret->nElements=rv.ctVal.i;
            deleteInstructionsAfter(instrBeforeExpr);
        }
        ret->nElements = 0; //array without given size
        if(!consume(RBRACKET))
        	tkerr(crtTk,"missing }");
        return 1;
    }
    return 0;
}

//exprCast: LPAR typeName RPAR exprCast | exprUnary ;
int exprCast(RetVal* rv){
	Token_t startTk=crtTk;
	Type t;
	RetVal rve;
	Instr* startLastInstr = lastInstruction;

    if(consume(LPAR) && typeName(&t) && consume(RPAR) && exprCast(&rve)){
        cast(&t,&rve.type);

        if(rv->type.nElements<0&&rv->type.typeBase!=TB_STRUCT){
        	switch(rve.type.typeBase){
        	case TB_CHAR:
        		switch(t.typeBase){
        		case TB_INT:addInstr(O_CAST_C_I);break;
        		case TB_DOUBLE:addInstr(O_CAST_C_D);break;
        		}
        		break;
        		case TB_DOUBLE:
        			switch(t.typeBase){
        			case TB_CHAR:addInstr(O_CAST_D_C);break;
        			case TB_INT:addInstr(O_CAST_D_I);break;
        			}
        			break;
        			case TB_INT:
        				switch(t.typeBase){
        				case TB_CHAR:addInstr(O_CAST_I_C);break;
        				case TB_DOUBLE:addInstr(O_CAST_I_D);break;
        				}
        				break;
        	}
        }
        rv->type=t;
        rv->isCtVal=rv->isLVal=0;
    	return 1;
    }

    deleteInstructionsAfter(startLastInstr);
    crtTk = startTk;
    return exprUnary(rv);
}

//exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
int exprMul(RetVal* rv){
	Instr *i1,*i2;Type t1,t2;
	RetVal rve;

    if(!exprCast(rv))
        return 0;

    while(consume(MUL) || consume(DIV)){
    	i1=getRVal(rv);t1=rv->type;
    	Token_t tkop=consumedTk;

        if(!exprCast(&rve))
        	tkerr(crtTk, "invalid exprMul expression");
        else{
            if(rv->type.nElements>-1||rve.type.nElements>-1)
                    tkerr(crtTk,"an array cannot be multiplied or divided");
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"a structure cannot be multiplied or divided");
            rv->type=getArithType(&rv->type,&rve.type);

            i2=getRVal(&rve);t2=rve.type;
            addCastInstr(i1,&t1,&rv->type);
            addCastInstr(i2,&t2,&rv->type);
            if(tkop->code==MUL){
            	switch(rv->type.typeBase){
            	case TB_INT:addInstr(O_MUL_I);break;
            	case TB_DOUBLE:addInstr(O_MUL_D);break;
            	case TB_CHAR:addInstr(O_MUL_C);break;
            	}
            }else{
            	switch(rv->type.typeBase){
            	case TB_INT:addInstr(O_DIV_I);break;
            	case TB_DOUBLE:addInstr(O_DIV_D);break;
            	case TB_CHAR:addInstr(O_DIV_C);break;
            	}
            }

            rv->isCtVal=rv->isLVal=0;
        }
    }

    return 1;
}

//exprAdd: exprUnary ( ADD | SUB ) exprMul | exprMul
int exprAdd(RetVal* rv){
	Instr *i1,*i2;Type t1,t2;

    if(!exprMul(rv))
       return 0;

    RetVal rve;
    while(consume(ADD) || consume(SUB)) {
    	i1=getRVal(rv);
    	t1=rv->type;
    	Token_t tkop = consumedTk;

        if(!exprMul(&rve))
        	tkerr(crtTk, "invalid add expression");
        else{
            if(rv->type.nElements>-1||rve.type.nElements>-1)
                    tkerr(crtTk,"an array cannot be added or subtracted");
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"a structure cannot be added or subtracted");
            rv->type=getArithType(&rv->type,&rve.type);

            i2=getRVal(&rve);t2=rve.type;
            addCastInstr(i1,&t1,&rv->type);
            addCastInstr(i2,&t2,&rv->type);
            if(tkop->code==ADD){
            	switch(rv->type.typeBase){
            	case TB_INT:addInstr(O_ADD_I);break;
            	case TB_DOUBLE:addInstr(O_ADD_D);break;
            	case TB_CHAR:addInstr(O_ADD_C);break;
            	}
            }else{
            	switch(rv->type.typeBase){
            	case TB_INT:addInstr(O_SUB_I);break;
            	case TB_DOUBLE:addInstr(O_SUB_D);break;
            	case TB_CHAR:addInstr(O_SUB_C);break;
            	}
            }

            rv->isCtVal=rv->isLVal=0;
        }
    }

    return 1;
}

//exprRel: exprUnary( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd ;
int exprRel(RetVal* rv){
	Instr *i1,*i2;Type t,t1,t2;

    if(!exprAdd(rv))
        return 0;

    RetVal rve;
    //printf("Expr Rel %d\n", crtTk->code);
    while(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
    	i1=getRVal(rv);
    	t1=rv->type;
    	Token_t tkop =consumedTk;

        if(!exprAdd(&rve))
        	 tkerr(crtTk, "invalid exprRel expression\n");
        else{
            if(rv->type.nElements>-1||rve.type.nElements>-1)
                    tkerr(crtTk,"an array cannot be compared");
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"a structure cannot be compared");
            i2=getRVal(&rve);t2=rve.type;
            t=getArithType(&t1,&t2);
            addCastInstr(i1,&t1,&t);
            addCastInstr(i2,&t2,&t);
            switch(tkop->code){
            case LESS:
            	switch(t.typeBase){
            	case TB_INT:addInstr(O_LESS_I);break;
            	case TB_DOUBLE:addInstr(O_LESS_D);break;
            	case TB_CHAR:addInstr(O_LESS_C);break;
            	}
            	break;
            	case LESSEQ:
            		switch(t.typeBase){
            		case TB_INT:addInstr(O_LESSEQ_I);break;
            		case TB_DOUBLE:addInstr(O_LESSEQ_D);break;
            		case TB_CHAR:addInstr(O_LESSEQ_C);break;
            		}
            		break;
            		case GREATER:
            			switch(t.typeBase){
            			case TB_INT:addInstr(O_GREATER_I);break;
            			case TB_DOUBLE:addInstr(O_GREATER_D);break;
            			case TB_CHAR:addInstr(O_GREATER_C);break;
            			}
            			break;
            			case GREATEREQ:
            				switch(t.typeBase){
            				case TB_INT:addInstr(O_GREATEREQ_I);break;
            				case TB_DOUBLE:addInstr(O_GREATEREQ_D);break;
            				case TB_CHAR:addInstr(O_GREATEREQ_C);break;
            				}
            				break;
            }

            rv->type=createType(TB_INT,-1);
            rv->isCtVal=rv->isLVal=0;
        }
    }

    return 1;
}

//exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel ;
int exprEq(RetVal* rv){
	Instr *i1,*i2;Type t,t1,t2;

    if(!exprRel(rv))
        return 0;

    RetVal rve;
    while(consume(EQUAL) || consume(NOTEQ)){
    	i1=rv->type.nElements<0?getRVal(rv):lastInstruction;
    	t1=rv->type;
    	Token_t tkop =consumedTk;

        if(!exprRel(&rve))
            tkerr(crtTk, "invalid and expression\n");
        else{
        	if(rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
        		tkerr(crtTk, "a structure cannot be compared");

        	if(rv->type.nElements>=0){      // vectors
        		addInstr(tkop->code==EQUAL?O_EQ_A:O_NOTEQ_A);
        	}else{  // non-vectors
        		i2=getRVal(&rve);t2=rve.type;
        		t=getArithType(&t1,&t2);
        		addCastInstr(i1,&t1,&t);
        		addCastInstr(i2,&t2,&t);
        		if(tkop->code==EQUAL){
        			switch(t.typeBase){
        			case TB_INT:addInstr(O_EQ_I);break;
        			case TB_DOUBLE:addInstr(O_EQ_D);break;
        			case TB_CHAR:addInstr(O_EQ_C);break;
        			}
        		}else{
        			switch(t.typeBase){
        			case TB_INT:addInstr(O_NOTEQ_I);break;
        			case TB_DOUBLE:addInstr(O_NOTEQ_D);break;
        			case TB_CHAR:addInstr(O_NOTEQ_C);break;
        			}
        		}
        	}

        	rv->type=createType(TB_INT, -1);
        	rv->isCtVal=rv->isLVal=0;
        }
    }

    return 1;
}

//exprAnd: exprAnd AND exprEq | exprEq ;
int exprAnd(RetVal* rv){
	Instr *i1,*i2;Type t,t1,t2;

    if(!exprEq(rv))
        return 0;

    RetVal rve;
    while(consume(AND)){
        i1=rv->type.nElements<0?getRVal(rv):lastInstruction;
        t1=rv->type;

        if(!exprEq(&rve))
        	tkerr(crtTk, "invalid and expression\n");
        else{
        	if(rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
        		tkerr(crtTk, "a structure cannot be logically tested");

        	if(rv->type.nElements>=0){      // vectors
        		addInstr(O_AND_A);
        	}else{  // non-vectors
        		i2=getRVal(&rve);t2=rve.type;
        		t=getArithType(&t1,&t2);
        		addCastInstr(i1,&t1,&t);
        		addCastInstr(i2,&t2,&t);
        		switch(t.typeBase){
        		case TB_INT:addInstr(O_AND_I);break;
        		case TB_DOUBLE:addInstr(O_AND_D);break;
        		case TB_CHAR:addInstr(O_AND_C);break;
        		}
        	}

        	rv->type=createType(TB_INT, -1);
        	rv->isCtVal=rv->isLVal=0;
        }
    }

    return 1;
}

//exprOr OR exprAnd | exprAnd
int exprOr(RetVal* rv){
	Instr *i1,*i2;Type t,t1,t2;

	if(!exprAnd(rv)) return 0;

	RetVal rve;
	while(consume(OR)){
		i1=rv->type.nElements<0?getRVal(rv):lastInstruction;
		t1=rv->type;
		if(!exprAnd(&rve))
			tkerr(crtTk,"invalid expression");
		else{
			if(rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
				tkerr(crtTk, "a structure cannot be logically tested");

			if(rv->type.nElements>=0){      // vectors
				addInstr(O_OR_A);
			}else{  // non-vectors
				i2=getRVal(&rve);t2=rve.type;
				t=getArithType(&t1,&t2);
				addCastInstr(i1,&t1,&t);
				addCastInstr(i2,&t2,&t);
				switch(t.typeBase){
				case TB_INT:addInstr(O_OR_I);break;
				case TB_DOUBLE:addInstr(O_OR_D);break;
				case TB_CHAR:addInstr(O_OR_C);break;
				}
			}

			rv->type=createType(TB_INT, -1);
			rv->isCtVal=rv->isLVal=0;
		}
	}

	return 1;
}

//exprUnary ASSIGN exprAssign | exprOr ;
int exprAssign(RetVal* rv){
	Token_t startTk=crtTk;
	Instr *startLastInstr = lastInstruction;
	Instr* i;

	RetVal rve;
	if(exprUnary(rv) && consume(ASSIGN) && exprAssign(&rve)){
		if(!rv->isLVal)
			tkerr(crtTk, "cannot assign a non-lval");
		if(rv->type.nElements > -1 || rve.type.nElements > -1)
			tkerr(crtTk, "the arrays can not be assigned");
		cast(&rv->type, &rve.type);
		i=getRVal(&rve);
        addCastInstr(i,&rve.type,&rv->type);
        //duplicate the value on top before the dst addr
        addInstrII(O_INSERT,
                        	sizeof(void*)+typeArgSize(&rv->type),
                            typeArgSize(&rv->type));
        addInstrI(O_STORE,typeArgSize(&rv->type));

		rv->isCtVal=rv->isLVal=0;
		return 1;
	}

	deleteInstructionsAfter(startLastInstr);
	crtTk=startTk; // restore crtTk to the entry value

	return exprOr(rv);
}

int expr(RetVal* rv){
	return exprAssign(rv);
}

int typeBase(Type* ret){
	Token_t startTk=crtTk;
	Instr *startLastInstr = lastInstruction;

	if(consume(INT)){
		ret->typeBase = TB_INT;
		return 1;
	}
	if(consume(DOUBLE)){
		ret->typeBase = TB_DOUBLE;
		return 1;
	}
	if(consume(CHAR)){
		ret->typeBase = TB_CHAR;
		return 1;
	}
	if(consume(STRUCT)){
		if(consume(ID)){
            char* tkName = consumedTk->text;
            Symbol *s = findSymbol(&symbols, tkName);
            if(s == NULL)
                tkerr(crtTk, "undefined symbol: %s", tkName);
            if(s->cls != CLS_STRUCT)
                tkerr(crtTk, "'%s' is not a struct", tkName);
            ret->typeBase = TB_STRUCT;
            ret->s = s;
			return 1;
		}

		deleteInstructionsAfter(startLastInstr);
		crtTk=startTk;
	}
	return 0;
}

//declVar:  typeBase ID arrayDecl? ( COMMA ID arrayDecl? )* SEMICOLON ;
int declVar(){

	Type t;
	if(!typeBase(&t))return 0;
	//printf("type:%d\n", t.typeBase);
    if(!consume(ID))
    	tkerr(crtTk,"invalid statement");

    neededTk = consumedTk;
    if(arrayDecl(&t) == 0)
    	t.nElements = -1;
    addVar(neededTk, &t);

    while(consume(COMMA)) {
        if(!consume(ID))
        	tkerr(crtTk,"invalid statement");

        neededTk = consumedTk;
        if(arrayDecl(&t) == 0)
            t.nElements = -1;
        addVar(neededTk, &t);
    }

    if(!consume(SEMICOLON))
    	tkerr(crtTk,"missing ;");

    return 1;
}

//typeName: typeBase arrayDecl?
int typeName(Type* ret){
	if(!typeBase(ret)) return 0;
	//not array
	if(arrayDecl(ret) == 0)
		ret->nElements = -1;

	return 1;
}

int stm(){
	RetVal rv, rv2, rv3;
	Token_t startTk=crtTk;
	Instr *startLastInstr = lastInstruction;
	Instr *i,*i1,*i2,*i3,*i4,*is,*ib3,*ibs;

    if(stmCompound())
        return 1;

    if(consume(IF)){
        if(!consume(LPAR))
        	tkerr(crtTk,"missing (");
        if(!expr(&rv))
        	tkerr(crtTk,"invalid expression");
        else{
        	if(rv.type.typeBase==TB_STRUCT)
        		tkerr(crtTk,"a structure cannot be logically tested");
        }
        if(!consume(RPAR))
        	tkerr(crtTk,"missing )");
        i1=createCondJmp(&rv);
        if(!stm())
        	tkerr(crtTk,"invalid expression");
        while(consume(ELSE)){
        	i2=addInstr(O_JMP);
            if(!stm())
            	tkerr(crtTk,"invalid expression");
            i1->args[0].addr=i2->next;
            i1=i2;
        }
        i1->args[0].addr=addInstr(O_NOP);
    }else if(consume(WHILE)){
        Instr *oldLoopEnd=crtLoopEnd;
        crtLoopEnd=createInstr(O_NOP);
        i1=lastInstruction;
        if(!consume(LPAR))
        	tkerr(crtTk,"missing (");
        if(!expr(&rv))
        	tkerr(crtTk,"invalid expression");
        else{
        	if(rv.type.typeBase==TB_STRUCT)
        		tkerr(crtTk,"a structure cannot be logically tested");
        }
        if(!consume(RPAR))
        	tkerr(crtTk,"missing )");
        i2=createCondJmp(&rv);
        if(!stm())
        	tkerr(crtTk,"invalid statement ;");
        addInstrA(O_JMP,i1->next);
        appendInstr(crtLoopEnd);
        i2->args[0].addr=crtLoopEnd;
        crtLoopEnd=oldLoopEnd;
    }else if(consume(FOR)){// for(rv1;rv2;rv3)stm
        Instr *oldLoopEnd=crtLoopEnd;
        crtLoopEnd=createInstr(O_NOP);
        if(!consume(LPAR))
        	tkerr(crtTk,"missing (");
        if (expr(&rv)){
        	if(typeArgSize(&rv.type))
        	   addInstrI(O_DROP,typeArgSize(&rv.type));
        }
        if(!consume(SEMICOLON))
        	tkerr(crtTk,"missing ;");
        i2=lastInstruction; /* i2 is before rv2 */
        if(expr(&rv2))
        	i4=createCondJmp(&rv2);
        else
        	i4 = NULL;
        if(rv2.type.typeBase==TB_STRUCT)
                tkerr(crtTk,"a structure cannot be logically tested");
        if(!consume(SEMICOLON))
        	tkerr(crtTk,"missing ;");
        ib3=lastInstruction; /* ib3 is before rv3 */
        if(expr(&rv3)){
        	if(typeArgSize(&rv3.type))
        	    addInstrI(O_DROP,typeArgSize(&rv3.type));
        }
        if(!consume(RPAR))
        	tkerr(crtTk,"missing )");
        ibs=lastInstruction;
        if(!stm())
        	tkerr(crtTk,"invalid for statement");
        if(ib3!=ibs){
        	i3=ib3->next;
        	is=ibs->next;
        	ib3->next=is;
        	is->last=ib3;
        	lastInstruction->next=i3;
        	i3->last=lastInstruction;
        	ibs->next=NULL;
        	lastInstruction=ibs;
        }
        addInstrA(O_JMP,i2->next);
        appendInstr(crtLoopEnd);
        if(i4)i4->args[0].addr=crtLoopEnd;
        crtLoopEnd=oldLoopEnd;

    }else if(consume(BREAK)){
        if(!consume(SEMICOLON))
        	tkerr(crtTk,"missing ;");
        if(!crtLoopEnd)tkerr(crtTk,"break without for or while");
        addInstrA(O_JMP,crtLoopEnd);
    }else if(consume(RETURN)){
        if(!expr(&rv)){
        	deleteInstructionsAfter(startLastInstr);
            crtTk = startTk;
        }else{
        	i=getRVal(&rv);
        	addCastInstr(i,&rv.type,&crtFunc->type);
            if(crtFunc->type.typeBase==TB_VOID)
                tkerr(crtTk,"a void function cannot return a value");
            cast(&crtFunc->type,&rv.type);
        }
        if(!consume(SEMICOLON))
        	tkerr(crtTk,"missing (");
        if(crtFunc->type.typeBase==TB_VOID){
        	addInstrII(O_RET,sizeArgs,0);
        }else{
        	addInstrII(O_RET,sizeArgs,typeArgSize(&crtFunc->type));
        }
    }
    else
    {
        if(!expr(&rv)){
        	deleteInstructionsAfter(startLastInstr);
            crtTk = startTk;
        }else{
        	if(typeArgSize(&rv.type))
        		addInstrI(O_DROP,typeArgSize(&rv.type));
        }
        return consume(SEMICOLON);
    }

    return 1;
}

int stmCompound(){

    if(!consume(LACC))
        return 0;

	Symbol *start=symbols.end[-1];
    crtDepth++;

    while(declVar() || stm());
    if(!consume(RACC))
    	tkerr(crtTk,"missing }");

    crtDepth--;
    deleteSymbolsAfter(&symbols, start);

    return 1;
}

//declStruct: STRUCT ID LACC declVar* RACC SEMICOLON ;
int declStruct(){

	char* tkName;
    if(!consume(STRUCT))
        return 0;
    if(!consume(ID))
    	tkerr(crtTk,"missing struct ID");

    tkName = consumedTk->text;
    if(!consume(LACC))
        return 0;

    offset=0;
    if(findSymbol(&symbols,tkName))
    	tkerr(crtTk,"symbol redefinition: %s",tkName);
    crtStruct=addSymbol(&symbols,tkName,CLS_STRUCT);
    initSymbols(&crtStruct->members);

    while(declVar());
    if(!consume(RACC))
    	tkerr(crtTk,"missing }");
    if(!consume(SEMICOLON))
    	tkerr(crtTk,"missing ;");

    crtStruct = NULL;
    return 1;
}

//funcArg: typeBase ID arrayDecl? ;
int funcArg(){
    Type t;
    char* tkName;

    if(!typeBase(&t))
        return 0;
    if(!consume(ID))
        tkerr(crtTk, "incorrect ID\n");

    tkName = consumedTk->text;
    if(arrayDecl(&t) == 0)
        t.nElements = -1;

    Symbol  *s=addSymbol(&symbols,tkName,CLS_VAR);
    s->mem=MEM_ARG;
    s->type=t;
    s=addSymbol(&crtFunc->args,tkName,CLS_VAR);
    s->mem=MEM_ARG;
    s->type=t;

    //for each "s" (the one as local var and the one as arg):
    s->offset=offset;
    //only once at the end, after "offset" is used and "s->type" is set
    offset+=typeArgSize(&s->type);

    return 1;
}

int declFunc()
{
	Type t;
	char* tkName;
	Symbol **ps;

    if(typeBase(&t)){
        if(consume(MUL))
        	t.nElements = 0;
        else
        	t.nElements = -1;
    }else{
        if(!consume(VOID))
            return 0;
        t.typeBase = TB_VOID;
    }

    if(!consume(ID))
    	tkerr(crtTk,"invalid statement");
    tkName = consumedTk->text;
    sizeArgs=offset=0;

    if(!consume(LPAR))
        return 0;

    if(findSymbol(&symbols,tkName))
    	tkerr(crtTk,"symbol redefinition: %s",tkName);
    crtFunc=addSymbol(&symbols,tkName,CLS_FUNC);
    crtFunc->mem = MEM_GLOBAL;
    initSymbols(&crtFunc->args);
    crtFunc->type=t;
    crtDepth++;

    if(funcArg(t)){
        while(consume(COMMA))
            if(!funcArg(t))
            	tkerr(crtTk,"invalid statement");
    }
    if(!consume(RPAR))
    	tkerr(crtTk,"missing )");
    crtDepth--;

    crtFunc->addr=addInstr(O_ENTER);
    sizeArgs=offset;
    //update args offsets for correct FP indexing
    for(ps=symbols.begin;ps!=symbols.end;ps++){
    	if((*ps)->mem==MEM_ARG){
    		//2*sizeof(void*) == sizeof(retAddr)+sizeof(FP)
    		(*ps)->offset-=sizeArgs+2*sizeof(void*);
    	}
    }
    offset=0;

    if(!stmCompound())
    	tkerr(crtTk,"invalid statement");

    ((Instr*)crtFunc->addr)->args[0].i = offset;  // setup the ENTER argument
    if(crtFunc->type.typeBase==TB_VOID){
            addInstrII(O_RET,sizeArgs,0);
    }

    deleteSymbolsAfter(&symbols, crtFunc);
    crtFunc = NULL;

    //printf("%p %p\n", symbols.begin, symbols.end);
   // showSyms();
    return 1;
}


int unit(){
	Token_t startTk=crtTk;
	Instr *startLastInstr = lastInstruction;

	Instr *labelMain=addInstr(O_CALL);
	addInstr(O_HALT);

    while(1){
        startTk = crtTk;
        if(declStruct()){
        	//labelMain->args[0].addr=requireSymbol(&symbols,"main")->addr;
            continue;
        }
        crtTk = startTk;
        //deleteInstructionsAfter(startLastInstr);
        if(declFunc()){
            continue;
        }
        crtTk = startTk;
       // deleteInstructionsAfter(startLastInstr);
        if(declVar()){
        	//labelMain->args[0].addr=requireSymbol(&symbols,"main")->addr;
            continue;
        }
        crtTk = startTk;
       // deleteInstructionsAfter(startLastInstr);
        break;
    }
    labelMain->args[0].addr=requireSymbol(&symbols,"main")->addr;

    if(consume(END))
        return 1;

    return 0;
}

int main(int argc, char* argv[]){

	/****************predefined functions***************/
	Symbol *s, *a;
	s=addExtFunc("put_s",createType(TB_VOID,-1), put_s);
	addFuncArg(s,"s",createType(TB_CHAR,0));

	s=addExtFunc("get_s",createType(TB_VOID,-1), get_s);
	addFuncArg(s,"s",createType(TB_CHAR,0));

	s=addExtFunc("put_i",createType(TB_VOID,-1),put_i);
	a=addSymbol(&s->args,"i",CLS_VAR);
	a->type=createType(TB_INT,-1);

	s=addExtFunc("get_i",createType(TB_INT,-1), get_i);

	s=addExtFunc("put_d",createType(TB_VOID,-1), put_d);
	addFuncArg(s,"s",createType(TB_DOUBLE,-1));

	s=addExtFunc("get_d",createType(TB_DOUBLE,-1), get_d);

	s=addExtFunc("put_c",createType(TB_VOID,-1), put_c);
	addFuncArg(s,"s",createType(TB_CHAR, -1));

	s=addExtFunc("get_c",createType(TB_CHAR,-1), get_c);

	s=addExtFunc("seconds",createType(TB_DOUBLE,-1), seconds);


	crtDepth = 0;
	crtFunc = crtStruct = NULL;

	/*********************start processing input file******************************/
	if (argc != 2)
		err("Invalid number of arguments");
	char *source = NULL;
	FILE *fp = fopen(argv[1], "r");

	if(fp == NULL){
		puts("Could not open file");
		exit(1);
	}

	//reads entire file in memory
	if (fp != NULL) {
		/* Go to the end of the file. */
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			long bufsize = ftell(fp);
			if (bufsize == -1) { /* Error */
			}

			/* Allocate our buffer to that size. */
			source = malloc(sizeof(char) * (bufsize + 1));

			/* Go back to the start of the file. */
			if (fseek(fp, 0L, SEEK_SET) != 0) {
				err("Could not get to start of file");
			}

			/* Read the entire file into memory. */
			size_t newLen = fread(source, sizeof(char), bufsize, fp);
			if ( ferror( fp ) != 0) {
				fputs("Error reading file", stderr);
			} else {
				source[newLen++] = '\0'; /* Just to be safe. */
			}
		}
	}

	pCrCh = source;
	int tkCode = getNextToken();

	//start parsing file
	while (tkCode != END) {
		//printf("Return Code: %d value : %s\n", tkCode, identities[tkCode]);
		tkCode = getNextToken();
	}

	printTokens();
	//in this moment we have out list of tokens and we need to do the syntactic analyzer
	crtTk = firstToken;

	int sw = 1;

	while(sw > 0){
		if(unit() == 1){
			sw = 0;
			puts("HERE");
		}else if(stm()){
			puts("HERE1");
        }else{
            sw = 0;
            puts("HERE2");
        }
	}
	//printf("Value: %d\n",unit());

	showSyms();

	//our virtual machine test
	//mvTest();
	run(instructions);

	free(source);
	fclose(fp);
	//err("This is a test");
	return 0;
}

