#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdlib.h>
#include <string.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState);

/**
 * Bison semantic actions.
 * Each function constructs the corresponding AST node.
 */

/* ── Program ─────────────────────────────────────────────────── */
Program * ProgramSemanticAction(PatternDef * patterns);

/* ── Pattern definition list ─────────────────────────────────── */
PatternDef * PatternDefSemanticAction(PatternInfo * info, PatternBody * body);
PatternDef * PatternDefListSemanticAction(PatternDef * head, PatternDef * rest);

/* ── patternInfo block ───────────────────────────────────────── */
PatternInfo  * PatternInfoSemanticAction(char * name, InfoPropList * props);
InfoPropList * InfoPropListSingleSemanticAction(InfoProp * prop);
InfoPropList * InfoPropListSemanticAction(InfoProp * prop, InfoPropList * next);
InfoProp     * ShapePropSemanticAction(ShapeType shape);
InfoProp     * MaxWidthPropSemanticAction(int value);
InfoProp     * MaxRingSizePropSemanticAction(int value);
InfoProp     * RowsPropSemanticAction(int value);

/* ── pattern body block ──────────────────────────────────────── */
PatternBody * PatternBodySemanticAction(char * name, RowList * rows);

/* ── Row list ────────────────────────────────────────────────── */
RowList * RowListSingleSemanticAction(RowDecl * row);
RowList * RowListSemanticAction(RowDecl * row, RowList * next);
RowDecl * RowDeclSemanticAction(RowRange * range, StitchList * stitches);
RowRange * SingleRowRangeSemanticAction(int n);
RowRange * RangeRowRangeSemanticAction(int from, int to);

/* ── Stitch list ─────────────────────────────────────────────── */
StitchList * StitchListSingleSemanticAction(StitchItem * item);
StitchList * StitchListSemanticAction(StitchItem * item, StitchList * next);

/* ── Stitch items ────────────────────────────────────────────── */
StitchItem * SimpleStitchSemanticAction(StitchType type, ModifierType mod, int count);
StitchItem * SimpleStitchNoCountSemanticAction(StitchType type, ModifierType mod);
StitchItem * RepeatBlockSemanticAction(StitchList * list, int times);

#endif
