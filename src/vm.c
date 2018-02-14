/*
 * vm.c
 *
 *  Created on: 21.05.2017
 *      Author: Andreea
 */
#include "lex.h"
#include "vm.h"
#include "sym.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Instr *instructions,*lastInstruction; // double linked list

void pushd(double d)
{
	if(SP+sizeof(double)>stackAfter)err("out of stack");
	*(double*)SP=d;
	SP+=sizeof(double);
}

void pushi(long int i){
	if(SP+sizeof(long int)>stackAfter)err("out of stack");
	*(long int*)SP=i;
	SP+=sizeof(long int);
}
void pusha(void *a)
{
	if(SP+sizeof(void*)>stackAfter)err("out of stack");
	*(void**)SP=a;
	SP+=sizeof(void*);
}

void pushc(char c){
	if(SP+sizeof(char)>stackAfter)err("out of stack");
	*(char*)SP=c;
	SP+=sizeof(char);
}

double popd()
{
	SP-=sizeof(double);
	if(SP<stack)err("not enough stack bytes for popd");
	return *(double*)SP;
}

long int popi(){
	SP-=sizeof(long int);
	if(SP<stack) err("not enough stack bytes for popi");
	return *(long int*)SP;
}

char popc(){
	SP-=sizeof(char);
	if(SP<stack)err("not enough stack bytes for popc");
	return *(char*)SP;
}

void *popa()
{
	SP-=sizeof(void*);
	if(SP<stack)err("not enough stack bytes for popa");
	return *(void**)SP;
}

Instr *createInstr(int opcode)
{
	Instr *i;
	SAFEALLOC2(i,Instr);
	i->opcode=opcode;
	return i;
}

void insertInstrAfter(Instr *after,Instr *i)
{
	i->next=after->next;
	i->last=after;
	after->next=i;
	if(i->next==NULL)
		lastInstruction=i;
}
Instr *addInstr(int opcode)
{
	Instr *i=createInstr(opcode);
	i->next=NULL;
	i->last=lastInstruction;
	if(lastInstruction){
		lastInstruction->next=i;
	}else{
		instructions=i;
	}
	lastInstruction=i;
	return i;
}

Instr *addInstrAfter(Instr *after,int opcode)
{
	Instr *i=createInstr(opcode);
	insertInstrAfter(after,i);
	return i;
}

//– adauga o instructiune setandu-i si arg[0].addr
Instr *addInstrA(int opcode,void *addr){
	Instr *i = addInstr(opcode);
	i->args[0].addr = addr;
	return i;
}

Instr *addInstrD(int opcode, double d){
	Instr *i = addInstr(opcode);
	i->args[0].d = d;
	return i;
}

//– adauga o instructiune setandu-i si arg[0].i
Instr *addInstrI(int opcode,long int val){
	Instr *i =  addInstr(opcode);
	i->args[0].i = val;
	return i;

}
// – adauga o instructiune setandu-i si arg[0].i, arg[1].i
Instr *addInstrII(int opcode,long int val1,long int val2){
	Instr *i= addInstr(opcode);
	i->args[0].i = val1;
	i->args[1].i= val2;
	return i;
}
//– sterge toate instructiunile de dupa instructiunea „start”
void deleteInstructionsAfter(Instr *start){
	Instr *i = NULL, *urm;
	if(start->next == NULL || start==NULL){
		return;
	}else{
		for(i = start->next, urm = i->next; urm; urm = urm->next){
			free(i);
			i = urm;
		}

		free(i);
		lastInstruction = start;
		start->next = NULL;
	}
}

void *allocGlobal(int size)
{
	void *p=globals+nGlobals;
	if(nGlobals+size>GLOBAL_SIZE)err("insufficient globals space");
	nGlobals+=size;
	return p;
}

void put_i()
{
	printf("#%ld\n",popi());
}

void put_c(){
	printf("#%c\n", popc());
}

void put_d(){
	printf("#%g\n", popd());
}

void put_s(){
	printf("#%s\n", (char*)popa());
}

void get_c(){
	char c;
	putchar('#');
	scanf("%c", &c);
	pushc(c);
}

void get_d(){
	double d;
	putchar('#');
	scanf("%lf", &d);
	pushd(d);
}

