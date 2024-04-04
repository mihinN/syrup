/***********************************************************************
 ***********************************************************************
 *
 * PROJECT : 	SYRUP LANGUAGE
 * PROJECT TYPE : HOBBY
 * DATE : 1st OCTOBER 2023 4.05 A.M SRI LANKA SUNDAY
 * AUTHOR : K.MIHIN
 *
 ***********************************************************************
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* if we are compiling on Windows */

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */

char* readline(char* prompt) {

	fputs(prompt,stdout);
	fgets(buffer,2048,stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy,buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

/* Fake add history function */

void add_history(char* unused) {
}
#else
#include <readline/readline.h>
#include <readline/history.h>

#endif
/* Create enumeration of possible Error types */

enum { LERR_DIV_ZERO , LERR_BAD_OP, LERR_BAD_NUM };

/* Create Enumiration of possible lval types */

enum { LVAL_NUM,LVAL_ERR,LVAL_SYM,LVAL_SEXPR };

/* Declare new lval struct */

typedef struct lval {

	int type;
	long num;
	char* err;
	char* sym;

	/* count and pointer to the List  of "lval" */
	
	int count;
	struct lval** cell;

} lval;

/* Create new number type lval */

lval* lval_num(long x) {

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/* construct a pointer to a new Error lval  */

lval* lval_err(char* m) {

  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m)+1);
  strcpy(v->err,m);
  return v;
}

/* construct a pointer to an new symbol lval */

lval* lval_sym(char* s) {

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s)+1);
	strcpy(v->sym,s);
	return v;
}

/* Pointer to a new empty Sub EXPR lval */
lval* lval_sexpr(void) {

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

/* Special function to Free memory */

void lval_del(lval* v){
	switch(v->type){
		/* do nothing special for number type */
		case LVAL_NUM: break;
		case LVAL_ERR: free(v->err);
		break;
		case LVAL_SYM: free(v->sym);
		break;
		/* if sexpr then delete all the elements inside */
		case LVAL_SEXPR: 
			for(int i=0;i<v->count;i++){
				lval_del(v->cell[i]);
			}
			/* free the memory allocation to contain the pointers */
		free(v->cell);
		break;
	}
	/* Free the memory allocvated for the "Lval" struct itself */
	free(v);

}

lval* lval_add(lval* v,lval* x) {
	v->count++;
	v->cell = realloc(v->cell,sizeof(lval*)*v->count);
	v->cell[v->count-1]=x;
	return v;
}

lval* lval_pop(lval* v,int i) {
	/* Find the item at "i" */
	lval* x = v->cell[i];
	/* Shift mempry after the item at "i" overthe top */
	memmove(&v->cell[i],&v->cell[i+1],sizeof(lval*)*(v->count-i-1));
	/* Decrease the count of items in the List */
	v->count--;
	/* Reallocate the memory used */
	v->cell = realloc(v->cell,sizeof(lval*)*v->count);
	return x;
}

lval* lval_take(lval* v,int i) {
	lval* x = lval_pop(v,i);
	lval_del(v);
	return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v,char open,char close) {
	putchar(open);
	for(int i=0;i<v->count;i++){
		/* Print value contained within */
		lval_print(v->cell[i]);
		/* Dont print the trailing element */
		if(i != (v->count-1)){
			putchar(' ');
		}
	}
	putchar(close);
}

/* Print an "lval" */
void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

/* Print an "lval" Followed by New Line */

void lval_println(lval* v) {	lval_print(v);putchar('\n');	}

/* Lval buitin op*/
lval* builtin_op(lval* a,char* op) {
	/* Ensure all arguents are numbers */
	for(int i=0;i<a->count;i++) {
		if(a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Cannot Operate on NON numbers \n");
		}
	}
	/* POP the first Element */
	lval* x = lval_pop(a,0);
	/* of no arguments and sub then perform unary negation */
	if ((strcmp(op,"-") == 0) && a->count ==0){
		x->num = -x->num;
	}
	/* while there are still elements remaining */
	while(a->count >0){
		/* POP the next Element */
		lval* y = lval_pop(a,0);
		/* Perform Operation */
		if (strcmp(op,"+") == 0){ x->num += y->num;}
		if (strcmp(op,"-") == 0){ x->num -= y->num;}
		if (strcmp(op,"*") == 0){ x->num *= y->num;}
		if (strcmp(op,"/") == 0){
			if (y->num == 0){
				lval_del(x);
				lval_del(y);
				x = lval_err("Division by Zero");
				break;
			}
			x->num /= y->num;
		}
		/* Delete element now finished with */
		lval_del(y);		
	}
	/* delete input Expression and return Result */
	lval_del(a);
	return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v){
	/* Evalute Children */
	for (int i=0;i<v->count;i++){
		v->cell[i] = lval_eval(v->cell[i]);
	}
	/* Error checking */
	for (int x=0;x<v->count;x++){
		if (v->cell[x]->type == LVAL_ERR){
			return(lval_take(v,x));
		}
	}
	/* Empty Expression */
	if (v->count == 0){
		return v;
	}
	/* Single Expr */
	if (v->count == 0){
		return lval_take(v,0);
	}
	/* Ensure first Element is a Symbol */
	lval* f = lval_pop(v,0);
	if (f->type != LVAL_SYM){
		lval_del(f);
		lval_del(v);
		return lval_err("Sub Expressions does not starting with Symbols \n");
	}
	/* Call builtint with the Operator */
	lval* result = builtin_op(v,f->sym);
	lval_del(f);
	return result;

}

lval* lval_eval(lval* v){
	/* Evalute Expressions */
	if (v->type == LVAL_SEXPR){
		return lval_eval_sexpr(v);
		/* All other lval types are remianing same */
		return v;
	}
}

lval* lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t->contents,NULL,10);
	return errno != ERANGE? 
	lval_num(x) : lval_err("Invalid Number \n");
}



lval* lval_read(mpc_ast_t* t) {
  
  /* If Symbol or Number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  
  /* If root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } 
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  
  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  
  return x;
}


int main(int argc,char** argv) {

	/* Create some parser */

	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Syrup = mpc_new("syrup");

	/* Define them with the folowing Language */

 mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      Symbol : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      syrup    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Symbol, Expr, Syrup);

	puts("Syrup Version 0.0.0.2");
	puts("Press Ctrl+c to Exit \n");

	while(1){
		char* input = readline("Syrup> ");
		add_history(input);
		/* Attemp to parse the user input */
		mpc_result_t r;
			if(mpc_parse("<stdin>",input,Syrup,&r)){
				lval* x = lval_eval(lval_read(r.output));
				lval_println(x);
				lval_del(x);
				mpc_ast_delete(r.output);
			}else{
				/* Otherwise print and delete the Error */
				mpc_err_print(r.error);
				mpc_err_delete(r.error);
			}
			free(input);
	}
	/* Undefined and Delete the parser .. */
	mpc_cleanup(5,Number,Symbol,Expr,Syrup);
	return 0;
  
}