#include "codegen.h"

char *BOILERPLATE =
"#include <stdlib.h>\n"
"#include <stdio.h>\n"
"#include <stdbool.h>\n"
"\n"
"typedef char *str;" CC_NEWLINE
"typedef unsigned long long u64;" CC_NEWLINE
"typedef unsigned int u32;" CC_NEWLINE
"typedef unsigned short u16;" CC_NEWLINE
"typedef unsigned char u8;" CC_NEWLINE
"typedef long long s64;" CC_NEWLINE
"typedef int s32;" CC_NEWLINE
"typedef short s16;" CC_NEWLINE
"typedef char s8;" CC_NEWLINE
"typedef float f32;" CC_NEWLINE
"typedef double f64;" CC_NEWLINE CC_NEWLINE
;

const char *NODE_NAME[] = {
	"IDENTIFIER_LIST_NODE", "IDENTIFIER_NODE", "LITERAL_NODE", "BINARY_EXPR_NODE",
	"UNARY_EXPR_NODE", "ARRAY_SUB_EXPR_NODE", "MEMBER_ACCESS_NODE",
	"PRIMARY_EXPR_NODE", "EXPR_NODE", "TYPE_NAME_NODE", "TYPE_LIT_NODE", "PAREN_EXPR_NODE",
	"ARRAY_TYPE_NODE", "POINTER_TYPE_NODE", "FIELD_DECL_NODE",
	"FIELD_DECL_LIST_NODE", "STRUCT_DECL_NODE", "STATEMENT_LIST_NODE",
	"BLOCK_NODE", "PARAMETER_SECTION_NODE", "PARAMETERS_NODE", "RECEIVER_NODE",
	"FUNCTION_SIGNATURE_NODE", "FUNCTION_DECL_NODE", "VARIABLE_DECL_NODE", "FUNCTION_CALL_NODE",
	"DECLARATION_NODE", "INC_DEC_STAT_NODE", "RETURN_STAT_NODE", "BREAK_STAT_NODE",
	"CONTINUE_STAT_NODE", "LEAVE_STAT_NODE", "ASSIGNMENT_NODE", "UNSTRUCTURED_STATEMENT_NODE",
	"ELSE_STAT_NODE", "IF_STAT_NODE", "MATCH_CLAUSE_STAT", "MATCH_STAT_NODE", "FOR_STAT_NODE",
	"STRUCTURED_STATEMENT_NODE", "STATEMENT_NODE", "TYPE_NODE"
};

CodeGenerator *createCodeGenerator(Vector *sourceFiles) {
	CodeGenerator *self = safeMalloc(sizeof(*self));
	self->abstractSyntaxTree = NULL;
	self->currentNode = 0;
	self->sourceFiles = sourceFiles;
	self->symtable = hashmap_new();
	return self;
}

void emitCode(CodeGenerator *self, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	switch (self->writeState) {
		case WRITE_SOURCE_STATE:
			vfprintf(self->currentSourceFile->outputFile, fmt, args);
			va_end(args);
			break;
		case WRITE_HEADER_STATE:
			vfprintf(self->currentSourceFile->headerFile->outputFile, fmt, args);
			va_end(args);
			break;
	}
}

void consumeAstNode(CodeGenerator *self) {
	self->currentNode += 1;
}

void consumeAstNodeBy(CodeGenerator *self, int amount) {
	self->currentNode += amount;
}

void emitType(CodeGenerator *self, Type *type) {
	switch (type->type) {
		case TYPE_NAME_NODE: 
			emitCode(self, "%s", type->typeName->name);
			break;
	}
}

void emitParameters(CodeGenerator *self, Parameters *params) {
	for (int i = 0; i < params->paramList->size; i++) {
		ParameterSection *param = getVectorItem(params->paramList, i);
		if (!param->mutable) {
			emitCode(self, "const ");
		}
		emitType(self, param->type);
		emitCode(self, "%s", param->name);

		if (params->paramList->size > 1 && i != params->paramList->size - 1) {
			emitCode(self, ", "); // cleaner formatting
		}
	}
}

void emitFieldList(CodeGenerator *self, FieldDeclList *list) {
	const int size = list->members->size;

	for (int i = 0; i < size; i++) {
		FieldDecl *decl = getVectorItem(list->members, i);
		emitCode(self, "\t");
		if (!decl->mutable) {
			emitCode(self, "const ");
		}
		emitType(self, decl->type);
		emitCode(self, " %s;" CC_NEWLINE, decl->name);
	}
}

void emitStructDecl(CodeGenerator *self, StructDecl *decl) {
	self->writeState = WRITE_HEADER_STATE;
	emitCode(self, "typedef struct {" CC_NEWLINE);
	emitFieldList(self, decl->fields);
	emitCode(self, "} %s;" CC_NEWLINE, decl->name);
}

