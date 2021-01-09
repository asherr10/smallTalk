#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/***************************************** DECLARATIONS *****************************************/
typedef int NUMBER;
typedef int NAME;
const int   NAMELENG = 25;    /* Maximum length of a name */
const int   MAXNAMES = 500;   /* Maximum number of different names */
const int   MAXINPUT = 40000; /* Maximum length of an input */
const char* PROMPT1 = "-> ";
const char* PROMPT2 = "> ";
const char  COMMENTCHAR = ';';
const int   TABCODE = 9;      /* in ASCII */

struct EXPLISTREC;
typedef EXPLISTREC* EXPLIST;
enum EXPTYPE {VALEXP, VAREXP, APEXP};

struct STVALUEREC;
typedef STVALUEREC* STVALUE;

struct CLASSREC;
typedef CLASSREC* CLASS;

struct EXPREC
{
	EXPTYPE etype; // what type of expression
	STVALUE value;
	NAME variable;
	NAME optr;
	EXPLIST args;
};
typedef EXPREC* EXP;

struct EXPLISTREC
{
	EXP head;
	EXPLIST tail;
};

struct VALUELISTREC
{
	STVALUE head;
	VALUELISTREC* tail;
};
typedef VALUELISTREC* VALUELIST;

struct NAMELISTREC
{
	NAME head;
	NAMELISTREC* tail;
};
typedef NAMELISTREC* NAMELIST;

struct ENVREC
{
	NAMELIST vars;
	VALUELIST values;
};
typedef ENVREC* ENV;

enum STVALUETYPE {INT, SYM, USER};

struct STVALUEREC
{
   CLASS owner;
   STVALUETYPE vtype;
   NUMBER intval; // if its INT
   NAME symval;   // if its SYM
   ENV userval;   // if its USER
};

struct FUNDEFREC
{
	NAME funname;
	NAMELIST formals;
	EXP body;
	FUNDEFREC* nextfundef;
};
typedef FUNDEFREC* FUNDEF;

struct CLASSREC
{
   NAME clname;
   CLASS clsuper;
   NAMELIST clrep;
   FUNDEF exported;
   CLASS nextclass;
};

FUNDEF   fundefs;                // pointer to global functions
ENV      globalEnv;              // pointer to global enviornment
EXP      currentExp;             // pointer to the expression to be eval
char     userinput[MAXINPUT];    // array to store the user input
int      inputleng, pos;
char*    printNames[MAXNAMES];   // symbol table
int      numNames, numBuiltins, numCtrlOps;
NAME     SELF;                   // location of self in the symbol table
CLASS    OBJECTCLASS;            // pointer to the abstract object class
CLASS    classes;                // pointer to all the classes
STVALUE  objectInst;             // pointer to the instance of object class
STVALUE  trueValue, falseValue;
int      quittingtime;

/************************************** DATA STRUCTURE OP'S **************************************/
/* mkVALEXP - return an EXP of type VALEXP with num n */
EXP mkVALEXP(STVALUE v)
{
   EXP e = new EXPREC;
   e->etype = VALEXP;
   e->value = v;
   return e;
} /* mkVAREXP */

/* mkAPEXP - return EXP of type APEXP w/ optr op and args el */
EXP mkAPEXP(NAME op, EXPLIST el)
{
   EXP e = new EXPREC;
   e->etype = APEXP;
   e->optr = op;
   e->args = el;
   return e;
} /* mkAPEXP */

/* mkVAREXP - return an EXP of type VAREXP with varble nm */
EXP mkVAREXP(NAME var)
{
   EXP e = new EXPREC;
   e->etype = VAREXP;
   e->variable = var;
   return e;
} /* mkVAREXP */

/* mkINT - return an STVALUE with integer value n */
STVALUE mkINT(int n)
{
   STVALUE newval = new STVALUEREC;
   newval->owner = OBJECTCLASS;
   newval->vtype = INT;
   newval->intval = n;
   return (newval);
} /* mkINT */

/* mkSYM - return an STVALUE with symbol value s */
STVALUE mkSYM(NAME s)
{
   STVALUE newval = new STVALUEREC;
   newval->owner = OBJECTCLASS;
   newval->vtype = SYM;
   newval->symval = s;
   return (newval);
} /* mkSYM */

/* mkUSER - return a USER-type STVALUE */
STVALUE mkUSER(ENV rho, CLASS ownr)
{
   STVALUE newval = new STVALUEREC;
   newval->owner = ownr;
   newval->vtype = USER;
   newval->userval = rho;
   return (newval);
} /* mkUSER */