void get_i(){
	long int i;
	putchar('#');
	scanf("%ld", &i);
	pushi(i);
}

void get_s(){
	char* ret = malloc(sizeof(char) * 51);
	putchar('#');
	scanf("%50s", ret);
	pusha(ret);
}

void seconds(){
	pushd(123456.2);
}

void run(Instr* IP){
	long int iVal1,iVal2;
	double dVal1, dVal2;
	char cVal1, cVal2;
	char *aVal1, *aVal2;
	char *FP=0, *oldSP;
	SP = stack;
	stackAfter = stack + STACK_SIZE;
	while(1){
		printf("%p/%d\t",IP,SP-stack);
		switch(IP->opcode){
		case O_ADD_C:
			cVal1=popc();
			cVal2=popc();
			printf("ADD_C\t(%c+%c -> %c)\n",cVal1,cVal2,cVal1+cVal2);
			pushc(cVal2+cVal1);
			IP=IP->next;
			break;
		case O_ADD_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("ADD_D\t(%f+%f -> %f)\n",dVal1,dVal2,dVal1+dVal2);
			pushd(dVal2+dVal1);
			IP=IP->next;
			break;
		case O_ADD_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("ADD_I\t(%ld+%ld -> %ld)\n",iVal1,iVal2,iVal1+iVal2);
			pushi(iVal2+iVal1);
			IP=IP->next;
			break;
		case O_AND_A:
			aVal1=popa();
			aVal2=popa();
			printf("AND_A\t(%p==%p -> %d)\n",aVal2,aVal1,aVal2 && aVal1);
			pushi(aVal2&&aVal1);
			IP=IP->next;
			break;
		case O_AND_C:
			cVal1=popc();
			cVal2=popc();
			printf("AND_C\t(%c&&%c -> %d)\n",cVal2,cVal1,cVal2 && cVal1);
			pushi(cVal2&&cVal1);
			IP=IP->next;
			break;
		case O_AND_I:
			iVal1=popi();
			iVal2=popi();
			printf("AND_I\t(%ld&&%ld -> %d)\n",iVal2,iVal1,iVal2 && iVal1);
			pushi(iVal2&&iVal1);
			IP=IP->next;
			break;
		case O_AND_D:
			dVal1=popd();
			dVal2=popd();
			printf("AND_D\t(%g&&%g -> %d)\n",dVal2,dVal1,dVal2 && dVal1);
			pushi(dVal2&&dVal1);
			IP=IP->next;
			break;
		case O_CALL:
			aVal1=IP->args[0].addr;
			printf("CALL\t%p\n",aVal1);
			pusha(IP->next);
			IP=(Instr*)aVal1;
			break;
		case O_CALLEXT:
			printf("CALLEXT\t%p\n",IP->args[0].addr);
			(*(void(*)())IP->args[0].addr)();
			IP=IP->next;
			break;
		case O_CAST_C_D:
			cVal1=popc();
			dVal1=(double)cVal1;
			printf("CAST_C_D\t(%c -> %g)\n",cVal1,dVal1);
			pushd(dVal1);
			IP=IP->next;
			break;
		case O_CAST_C_I:
			cVal1=popc();
			iVal1=(long int)cVal1;
			printf("CAST_C_I\t(%c -> %ld)\n",cVal1,iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_CAST_D_C:
			dVal1=popd();
			cVal1=(char)dVal1;
			printf("CAST_D_C\t(%g -> %c)\n",dVal1,cVal1);
			pushc(cVal1);
			IP=IP->next;
			break;
		case O_CAST_D_I:
			dVal1=popd();
			iVal1=(long int)dVal1;
			printf("CAST_D_I\t(%g -> %ld)\n",dVal1,iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_CAST_I_C:
			iVal1=popi();
			cVal1=(char)iVal1;
			printf("CAST_I_C\t(%ld -> %c)\n",iVal1,cVal1);
			pushc(cVal1);
			IP=IP->next;
			break;
		case O_CAST_I_D:
			iVal1=popi();
			dVal1=(double)iVal1;
			printf("CAST_I_D\t(%ld -> %g)\n",iVal1,dVal1);
			pushd(dVal1);
			IP=IP->next;
			break;
		case O_DIV_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("DIV_I\t(%ld/%ld -> %ld)\n",iVal1,iVal2,iVal1/iVal2);
			pushi(iVal2/iVal1);
			IP=IP->next;
			break;
		case O_DIV_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("DIV_D\t(%g/%g -> %g)\n",dVal1,dVal2,dVal1/dVal2);
			pushd(dVal2/dVal1);
			IP=IP->next;
			break;
		case O_DIV_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("DIV_C\t(%c/%c -> %c)\n",cVal1,cVal2,cVal1/cVal2);
			pushc(cVal2/cVal1);
			IP=IP->next;
			break;
		case O_DROP:
			iVal1=IP->args[0].i;
			printf("DROP\t%ld\n",iVal1);
			if(SP-iVal1<stack)err("not enough stack bytes");
			SP-=iVal1;
			IP=IP->next;
			break;
		case O_ENTER:
			iVal1=IP->args[0].i;
			printf("ENTER\t%ld\n",iVal1);
			pusha(FP);
			FP=SP;
			SP+=iVal1;
			IP = IP->next;
			break;
		case O_EQ_A:
			aVal1=popa();
			aVal2=popa();
			printf("EQ_A\t(%p==%p -> %d)\n",aVal2,aVal1,aVal2==aVal1);
			pushi(aVal2==aVal1);
			IP=IP->next;
			break;
		case O_EQ_C:
			cVal1=popc();
			cVal2=popc();
			printf("EQ_C\t(%c==%c -> %d)\n",cVal2,cVal1,cVal2==cVal1);
			pushi(cVal2==cVal1);
			IP=IP->next;
			break;
		case O_EQ_D:
			dVal1=popd();
			dVal2=popd();
			printf("EQ_D\t(%g==%g -> %d)\n",dVal2,dVal1,dVal2==dVal1);
			pushi(dVal2==dVal1);
			IP=IP->next;
			break;
		case O_EQ_I:
			iVal1=popi();
			iVal2=popi();
			printf("EQ_I\t(%ld==%ld -> %d)\n",iVal2,iVal1,iVal2==iVal1);
			pushi(iVal2==iVal1);
			IP=IP->next;
			break;
		case O_GREATER_C:
			cVal1=popc();
			cVal2=popc();
			printf("GREATER_C\t(%c>%c -> %d)\n",cVal2,cVal1,cVal2>cVal1);
			pushi(cVal2>cVal1);
			IP=IP->next;
			break;
		case O_GREATER_D:
			dVal1=popd();
			dVal2=popd();
			printf("GREATER_D\t(%g>%g -> %d)\n",dVal2,dVal1,dVal2>dVal1);
			pushi(dVal2>dVal1);
			IP=IP->next;
			break;
		case O_GREATEREQ_I:
			iVal1=popi();
			iVal2=popi();
			printf("GREATEREQ_I\t(%ld>=%ld -> %d)\n",iVal2,iVal1,iVal2>=iVal1);
			pushi(iVal2>=iVal1);
			IP=IP->next;
			break;
		case O_GREATEREQ_C:
			cVal1=popc();
			cVal2=popc();
			printf("GREATEREQ_C\t(%c>=%c -> %d)\n",cVal2,cVal1,cVal2>=cVal1);
			pushi(cVal2>=cVal1);
			IP=IP->next;
			break;
		case O_GREATEREQ_D:
			dVal1=popd();
			dVal2=popd();
			printf("GREATEREQ_D\t(%g>=%g -> %d)\n",dVal2,dVal1,dVal2>=dVal1);
			pushi(dVal2>=dVal1);
			IP=IP->next;
			break;
		case O_GREATER_I:
			iVal1=popi();
			iVal2=popi();
			printf("GREATER_I\t(%ld>%ld -> %d)\n",iVal2,iVal1,iVal2>iVal1);
			pushi(iVal2>iVal1);
			IP=IP->next;
			break;
		case O_HALT:
			printf("HALT\n");
			return;
		case O_INSERT:
			iVal1=IP->args[0].i; // iDst
			iVal2=IP->args[1].i; // nBytes
			printf("INSERT\t%ld,%ld\n",iVal1,iVal2);
			if(SP+iVal2>stackAfter)err("out of stack");
			memmove(SP-iVal1+iVal2,SP-iVal1,iVal1); //make room
			memmove(SP-iVal1,SP+iVal2,iVal2); //dup
			SP+=iVal2;
			IP=IP->next;
			break;
		case O_JF_A:
			aVal1=popa();
			printf("JF\t%p\t(%p)\n",IP->args[0].addr,aVal1);
			IP=aVal1?IP->next:IP->args[0].addr;
			break;
		case O_JF_C:
			cVal1=popc();
			printf("JF\t%p\t(%c)\n",IP->args[0].addr,cVal1);
			IP=cVal1?IP->next:IP->args[0].addr;
			break;
		case O_JF_D:
			dVal1=popd();
			printf("JF\t%p\t(%g)\n",IP->args[0].addr,dVal1);
			IP=dVal1?IP->next:IP->args[0].addr;
			break;
		case O_JF_I:
			iVal1=popi();
			printf("JF\t%p\t(%ld)\n",IP->args[0].addr,iVal1);
			IP=iVal1?IP->next:IP->args[0].addr;
			break;
		case O_JMP:
			printf("JMP\t%p\n",IP->args[0].addr);
			IP=IP->args[0].addr;
			break;
		case O_JT_A:
			aVal1=popa();
			printf("JT\t%p\t(%p)\n",IP->args[0].addr,aVal1);
			IP=aVal1?IP->args[0].addr:IP->next;
			break;
		case O_JT_C:
			cVal1=popc();
			printf("JT\t%p\t(%c)\n",IP->args[0].addr,cVal1);
			IP=cVal1?IP->args[0].addr:IP->next;
			break;
		case O_JT_D:
			dVal1=popd();
			printf("JT\t%p\t(%g)\n",IP->args[0].addr,dVal1);
			IP=dVal1?IP->args[0].addr:IP->next;
			break;
		case O_JT_I:
			iVal1=popi();
			printf("JT\t%p\t(%ld)\n",IP->args[0].addr,iVal1);
			IP=iVal1?IP->args[0].addr:IP->next;
			break;
		case O_LESS_C:
			cVal1=popc();
			cVal2=popc();
			printf("LESS_C\t(%c>%c -> %d)\n",cVal2,cVal1,cVal2<cVal1);
			pushi(cVal2<cVal1);
			IP=IP->next;
			break;
		case O_LESS_D:
			dVal1=popd();
			dVal2=popd();
			printf("LESS_D\t(%g>%g -> %d)\n",dVal2,dVal1,dVal2<dVal1);
			pushi(dVal2<dVal1);
			IP=IP->next;
			break;
		case O_LESSEQ_I:
			iVal1=popi();
			iVal2=popi();
			printf("LESSEQ_I\t(%ld<=%ld -> %d)\n",iVal2,iVal1,iVal2<=iVal1);
			pushi(iVal2<=iVal1);
			IP=IP->next;
			break;
		case O_LESSEQ_C:
			cVal1=popc();
			cVal2=popc();
			printf("LESSEQ_C\t(%c<=%c -> %d)\n",cVal2,cVal1,cVal2<=cVal1);
			pushi(cVal2<=cVal1);
			IP=IP->next;
			break;
		case O_LESSEQ_D:
			dVal1=popd();
			dVal2=popd();
			printf("LESSEQ_D\t(%g<=%g -> %d)\n",dVal2,dVal1,dVal2<=dVal1);
			pushi(dVal2<=dVal1);
			IP=IP->next;
			break;
		case O_LESS_I:
			iVal1=popi();
			iVal2=popi();
			printf("LESS_I\t(%ld<%ld -> %d)\n",iVal2,iVal1,iVal2<iVal1);
			pushi(iVal2<iVal1);
			IP=IP->next;
			break;
		case O_LOAD:
			iVal1=IP->args[0].i;
			aVal1=popa();
			printf("LOAD\t%ld\t(%p)\n",iVal1,aVal1);
			if(SP+iVal1>stackAfter)err("out of stack");
			memcpy(SP,aVal1,iVal1);
			SP+=iVal1;
			IP=IP->next;
			break;
		case O_MUL_C:
			cVal1=popc();
			cVal2=popc();
			printf("MUL_C\t(%c*%c -> %c)\n",cVal2,cVal1,cVal2*cVal1);
			pushc(cVal2*cVal1);
			IP=IP->next;
			break;
		case O_MUL_D:
			dVal1=popd();
			dVal2=popd();
			printf("MUL_D\t(%g*%g -> %g)\n",dVal2,dVal1,dVal2*dVal1);
			pushd(dVal2*dVal1);
			IP=IP->next;
			break;
		case O_MUL_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("MUL_I\t(%ld*%ld -> %ld)\n",iVal2,iVal1,iVal2*iVal1);
			pushi(iVal2*iVal1);
			IP=IP->next;
			break;
		case O_NEG_D:
			dVal1=-popd();
			printf("NEG_D\t(%g)\n",dVal1);
			pushd(dVal1);
			IP=IP->next;
			break;
		case O_NEG_C:
			cVal1=-popc();
			printf("NEG_C\t(%c)\n",cVal1);
			pushc(cVal1);
			IP=IP->next;
			break;
		case O_NEG_I:
			iVal1=-popi();
			printf("NEG_I\t(%ld)\n",iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_NOP:
			IP=IP->next;
			break;
		case O_NOT_D:
			iVal1=!popd();
			printf("NOT_D\t(%d)\n",(int)iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_NOT_C:
			iVal1=!popc();
			printf("NOT_C\t(%d)\n",(int)iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_NOT_I:
			iVal1=!popi();
			printf("NOT_I\t(%d)\n",(int)iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_NOT_A:
			iVal1=!popa();
			printf("NOT_A\t(%d)\n",(int)iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_NOTEQ_I:
			iVal1=popi();
			iVal2=popi();
			printf("NOTEQ_I\t(%ld!=%ld -> %d)\n",iVal2,iVal1,iVal2!=iVal1);
			pushi(iVal2!=iVal1);
			IP=IP->next;
			break;
		case O_NOTEQ_C:
			cVal1=popc();
			cVal2=popc();
			printf("NOTEQ_C\t(%c!=%c -> %d)\n",cVal2,cVal1,cVal2!=cVal1);
			pushi(cVal2!=cVal1);
			IP=IP->next;
			break;
		case O_NOTEQ_D:
			dVal1=popd();
			dVal2=popd();
			printf("NOTEQ_D\t(%g!=%g -> %d)\n",dVal2,dVal1,dVal2!=dVal1);
			pushi(dVal2!=dVal1);
			IP=IP->next;
			break;
		case O_NOTEQ_A:
			aVal1=popa();
			aVal2=popa();
			printf("NOTEQ_A\t(%p!=%p -> %d)\n",aVal2,aVal1,aVal2!=aVal1);
			pushi(aVal2!=aVal1);
			IP=IP->next;
			break;
		case O_OFFSET:
			iVal1=popi();
			aVal1=popa();
			printf("OFFSET\t(%p+%ld -> %p)\n",aVal1,iVal1,aVal1+iVal1);
			pusha(aVal1+iVal1);
			IP=IP->next;
			break;
		case O_OR_I:
			iVal1=popi();
			iVal2=popi();
			printf("OR_I\t(%ld||%ld -> %d)\n",iVal2,iVal1,iVal2||iVal1);
			pushi(iVal2||iVal1);
			IP=IP->next;
			break;
		case O_OR_C:
			cVal1=popc();
			cVal2=popc();
			printf("OR_C\t(%c||%c -> %d)\n",cVal2,cVal1,cVal2||cVal1);
			pushi(cVal2||cVal1);
			IP=IP->next;
			break;
		case O_OR_D:
			dVal1=popd();
			dVal2=popd();
			printf("OR_D\t(%g||%g -> %d)\n",dVal2,dVal1,dVal2||dVal1);
			pushi(dVal2||dVal1);
			IP=IP->next;
			break;
		case O_OR_A:
			aVal1=popa();
			aVal2=popa();
			printf("OR_A\t(%p||%p -> %d)\n",aVal2,aVal1,aVal2||aVal1);
			pushi(aVal2||aVal1);
			IP=IP->next;
			break;
		case O_PUSHFPADDR:
			iVal1=IP->args[0].i;
			printf("PUSHFPADDR\t%ld\t(%p)\n",iVal1,FP+iVal1);
			pusha(FP+iVal1);
			IP=IP->next;
			break;
		case O_PUSHCT_A:
			aVal1=IP->args[0].addr;
			printf("PUSHCT_A\t%p\n",aVal1);
			pusha(aVal1);
			IP=IP->next;
			break;
		case O_PUSHCT_C:
			cVal1=IP->args[0].i;
			printf("PUSHCT_C\t%c\n",cVal1);
			pushc(cVal1);
			IP=IP->next;
			break;
		case O_PUSHCT_D:
			dVal1=IP->args[0].d;
			printf("PUSHCT_D\t%f\n",dVal1);
			pushd(dVal1);
			IP=IP->next;
			break;
		case O_PUSHCT_I:
			iVal1=IP->args[0].i;
			printf("PUSHCT_I\t%ld\n",iVal1);
			pushi(iVal1);
			IP=IP->next;
			break;
		case O_RET:
			iVal1=IP->args[0].i; // sizeArgs
			iVal2=IP->args[1].i; // sizeof(retType)
			printf("RET\t%ld,%ld\n",iVal1,iVal2);
			oldSP=SP;
			SP=FP;
			FP=popa();
			IP=popa();
			if(SP-iVal1<stack)err("not enough stack bytes");
			SP-=iVal1;
			memmove(SP,oldSP-iVal2,iVal2);
			SP+=iVal2;
			break;
		case O_STORE:
			iVal1=IP->args[0].i;
			if(SP-(sizeof(void*)+iVal1)<stack)err("not enough stack bytes for SET");
			aVal1=*(void**)(SP-((sizeof(void*)+iVal1)));
			printf("STORE\t%ld\t(%p)\n",iVal1,aVal1);
			memcpy(aVal1,SP-iVal1,iVal1);
			SP-=sizeof(void*)+iVal1;
			IP=IP->next;
			break;
		case O_SUB_C:
			cVal1=popc();
			cVal2=popc();
			printf("SUB_C\t(%c-%c -> %c)\n",cVal2,cVal1,cVal2-cVal1);
			pushc(cVal2-cVal1);
			IP=IP->next;
			break;
		case O_SUB_D:
			dVal1=popd();
			dVal2=popd();
			printf("SUB_D\t(%g-%g -> %g)\n",dVal2,dVal1,dVal2-dVal1);
			pushd(dVal2-dVal1);
			IP=IP->next;
			break;
		case O_SUB_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("SUB_I\t(%ld-%ld -> %ld)\n",iVal2,iVal1,iVal2-iVal1);
			pushi(iVal2-iVal1);
			IP=IP->next;
			break;
		default:
			err("invalid opcode: %d",IP->opcode);
		}
	}
}

void mvTest()
{
	Instr *L1;
	int *v=allocGlobal(sizeof(long int));
	addInstrA(O_PUSHCT_A,v);
	addInstrI(O_PUSHCT_I,3);
	addInstrI(O_STORE,sizeof(long int));
	L1=addInstrA(O_PUSHCT_A,v);
	addInstrI(O_LOAD,sizeof(long int));
	addInstrA(O_CALLEXT,requireSymbol(&symbols,"put_i")->addr);
	addInstrA(O_PUSHCT_A,v);
	addInstrA(O_PUSHCT_A,v);
	addInstrI(O_LOAD,sizeof(long int));
	addInstrI(O_PUSHCT_I,1);
	addInstr(O_SUB_I);
	addInstrI(O_STORE,sizeof(long int));
	addInstrA(O_PUSHCT_A,v);
	addInstrI(O_LOAD,sizeof(long int));
	addInstrA(O_JT_I,L1);
	addInstrI(O_PUSHCT_C, 'a');
	addInstrI(O_PUSHCT_C, 'c');
	addInstr(O_ADD_C);
	addInstrD(O_PUSHCT_D, 2.5);
	addInstrD(O_PUSHCT_D, 3.2);
	addInstr(O_DIV_D);
	addInstrI(O_PUSHCT_I, 2);
	addInstrI(O_PUSHCT_I, 6);
	addInstr(O_GREATER_I);
	addInstrD(O_PUSHCT_D, 5.2);
	addInstr(O_CAST_D_I);
	addInstr(O_HALT);
}
