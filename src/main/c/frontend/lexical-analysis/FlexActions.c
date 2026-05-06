#include "FlexActions.h"

/* MODULE INTERNAL STATE */

static bool _logIgnoredLexemes = true;
static LexicalAnalyzer * _lexicalAnalyzer = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownFlexActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: FlexActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_lexicalAnalyzer = NULL;
}

ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer) {
	_lexicalAnalyzer = lexicalAnalyzer;
	_logger = createLogger("FlexActions");
	_logIgnoredLexemes = getBooleanOrDefault("LOG_IGNORED_LEXEMES", _logIgnoredLexemes);
	return _shutdownFlexActionsModule;
}

/* PRIVATE FUNCTIONS */

static void _logTokenAction(const char * actionName, Token * token) {
	char * _lexeme = escape(token->lexeme);
	logDebugging(_logger, WARNING_COLOR "%s" DEFAULT_COLOR
		": Token(context=%d, label=%d, length=%d, lexeme=%s\"%s\"%s, line=%d, semanticValue=%p)",
		actionName,
		token->context,
		token->label,
		token->length,
		INFORMATION_COLOR, _lexeme, DEFAULT_COLOR,
		token->line,
		token->semanticValue);
	free(_lexeme);
}

/**
 * Generic helper: create a token with the given label, push it,
 * and destroy it. The semantic value is left as zero-initialised
 * (appropriate for pure TokenLabel terminals).
 */
static CompilationStatus _simplePushToken(const char * actionName, TokenLabel label) {
	Token * token = createToken(_lexicalAnalyzer, label);
	token->semanticValue->token = label;
	_logTokenAction(actionName, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

/* PUBLIC FUNCTIONS */

/* ── Shared / infrastructure ──────────────────────────────────── */

CompilationStatus EOFLexemeAction() {
	CompilationStatus status = IN_PROGRESS;
	Token * token = createToken(_lexicalAnalyzer, 0);
	_logTokenAction(__FUNCTION__, token);
	if (!popInputBuffer(_lexicalAnalyzer)) {
		status = pushToken(_lexicalAnalyzer, token);
		FlexContext context = currentLexicalAnalyzerContext(_lexicalAnalyzer);
		if (0 < context) {
			logError(_logger, "The final context is not closed (context=%d).", context);
			status = FAILED;
		}
	}
	destroyToken(token);
	return status;
}

CompilationStatus IgnoredLexemeAction() {
	if (_logIgnoredLexemes) {
		Token * token = createToken(_lexicalAnalyzer, IGNORED);
		_logTokenAction(__FUNCTION__, token);
		destroyToken(token);
	}
	return IN_PROGRESS;
}

CompilationStatus UnknownLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	_logTokenAction(__FUNCTION__, token);
	destroyToken(token);
	return FAILED;
}

/* ── Comments ─────────────────────────────────────────────────── */

CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context) {
	if (_logIgnoredLexemes) {
		Token * token = createToken(_lexicalAnalyzer, OPEN_COMMENT);
		_logTokenAction(__FUNCTION__, token);
		destroyToken(token);
	}
	enterLexicalAnalyzerContext(_lexicalAnalyzer, context);
	return IN_PROGRESS;
}

CompilationStatus LeaveMultilineCommentLexemeAction() {
	leaveLexicalAnalyzerContext(_lexicalAnalyzer);
	if (_logIgnoredLexemes) {
		Token * token = createToken(_lexicalAnalyzer, CLOSE_COMMENT);
		_logTokenAction(__FUNCTION__, token);
		destroyToken(token);
	}
	return IN_PROGRESS;
}

/* ── Keywords ─────────────────────────────────────────────────── */

CompilationStatus KeywordLexemeAction(TokenLabel label) {
	return _simplePushToken(__FUNCTION__, label);
}

/* ── Shape types ──────────────────────────────────────────────── */

CompilationStatus ShapeTypeLexemeAction(TokenLabel label) {
	return _simplePushToken(__FUNCTION__, label);
}

/* ── Stitch types ─────────────────────────────────────────────── */

CompilationStatus StitchLexemeAction(TokenLabel label) {
	return _simplePushToken(__FUNCTION__, label);
}

/* ── Modifiers ────────────────────────────────────────────────── */

CompilationStatus ModifierLexemeAction(TokenLabel label) {
	return _simplePushToken(__FUNCTION__, label);
}

/* ── Punctuation & operators ──────────────────────────────────── */

CompilationStatus PunctuationLexemeAction(TokenLabel label) {
	return _simplePushToken(__FUNCTION__, label);
}

/* ── Integer literals ─────────────────────────────────────────── */

CompilationStatus IntegerLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, INTEGER);
	token->semanticValue->integer = atoi(token->lexeme);
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

/* ── Identifiers (pattern names) ──────────────────────────────── */

CompilationStatus IdentifierLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, IDENTIFIER);
	token->semanticValue->string = strdup(token->lexeme);
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}
