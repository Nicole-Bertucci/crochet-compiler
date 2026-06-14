#include "Generator.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>


static Logger * _logger     = NULL;
static FILE   * _outputFile = NULL;


#define CELL_SIZE       28
#define RING_STEP       30
#define INNER_RADIUS    20
#define PADDING         60
#define FONT_SIZE       10
#define PATTERN_GAP     2
#define STITCH_DOT_R    8
#define LEGEND_ROW_H    20

/*could add colors for each stitch type but all of them being black
 is more true to the actual diagram*/
static const char * _stitchColor(StitchType type) {
      return "#111111";
     
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

static void _out(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(_outputFile, fmt, args);
    fflush(_outputFile);
    va_end(args);
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
            }
        } else {
            for (int t = 0; t < item->repeat.times; t++) {
                _expandStitchList(item->repeat.list, sa);
            }
        }
    }
}

static void _drawStitch(double cx, double cy, StitchType type, ModifierType mod) {
    const char * color = _stitchColor(type);
    double s = (double)STITCH_DOT_R;

    switch (type) {

        case ST_CH:
           
            _out("  <ellipse cx=\"%.2f\" cy=\"%.2f\" rx=\"%.2f\" ry=\"%.2f\""
                 " fill=\"none\" stroke=\"%s\" stroke-width=\"1.5\"/>\n",
                 cx, cy, s * 1.4, s * 0.7, color);
            break;

        case ST_SLST:
            
            _out("  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\""
                 " fill=\"%s\"/>\n",
                 cx, cy, s * 0.55, color);
            break;

        case ST_SC:
           
            _out("  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
                 " stroke=\"%s\" stroke-width=\"1.8\" stroke-linecap=\"round\"/>\n",
                 cx - s * 0.75, cy - s * 0.9,
                 cx + s * 0.75, cy + s * 0.9, color);
            _out("  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
                 " stroke=\"%s\" stroke-width=\"1.8\" stroke-linecap=\"round\"/>\n",
                 cx + s * 0.75, cy - s * 0.9,
                 cx - s * 0.75, cy + s * 0.9, color);
            break;

        case ST_HDC:
            
            _out("  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
                 " stroke=\"%s\" stroke-width=\"1.6\" stroke-linecap=\"round\"/>\n",
                 cx, cy - s * 0.9, cx, cy + s * 0.9, color);
            _out("  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
                 " stroke=\"%s\" stroke-width=\"1.6\" stroke-linecap=\"round\"/>\n",
                 cx - s * 0.75, cy - s * 0.9,
                 cx + s * 0.75, cy - s * 0.9, color);
            break;

        case ST_DC:
           
            _out("  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
                 " stroke=\"%s\" stroke-width=\"1.6\" stroke-linecap=\"round\"/>\n",
                 cx, cy - s * 0.9, cx, cy + s * 0.9, color);
            _out("  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
                 " stroke=\"%s\" stroke-width=\"1.6\" stroke-linecap=\"round\"/>\n",
                 cx - s * 0.75, cy - s * 0.9,
                 cx + s * 0.75, cy - s * 0.9, color);
            _out("  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
                 " stroke=\"%s\" stroke-width=\"1.6\" stroke-linecap=\"round\"/>\n",
                 cx - s * 0.4, cy - s * 0.15,
                 cx + s * 0.4, cy - s * 0.15, color);
            break;

        case ST_MR:
            
            _out("  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\""
                 " fill=\"none\" stroke=\"%s\" stroke-width=\"1.5\"/>\n",
                 cx, cy, s * 0.9, color);
            _out("  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\""
                 " fill=\"%s\"/>\n",
                 cx, cy, s * 0.28, color);
            break;

        default:
            _out("  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\""
                 " fill=\"%s\" opacity=\"0.7\"/>\n",
                 cx, cy, s * 0.55, color);
            break;
    }

  
    double modY = cy + s * 1.5;

    switch (mod) {
        case MT_INC:
            _out("  <text x=\"%.2f\" y=\"%.2f\" font-size=\"14\""
                 " font-family=\"Times New Roman\" fill=\"%s\""
                 " text-anchor=\"middle\" dominant-baseline=\"middle\">+</text>\n",
                 cx, modY, color);
            break;

        case MT_DEC:
            _out("  <text x=\"%.2f\" y=\"%.2f\" font-size=\"14\""
                 " font-family=\"Times New Roman\" fill=\"%s\""
                 " text-anchor=\"middle\" dominant-baseline=\"middle\">-</text>\n",
                 cx, modY, color);
            break;

        case MT_BLO:
            
            _out("  <path d=\"M %.2f %.2f A %.2f %.2f 0 0 1 %.2f %.2f\""
                 " fill=\"none\" stroke=\"%s\" stroke-width=\"1.5\""
                 " stroke-linecap=\"round\"/>\n",
                 cx - s * 0.55, modY,
                 s * 0.55, s * 0.45,
                 cx + s * 0.55, modY,
                 color);
            break;

        case MT_FLO:
            
            _out("  <path d=\"M %.2f %.2f A %.2f %.2f 0 0 0 %.2f %.2f\""
                 " fill=\"none\" stroke=\"%s\" stroke-width=\"1.5\""
                 " stroke-linecap=\"round\"/>\n",
                 cx - s * 0.55, modY - s * 0.45,
                 s * 0.55, s * 0.45,
                 cx + s * 0.55, modY - s * 0.45,
                 color);
            break;

        default:
            break;
    }
}

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
    double lx = x;
    double legendR = STITCH_DOT_R * 0.55; 
    for (int i = 0; i < us->count; i++) {
        StitchType t  = us->types[i];
        const char * color = _stitchColor(t);
        const char * label = _stitchLabel(t);
       
        double r = legendR;
        switch (t) {
            case ST_CH:
                _out("  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\""
                     " fill=\"none\" stroke=\"%s\" stroke-width=\"1.2\"/>\n",
                     lx, y, r * 1.4, r * 0.7, color);
                break;
            case ST_SLST:
                _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\""
                     " fill=\"%s\"/>\n", lx, y, r * 0.55, color);
                break;
            case ST_SC:
                _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
                     " stroke=\"%s\" stroke-width=\"1.3\" stroke-linecap=\"round\"/>\n",
                     lx - r*0.75, y - r*0.9, lx + r*0.75, y + r*0.9, color);
                _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
                     " stroke=\"%s\" stroke-width=\"1.3\" stroke-linecap=\"round\"/>\n",
                     lx + r*0.75, y - r*0.9, lx - r*0.75, y + r*0.9, color);
                break;
            case ST_HDC:
                _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
                     " stroke=\"%s\" stroke-width=\"1.3\" stroke-linecap=\"round\"/>\n",
                     lx, y - r*0.9, lx, y + r*0.9, color);
                _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
                     " stroke=\"%s\" stroke-width=\"1.3\" stroke-linecap=\"round\"/>\n",
                     lx - r*0.75, y - r*0.9, lx + r*0.75, y - r*0.9, color);
                break;
            case ST_DC:
                _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
                     " stroke=\"%s\" stroke-width=\"1.3\" stroke-linecap=\"round\"/>\n",
                     lx, y - r*0.9, lx, y + r*0.9, color);
                _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
                     " stroke=\"%s\" stroke-width=\"1.3\" stroke-linecap=\"round\"/>\n",
                     lx - r*0.75, y - r*0.9, lx + r*0.75, y - r*0.9, color);
                _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
                     " stroke=\"%s\" stroke-width=\"1.3\" stroke-linecap=\"round\"/>\n",
                     lx - r*0.4, y - r*0.15, lx + r*0.4, y - r*0.15, color);
                break;
            case ST_MR:
                _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\""
                     " fill=\"none\" stroke=\"%s\" stroke-width=\"1.2\"/>\n",
                     lx, y, r * 0.9, color);
                _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\""
                     " fill=\"%s\"/>\n", lx, y, r * 0.28, color);
                break;
            default:
                _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\""
                     " fill=\"%s\" opacity=\"0.7\"/>\n", lx, y, r*0.5, color);
                break;
        }
        _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"8\""
             " font-family=\"Times New Roman\" fill=\"#111111\""
             " dominant-baseline=\"middle\">%s</text>\n",
             lx + legendR + 3, y, label);
        lx += legendR * 2 + 28;
    }
}


