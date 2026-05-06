#ifndef FLEX_ACTIONS_HEADER
#define FLEX_ACTIONS_HEADER

#include "../../support/configuration/Environment.h"
#include "../../support/language/String.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/LexicalAnalyzer.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/Token.h"
#include "../../support/type/TokenLabel.h"
#include "../Frontend.h"

/** Initialize module's internal state. */
ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer);

/**
 * All lexeme actions. Each one creates the appropriate token,
 * sets its semantic value if needed, pushes it to the parser,
 * and returns the resulting CompilationStatus.
 */

/* ── Shared / infrastructure ─────────────────────────────── */
CompilationStatus EOFLexemeAction();
CompilationStatus IgnoredLexemeAction();
CompilationStatus UnknownLexemeAction();

/* ── Comments ────────────────────────────────────────────── */
CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
CompilationStatus LeaveMultilineCommentLexemeAction();

/* ── Keywords ────────────────────────────────────────────── */
CompilationStatus KeywordLexemeAction(TokenLabel label);

/* ── Shape types ─────────────────────────────────────────── */
CompilationStatus ShapeTypeLexemeAction(TokenLabel label);

/* ── Stitch types ────────────────────────────────────────── */
CompilationStatus StitchLexemeAction(TokenLabel label);

/* ── Modifiers ───────────────────────────────────────────── */
CompilationStatus ModifierLexemeAction(TokenLabel label);

/* ── Punctuation & operators ─────────────────────────────── */
CompilationStatus PunctuationLexemeAction(TokenLabel label);

/* ── Literals ────────────────────────────────────────────── */
CompilationStatus IntegerLexemeAction();
CompilationStatus IdentifierLexemeAction();

#endif
