%{
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * The error reporting function for Bison.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 */
void yyerror(const YYLTYPE * location, const char * message) {}
%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */
	signed int   integer;
	TokenLabel   token;
	char       * string;    /* for IDENTIFIER lexemes */

	/** Non-terminals. */
	InfoProp     * infoProp;
	InfoPropList * infoPropList;
	ModifierType   modifierType;
	PatternBody  * patternBody;
	PatternDef   * patternDef;
	PatternInfo  * patternInfo;
	Program      * program;
	RowDecl      * rowDecl;
	RowList      * rowList;
	RowRange     * rowRange;
	ShapeType      shapeType;
	StitchItem   * stitchItem;
	StitchList   * stitchList;
	StitchType     stitchType;
}

/**
 * Destructors — run automatically on symbols discarded during error recovery.
 * We do NOT add a destructor for <program> so the root AST survives after a
 * successful parse (same pattern as the base calculator example).
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { destroyInfoProp($$);      } <infoProp>
%destructor { destroyInfoPropList($$);  } <infoPropList>
%destructor { destroyPatternBody($$);   } <patternBody>
%destructor { destroyPatternDef($$);    } <patternDef>
%destructor { destroyPatternInfo($$);   } <patternInfo>
%destructor { destroyRowDecl($$);       } <rowDecl>
%destructor { destroyRowList($$);       } <rowList>
%destructor { destroyRowRange($$);      } <rowRange>
%destructor { destroyStitchItem($$);    } <stitchItem>
%destructor { destroyStitchList($$);    } <stitchList>
%destructor { free($$);                 } <string>

/** ── Terminals ─────────────────────────────────────────────── */

/* Literals */
%token <integer>  INTEGER
%token <string>   IDENTIFIER

/* Keywords */
%token <token>    PATTERN_INFO
%token <token>    PATTERN
%token <token>    ROW
%token <token>    SHAPE
%token <token>    MAX_WIDTH
%token <token>    MAX_RING_SIZE
%token <token>    ROWS

/* shape values */
%token <token>    LINEAR
%token <token>    ROUND

/* Stitch types */
%token <token>    CH
%token <token>    SC
%token <token>    HDC
%token <token>    DC
%token <token>    SLST
%token <token>    MR

/* Modifiers */
%token <token>    MOD_INC
%token <token>    MOD_DEC
%token <token>    MOD_BLO
%token <token>    MOD_FLO

/* Punctuation */
%token <token>    OPEN_BRACE
%token <token>    CLOSE_BRACE
%token <token>    OPEN_BRACKET
%token <token>    CLOSE_BRACKET
%token <token>    COMMA
%token <token>    COLON
%token <token>    SEMICOLON
%token <token>    REPEAT_OP
%token <token>    DASH

/* Infrastructure */
%token <token>    IGNORED
%token <token>    OPEN_COMMENT
%token <token>    CLOSE_COMMENT
%token <token>    UNKNOWN

/** ── Non-terminals ─────────────────────────────────────────── */
%type <program>       program
%type <patternDef>    patternDefList
%type <patternDef>    patternDef
%type <patternInfo>   patternInfoBlock
%type <patternBody>   patternBodyBlock
%type <infoPropList>  infoPropList
%type <infoProp>      infoProp
%type <shapeType>     shapeType
%type <rowList>       rowList
%type <rowDecl>       rowDecl
%type <rowRange>      rowRange
%type <stitchList>    stitchList
%type <stitchItem>    stitchItem
%type <stitchType>    stitchBase
%type <modifierType>  optionalModifier

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.

/* ── Root ──────────────────────────────────────────────────────── */

program:
	  patternDefList						{ $$ = ProgramSemanticAction($1); }
	;

/* ── Pattern definition list ───────────────────────────────────── */

patternDefList:
	  patternDef							{ $$ = $1; }
	| patternDef patternDefList				{ $$ = PatternDefListSemanticAction($1, $2); }
	;

patternDef:
	  patternInfoBlock patternBodyBlock		{ $$ = PatternDefSemanticAction($1, $2); }
	;

/* ── patternInfo block ─────────────────────────────────────────── */

patternInfoBlock:
	  PATTERN_INFO IDENTIFIER OPEN_BRACE infoPropList CLOSE_BRACE
	  										{ $$ = PatternInfoSemanticAction($2, $4); }
	;

infoPropList:
	  infoProp								{ $$ = InfoPropListSingleSemanticAction($1); }
	| infoProp infoPropList					{ $$ = InfoPropListSemanticAction($1, $2); }
	;

infoProp:
	  SHAPE COLON shapeType SEMICOLON		{ $$ = ShapePropSemanticAction($3); }
	| MAX_WIDTH COLON INTEGER SEMICOLON		{ $$ = MaxWidthPropSemanticAction($3); }
	| MAX_RING_SIZE COLON INTEGER SEMICOLON	{ $$ = MaxRingSizePropSemanticAction($3); }
	| ROWS COLON INTEGER SEMICOLON			{ $$ = RowsPropSemanticAction($3); }
	;

shapeType:
	  LINEAR								{ $$ = SHAPE_LINEAR; }
	| ROUND									{ $$ = SHAPE_ROUND; }
	;

/* ── pattern body block ────────────────────────────────────────── */

patternBodyBlock:
	  PATTERN IDENTIFIER OPEN_BRACE rowList CLOSE_BRACE
	  										{ $$ = PatternBodySemanticAction($2, $4); }
	;

/* ── Row list ──────────────────────────────────────────────────── */

rowList:
	  rowDecl								{ $$ = RowListSingleSemanticAction($1); }
	| rowDecl rowList						{ $$ = RowListSemanticAction($1, $2); }
	;

rowDecl:
	  ROW rowRange COLON stitchList			{ $$ = RowDeclSemanticAction($2, $4); }
	;

rowRange:
	  INTEGER								{ $$ = SingleRowRangeSemanticAction($1); }
	| INTEGER DASH INTEGER					{ $$ = RangeRowRangeSemanticAction($1, $3); }
	;

/* ── Stitch list ───────────────────────────────────────────────── */

stitchList:
	  stitchItem							{ $$ = StitchListSingleSemanticAction($1); }
	| stitchItem COMMA stitchList			{ $$ = StitchListSemanticAction($1, $3); }
	;

stitchItem:
	  stitchBase optionalModifier INTEGER
	  										{ $$ = SimpleStitchSemanticAction($1, $2, $3); }
	| stitchBase optionalModifier
	  										{ $$ = SimpleStitchNoCountSemanticAction($1, $2); }
	| OPEN_BRACKET stitchList CLOSE_BRACKET REPEAT_OP INTEGER
	  										{ $$ = RepeatBlockSemanticAction($2, $5); }
	;

optionalModifier:
	  %empty								{ $$ = MT_NONE; }
	| MOD_INC								{ $$ = MT_INC; }
	| MOD_DEC								{ $$ = MT_DEC; }
	| MOD_BLO								{ $$ = MT_BLO; }
	| MOD_FLO								{ $$ = MT_FLO; }
	;

stitchBase:
	  CH									{ $$ = ST_CH; }
	| SC									{ $$ = ST_SC; }
	| HDC									{ $$ = ST_HDC; }
	| DC									{ $$ = ST_DC; }
	| SLST									{ $$ = ST_SLST; }
	| MR									{ $$ = ST_MR; }
	;

%%
