#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"

/**
 * Initializes the semantic analyzer module.
 * Must be called before executeSemanticAnalysis().
 */
void initializeSemanticAnalyzerModule();

/**
 * Shuts down the semantic analyzer module, freeing internal resources.
 */
void shutdownSemanticAnalyzerModule();

/**
 * Performs the full semantic analysis pass over the AST rooted at `program`.
 * Returns COMPILATION_SUCCESS if all validations pass, FAILED otherwise.
 * All errors are logged with logError().
 */
CompilationStatus executeSemanticAnalysis(Program * program);

#endif 