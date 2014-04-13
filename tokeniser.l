%{
#include "InstructionTree.h"
#include "grammar.tab.hpp"
#include <stdio.h>
#include "shared.h"
using namespace std;
%}

%option noyywrap

%%

[0-9]+   { yylval.p = new Value(atoi(yytext)); return NUM; }
0x[0-9a-f]+   { yylval.p = new Value(strtol(yytext + 2, 0 , 16)); return NUM; }
\-[0-9]+   { yylval.p = new Value(atoi(yytext)); return NUM; }
";"      { return SEMICOLON; }
","      { return COMMA; }
<<EOF>>  { return 0; }
[ \t\n]+ { }
"s"			{ return SETFLAGS; }
"a"			{ return ABSOLUTE; }
^[A-Z][A-Z0-9]*:	{ yylval.p = new Label(yytext, true); return LABEL_DEF; }
\#[A-Z][A-Z0-9]*\#	{ yylval.p = new Label(yytext, false); return LABEL_TARGET; }
.        { printf("Unrecognized token! \"%s\"\n", yytext); exit(1); }

"never"		{ yylval.p = new InstructionCondition(kNever); return NEVER; }
"al"		{ yylval.p = new InstructionCondition(kAlways); return AL; }
"zs"		{ yylval.p = new InstructionCondition(kZset); return ZS; }
"zc"		{ yylval.p = new InstructionCondition(kZclear); return ZC; }
"ns"		{ yylval.p = new InstructionCondition(kNset); return NS; }
"nc"		{ yylval.p = new InstructionCondition(kNclear); return NC; }
"cs"		{ yylval.p = new InstructionCondition(kCset); return CS; }
"cc"		{ yylval.p = new InstructionCondition(kCclear); return CC; }

"il"		{ return IL; }
"bl"		{ return BR; }

ra[0-9]+	{ yylval.p = new Register(Register::kRa, atoi(yytext + 2)); return RA_REG; }
rb[0-9]+	{ yylval.p = new Register(Register::kRb, atoi(yytext + 2)); return RB_REG; }
acc[0-9]+	{ yylval.p = new Register(Register::kAcc, atoi(yytext + 3)); return ACC_REG; }

"nop"		{ return NOP; }

"sem"		{ return SEM; }
"inc"		{ return INC; }
"dec"		{ return DEC; }

"allzs"		{ yylval.p = new BrCondition(kAllZSet); return ALLZS; }
"allzc"		{ yylval.p = new BrCondition(kAllZClear); return ALLZC; }

"anyzs"		{ yylval.p = new BrCondition(kAnyZSet); return ANYZS; }
"anyzc"		{ yylval.p = new BrCondition(kAnyZClear); return ANYZC; }

"allns"		{ yylval.p = new BrCondition(kAllNSet); return ALLNS; }
"allnc"		{ yylval.p = new BrCondition(kAllNClear); return ALLNC; }

"anyns"		{ yylval.p = new BrCondition(kAnyNSet); return ANYNS; }
"anync"		{ yylval.p = new BrCondition(kAnyNClear); return ANYNC; }

"allcs"		{ yylval.p = new BrCondition(kAllCSet); return ALLCS; }
"allcc"		{ yylval.p = new BrCondition(kAllCClear); return ALLCC; }

"anycs"		{ yylval.p = new BrCondition(kAnyCSet); return ANYCS; }
"anycc"		{ yylval.p = new BrCondition(kAnyCClear); return ANYCC; }

