#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeAbstractSyntaxTreeModule();

/**
 * ============================================================
 *  AST node types for the Crochet DSL
 * ============================================================
 *
 * Self-referencing typedef declarations (required in C before
 * the struct bodies are defined).
 */

typedef enum ShapeType       ShapeType;
typedef enum StitchType      StitchType;
typedef enum ModifierType    ModifierType;
typedef enum InfoPropKind    InfoPropKind;
typedef enum StitchItemKind  StitchItemKind;

typedef struct InfoProp      InfoProp;
typedef struct InfoPropList  InfoPropList;
typedef struct PatternInfo   PatternInfo;
typedef struct PatternBody   PatternBody;
typedef struct PatternDef    PatternDef;
typedef struct Program       Program;
typedef struct RowRange      RowRange;
typedef struct StitchItem    StitchItem;
typedef struct StitchList    StitchList;
typedef struct RowDecl       RowDecl;
typedef struct RowList       RowList;

/* ─── Enumerations ──────────────────────────────────────────── */

enum ShapeType {
	SHAPE_LINEAR,
	SHAPE_ROUND
};

enum StitchType {
	ST_CH,
	ST_SC,
	ST_HDC,
	ST_DC,
	ST_SLST,
	ST_MR
};

enum ModifierType {
	MT_NONE,
	MT_INC,
	MT_DEC,
	MT_BLO,
	MT_FLO
};

enum InfoPropKind {
	INFO_SHAPE,
	INFO_MAX_WIDTH,
	INFO_MAX_RING_SIZE,
	INFO_ROWS
};

enum StitchItemKind {
	STITCH_ITEM_SIMPLE,
	STITCH_ITEM_REPEAT
};

/* ─── AST node structs ──────────────────────────────────────── */


struct InfoProp {
	InfoPropKind kind;
	union {
		ShapeType shapeValue;   
		int       intValue;     
	};
};


struct InfoPropList {
	InfoProp    * prop;
	InfoPropList * next;
};

struct PatternInfo {
	char         * name;
	InfoPropList * props;
};


struct RowRange {
	int from;
	int to;
};

struct StitchItem {
	StitchItemKind kind;
	union {
		struct {
			StitchType   type;
			ModifierType modifier;
			int          count;   
		} simple;
		struct {
			StitchList * list;
			int          times;
		} repeat;
	};
};


struct StitchList {
	StitchItem * item;
	StitchList * next;
};


struct RowDecl {
	RowRange   * range;
	StitchList * stitches;
};


struct RowList {
	RowDecl * row;
	RowList * next;
};

struct PatternBody {
	char    * name;
	RowList * rows;
};


struct PatternDef {
	PatternInfo * info;
	PatternBody * body;
	PatternDef  * next;  /* sibling in the program's list */
};


struct Program {
	PatternDef * patterns;
};

/* ─── Destructors ───────────────────────────────────────────── */

void destroyProgram(Program * program);
void destroyPatternDef(PatternDef * patternDef);
void destroyPatternInfo(PatternInfo * patternInfo);
void destroyInfoPropList(InfoPropList * list);
void destroyInfoProp(InfoProp * prop);
void destroyPatternBody(PatternBody * patternBody);
void destroyRowList(RowList * rowList);
void destroyRowDecl(RowDecl * rowDecl);
void destroyRowRange(RowRange * rowRange);
void destroyStitchList(StitchList * stitchList);
void destroyStitchItem(StitchItem * stitchItem);

#endif
