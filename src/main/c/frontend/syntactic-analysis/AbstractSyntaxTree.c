#include "AbstractSyntaxTree.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* PUBLIC FUNCTIONS */

void destroyInfoProp(InfoProp * prop) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (prop != NULL) {
		free(prop);
	}
}

void destroyInfoPropList(InfoPropList * list) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (list != NULL) {
		destroyInfoPropList(list->next);
		destroyInfoProp(list->prop);
		free(list);
	}
}

void destroyPatternInfo(PatternInfo * patternInfo) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (patternInfo != NULL) {
		free(patternInfo->name);
		destroyInfoPropList(patternInfo->props);
		free(patternInfo);
	}
}

void destroyRowRange(RowRange * rowRange) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (rowRange != NULL) {
		free(rowRange);
	}
}

void destroyStitchItem(StitchItem * stitchItem) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (stitchItem != NULL) {
		if (stitchItem->kind == STITCH_ITEM_REPEAT) {
			destroyStitchList(stitchItem->repeat.list);
		}
		free(stitchItem);
	}
}

void destroyStitchList(StitchList * stitchList) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (stitchList != NULL) {
		destroyStitchList(stitchList->next);
		destroyStitchItem(stitchList->item);
		free(stitchList);
	}
}

void destroyRowDecl(RowDecl * rowDecl) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (rowDecl != NULL) {
		destroyRowRange(rowDecl->range);
		destroyStitchList(rowDecl->stitches);
		free(rowDecl);
	}
}

void destroyRowList(RowList * rowList) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (rowList != NULL) {
		destroyRowList(rowList->next);
		destroyRowDecl(rowList->row);
		free(rowList);
	}
}

void destroyPatternBody(PatternBody * patternBody) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (patternBody != NULL) {
		free(patternBody->name);
		destroyRowList(patternBody->rows);
		free(patternBody);
	}
}

void destroyPatternDef(PatternDef * patternDef) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (patternDef != NULL) {
		destroyPatternDef(patternDef->next);
		destroyPatternInfo(patternDef->info);
		destroyPatternBody(patternDef->body);
		free(patternDef);
	}
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL) {
		destroyPatternDef(program->patterns);
		free(program);
	}
}