/* mkExplist - return an EXPLIST with head e and tail el */
EXPLIST mkEXPLIST(EXP e, EXPLIST el)
{
   EXPLIST newEL = new EXPLISTREC;
   newEL->head = e;
   newEL->tail = el;
   return newEL;
} /* mkExplist */

/* mkNamelist - return a NAMELIST with head n and tail nl */
NAMELIST mkNAMELIST(NAME nm, NAMELIST nl)
{
   NAMELIST newnl = new NAMELISTREC;
   newnl->head = nm;
   newnl->tail = nl;
   return newnl;
} /* mkNamelist */

/* mkValuelist - return an VALUELIST with head n and tail vl */
VALUELIST mkVALUELIST(STVALUE n, VALUELIST vl)
{
   VALUELIST newvl = new VALUELISTREC;
   newvl->head = n;
   newvl->tail = vl;
   return newvl;
} /* mkValuelist */

/* mkEnv - return an ENV with vars nl and values vl */
ENV mkENV(NAMELIST nl, VALUELIST vl)
{
   ENV rho = new ENVREC;
   rho->vars = nl;
   rho->values = vl;
   return rho;
} /* mkEnv */

/* lengthVL - return length of VALUELIST vl */
int lengthVL(VALUELIST vl)
{
   int i = 0;
   while (vl != 0)
   {
	   i++;
	   vl = vl->tail;
   }
   return i;
} /* lengthVL */

/* lengthNL - return length of NAMELIST nl */
int lengthNL(NAMELIST nl)
{
   int i = 0;
   while (nl != 0)
   {
	   i++;
	   nl = nl->tail;
   }
   return i;
} /* lengthNL */

/**************************************** NAME MANAGEMENT ****************************************/
/* fetchFun - get function definition of NAME fname from fenv */
FUNDEF fetchFun(NAME fname, FUNDEF fenv)
{
   while (fenv != 0) 
   {
      if (fenv->funname == fname)
        return fenv;
      fenv = fenv->nextfundef;
   }
   return 0;
} /* fetchFun */

/* fetchClass - get class definition of NAME cname */
CLASS fetchClass(NAME cname)
{
   CLASS c = new CLASSREC;
   c = classes;
   while (c != 0)
   {
      if (c->clname == cname)
         return c;
      c = c->nextclass;
   }
   return 0;
} /* fetchClass */

/* newClass - add new class cname to classes */
CLASS newClass (NAME cname, CLASS super)
{
   CLASS cl = new CLASSREC;
   cl->clname = cname;
   cl->clsuper = super;
   cl->nextclass = classes;
   classes = cl;
   return classes;
} /* newClass */

/* newFunDef - add new function fname to fenv */
FUNDEF newFunDef(NAME fname, FUNDEF& fenv)
{
   FUNDEF f = new FUNDEFREC;
   f->funname = fname;
   f->nextfundef = fenv;
   fenv = f;
   return f; 
} /* newFunDef */

/* initNames - place all pre-defined names into printNames */

void initNames()
{
   int i =0;
   fundefs = 0;
   printNames[i] = (char* )"if";    i++;   //0
   printNames[i] = (char* )"while"; i++;   //1
   printNames[i] = (char* )"set";   i++;   //2
   printNames[i] = (char* )"begin"; i++;   //3
   numCtrlOps    = i;
   printNames[i] = (char* )"new";i++;      //4
   printNames[i] = (char* )"+"; i++;       //5
   printNames[i] = (char* )"-"; i++;       //6
   printNames[i] = (char* )"*"; i++;       //7
   printNames[i] = (char* )"/"; i++;       //8
   printNames[i] = (char* )"="; i++;       //9
   printNames[i] = (char* )"<"; i++;       //10
   printNames[i] = (char* )">"; i++;       //11
   printNames[i] = (char* )"print";i++;    //12
   printNames[i] = (char* )"self";         //13
   SELF = i;
   numNames = i;
   numBuiltins = i-1;
}//initNames


/* install - insert new name into printNames */
NAME install(char* nm)
{
   int i = 0;
   while (i <= numNames)
      if (strcmp(nm, printNames[i]) == 0)
	      break;
	   else
	      i++;
   if (i > numNames)
   {
	  numNames = i;
	  printNames[i] = new char[strlen(nm) + 1];
	  strcpy(printNames[i], nm);
   }
   //std::cout<< printNames[i];
   return i;
} /* install */