void emitFunctionDecl(CodeGenerator *self, FunctionDecl *decl) {
	// prototype in header
	self->writeState = WRITE_HEADER_STATE;
	emitType(self, decl->signature->type);
	emitCode(self, " %s(", decl->signature->name);
	emitParameters(self, decl->signature->parameters);
	emitCode(self, ");");

	// definition
	self->writeState = WRITE_SOURCE_STATE;
	emitType(self, decl->signature->type);
	emitCode(self, " %s(", decl->signature->name);
	emitParameters(self, decl->signature->parameters);
	emitCode(self, ") {" CC_NEWLINE);
	// TODO: block
	emitCode(self, "}" CC_NEWLINE);
}

void emitVariableDecl(CodeGenerator *self, VariableDecl *decl) {

}

void emitDeclaration(CodeGenerator *self, Declaration *decl) {
	switch (decl->type) {
		case FUNCTION_DECL_NODE: emitFunctionDecl(self, decl->funcDecl); break;
		case STRUCT_DECL_NODE: emitStructDecl(self, decl->structDecl); break;
		case VARIABLE_DECL_NODE: emitVariableDecl(self, decl->varDecl); break;
		default:
			printf("unknown node %s\n", NODE_NAME[decl->type]);
			break;
	}
}

void emitUnstructuredStat(CodeGenerator *self, UnstructuredStatement *stmt) {
	switch (stmt->type) {
		case DECLARATION_NODE: emitDeclaration(self, stmt->decl); break;
	}
}

void emitStructuredStat(CodeGenerator *self, StructuredStatement *stmt) {
	switch (stmt->type) {

	}
}

void traverseAST(CodeGenerator *self) {
	int i;
	for (i = 0; i < self->abstractSyntaxTree->size; i++) {
		Statement *stmt = getVectorItem(self->abstractSyntaxTree, i);

		switch (stmt->type) {
			case UNSTRUCTURED_STATEMENT_NODE: 
				emitUnstructuredStat(self, stmt->unstructured);
				break;
			case STRUCTURED_STATEMENT_NODE: 

				break;
			default:
				printf("oh shit\n");
				break;
		}
	}
}

void startCodeGeneration(CodeGenerator *self) {
	int i;
	for (i = 0; i < self->sourceFiles->size; i++) {
		SourceFile *sf = getVectorItem(self->sourceFiles, i);
		self->currentNode = 0;
		self->currentSourceFile = sf;
		self->abstractSyntaxTree = self->currentSourceFile->ast;
		
		writeFiles(self->currentSourceFile);

		self->writeState = WRITE_SOURCE_STATE;
		// _gen_name.h is the typical name for the headers and c files that are generated
		emitCode(self, "#include \"_gen_%s.h\"\n", self->currentSourceFile->name);

		// write to header
		self->writeState = WRITE_HEADER_STATE;
		emitCode(self, "#ifndef __%s_H\n", self->currentSourceFile->name);
		emitCode(self, "#define __%s_H\n\n", self->currentSourceFile->name);

		emitCode(self, BOILERPLATE);

		// compile code
		traverseAST(self);

		// write to header
		self->writeState = WRITE_HEADER_STATE;
		emitCode(self, "\n");
		emitCode(self, "#endif // __%s_H\n", self->currentSourceFile->name);

		// close files
		closeFiles(self->currentSourceFile);
	}

	sds buildCommand = sdsempty();

	// append the compiler to use etc
	buildCommand = sdscat(buildCommand, COMPILER);
	buildCommand = sdscat(buildCommand, " ");
	buildCommand = sdscat(buildCommand, ADDITIONAL_COMPILER_ARGS);
	buildCommand = sdscat(buildCommand, " -o ");
	buildCommand = sdscat(buildCommand, OUTPUT_EXECUTABLE_NAME);
	buildCommand = sdscat(buildCommand, " ");

	// append the filename to the build string
	for (i = 0; i < self->sourceFiles->size; i++) {
		SourceFile *sourceFile = getVectorItem(self->sourceFiles, i);
		buildCommand = sdscat(buildCommand, sourceFile->generatedSourceName);

		if (i != self->sourceFiles->size - 1) // stop whitespace at the end!
			buildCommand = sdscat(buildCommand, " ");
	}

	// just for debug purposes
	verboseModeMessage("running cl args: `%s`", buildCommand);
	system(buildCommand);
	sdsfree(buildCommand); // deallocate dat shit baby
}

void destroyCodeGenerator(CodeGenerator *self) {
	int i;
	for (i = 0; i < self->sourceFiles->size; i++) {
		SourceFile *sourceFile = getVectorItem(self->sourceFiles, i);
		destroySourceFile(sourceFile);
		verboseModeMessage("Destroyed source files on %d iteration.", i);
	}

	hashmap_free(self->symtable);
	free(self);
	verboseModeMessage("Destroyed compiler");
}
