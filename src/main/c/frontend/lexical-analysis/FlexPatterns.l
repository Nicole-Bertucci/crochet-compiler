%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * Declarations of all lexeme action functions defined in FlexActions.c.
 *
 * @see https://westes.github.io/flex/manual/Definitions-Section.html
 */

//Shared / infrastructure
extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus UnknownLexemeAction();

//Comments
extern CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
extern CompilationStatus LeaveMultilineCommentLexemeAction();

// Keywords
extern CompilationStatus KeywordLexemeAction(TokenLabel label);

// Shape Types
extern CompilationStatus ShapeTypeLexemeAction(TokenLabel label);

//Stitch types
extern CompilationStatus StitchLexemeAction(TokenLabel label);

// Modifiers
extern CompilationStatus ModifierLexemeAction(TokenLabel label);

//Punctuation adn operation
extern CompilationStatus PunctuationLexemeAction(TokenLabel label);

extern CompilationStatus IntegerLexemeAction();
extern CompilationStatus IdentifierLexemeAction();

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x MULTILINE_COMMENT

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace		[[:space:]]+
digit			[[:digit:]]
letter			[[:alpha:]]
identifier		{letter}([[:alnum:]]|_)*

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html
 */

%}


"/*"								{ return EnterMultilineCommentLexemeAction(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"				{ return LeaveMultilineCommentLexemeAction(); }
<MULTILINE_COMMENT>[^*]+			{ return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.				{ return IgnoredLexemeAction(); }

{whitespace}						{ return IgnoredLexemeAction(); }


"patternInfo"						{ return KeywordLexemeAction(PATTERN_INFO); }
"pattern"							{ return KeywordLexemeAction(PATTERN); }
"row"								{ return KeywordLexemeAction(ROW); }
"Shape"								{ return KeywordLexemeAction(SHAPE); }
"Max_width"							{ return KeywordLexemeAction(MAX_WIDTH); }
"Max_ring_size"						{ return KeywordLexemeAction(MAX_RING_SIZE); }
"Rows"								{ return KeywordLexemeAction(ROWS); }


"linear"							{ return ShapeTypeLexemeAction(LINEAR); }
"round"								{ return ShapeTypeLexemeAction(ROUND); }


"-inc"								{ return ModifierLexemeAction(MOD_INC); }
"-dec"								{ return ModifierLexemeAction(MOD_DEC); }
"-blo"								{ return ModifierLexemeAction(MOD_BLO); }
"-flo"								{ return ModifierLexemeAction(MOD_FLO); }
 
"slst"								{ return StitchLexemeAction(SLST); }
"hdc"								{ return StitchLexemeAction(HDC); }
"ch"								{ return StitchLexemeAction(CH); }
"sc"								{ return StitchLexemeAction(SC); }
"dc"								{ return StitchLexemeAction(DC); }
"mr"								{ return StitchLexemeAction(MR); }


"{"									{ return PunctuationLexemeAction(OPEN_BRACE); }
"}"									{ return PunctuationLexemeAction(CLOSE_BRACE); }
"["									{ return PunctuationLexemeAction(OPEN_BRACKET); }
"]"									{ return PunctuationLexemeAction(CLOSE_BRACKET); }
","									{ return PunctuationLexemeAction(COMMA); }
":"									{ return PunctuationLexemeAction(COLON); }
";"									{ return PunctuationLexemeAction(SEMICOLON); }
"x"									{ return PunctuationLexemeAction(REPEAT_OP); }
"-"									{ return PunctuationLexemeAction(DASH); }

{digit}+							{ return IntegerLexemeAction(); }


{identifier}						{ return IdentifierLexemeAction(); }


.									{ return UnknownLexemeAction(); }


<<EOF>>								{ return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html
 */

#include "FlexActions.h"
#include "FlexExport.h"