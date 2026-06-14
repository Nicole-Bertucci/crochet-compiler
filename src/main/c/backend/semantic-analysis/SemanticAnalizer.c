#include "SemanticAnalizer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



static Logger * _logger = NULL;

void initializeSemanticAnalyzerModule() {
    _logger = createLogger("SemanticAnalyzer");
}

void shutdownSemanticAnalyzerModule() {
    if (_logger != NULL) {
        logDebugging(_logger, "Destroying module: SemanticAnalyzer...");
        destroyLogger(_logger);
        _logger = NULL;
    }
}

/* ── Helpers ────────────────────────────────────────────────────────────── */

static int _countStitches(StitchList * list) {
    int total = 0;
    for (StitchList * sl = list; sl != NULL; sl = sl->next) {
        StitchItem * item = sl->item;
        if (item->kind == STITCH_ITEM_SIMPLE) {
            int base = item->simple.count;
            switch (item->simple.modifier) {
                case MT_INC: total += base * 2; break;  
                case MT_DEC: total += base;     break; 
                default:     total += base;     break;
            }
        } else { 
            total += _countStitches(item->repeat.list) * item->repeat.times;
        }
    }
    return total;
}


static int _rangeEnd(RowRange * rr) {
    return rr->to + 1;
}

/* ── Per-pattern validators ─────────────────────────────────────────────── */

static bool _validateStitchList(StitchList * list, const char * patternName, int rowFrom) {
    bool ok = true;
    for (StitchList * sl = list; sl != NULL; sl = sl->next) {
        StitchItem * item = sl->item;
        if (item->kind == STITCH_ITEM_SIMPLE) {
            if (item->simple.count < 1) {
                logError(_logger,
                    "Pattern '%s', row %d: stitch count must be >= 1 (got %d).",
                    patternName, rowFrom, item->simple.count);
                ok = false;
            }
        } else { 
            if (item->repeat.times < 1) {
                logError(_logger,
                    "Pattern '%s', row %d: repeat count must be >= 1 (got %d).",
                    patternName, rowFrom, item->repeat.times);
                ok = false;
            }
            
            if (!_validateStitchList(item->repeat.list, patternName, rowFrom)) {
                ok = false;
            }
        }
    }
    return ok;
}


static bool _validateDecModifiers(StitchList * list, const char * patternName, int rowFrom) {
    bool ok = true;
    int cumulative = 0;
    for (StitchList * sl = list; sl != NULL; sl = sl->next) {
        StitchItem * item = sl->item;
        if (item->kind == STITCH_ITEM_SIMPLE) {
            if (item->simple.modifier == MT_DEC) {
               
                cumulative += item->simple.count * 2; 
                if (cumulative < 2) {
                    logError(_logger,
                        "Pattern '%s', row %d: -dec requires at least 2 stitches available.",
                        patternName, rowFrom);
                    ok = false;
                }
            } else {
                int base = item->simple.count;
                cumulative += (item->simple.modifier == MT_INC) ? base * 2 : base;
            }
        } else {
            
            cumulative += _countStitches(item->repeat.list) * item->repeat.times;
        }
    }
    return ok;
}


static bool _validateMR(RowList * rows, const char * patternName, ShapeType shape) {
    bool ok = true;
    bool firstRow = true;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        RowDecl * rd = rl->row;
        StitchList * sl = rd->stitches;
        bool firstItem = true;
        for (; sl != NULL; sl = sl->next) {
            StitchItem * item = sl->item;
            if (item->kind == STITCH_ITEM_SIMPLE && item->simple.type == ST_MR) {
                if (shape != SHAPE_ROUND) {
                    logError(_logger,
                        "Pattern '%s': mr stitch is only valid in Round patterns.",
                        patternName);
                    ok = false;
                } else if (!firstRow || !firstItem) {
                    logError(_logger,
                        "Pattern '%s': mr may only appear as the first stitch of row 1.",
                        patternName);
                    ok = false;
                }
            }
           
            if (item->kind == STITCH_ITEM_REPEAT) {
                for (StitchList * inner = item->repeat.list; inner != NULL; inner = inner->next) {
                    if (inner->item->kind == STITCH_ITEM_SIMPLE &&
                        inner->item->simple.type == ST_MR) {
                        logError(_logger,
                            "Pattern '%s': mr may not appear inside a repeat block.",
                            patternName);
                        ok = false;
                    }
                }
            }
            firstItem = false;
        }
        firstRow = false;
    }
    
    if (shape == SHAPE_ROUND && rows != NULL) {
        RowDecl * first = rows->row;
        StitchList * firstStitch = first->stitches;
        if (firstStitch == NULL ||
            firstStitch->item->kind != STITCH_ITEM_SIMPLE ||
            firstStitch->item->simple.type != ST_MR) {
            logError(_logger,
                "Pattern '%s': the first stitch of row 1 must be 'mr' for Round patterns.",
                patternName);
            ok = false;
        }
    }
    return ok;
}


