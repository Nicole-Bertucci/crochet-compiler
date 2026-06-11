#include "Generator.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>



static Logger * _logger = NULL;



/* Cell size for one stitch in a Linear grid */
#define CELL_SIZE       24
/* Radius step between rings*/
#define RING_STEP       28
/* Radius of the innermost ring (row 1, magic ring) */
#define INNER_RADIUS    20
#define PADDING         40
#define FONT_SIZE       10
#define PATTERN_GAP     60
#define STITCH_DOT_R    6
#define LEGEND_ROW_H    22


static const char * _stitchColor(StitchType type) {
    switch (type) {
        case ST_CH:   return "#4A90D9"; /* blue  */
        case ST_SC:   return "#27AE60"; /* green */
        case ST_HDC:  return "#E67E22"; /* orange */
        case ST_DC:   return "#8E44AD"; /* purple */
        case ST_SLST: return "#E74C3C"; /* red */
        case ST_MR:   return "#F39C12"; /* amber */
        default:      return "#95A5A6"; /* grey  */
    }
}

static const char * _stitchLabel(StitchType type) {
    switch (type) {
        case ST_CH:   return "ch";
        case ST_SC:   return "sc";
        case ST_HDC:  return "hdc";
        case ST_DC:   return "dc";
        case ST_SLST: return "slst";
        case ST_MR:   return "mr";
        default:      return "?";
    }
}

static const char * _modifierSuffix(ModifierType mod) {
    switch (mod) {
        case MT_INC: return "+";   
        case MT_DEC: return "−";
        case MT_BLO: return "b";
        case MT_FLO: return "f";
        default:     return "";
    }
}


static void _out(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fflush(stdout);
}

typedef struct {
    StitchType   type;
    ModifierType mod;
} RenderedStitch;

typedef struct {
    RenderedStitch * data;
    int              count;
    int              capacity;
} StitchArray;

static void _saInit(StitchArray * sa) {
    sa->capacity = 64;
    sa->count    = 0;
    sa->data     = malloc(sa->capacity * sizeof(RenderedStitch));
}

static void _saFree(StitchArray * sa) {
    free(sa->data);
    sa->data     = NULL;
    sa->count    = 0;
    sa->capacity = 0;
}

static void _saPush(StitchArray * sa, StitchType type, ModifierType mod) {
    if (sa->count == sa->capacity) {
        sa->capacity *= 2;
        sa->data = realloc(sa->data, sa->capacity * sizeof(RenderedStitch));
    }
    sa->data[sa->count].type = type;
    sa->data[sa->count].mod  = mod;
    sa->count++;
}

static void _expandStitchList(StitchList * list, StitchArray * sa) {
    for (StitchList * sl = list; sl != NULL; sl = sl->next) {
        StitchItem * item = sl->item;
        if (item->kind == STITCH_ITEM_SIMPLE) {
            int n = item->simple.count;
            for (int i = 0; i < n; i++) {
                _saPush(sa, item->simple.type, item->simple.modifier);
        
                if (item->simple.modifier == MT_INC) {
                    _saPush(sa, item->simple.type, MT_INC);
                }
                /* dec consumes one extra stitch (mark second slot as "consumed") */
                
            }
        } else { /* STITCH_ITEM_REPEAT */
            for (int t = 0; t < item->repeat.times; t++) {
                _expandStitchList(item->repeat.list, sa);
            }
        }
    }
}

/* 
 * LEGEND
  */

typedef struct {
    StitchType types[6];
    int        count;
} UsedStitches;

static bool _typeInSet(UsedStitches * us, StitchType t) {
    for (int i = 0; i < us->count; i++) {
        if (us->types[i] == t) return true;
    }
    return false;
}