/* prName - print name nm */
void prName(NAME nm)
{
	std::cout << printNames[nm];
} /* prName */

/********************************************* INPUT *********************************************/
/* isDelim - check if c is a delimiter */
int isDelim(char c)
{
   return ((c == '(') || (c == ')') || (c == ' ') || (c == COMMENTCHAR));
}

/* skipblanks - return next non-blank position in userinput */
int skipblanks(int p)
{
   while (userinput[p] == ' ')
   	p++;
   return p;
} /* skipblanks */

/* matches - check if string nm matches userinput[s .. s+leng] */
int matches(int s, int leng,  char* nm)
{
   int i = 0;
   while (i < leng)
   {
	   if (userinput[s] != nm[i])
	      return 0;
	   i++;
	   s++;
   }
   if (!isDelim(userinput[s]))
	   return 0;
   return 1;
} /* matches */

/* nextchar - read next char - filter tabs and comments */
void nextchar(char& c)
{

   scanf("%c", &c);
   if (c == COMMENTCHAR)
	   while (c != '\n')
		   scanf("%c",&c);
} /* nextchar */

/* readParens - read char's, ignoring newlines, to matching ')' */
void readParens()
{
   int parencnt = 1; /* current depth of parentheses, '(' just read */
   char c;
   do
   {
	   if (c == '\n')
	      std::cout << PROMPT2;
	   nextchar(c);
	   pos++;
	   if (pos == MAXINPUT )
	   {
		   std::cout << "User input too long\n";
		   exit(1);
	   }
	   if (c == '\n' )
		   userinput[pos] = ' ';
	   else
		   userinput[pos] = c;
	   if (c == '(')
		   ++parencnt;
	   if (c == ')')
	      parencnt--;
	} while (parencnt != 0);
} /* readParens */

/* readInput - read char's into userinput */
void readInput()
{
   char c;
   std::cout << PROMPT1;
   pos = 0;
   do
	{
	   pos++;
	   if (pos == MAXINPUT)
	   {
		   std::cout << "User input too long\n";
		   exit(1);
      }
	   nextchar(c);
	   if (c == '\n' )
		   userinput[pos] = ' ';
	   else
		   userinput[pos] = c;
	   if (userinput[pos] == '(')
		   readParens();
	} while (c != '\n');
	inputleng = pos;
	userinput[pos+1] = COMMENTCHAR; // sentinel
} /* readInput */

/* reader - read char's into userinput; be sure input not blank */
void reader ()
{
   do
   {
	   readInput();
	   pos = skipblanks(1);
   } while (pos > inputleng); // ignore blank lines
} /* reader */

/* parseName - return (installed) NAME starting at userinput[pos] */
NAME parseName()
{
   char nm[25]; // array to accumulate characters
   int leng = 0; // length of name
   while ((pos <= inputleng) && !isDelim(userinput[pos]) && userinput[pos] != ' ')
   {
	   leng++;
	   nm[leng-1] = userinput[pos];
	   pos++;
   }
   if (leng == 0)
   {
	   std::cout << "Error: expected name, instead read: " << userinput[pos] << std::endl;
	   exit(1);
   }
   nm[leng] = '\0';
   pos = skipblanks(pos); // skip blanks after name
   return (install(nm));
} /* parseName */

/* isDigits - check if sequence of digits begins at pos */
int isDigits(int pos)
{
   if ((userinput[pos] < '0') || (userinput[pos] > '9'))
	   return 0;
   while ((userinput[pos] >= '0') && (userinput[pos] <= '9'))
	   pos++;
   if (!isDelim(userinput[pos]))
	   return 0;
   return 1;
} /* isDigits */

/* isNumber - check if a number begins at pos */
int isNumber(int pos)
{
   return (isDigits(pos) || ((userinput[pos] == '-') && isDigits(pos+1)));
} /* isNumber */

/* parseInt - return number starting at userinput[pos] */
int parseInt()
{
   int n = 0, sign = 1;
   if (userinput[pos] == '-')
   {
      sign = -1;
      pos++;
   }
   while ((userinput[pos] >= '0') && (userinput[pos] <= '9'))
   {
      n = 10 * n + userinput[pos] - '0';
      pos++;
   }
   pos = skipblanks(pos); // skip blanks after number
   return (n * sign);
} /* parseInt */