"addnop"	{ yylval.p = new Opcode(kAddNop); return ADDNOP; }
"fadd"		{ yylval.p = new Opcode(kFadd); return FADD; }
"fsub"		{ yylval.p = new Opcode(kFsub); return FSUB; }
"fmin"		{ yylval.p = new Opcode(kFmin); return FMIN; }
"fmax"		{ yylval.p = new Opcode(kFmax); return FMAX; } 
"fminabs"	{ yylval.p = new Opcode(kFminAbs); return FMINABS; }
"fmaxabs"	{ yylval.p = new Opcode(kFmaxAbs); return FMAXABS; }
"ftoi"		{ yylval.p = new Opcode(kFtoi); return FTOI; }
"itof"		{ yylval.p = new Opcode(kItof); return ITOF; }
"add"		{ yylval.p = new Opcode(kAdd); return ADD; }
"sub"		{ yylval.p = new Opcode(kSub); return SUB; }
"shr"		{ yylval.p = new Opcode(kShr); return SHR; }
"asr"		{ yylval.p = new Opcode(kAsr); return ASR; }
"ror"		{ yylval.p = new Opcode(kRor); return ROR; }
"shl"		{ yylval.p = new Opcode(kShl); return SHL; }
"min"		{ yylval.p = new Opcode(kMin); return MIN; }
"max"		{ yylval.p = new Opcode(kMax); return MAX; }
"and"		{ yylval.p = new Opcode(kAnd); return AND; }
"or"		{ yylval.p = new Opcode(kOr); return OR; }
"xor"		{ yylval.p = new Opcode(kXor); return XOR; }
"not"		{ yylval.p = new Opcode(kNot); return NOT; }
"clz"		{ yylval.p = new Opcode(kClz); return CLZ; }
"addv8adds"	{ yylval.p = new Opcode(kV8addsAdd); return ADDV8ADDS; }
"addv8subs"	{ yylval.p = new Opcode(kV8subsAdd); return ADDV8SUBSS; }

"mulnop"	{ yylval.p = new Opcode(kMulNop); return MULNOP; }
"fmul"		{ yylval.p = new Opcode(kFmul); return FMUL; }
"mul24"		{ yylval.p = new Opcode(kMul24); return MUL24; }
"v8muld"	{ yylval.p = new Opcode(kV8muld); return V8MULD; }
"v8min"		{ yylval.p = new Opcode(kV8min); return V8MIN; }
"v8max"		{ yylval.p = new Opcode(kV8max); return V8MAX; }
"mulv8adds"	{ yylval.p = new Opcode(kV8addsMul); return MULV8ADDS; }
"mulv8subs"	{ yylval.p = new Opcode(kV8subsMul); return MULV8SUBS; }

"Breakpoint"		{ yylval.p = new AluSignal(kBreakpoint); return BREAKPOINT; }
"NoSignal"			{ yylval.p = new AluSignal(kNoSignal); return NOSIGNAL; }
"ThreadSwitch"		{ yylval.p = new AluSignal(kThreadSwitch); return THREADSWITCH; }
"ProgramEnd"		{ yylval.p = new AluSignal(kProgramEnd); return PROGRAMEND; }
"ScoreboardWait"	{ yylval.p = new AluSignal(kScoreboardWait); return SCOREBOARDWAIT; }
"ScoreboardUnlock"	{ yylval.p = new AluSignal(kScoreboardUnlock); return SCOREBOARDUNLOCK; }
"LastThreadSwitch"	{ yylval.p = new AluSignal(kLastThreadSwitch); return LASTHREADSWITCH; }
"CoverageLoad"		{ yylval.p = new AluSignal(kCoverageLoad); return COVERAGELOAD; }
"ColourLoad"		{ yylval.p = new AluSignal(kColourLoad); return COLOURLOAD; }
"ColourLoadPe"		{ yylval.p = new AluSignal(kColourLoadPe); return COLOURLOADPE; }
"LoadTmu0"			{ yylval.p = new AluSignal(kLoadTmu0); return LOADTMU0; }
"LoadTmu1"			{ yylval.p = new AluSignal(kLoadTmu1); return LOADTMU1; }
"AlphaMaskLoad"		{ yylval.p = new AluSignal(kAlphaMaskLoad); return ALPHAMASKLOAD; }
"SmallImmOrVecRot"	{ yylval.p = new AluSignal(kSmallImmOrVecRot); return SMALLIMMORVECROT; }
"LoadImm"			{ yylval.p = new AluSignal(kLoadImm); return LOADIMM; }
"Branch"			{ yylval.p = new AluSignal(kBranch); return BRANCH; }

"\.word"			{ return WORD; }
"\.short"			{ return SHORT; }
"\.byte"			{ return BYTE; }

%%