static void _collectUsedStitches(RowList * rows, UsedStitches * us) {
    us->count = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        StitchArray sa;
        _saInit(&sa);
        _expandStitchList(rl->row->stitches, &sa);
        for (int i = 0; i < sa.count; i++) {
            StitchType t = sa.data[i].type;
            if (!_typeInSet(us, t) && us->count < 6) {
                us->types[us->count++] = t;
            }
        }
        _saFree(&sa);
    }
}

static void _generateLegend(double x, double y, UsedStitches * us) {
    _out("  <!-- Legend -->\n");
    double cx = x;
    for (int i = 0; i < us->count; i++) {
        StitchType t = us->types[i];
        _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%d\" fill=\"%s\" opacity=\"0.9\"/>\n",
             cx, y, STITCH_DOT_R, _stitchColor(t));
        _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"%d\" font-family=\"monospace\" "
             "fill=\"#333\" dominant-baseline=\"middle\">%s</text>\n",
             cx + STITCH_DOT_R + 3, y, FONT_SIZE, _stitchLabel(t));
        cx += 50;
    }
}

/* 
 * LINEAR PATTERN GENERATOR
*/

/**
 * Counts the maximum stitch count across all row declarations in a linear
 * pattern (used to determine the actual canvas width needed).
 */
static int _maxRowWidth(RowList * rows) {
    int maxW = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        StitchArray sa;
        _saInit(&sa);
        _expandStitchList(rl->row->stitches, &sa);
        if (sa.count > maxW) maxW = sa.count;
        _saFree(&sa);
    }
    return maxW;
}

/**
 * Returns the total number of individual rows .
 */
static int _totalRows(RowList * rows) {
    int n = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        n += rl->row->range->to - rl->row->range->from + 1;
    }
    return n;
}

static void _generateLinearPattern(const char * name, PatternInfo * pi, RowList * rows,
                                    double offsetY) {
    int maxW  = _maxRowWidth(rows);
    int nRows = _totalRows(rows);

    double gridW = maxW  * CELL_SIZE;
    double gridH = nRows * CELL_SIZE;

    double originX = PADDING;
    double originY = offsetY + PADDING;

  
    _out("  <!-- Pattern: %s (Linear) -->\n", name);
    _out("  <rect x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" height=\"%.1f\" "
         "fill=\"#FAFAFA\" stroke=\"#CCC\" stroke-width=\"1\" rx=\"4\"/>\n",
         originX, originY, gridW, gridH);

    /* ── Grid lines ── */
    
    for (int c = 0; c <= maxW; c++) {
        double lx = originX + c * CELL_SIZE;
        _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" "
             "stroke=\"#DDD\" stroke-width=\"0.5\"/>\n",
             lx, originY, lx, originY + gridH);
    }
   
    for (int r = 0; r <= nRows; r++) {
        double ly = originY + r * CELL_SIZE;
        _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" "
             "stroke=\"#DDD\" stroke-width=\"0.5\"/>\n",
             originX, ly, originX + gridW, ly);
    }

  
    int rowIndex = 0; /* visual row index (0 = top) */
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        int rangeSpan = rl->row->range->to - rl->row->range->from + 1;

        StitchArray sa;
        _saInit(&sa);
        _expandStitchList(rl->row->stitches, &sa);

        /* Render this same stitch list for each row in the range */
        for (int rep = 0; rep < rangeSpan; rep++) {
            double rowY = originY + (rowIndex + 0.5) * CELL_SIZE;

            /* Row number label (left margin) */
            _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"%d\" font-family=\"monospace\" "
                 "fill=\"#999\" text-anchor=\"end\" dominant-baseline=\"middle\">%d</text>\n",
                 originX - 4, rowY, FONT_SIZE - 1,
                 rl->row->range->from + rep);

            for (int s = 0; s < sa.count; s++) {
                double cx = originX + (s + 0.5) * CELL_SIZE;
                double cy = rowY;
                const char * color  = _stitchColor(sa.data[s].type);
                const char * suffix = _modifierSuffix(sa.data[s].mod);

                _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\" "
                     "fill=\"%s\" opacity=\"0.85\"/>\n",
                     cx, cy, (double)STITCH_DOT_R, color);

                if (suffix[0] != '\0') {
                    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"7\" "
                         "font-family=\"monospace\" fill=\"white\" "
                         "text-anchor=\"middle\" dominant-baseline=\"middle\">%s</text>\n",
                         cx, cy, suffix);
                }
            }
            rowIndex++;
        }
        _saFree(&sa);
    }

    
    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"13\" font-family=\"sans-serif\" "
         "font-weight=\"bold\" fill=\"#333\">%s</text>\n",
         originX, originY - 10, name);

    
    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"%d\" font-family=\"monospace\" "
         "fill=\"#888\">linear · %dx%d</text>\n",
         originX + 80, originY - 10, FONT_SIZE, maxW, nRows);

  
    UsedStitches us;
    _collectUsedStitches(rows, &us);
    _generateLegend(originX, originY + gridH + LEGEND_ROW_H, &us);
}

