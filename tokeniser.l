%{
#include "InstructionTree.h"
#include "grammar.tab.hpp"
#include <stdio.h>
#include <string>
#include <assert.h>
#include "shared.h"
using namespace std;

//needs to be in sync with the grammar
#define AT 27

union
{
	float f;
	unsigned int i;
} swappa;
			
%}

%option noyywrap

%%

"//".*$		{}
"\.file".*$	{}
"\.text".*$	{}
"\.globl".*$	{
					char *p = yytext;
					//find globl
					p = strstr(yytext, ".globl");
					assert(p);
					//move to whitespace
					p += 6;
					assert(*p == ' ' || *p == '\t');
					do
					{
						p++;
					} while (*p == ' ' || *p == '\t');
					yylval.p = new Label(p, true); return LABEL_DEF;
				}
"\.weak".*$		{
					char *p = yytext;
					//find globl
					p = strstr(yytext, ".weak");
					assert(p);
					//move to whitespace
					p += 5;
					assert(*p == ' ' || *p == '\t');
					do
					{
						p++;
					} while (*p == ' ' || *p == '\t');
					yylval.p = new Label(p, true); return LABEL_DEF;
				}
"\.type".*$	{}
"\.size".*$	{}
"\.section".*$	{}


"\.set"[ \t]+"reorder".*$	{ return REORDER_BEGIN; }
"\.set"[ \t]+"noreorder".*$	{ return REORDER_END; }

"\.align".*$	{}
"/*".*"*/" { /* not a very good way of getting rid of comments */ }

"1\.0"		{ yylval.p = new Value(1, 1); return NUM; }
"2\.0"		{ yylval.p = new Value(2, 1); return NUM; }
"4\.0"		{ yylval.p = new Value(4, 1); return NUM; }
"8\.0"		{ yylval.p = new Value(8, 1); return NUM; }
"16\.0"		{ yylval.p = new Value(16, 1); return NUM; }
"32\.0"		{ yylval.p = new Value(32, 1); return NUM; }
"64\.0"		{ yylval.p = new Value(64, 1); return NUM; }
"128\.0"		{ yylval.p = new Value(128, 1); return NUM; }
"1/256"		{ yylval.p = new Value(1, 256); return NUM; }
"1/128"		{ yylval.p = new Value(1, 128); return NUM; }
"1/64"		{ yylval.p = new Value(1, 64); return NUM; }
"1/32"		{ yylval.p = new Value(1, 32); return NUM; }
"1/16"		{ yylval.p = new Value(1, 16); return NUM; }
"1/8"		{ yylval.p = new Value(1, 8); return NUM; }
"1/4"		{ yylval.p = new Value(1, 4); return NUM; }
"1/2"		{ yylval.p = new Value(1, 2); return NUM; }
[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?f	{ swappa.f = std::stof(yytext); yylval.p = new Value(swappa.i); ((Value *)yylval.p)->SetAsFloat(); return NUM; }

[0-9]+		{ yylval.p = new Value(atoll(yytext)); return NUM; }
0x[0-9a-f]+   { yylval.p = new Value(strtoll(yytext + 2, 0 , 16)); return NUM; }
\-[0-9]+   { yylval.p = new Value(atoll(yytext)); return NUM; }
";"      { return SEMICOLON; }
","      { return COMMA; }
<<EOF>>  { return 0; }
[ \t\n]+ { }
"s"			{ return SETFLAGS; }
"a"			{ return ABSOLUTE; }
^[_a-zA-Z$][a-zA-Z0-9_]*:	{ yylval.p = new Label(yytext, true); return LABEL_DEF; }
\#[_a-zA-Z$][a-zA-Z0-9_]*\#	{ yylval.p = new Label(yytext, false); return LABEL_TARGET; }
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
acc[0-5]	{ yylval.p = new Register(Register::kAcc, atoi(yytext + 3)); return ACC_REG; }

at			{ yylval.p = new Register(Register::kRb, AT); return RB_REG; }
gp			{ yylval.p = new Register(Register::kRa, 28); return RA_REG; }
fp			{ yylval.p = new Register(Register::kRa, 29); return RA_REG; }
sp			{ yylval.p = new Register(Register::kRa, 30); return RA_REG; }
lr			{ yylval.p = new Register(Register::kRa, 31); return RA_REG; }

rda_unif	{ yylval.p = new Register(Register::kRa, 32); return UNIFRA; }
rdb_unif	{ yylval.p = new Register(Register::kRb, 32); return UNIFRB; }

wra_acc0	{ yylval.p = new Register(Register::kRa, 32); return ACC0WA; }
wrb_acc0	{ yylval.p = new Register(Register::kRb, 32); return ACC0WB; }
wra_acc1	{ yylval.p = new Register(Register::kRa, 33); return ACC1WA; }
wrb_acc1	{ yylval.p = new Register(Register::kRb, 33); return ACC1WB; }
wra_acc2	{ yylval.p = new Register(Register::kRa, 34); return ACC2WA; }
wrb_acc2	{ yylval.p = new Register(Register::kRb, 34); return ACC2WB; }
wra_acc3	{ yylval.p = new Register(Register::kRa, 35); return ACC3WA; }
wrb_acc3	{ yylval.p = new Register(Register::kRb, 35); return ACC3WB; }
wra_acc5	{ yylval.p = new Register(Register::kRa, 37); return ACC5WA; }
wrb_acc5	{ yylval.p = new Register(Register::kRb, 37); return ACC5WB; }

elem_num	{ yylval.p = new Register(Register::kRa, 38); return ELEMRA; }
qpu_num		{ yylval.p = new Register(Register::kRb, 38); return QPURB; }

rda_nop		{ yylval.p = new Register(Register::kRa, 39); return NOPRA; }
rdb_nop		{ yylval.p = new Register(Register::kRb, 39); return NOPRB; }
wra_nop		{ yylval.p = new Register(Register::kRa, 39); return NOPWA; }
wrb_nop		{ yylval.p = new Register(Register::kRb, 39); return NOPWB; }

rda_vpm_dat	{ yylval.p = new Register(Register::kRa, 48); return VPMRA; }
rdb_vpm_dat	{ yylval.p = new Register(Register::kRb, 48); return VPMRB; }

wra_vpm_dat	{ yylval.p = new Register(Register::kRa, 48); return VPMWA; }
wrb_vpm_dat	{ yylval.p = new Register(Register::kRb, 48); return VPMWB; }

vpm_ld_busy	{ yylval.p = new Register(Register::kRa, 49); return VPMRABUSY; }
vpm_st_busy	{ yylval.p = new Register(Register::kRb, 49); return VPMRBBUSY; }

vpm_ld_setup	{ yylval.p = new Register(Register::kRa, 49); return VPMWASETUP; }
vpm_st_setup	{ yylval.p = new Register(Register::kRb, 49); return VPMWBSETUP; }

vpm_ld_wait	{ yylval.p = new Register(Register::kRa, 50); return VPMRAWAIT; }
vpm_st_wait	{ yylval.p = new Register(Register::kRb, 50); return VPMRBWAIT; }

vpm_ld_addr	{ yylval.p = new Register(Register::kRa, 50); return VPMWAADDR; }
vpm_st_addr	{ yylval.p = new Register(Register::kRb, 50); return VPMWBADDR; }

"nop"		{ return NOP; }
"mov"		{ return MOV; }

"load_word"	{ return LOAD_WORD; }
"store_word" { return STORE_WORD; }
"lea" 		{ return LEA; }
"vpm"		{ return VPM; }

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

"vecrot"	{ return VECROT; }

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