static bool _validateRowOrder(RowList * rows, const char * patternName) {
    bool ok = true;
    int expected = 1;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        RowRange * rr = rl->row->range;
     
        if (rr->from != expected) {
            logError(_logger,
                "Pattern '%s': expected row %d but found row %d.",
                patternName, expected, rr->from);
            ok = false;
           
            expected = _rangeEnd(rr);
            continue;
        }
        if (rr->to < rr->from) {
            logError(_logger,
                "Pattern '%s': row range %d-%d is invalid (to < from).",
                patternName, rr->from, rr->to);
            ok = false;
        }
        expected = _rangeEnd(rr);
    }
    return ok;
}


static int _validatePatternInfo(PatternInfo * pi) {
    bool ok = true;
    bool hasShape       = false;
    bool hasMaxWidth    = false;
    bool hasMaxRingSize = false;
    bool hasRows        = false;
    ShapeType shape     = SHAPE_LINEAR; 

    for (InfoPropList * ipl = pi->props; ipl != NULL; ipl = ipl->next) {
        InfoProp * prop = ipl->prop;
        switch (prop->kind) {
            case INFO_SHAPE:
                if (hasShape) {
                    logError(_logger,
                        "Pattern '%s': duplicate 'shape' property.", pi->name);
                    ok = false;
                }
                hasShape = true;
                shape = prop->shapeValue;
                break;
            case INFO_MAX_WIDTH:
                if (hasMaxWidth) {
                    logError(_logger,
                        "Pattern '%s': duplicate 'maxWidth' property.", pi->name);
                    ok = false;
                }
                hasMaxWidth = true;
                if (prop->intValue < 1) {
                    logError(_logger,
                        "Pattern '%s': maxWidth must be >= 1 (got %d).",
                        pi->name, prop->intValue);
                    ok = false;
                }
                break;
            case INFO_MAX_RING_SIZE:
                if (hasMaxRingSize) {
                    logError(_logger,
                        "Pattern '%s': duplicate 'maxRingSize' property.", pi->name);
                    ok = false;
                }
                hasMaxRingSize = true;
                if (prop->intValue < 1) {
                    logError(_logger,
                        "Pattern '%s': maxRingSize must be >= 1 (got %d).",
                        pi->name, prop->intValue);
                    ok = false;
                }
                break;
            case INFO_ROWS:
                if (hasRows) {
                    logError(_logger,
                        "Pattern '%s': duplicate 'rows' property.", pi->name);
                    ok = false;
                }
                hasRows = true;
                if (prop->intValue < 1) {
                    logError(_logger,
                        "Pattern '%s': rows must be >= 1 (got %d).",
                        pi->name, prop->intValue);
                    ok = false;
                }
                break;
        }
    }

    if (!hasShape) {
        logError(_logger, "Pattern '%s': missing required 'shape' property.", pi->name);
        ok = false;
        return -1; 
    }

    if (shape == SHAPE_LINEAR) {
        if (!hasMaxWidth) {
            logError(_logger,
                "Pattern '%s': Linear pattern requires 'maxWidth'.", pi->name);
            ok = false;
        }
        if (!hasRows) {
            logError(_logger,
                "Pattern '%s': Linear pattern requires 'rows'.", pi->name);
            ok = false;
        }
        if (hasMaxRingSize) {
            logError(_logger,
                "Pattern '%s': Linear pattern must not have 'maxRingSize'.", pi->name);
            ok = false;
        }
    } else { 
        if (!hasMaxRingSize) {
            logError(_logger,
                "Pattern '%s': Round pattern requires 'maxRingSize'.", pi->name);
            ok = false;
        }
        if (hasMaxWidth) {
            logError(_logger,
                "Pattern '%s': Round pattern must not have 'maxWidth'.", pi->name);
            ok = false;
        }
        if (hasRows) {
            logError(_logger,
                "Pattern '%s': Round pattern must not have 'rows'.", pi->name);
            ok = false;
        }
    }

    return ok ? (int)shape : -1;
}


