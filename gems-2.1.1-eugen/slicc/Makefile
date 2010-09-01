# Makefile 
#
# $Id$
#

# GEMS_ROOT points to the base of the CVS tree
GEMS_ROOT = ..
HOST_TYPE := $(shell $(GEMS_ROOT)/scripts/calc_host.sh)
GENERATED_DIR := generated
VPATH = .:$(GEMS_ROOT)/common:$(GENERATED_DIR):symbols:generator:ast

include $(GEMS_ROOT)/common/Makefile.common

INCLUDES =

SPECIAL_FLAGS = -DYY_NO_UNPUT

#
# all the sources
#
SRC_C = \
	parser.c \
	lexer.c \

SRC_CPP = \
	main.C \
	util.C \
	fileio.C \
	html_gen.C \
	mif_gen.C \
	Func.C \
	StateMachine.C \
	Symbol.C \
	SymbolTable.C \
	Transition.C \
	Type.C \
	Var.C \
	Location.C \
	AST.C \
	AssignStatementAST.C \
	ReturnStatementAST.C \
	DeclAST.C \
	DeclListAST.C \
	EnqueueStatementAST.C \
	EnumExprAST.C \
	ExprAST.C \
	ExprStatementAST.C \
	FormalParamAST.C \
	FuncCallExprAST.C \
	IfStatementAST.C \
	InfixOperatorExprAST.C \
	LiteralExprAST.C \
	MemberExprAST.C \
	MethodCallExprAST.C \
	PairAST.C \
	PairListAST.C \
	PeekStatementAST.C \
	CheckAllocateStatementAST.C \
	CheckStopSlotsStatementAST.C \
	CopyHeadStatementAST.C \
	StatementAST.C \
	StatementListAST.C \
	TypeAST.C \
	TypeFieldEnumAST.C \
	TypeFieldMemberAST.C \
	TypeFieldMethodAST.C \
	VarExprAST.C \
	ActionDeclAST.C \
	EnumDeclAST.C \
	FuncDeclAST.C \
	InPortDeclAST.C \
	MachineAST.C \
	ObjDeclAST.C \
	OutPortDeclAST.C \
	TransitionDeclAST.C \
	TypeDeclAST.C \
	TypeFieldAST.C \
	ChipComponentAccessAST.C \

all:
	@echo "SLICC: Running flex and bison"
	$(MAKE) $(GENERATED_DIR)/generated
	@echo "SLICC: Building slicc executable"
	$(MAKE) $(BIN_DIR)/slicc.exec

$(GENERATED_DIR)/generated: $(GENERATED_DIR)/created $(DESC) parser/parser.y parser/lexer.l
	@echo "SLICC: Generating parser"
	$(YACC) $(YFLAGS) -o $(GENERATED_DIR)/parser.c parser/parser.y
	@echo "  If you see bison complains, check generated/parser.output"
	$(LEX) $(LFLAGS) -o$(GENERATED_DIR)/lexer.c parser/lexer.l
	touch $(GENERATED_DIR)/generated

$(BIN_DIR)/slicc.exec: $(BIN_DIR)/created $(OBJ_DIR)/created $(OBJ)
#	uncomment this for a static linked (huge) executable
#	$(CC) -static -o $@ $(LDFLAGS) $(OBJ)
#	this is default linking
	$(CC) -o $@ $(OPT_FLAGS) $(LDFLAGS) $(OBJ)
	@echo "SLICC: my work is done here..."

clean:
	rm -rf $(HOST_TYPE)
	rm -rf generated
	rm -f core *~ .pure
	rm -f */core */*~ */.pure

include $(GEMS_ROOT)/common/Makefile.dep