static int _stitchSlots(ModifierType mod) {
    return (mod == MT_INC || mod == MT_DEC) ? 2 : 1;
}

static int _totalSlots(StitchArray * sa) {
    int slots = 0;
    for (int i = 0; i < sa->count; i++) {
        slots += _stitchSlots(sa->data[i].mod);
    }
    return slots;
}

static int _maxRowWidth(RowList * rows) {
    int maxW = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        StitchArray sa;
        _saInit(&sa);
        _expandStitchList(rl->row->stitches, &sa);
        int slots = _totalSlots(&sa);
        if (slots > maxW) maxW = slots;
        _saFree(&sa);
    }
    return maxW;
}

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
    _out("  <rect x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" height=\"%.1f\""
         " fill=\"#FAFAFA\" stroke=\"#CCC\" stroke-width=\"1\" rx=\"4\"/>\n",
         originX, originY, gridW, gridH);

  
    for (int c = 0; c <= maxW; c++) {
        double lx = originX + c * CELL_SIZE;
        _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
             " stroke=\"#DDD\" stroke-width=\"0.5\"/>\n",
             lx, originY, lx, originY + gridH);
    }

    for (int r = 0; r <= nRows; r++) {
        double ly = originY + r * CELL_SIZE;
        _out("  <line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\""
             " stroke=\"#DDD\" stroke-width=\"0.5\"/>\n",
             originX, ly, originX + gridW, ly);
    }

    int rowIndex = 0;
    for (RowList * rl = rows; rl != NULL; rl = rl->next) {
        int rangeSpan = rl->row->range->to - rl->row->range->from + 1;

        StitchArray sa;
        _saInit(&sa);
        _expandStitchList(rl->row->stitches, &sa);

        for (int rep = 0; rep < rangeSpan; rep++) {
            double rowY = originY + (rowIndex + 0.5) * CELL_SIZE;

            _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"%d\""
                 " font-family=\"Times New Roman\" fill=\"#999\""
                 " text-anchor=\"end\" dominant-baseline=\"middle\">%d</text>\n",
                 originX - 4, rowY, FONT_SIZE - 1,
                 rl->row->range->from + rep);

            int rowSlots = _totalSlots(&sa);
            double slotScale = (rowSlots > 0) ? (double)maxW / rowSlots : 1.0;
            double slotPos = 0.0;
            for (int s = 0; s < sa.count; s++) {
                int    spans     = _stitchSlots(sa.data[s].mod);
                double scaledW   = spans * slotScale;
                double cx = originX + (slotPos + scaledW * 0.5) * CELL_SIZE;
                double cy = rowY;
                _drawStitch(cx, cy, sa.data[s].type, sa.data[s].mod);
                slotPos += scaledW;
            }
            rowIndex++;
        }
        _saFree(&sa);
    }

    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"15\""
         " font-family=\"Times New Roman\" font-weight=\"bold\" fill=\"#111111\""
         " text-anchor=\"middle\">%s</text>\n",
         originX + gridW * 0.5, originY - 18, name);

    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"10\""
         " font-family=\"Times New Roman\" fill=\"#111111\""
         " text-anchor=\"middle\">Number of rows: %d</text>\n",
         originX + gridW * 0.5, originY - 6, nRows);

    UsedStitches us;
    _collectUsedStitches(rows, &us);
    _generateLegend(originX, originY + gridH + LEGEND_ROW_H, &us);
}


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void _generateRoundPattern(const char * name, PatternInfo * pi, RowList * rows,
                                   double offsetY) {
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

            _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\""
                 " fill=\"none\" stroke=\"#DDD\" stroke-width=\"1\"/>\n",
                 cx, cy, radius);

            _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"%d\""
                 " font-family=\"Times New Roman\" fill=\"#AAA\""
                 " text-anchor=\"start\" dominant-baseline=\"middle\">%d</text>\n",
                 cx + radius + 3, cy, FONT_SIZE - 1,
                 rl->row->range->from + rep);

            double angleStep  = 2.0 * M_PI / n;
            double startAngle = -M_PI / 2.0;

            for (int s = 0; s < n; s++) {
                double angle = startAngle + s * angleStep;
                double sx = cx + radius * cos(angle);
                double sy = cy + radius * sin(angle);
                _drawStitch(sx, sy, sa.data[s].type, sa.data[s].mod);
            }

            ringIdx++;
        }
        _saFree(&sa);
    }

   
    _out("  <circle cx=\"%.1f\" cy=\"%.1f\" r=\"5\" fill=\"%s\"/>\n",
         cx, cy, _stitchColor(ST_MR));
    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"7\" font-family=\"Times New Roman\""
         " fill=\"white\" text-anchor=\"middle\" dominant-baseline=\"middle\">mr</text>\n",
         cx, cy);

    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"15\""
         " font-family=\"Times New Roman\" font-weight=\"bold\" fill=\"#111111\""
         " text-anchor=\"middle\">%s</text>\n",
         cx, offsetY + PADDING - 25, name);

    int maxRingSize = -1;
    for (InfoPropList * ipl = pi->props; ipl != NULL; ipl = ipl->next) {
        if (ipl->prop->kind == INFO_MAX_RING_SIZE) {
            maxRingSize = ipl->prop->intValue;
        }
    }
    _out("  <text x=\"%.1f\" y=\"%.1f\" font-size=\"10\""
         " font-family=\"Times New Roman\" fill=\"#111111\""
         " text-anchor=\"middle\">number of rows: %d</text>\n",
         cx, offsetY + PADDING - 13, nRings);

    UsedStitches us;
    _collectUsedStitches(rows, &us);
    _generateLegend(cx - maxRadius, offsetY + PADDING + maxRadius * 2 + LEGEND_ROW_H, &us);
}

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
    _out("<svg xmlns=\"http://www.w3.org/2000/svg\""
         " width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\">\n",
         totalW, totalH, totalW, totalH);
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

static char * _buildOutputPath(const char * inputPath) {
    const char * dot = strrchr(inputPath, '.');
    const char * sep = strrchr(inputPath, '/');
    size_t baseLen;
    if (dot != NULL && (sep == NULL || dot > sep)) {
        baseLen = (size_t)(dot - inputPath);
    } else {
        baseLen = strlen(inputPath);
    }
    char * out = malloc(baseLen + 5);
    strncpy(out, inputPath, baseLen);
    strcpy(out + baseLen, ".svg");
    return out;
}

void executeGenerator(CompilerState * compilerState) {
    const char * inputPath  = compilerState->inputFilePath;
    char       * outputPath = _buildOutputPath(inputPath);
    logDebugging(_logger, "Generating SVG output to: %s", outputPath);

    _outputFile = fopen(outputPath, "w");
    if (_outputFile == NULL) {
        logError(_logger, "Could not open output file: %s", outputPath);
        free(outputPath);
        return;
    }

    _generateProgram(compilerState->abstractSyntaxtTree);

    fclose(_outputFile);
    _outputFile = NULL;
    logDebugging(_logger, "SVG generation complete: %s", outputPath);
    free(outputPath);
}