static int _getIntProp(PatternInfo * pi, InfoPropKind kind) {
    for (InfoPropList * ipl = pi->props; ipl != NULL; ipl = ipl->next) {
        if (ipl->prop->kind == kind) {
            return ipl->prop->intValue;
        }
    }
    return -1;
}

static ShapeType _getShape(PatternInfo * pi) {
    for (InfoPropList * ipl = pi->props; ipl != NULL; ipl = ipl->next) {
        if (ipl->prop->kind == INFO_SHAPE) {
            return ipl->prop->shapeValue;
        }
    }
    return SHAPE_LINEAR;
}


static int _countDeclaredRows(RowList * rows) {
    int total = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        RowRange * rr = rl->row->range;
        total += (rr->to - rr->from + 1);
    }
    return total;
}


static int _countConsumed(StitchList * list) {
    int total = 0;
    for (StitchList * sl = list; sl != NULL; sl = sl->next) {
        StitchItem * item = sl->item;
        if (item->kind == STITCH_ITEM_SIMPLE) {
            int base = item->simple.count;
            if (item->simple.type == ST_MR || item->simple.type == ST_CH) {
                
                continue;
            }
            if (item->simple.modifier == MT_DEC) {
                total += base * 2;
            } else {
                total += base;
            }
        } else { 
            total += _countConsumed(item->repeat.list) * item->repeat.times;
        }
    }
    return total;
}


static bool _validateRowContinuity(RowList * rows, const char * patternName) {
    bool ok = true;
    int prevOutput = 0;  
    bool isFirstRow = true;

    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        RowDecl * rd      = rl->row;
        int       rowFrom = rd->range->from;
        int       rangeSpan = rd->range->to - rd->range->from + 1;

        int consumed = _countConsumed(rd->stitches);
        int produced = _countStitches(rd->stitches);

        if (isFirstRow) {
            
            if (consumed > 0) {
                logError(_logger,
                    "Pattern '%s', row %d: stitches that work into a previous row "
                    "cannot appear in the first row (no prior row exists).",
                    patternName, rowFrom);
                ok = false;
            }
        } else {
            
            if (consumed > prevOutput) {
                logError(_logger,
                    "Pattern '%s', row %d: requires %d stitch(es) from the previous "
                    "row but only %d are available.",
                    patternName, rowFrom, consumed, prevOutput);
                ok = false;
            }
        }

        
        for (int rep = 1; rep < rangeSpan; rep++) {
            if (produced < consumed) {
                logError(_logger,
                    "Pattern '%s', row %d (repetition %d): requires %d stitch(es) "
                    "but previous repetition only produced %d.",
                    patternName, rowFrom + rep, rep + 1, consumed, produced);
                ok = false;
                break;
            }
        }

        prevOutput  = produced;
        isFirstRow  = false;
    }
    return ok;
}

