#ifndef GENERATOR_H
#define GENERATOR_H

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"

/**
 * Initializes the generator module.
 */
ModuleDestructor initializeGeneratorModule();

/**
 * Generates SVG output for all patterns in the AST.
 * Writes the SVG text to stdout.
 */
void executeGenerator(CompilerState * compilerState);

#endif