/* isValue - returns 0 or 1 if valid value at pos */
int isValue(int pos)
{
   return ((userinput[pos] == '#') || isNumber(pos));
} /* isValue */

/* parseVal - return number starting at userinput[pos] */
STVALUE parseVal()
{
   if (userinput[pos] == '#') // if symbol
   {
      pos++;
      return mkSYM(parseName());
   }
   if (isNumber(pos)) // if int
      return mkINT(parseInt());
}

EXPLIST parseEL(); // forward declaration
/* parseExp - return EXP starting at userinput[pos] */
EXP parseExp()
{
   pos = skipblanks(pos);
   if (userinput[pos] == '(')
   { // APEXP
	   pos = skipblanks(pos+1); // skip '( ..'
	   NAME nm = parseName();
	   EXPLIST el = parseEL();
	   return (mkAPEXP(nm, el));
   }
   else if (isValue(pos))
	   return (mkVALEXP(parseVal())); // VALEXP
   else
      return (mkVAREXP(parseName())); // VAREXP
} /* parseExp */

/* parseEL - return EXPLIST starting at userinput[pos] */
EXPLIST parseEL()
{
   pos = skipblanks(pos);
   if (userinput[pos] == ')')
   {
	   pos = skipblanks(pos+1); // skip ') ..'
	   return 0;
   }
   EXP e = parseExp();
   EXPLIST el = parseEL();
   return (mkEXPLIST(e, el));
} /* parseEL */

/* parseNL - return NAMELIST starting at userinput[pos] */
NAMELIST parseNL()
{
   if (userinput[pos] == ')')
   {
	   pos = skipblanks(pos + 1); // skip ') ..'
	   return 0;
   }
   NAME nm = parseName();
   NAMELIST nl = parseNL();
   return (mkNAMELIST(nm, nl));
} /* parseNL */

/* parseDef - parse function definition at userinput[pos] */
NAME parseDef(FUNDEF& fenv)
{
   pos = skipblanks(pos);
   pos = skipblanks(pos+1); // skip '( ..'
   pos = skipblanks(pos+6); // skip 'define ..'
   NAME fname = parseName();
   FUNDEF newfun = newFunDef(fname, fenv);
   pos = skipblanks(pos+1); // skip '( ..'
   newfun->formals = parseNL();
   newfun->body = parseExp();
   pos = skipblanks(pos);
   pos = skipblanks(pos+1); // skip ') ..'
   return(fname);
} /* parseDef */

/* parseClass - parse class definition at userinput[pos] */
NAME parseClass()
{
   NAME fname;
   pos = skipblanks(pos);
   pos = skipblanks(pos+1); // skip '( ..'
   pos = skipblanks(pos);
   pos = skipblanks(pos+5); // skip 'class ...'
   NAME cname = parseName();
   NAME sname = parseName();
   CLASS superclass = fetchClass(sname);
  
   CLASS thisclass = newClass(cname,superclass);
   pos = skipblanks(pos);
   pos = skipblanks(pos+1); // skip '( ...'
   NAMELIST rep = parseNL(); // class variable names
   
   // Now link all the inhereited variables
   if (rep == 0)
      thisclass->clrep = superclass->clrep;
   else
   {
      thisclass->clrep = rep;
      NAMELIST temp = rep;
      while (temp ->tail != 0 )
         temp = temp->tail;
      temp->tail = superclass->clrep;
   }
   pos = skipblanks(pos);
   // now parse all the methods
   FUNDEF cenv = 0;
   while (userinput[pos]=='(')
   {
         pos = skipblanks(pos);
         fname = parseDef(cenv);
         prName(fname);
         std::cout << std::endl;
   }
   thisclass->exported = cenv;
   
   pos = skipblanks(pos);
   pos = skipblanks(pos+1); // skip ' ..)'
   return cname;
} /* parseClass */

/***************************************** ENVIRONMENTS *****************************************/
/* emptyEnv - return an environment with no bindings */
ENV emptyEnv()
{
   return mkENV(0, 0);
} /* emptyEnv */

/* bindVar - bind variable nm to value n in environment rho */
void bindVar(NAME nm, STVALUE v, ENV rho)
{
   rho->vars = mkNAMELIST(nm, rho->vars);
   rho->values = mkVALUELIST(v, rho->values);
} /* bindVar */

