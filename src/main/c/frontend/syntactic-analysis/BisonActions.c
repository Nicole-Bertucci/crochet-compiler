#include "BisonActions.h"

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	return _shutdownBisonActionsModule;
}

/* PRIVATE FUNCTIONS */

static void _logSyntacticAnalyzerAction(const char * functionName) {
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

/* ── Program ────────────────────────────────────────────────── */

Program * ProgramSemanticAction(PatternDef * patterns) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->patterns = patterns;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}

/* ── Pattern definition list ────────────────────────────────── */

PatternDef * PatternDefSemanticAction(PatternInfo * info, PatternBody * body) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	PatternDef * pd = calloc(1, sizeof(PatternDef));
	pd->info = info;
	pd->body = body;
	pd->next = NULL;
	return pd;
}

PatternDef * PatternDefListSemanticAction(PatternDef * head, PatternDef * rest) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	head->next = rest;
	return head;
}

/* ── patternInfo block ──────────────────────────────────────── */

PatternInfo * PatternInfoSemanticAction(char * name, InfoPropList * props) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	PatternInfo * pi = calloc(1, sizeof(PatternInfo));
	pi->name  = name;   /* already strdup'd in FlexActions */
	pi->props = props;
	return pi;
}

InfoPropList * InfoPropListSingleSemanticAction(InfoProp * prop) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	InfoPropList * ipl = calloc(1, sizeof(InfoPropList));
	ipl->prop = prop;
	ipl->next = NULL;
	return ipl;
}

InfoPropList * InfoPropListSemanticAction(InfoProp * prop, InfoPropList * next) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	InfoPropList * ipl = calloc(1, sizeof(InfoPropList));
	ipl->prop = prop;
	ipl->next = next;
	return ipl;
}

InfoProp * ShapePropSemanticAction(ShapeType shape) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	InfoProp * ip = calloc(1, sizeof(InfoProp));
	ip->kind       = INFO_SHAPE;
	ip->shapeValue = shape;
	return ip;
}

InfoProp * MaxWidthPropSemanticAction(int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	InfoProp * ip = calloc(1, sizeof(InfoProp));
	ip->kind     = INFO_MAX_WIDTH;
	ip->intValue = value;
	return ip;
}

InfoProp * MaxRingSizePropSemanticAction(int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	InfoProp * ip = calloc(1, sizeof(InfoProp));
	ip->kind     = INFO_MAX_RING_SIZE;
	ip->intValue = value;
	return ip;
}

InfoProp * RowsPropSemanticAction(int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	InfoProp * ip = calloc(1, sizeof(InfoProp));
	ip->kind     = INFO_ROWS;
	ip->intValue = value;
	return ip;
}

/* ── pattern body block ─────────────────────────────────────── */

PatternBody * PatternBodySemanticAction(char * name, RowList * rows) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	PatternBody * pb = calloc(1, sizeof(PatternBody));
	pb->name = name;
	pb->rows = rows;
	return pb;
}

/* ── Row list ───────────────────────────────────────────────── */

RowList * RowListSingleSemanticAction(RowDecl * row) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	RowList * rl = calloc(1, sizeof(RowList));
	rl->row  = row;
	rl->next = NULL;
	return rl;
}

RowList * RowListSemanticAction(RowDecl * row, RowList * next) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	RowList * rl = calloc(1, sizeof(RowList));
	rl->row  = row;
	rl->next = next;
	return rl;
}

RowDecl * RowDeclSemanticAction(RowRange * range, StitchList * stitches) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	RowDecl * rd = calloc(1, sizeof(RowDecl));
	rd->range    = range;
	rd->stitches = stitches;
	return rd;
}

RowRange * SingleRowRangeSemanticAction(int n) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	RowRange * rr = calloc(1, sizeof(RowRange));
	rr->from = n;
	rr->to   = n;
	return rr;
}

RowRange * RangeRowRangeSemanticAction(int from, int to) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	RowRange * rr = calloc(1, sizeof(RowRange));
	rr->from = from;
	rr->to   = to;
	return rr;
}

/* ── Stitch list ────────────────────────────────────────────── */

StitchList * StitchListSingleSemanticAction(StitchItem * item) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StitchList * sl = calloc(1, sizeof(StitchList));
	sl->item = item;
	sl->next = NULL;
	return sl;
}

StitchList * StitchListSemanticAction(StitchItem * item, StitchList * next) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StitchList * sl = calloc(1, sizeof(StitchList));
	sl->item = item;
	sl->next = next;
	return sl;
}

/* ── Stitch items ───────────────────────────────────────────── */

StitchItem * SimpleStitchSemanticAction(StitchType type, ModifierType mod, int count) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StitchItem * si = calloc(1, sizeof(StitchItem));
	si->kind            = STITCH_ITEM_SIMPLE;
	si->simple.type     = type;
	si->simple.modifier = mod;
	si->simple.count    = count;
	return si;
}

StitchItem * SimpleStitchNoCountSemanticAction(StitchType type, ModifierType mod) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StitchItem * si = calloc(1, sizeof(StitchItem));
	si->kind            = STITCH_ITEM_SIMPLE;
	si->simple.type     = type;
	si->simple.modifier = mod;
	si->simple.count    = 1;
	return si;
}

StitchItem * RepeatBlockSemanticAction(StitchList * list, int times) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StitchItem * si = calloc(1, sizeof(StitchItem));
	si->kind         = STITCH_ITEM_REPEAT;
	si->repeat.list  = list;
	si->repeat.times = times;
	return si;
}
