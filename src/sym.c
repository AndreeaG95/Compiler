/*
 * sym.c

 *
 *  Created on: 21.05.2017
 *      Author: Andreea
 */
#include <stdlib.h>
#include "sym.h"

int crtDepth; //("adancimea" contextului curent, initial 0)
Symbol* crtFunc; //(pointer la simbolul functiei daca in functie, altfel NULL)
Symbol* crtStruct; //(pointer la simbolul structurii daca in structura, altfel NULL)

void initSymbols(Symbols *symbols){
	symbols->begin=NULL;
	symbols->end=NULL;
	symbols->after=NULL;
}