/*
 * ROUND PATTERN GENERATOR
*/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void _generateRoundPattern(const char * name, PatternInfo * pi, RowList * rows,
                                   double offsetY) {
    /* Count the number of rings */
    int nRings = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        nRings += rl->row->range->to - rl->row->range->from + 1;
    }

    double maxRadius = INNER_RADIUS + nRings * RING_STEP;
    double cx = PADDING + maxRadius;
    double cy = offsetY + PADDING + maxRadius;

    _out("  <!-- Pattern: %s (Round) -->\n", name);

    int ringIdx = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        int rangeSpan = rl->row->range->to - rl->row->range->from + 1;

        StitchArray sa;
        _saInit(&sa);
        _expandStitchList(rl->row->stitches, &sa);

        for (int rep = 0; rep < rangeSpan; rep++) {
            double radius = INNER_RADIUS + (ringIdx + 1) * RING_STEP;
            int    n      = sa.count;
            if (n == 0) { ringIdx++; continue; }

            _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\" "
                 "fill=\"none\" stroke=\"#DDD\" stroke-width=\"1\"/>\n",
                 cx, cy, radius);

            _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"%d\" "
                 "font-family=\"monospace\" fill=\"#AAA\" "
                 "text-anchor=\"start\" dominant-baseline=\"middle\">%d</text>\n",
                 cx + radius + 3, cy, FONT_SIZE - 1,
                 rl->row->range->from + rep);

      
            double angleStep = 2.0 * M_PI / n;
            double startAngle = -M_PI / 2.0; /* start at top */

            for (int s = 0; s < n; s++) {
                double angle = startAngle + s * angleStep;
                double sx = cx + radius * cos(angle);
                double sy = cy + radius * sin(angle);

                const char * color  = _stitchColor(sa.data[s].type);
                const char * suffix = _modifierSuffix(sa.data[s].mod);

                _out("  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.1f\" "
                     "fill=\"%s\" opacity=\"0.85\"/>\n",
                     sx, sy, (double)STITCH_DOT_R, color);

                if (suffix[0] != '\0') {
                    _out("  <text x=\"%.2f\" y=\"%.2f\" font-size=\"7\" "
                         "font-family=\"monospace\" fill=\"white\" "
                         "text-anchor=\"middle\" dominant-baseline=\"middle\">%s</text>\n",
                         sx, sy, suffix);
                }
            }

            ringIdx++;
        }
        _saFree(&sa);
    }

    _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"5\" fill=\"%s\"/>\n",
         cx, cy, _stitchColor(ST_MR));
    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"7\" font-family=\"monospace\" "
         "fill=\"white\" text-anchor=\"middle\" dominant-baseline=\"middle\">mr</text>\n",
         cx, cy);

  
    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"13\" font-family=\"sans-serif\" "
         "font-weight=\"bold\" fill=\"#333\">%s</text>\n",
         cx - maxRadius, offsetY + PADDING - 12, name);

   
    int maxRingSize = -1;
    for (InfoPropList * ipl = pi->props; ipl != NULL; ipl = ipl->next) {
        if (ipl->prop->kind == INFO_MAX_RING_SIZE) {
            maxRingSize = ipl->prop->intValue;
        }
    }
    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"%d\" "
         "font-family=\"monospace\" fill=\"#888\">round · max_ring=%d</text>\n",
         cx - maxRadius + 90, offsetY + PADDING - 12, FONT_SIZE, maxRingSize);

    /* ── Legend ── */
    UsedStitches us;
    _collectUsedStitches(rows, &us);
    _generateLegend(cx - maxRadius, offsetY + PADDING + maxRadius * 2 + LEGEND_ROW_H, &us);
}