/* findVar - look up nm in rho */
VALUELIST findVar(NAME nm, ENV rho)
{
   NAMELIST nl = rho->vars;
   VALUELIST vl = rho->values;
   while (nl != 0)
	   if (nl->head == nm)
	      break;
	   else
      {
		   nl = nl->tail;
		   vl = vl->tail;
		}
   return vl;
} /* findVar */

/* assign - assign value n to variable nm in rho */
void assign(NAME nm,  STVALUE v, ENV rho)
{
   VALUELIST varloc = findVar(nm, rho);
   varloc->head = v;
} /* assign */

/* fetch - return number bound to nm in rho */
STVALUE fetch(NAME nm, ENV rho)
{
   VALUELIST vl = findVar(nm, rho);
   return (vl->head);
} /* fetch */

/* isBound - check if nm is bound in rho */
int isBound(NAME nm, ENV rho)
{
   return (findVar(nm, rho) != 0);
} /* isBound */

/******************************************** VALUES ********************************************/
// prValue - print value v
void prValue(STVALUE v)
{
   if (v->vtype == INT)
      std::cout << (v->intval);
   else if (v->vtype == SYM)
      prName(v->symval);
   else
      std::cout << "<userval>";
} /* prValue */

/* isTrueVal - return true if v is true (non-zero) value */
int isTrueVal(STVALUE v)
{
   if ((v->vtype == USER) || (v->vtype == SYM))
      return 1;
   return (v->intval != 0);
} /* isTrueVal */

/* arity - return number of arguments expected by op */
int arity(int op)
{
	if ((op > 4) && (op < 12))
	   return 2;
	return  1;
} /* arity */

/* applyValueOp - apply VALUEOP op to arguments in VALUELIST vl */
STVALUE applyValueOp(int op, VALUELIST vl)
{
   if (arity(op) != lengthVL(vl))
   {
      std::cout << "Wrong number of arguments to ";
      //std::cout << op;
      prName(op);
      std::cout << std::endl;
   }

   STVALUE s, s1, s2;
   s1 = vl->head;
   if (arity(op) == 2)
      s2 = vl->tail->head;

   switch (op)
   {
      case 5: // +
         s =  mkINT(s1->intval + s2->intval);
         break;
      case 6: // -
         s = mkINT(s1->intval - s2->intval);
         break;
      case 7: // *
         s = mkINT(s1->intval * s2->intval);
         break;
      case 8: // /
         s = mkINT(s1->intval / s2->intval);

         break;
      case 9: // =
         if (s1->vtype == s2->vtype)
   	      if ((s1->vtype == INT) && (s1->intval == s2->intval))
   	   	   return trueValue;
   	      if ((s1->vtype == SYM) && (s1->symval == s2->symval))
   	   	   return trueValue;
		   return falseValue;
      case 10: // <
         s= mkINT(s1->intval < s2->intval);
         break;
      case 11: // >
         s = mkINT(s2->intval > s2->intval);
         break;
      case 12: // print
         prValue(s1);
         std::cout << std::endl;
         s = s1;
         break;
   };
   return s;
} /* applyValueOp */

/****************************************** EVALUATION ******************************************/
STVALUE eval(EXP e, ENV rho, STVALUE rcvr);
/* evalList - evaluate each expression in el */
VALUELIST evalList(EXPLIST el, ENV rho, STVALUE rcvr)
{
   if (el == 0)
		return 0;
	STVALUE h = eval(el->head, rho, rcvr);
	VALUELIST v = evalList(el->tail, rho, rcvr);
	return mkVALUELIST(h, v);
} /* evalList */

/* applyGlobalFun - apply function defined at top level */
STVALUE applyGlobalFun(NAME optr, VALUELIST actuals, STVALUE rcvr)
{
   FUNDEF f = fetchFun(optr, fundefs);   
   ENV rho = mkENV(f->formals, actuals);   
   return eval(f->body, rho, rcvr);
} /* applyGlobalFun */

/* methodSearch - find class of optr, if any, starting at cl */
FUNDEF methodSearch(NAME optr, CLASS cl)
{
   FUNDEF f = 0;
	while (cl != 0)
   {
		f = fetchFun(optr, cl->exported);
		if (f != 0)
		   return f;
		cl = cl->clsuper;
	}
	return 0;
} /* methodSearch */

/* applyMethod - apply method f to actuals */
STVALUE applyMethod(FUNDEF f, VALUELIST actuals)
{
   ENV rho = mkENV(f->formals, actuals->tail);
   return eval(f->body, rho, actuals->head);
} /* applyMethod */