CompilationStatus executeSemanticAnalysis(Program * program) {
    logDebugging(_logger, "Starting semantic analysis...");

    if (program == NULL) {
        logError(_logger, "Program AST is NULL.");
        return FAILED;
    }

    bool ok = true;

    
    for (PatternDef * pd = program->patterns; pd != NULL; pd = pd->next) {
        const char * name = pd->info ? pd->info->name : NULL;
        if (name == NULL) {
            logError(_logger, "Found a pattern with no name.");
            ok = false;
            continue;
        }
        
        for (PatternDef * other = pd->next; other != NULL; other = other->next) {
            if (other->info && strcmp(other->info->name, name) == 0) {
                logError(_logger, "Duplicate pattern name: '%s'.", name);
                ok = false;
            }
        }
    }

    
    for (PatternDef * pd = program->patterns; pd != NULL; pd = pd->next) {
        PatternInfo * pi   = pd->info;
        PatternBody * body = pd->body;

        if (pi == NULL || body == NULL) {
            logError(_logger, "Pattern definition is missing info or body block.");
            ok = false;
            continue;
        }

        const char * infoName = pi->name;
        const char * bodyName = body->name;

       
        if (strcmp(infoName, bodyName) != 0) {
            logError(_logger,
                "patternInfo name '%s' does not match pattern body name '%s'.",
                infoName, bodyName);
            ok = false;
        }

        
        const char * pname = infoName;

        /* Validate patternInfo properties */
        int shapeInt = _validatePatternInfo(pi);
        if (shapeInt < 0) {
            ok = false;
           
            continue;
        }
        ShapeType shape = (ShapeType)shapeInt;

        RowList * rows = body->rows;

        
        if (rows == NULL) {
            logError(_logger, "Pattern '%s': has no rows.", pname);
            ok = false;
            continue;
        }

        
        if (!_validateRowOrder(rows, pname)) {
            ok = false;
        }

        
        if (!_validateMR(rows, pname, shape)) {
            ok = false;
        }

       
        if (!_validateRowContinuity(rows, pname)) {
            ok = false;
        }

        
        for (RowList * rl = rows; rl != NULL; rl = rl->next) {
            RowDecl * rd    = rl->row;
            int       rowFrom = rd->range->from;

            if (rd->stitches == NULL) {
                logError(_logger, "Pattern '%s', row %d: row has no stitches.", pname, rowFrom);
                ok = false;
                continue;
            }

            if (!_validateStitchList(rd->stitches, pname, rowFrom)) {
                ok = false;
            }

            if (!_validateDecModifiers(rd->stitches, pname, rowFrom)) {
                ok = false;
            }
        }

    
        if (shape == SHAPE_LINEAR) {
            int declaredRows = _getIntProp(pi, INFO_ROWS);
            int actualRows   = _countDeclaredRows(rows);
            if (declaredRows > 0 && actualRows != declaredRows) {
                logError(_logger,
                    "Pattern '%s': declared rows=%d but body contains %d row(s).",
                    pname, declaredRows, actualRows);
                ok = false;
            }

            int maxWidth = _getIntProp(pi, INFO_MAX_WIDTH);
            if (maxWidth > 0) {
                for (RowList * rl = rows; rl != NULL; rl = rl->next) {
                    int width = _countStitches(rl->row->stitches);
                    if (width > maxWidth) {
                        logError(_logger,
                            "Pattern '%s', row %d: stitch count %d exceeds maxWidth=%d.",
                            pname, rl->row->range->from, width, maxWidth);
                        ok = false;
                    }
                }
            }
        } else { 
            int maxRingSize = _getIntProp(pi, INFO_MAX_RING_SIZE);
            if (maxRingSize > 0) {
            
                RowList * lastRl = rows;
                while (lastRl->next != NULL) {
                    lastRl = lastRl->next;
                }
                int lastWidth = _countStitches(lastRl->row->stitches);
                if (lastWidth > maxRingSize) {
                    logError(_logger,
                        "Pattern '%s', last row %d: stitch count %d exceeds maxRingSize=%d.",
                        pname, lastRl->row->range->from, lastWidth, maxRingSize);
                    ok = false;
                }
            }
        }
    }

    if (ok) {
        logDebugging(_logger, "Semantic analysis completed successfully.");
        return SUCCEEDED;
    } else {
        logDebugging(_logger, "Semantic analysis failed with errors.");
        return FAILED;
    }
}