/* 
 * CANVAS SIZE 
  */

static void _patternDimensions(PatternDef * pd, double * outW, double * outH) {
    ShapeType shape = SHAPE_LINEAR;
    for (InfoPropList * ipl = pd->info->props; ipl != NULL; ipl = ipl->next) {
        if (ipl->prop->kind == INFO_SHAPE) {
            shape = ipl->prop->shapeValue;
            break;
        }
    }

    RowList * rows = pd->body->rows;

    if (shape == SHAPE_LINEAR) {
        int maxW  = _maxRowWidth(rows);
        int nRows = _totalRows(rows);
        *outW = 2 * PADDING + maxW * CELL_SIZE + 30;
        *outH = 2 * PADDING + nRows * CELL_SIZE + LEGEND_ROW_H + 20;
    } else {
        int nRings = _totalRows(rows);
        double radius = INNER_RADIUS + nRings * RING_STEP;
        *outW = 2 * PADDING + 2 * radius + 30;
        *outH = 2 * PADDING + 2 * radius + LEGEND_ROW_H + 20;
    }
}

/* 
 * PROGRAM GENERATOR  
*/

static void _generateProgram(Program * program) {
    double totalW = 0;
    double totalH = 0;
    for (PatternDef * pd = program->patterns; pd != NULL; pd = pd->next) {
        double w, h;
        _patternDimensions(pd, &w, &h);
        if (w > totalW) totalW = w;
        totalH += h + PATTERN_GAP;
    }
    totalH -= PATTERN_GAP; 

    _out("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    _out("<svg xmlns=\"http://www.w3.org/2000/svg\" "
         "width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\">\n",
         totalW, totalH, totalW, totalH);

    /* White background */
    _out("  <rect width=\"100%%\" height=\"100%%\" fill=\"white\"/>\n");

    
    double offsetY = 0;
    for (PatternDef * pd = program->patterns; pd != NULL; pd = pd->next) {
        ShapeType shape = SHAPE_LINEAR;
        for (InfoPropList * ipl = pd->info->props; ipl != NULL; ipl = ipl->next) {
            if (ipl->prop->kind == INFO_SHAPE) {
                shape = ipl->prop->shapeValue;
                break;
            }
        }

        if (shape == SHAPE_LINEAR) {
            _generateLinearPattern(pd->info->name, pd->info, pd->body->rows, offsetY);
        } else {
            _generateRoundPattern(pd->info->name, pd->info, pd->body->rows, offsetY);
        }

        double w, h;
        _patternDimensions(pd, &w, &h);
        offsetY += h + PATTERN_GAP;
    }

   
    _out("</svg>\n");
}


static void _shutdownGeneratorModule() {
    if (_logger != NULL) {
        logDebugging(_logger, "Destroying module: Generator...");
        destroyLogger(_logger);
        _logger = NULL;
    }
}

ModuleDestructor initializeGeneratorModule() {
    _logger = createLogger("Generator");
    return _shutdownGeneratorModule;
}

void executeGenerator(CompilerState * compilerState) {
    logDebugging(_logger, "Generating SVG output...");
    _generateProgram(compilerState->abstractSyntaxtTree);
    logDebugging(_logger, "SVG generation complete.");
}