/* mkRepFor - make list of all zeros of same length as nl */
VALUELIST mkRepFor(NAMELIST nl)
{
   if (nl == 0)
      return 0;
   return mkVALUELIST(falseValue, mkRepFor(nl->tail));
} /* mkRepFor */

/* applyCtrlOp - apply CONTROLOP op to args in rho */
STVALUE applyCtrlOp(int op, EXPLIST args, ENV rho, STVALUE rcvr)
{
   STVALUE v;
   NAME x;
   EXPLIST temp = args;
   switch(op)
   {
      case 0: // if
         if (isTrueVal(eval(args->head, rho, rcvr)))
            return eval(args->tail->head, rho, rcvr);
         return eval(args->tail->tail->head, rho, rcvr);
      case 1: // while
         while (isTrueVal(eval(args->head, rho, rcvr)))
            eval(args->tail->head, rho, rcvr);     
         return eval(args->head, rho, rcvr);
      case 2: // set
         v = eval(args->tail->head, rho, rcvr);
         x = args->head->variable;
         if (isBound(x, rho))
            assign(x, v, rho);
         else if (isBound(x, rcvr->userval))
            assign(x, v, rcvr->userval);
        else bindVar(x, v, globalEnv);
         return v;
      case 3: // begin
         while (temp != 0)      
         {         
            v = eval(temp->head,rho, rcvr);         
            temp = temp->tail;      
         }      
         return v;
      case 4: // new
         CLASS cl = fetchClass(args->head->variable);
         VALUELIST vl = mkRepFor(cl->clrep);
         ENV rho = mkENV(cl->clrep, vl);
         STVALUE newval = mkUSER(rho, cl);
         assign(SELF, newval, newval->userval);
         return newval;
   }
   return v;
} /* applyCtrlOp */


/* eval - return value of e in environment rho, receiver rcvr */

STVALUE eval(EXP e, ENV rho, STVALUE rcvr)
{
   VALUELIST vl;
   FUNDEF f;
   switch (e->etype)
   {
      case VALEXP:
         return (e->value);
      case VAREXP:
         if (isBound(e->variable, rho))
            return fetch(e->variable, rho);
         if (isBound(e->variable, rcvr->userval))
            return fetch(e->variable, rcvr->userval);
         if (isBound(e->variable, globalEnv))
            return fetch(e->variable, globalEnv);
         break;
      case APEXP:
         if (e->optr <= numCtrlOps)
            return applyCtrlOp(e->optr, e->args, rho, rcvr);
         vl = evalList(e->args, rho, rcvr);
         if (vl == 0)
            return applyGlobalFun(e->optr, vl, rcvr);
         f = methodSearch(e->optr, vl->head->owner);
         if (f!= 0)
            return applyMethod(f, vl);
         if (e->optr <= numBuiltins)
            return applyValueOp(e->optr, vl);
         return applyGlobalFun(e->optr, vl, rcvr);
         break;
   }
} /* eval */

/************************************* READ-EVAL-PRINT LOOP *************************************/
/* initHierarchy - allocate class Object and create an instance */
void initHierarchy()
{
   classes = 0;
   OBJECTCLASS = newClass(install((char* )"Object"), 0);
   OBJECTCLASS->exported = 0;
   OBJECTCLASS->clrep = mkNAMELIST(SELF, 0);
   objectInst = mkUSER(mkENV(OBJECTCLASS->clrep, mkVALUELIST(mkINT(0), 0)), OBJECTCLASS);
} /* initHierarchy */

int main()
{
   initNames();
   initHierarchy();
   globalEnv = emptyEnv();

   trueValue = mkINT(1);
   falseValue = mkINT(0);
   quittingtime = 0;

   while (!quittingtime)
   {
      reader();
      if (matches(pos, 4,(char* ) "quit"))
         quittingtime = 	1;
      else if ((userinput[pos] == '(') && (matches(skipblanks(pos+1), 6,(char* ) "define")))
      {
         prName(parseDef(fundefs));
         std::cout << std::endl;
      }
      else if ((userinput[pos]=='(') && (matches(skipblanks(pos+1), 5,(char* ) "class")))
      {
         prName(parseClass());
         std::cout << std::endl;
      }
      else
      {
         currentExp = parseExp();
         prValue(eval(currentExp, emptyEnv(), objectInst));
         std::cout << std::endl;
      }
   } // while
   return 0;
} // smalltalk
