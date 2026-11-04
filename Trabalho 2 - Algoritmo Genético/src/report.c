#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "report.h"

#define DAYS_PER_WEEK 6
#define SLOTS_PER_DAY 12
#define SLOTS_PER_HALF 6
#define CELL_WIDTH 16

static const char *DAY_LABELS[DAYS_PER_WEEK] = {"SEG", "TER", "QUA", "QUI", "SEX", "SAB"};

void summary_compute(RunSummary *summary, const int *penalties, int runs)
{
    int index;
    int zero_count = 0;
    double sum = 0.0;
    double variance = 0.0;
    double difference;

    summary->runs = runs > 0 ? runs : 0;
    summary->best_penalty = 0;
    summary->mean_penalty = 0.0;
    summary->standard_deviation = 0.0;
    summary->conflict_free_rate = 0.0;
    if (runs <= 0) {
        return;
    }

    summary->best_penalty = penalties[0];
    for (index = 0; index < runs; index++) {
        if (penalties[index] < summary->best_penalty) {
            summary->best_penalty = penalties[index];
        }
        if (penalties[index] == 0) {
            zero_count++;
        }
        sum += penalties[index];
    }
    summary->mean_penalty = sum / runs;
    for (index = 0; index < runs; index++) {
        difference = penalties[index] - summary->mean_penalty;
        variance += difference * difference;
    }
    summary->standard_deviation = sqrt(variance / runs);
    summary->conflict_free_rate = (double)zero_count / runs;
}

void report_print_schedule(FILE *output, const Individual *best, const BlockList *list, const Dataset *dataset, int periods)
{
    int period;
    int block_index;
    int slot;
    int day;
    int offset;
    char label[8];
    const char *cells[SLOTS_PER_DAY][DAYS_PER_WEEK];
    const Course *course;
    const Block *block;

    for (period = 1; period <= periods; period++) {
        memset(cells, 0, sizeof(cells));
        for (block_index = 0; block_index < list->block_count; block_index++) {
            block = &list->blocks[block_index];
            course = &dataset->courses[block->course];
            if (course->period != period) {
                continue;
            }
            for (slot = best->genes[block_index]; slot < best->genes[block_index] + block->length; slot++) {
                cells[slot % SLOTS_PER_DAY][slot / SLOTS_PER_DAY] = course->code;
            }
        }

        fprintf(output, "Periodo %d\n", period);
        fprintf(output, "%-8s", "Horario");
        for (day = 0; day < DAYS_PER_WEEK; day++) {
            fprintf(output, " %-*s", CELL_WIDTH, DAY_LABELS[day]);
        }
        fprintf(output, "\n");
        for (offset = 0; offset < SLOTS_PER_DAY; offset++) {
            snprintf(label, sizeof(label), "%c%d", offset < SLOTS_PER_HALF ? 'M' : 'T', offset % SLOTS_PER_HALF + 1);
            fprintf(output, "%-8s", label);
            for (day = 0; day < DAYS_PER_WEEK; day++) {
                fprintf(output, " %-*s", CELL_WIDTH, cells[offset][day] != NULL ? cells[offset][day] : "-");
            }
            fprintf(output, "\n");
        }
        fprintf(output, "\n");
    }
}

#define MAX_CELL_BLOCKS 4
#define SATURDAY_INDEX 5

static const char *START_TIMES[SLOTS_PER_DAY] = {
    "07:30", "08:20", "09:10", "10:20", "11:10", "12:00",
    "13:00", "13:50", "14:40", "15:50", "16:40", "17:30"
};

static const char *DAY_NAMES[DAYS_PER_WEEK] = {
    "Segunda", "Ter\xc3\xa7" "a", "Quarta", "Quinta", "Sexta", "S\xc3\xa1" "bado"
};

static const char *HTML_HEAD =
    "<!DOCTYPE html>\n"
    "<html lang=\"pt-BR\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
    "<title>Grade de hor\xc3\xa1rios</title>\n"
    "<style>\n"
    ":root{--bg:#f6f7f9;--surface:#fff;--text:#1b1f24;--muted:#5b6570;--line:#d5dae0;--accent:#1f5fbf;--accent-text:#fff;--head:#e8edf3;--warn-bg:#fff4d6;--warn-text:#5c4300}\n"
    "@media (prefers-color-scheme:dark){:root{--bg:#101318;--surface:#181c22;--text:#e7eaee;--muted:#9aa4af;--line:#2d343d;--accent:#6ea2ff;--accent-text:#0b1220;--head:#222831;--warn-bg:#3a2f12;--warn-text:#f1d98a}}\n"
    "*{box-sizing:border-box}\n"
    "body{margin:0;background:var(--bg);color:var(--text);font-family:system-ui,sans-serif;line-height:1.4}\n"
    "main{max-width:960px;margin:0 auto;padding:16px}\n"
    "h1{font-size:1.25rem;margin:0 0 8px}\n"
    ".warning{background:var(--warn-bg);color:var(--warn-text);padding:8px 12px;border-radius:6px;margin:0 0 12px}\n"
    ".nav{display:flex;align-items:center;justify-content:space-between;gap:8px;margin-bottom:12px}\n"
    ".nav button{background:var(--accent);color:var(--accent-text);border:0;border-radius:6px;padding:8px 14px;font:inherit;cursor:pointer}\n"
    ".nav button:disabled{opacity:.4;cursor:default}\n"
    ".counter{font-weight:600;text-align:center}\n"
    ".slide{display:none}\n"
    ".slide.active{display:block}\n"
    ".scroll{overflow-x:auto;border:1px solid var(--line);border-radius:6px;background:var(--surface)}\n"
    "table{border-collapse:collapse;width:100%;min-width:640px}\n"
    "th,td{border:1px solid var(--line);padding:6px 8px;text-align:left;vertical-align:top;font-size:.85rem}\n"
    "th{background:var(--head)}\n"
    "td.empty{background:transparent}\n"
    ".time{color:var(--muted);font-size:.75rem;white-space:nowrap}\n"
    ".code{font-weight:600;display:block}\n"
    ".prof{color:var(--muted);display:block;font-size:.75rem}\n"
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "<main>\n"
    "<h1>Grade de hor\xc3\xa1rios</h1>\n"
    "<p class=\"warning\">Grade simulada a partir dos professores de 2026/2. N\xc3\xa3o \xc3\xa9 a grade oficial.</p>\n"
    "<div class=\"nav\">\n"
    "<button type=\"button\" id=\"prev\">Anterior</button>\n"
    "<span class=\"counter\" id=\"counter\"></span>\n"
    "<button type=\"button\" id=\"next\">Pr\xc3\xb3ximo</button>\n"
    "</div>\n";

static const char *HTML_TAIL =
    "</main>\n"
    "<script>\n"
    "(function(){\n"
    "var slides=document.querySelectorAll('.slide');\n"
    "var counter=document.getElementById('counter');\n"
    "var prev=document.getElementById('prev');\n"
    "var next=document.getElementById('next');\n"
    "var current=0;\n"
    "function show(index){\n"
    "if(index<0||index>=slides.length){return;}\n"
    "slides[current].classList.remove('active');\n"
    "current=index;\n"
    "slides[current].classList.add('active');\n"
    "counter.textContent='Per\\u00edodo '+(current+1)+' de '+slides.length;\n"
    "prev.disabled=current===0;\n"
    "next.disabled=current===slides.length-1;\n"
    "}\n"
    "prev.addEventListener('click',function(){show(current-1);});\n"
    "next.addEventListener('click',function(){show(current+1);});\n"
    "document.addEventListener('keydown',function(event){\n"
    "if(event.key==='ArrowLeft'){show(current-1);}\n"
    "if(event.key==='ArrowRight'){show(current+1);}\n"
    "});\n"
    "show(0);\n"
    "})();\n"
    "</script>\n"
    "</body>\n"
    "</html>\n";

static void html_escape(FILE *file, const char *text)
{
    for (; *text != '\0'; text++) {
        if (*text == '&') {
            fputs("&amp;", file);
        } else if (*text == '<') {
            fputs("&lt;", file);
        } else if (*text == '>') {
            fputs("&gt;", file);
        } else {
            fputc(*text, file);
        }
    }
}

static void html_write_cell(FILE *file, const int *cell_blocks, int cell_count, const BlockList *list, const Dataset *dataset)
{
    int index;
    const Block *block;

    if (cell_count == 0) {
        fputs("<td class=\"empty\"></td>", file);
        return;
    }
    fputs("<td>", file);
    for (index = 0; index < cell_count; index++) {
        block = &list->blocks[cell_blocks[index]];
        fputs("<span class=\"code\">", file);
        html_escape(file, dataset->courses[block->course].code);
        fputs("</span><span class=\"prof\">", file);
        if (block->professor >= 0 && block->professor < dataset->professor_count) {
            html_escape(file, dataset->professors[block->professor]);
        }
        fputs("</span>", file);
    }
    fputs("</td>", file);
}

static void html_write_period(FILE *file, int period, int periods, const Individual *best, const BlockList *list, const Dataset *dataset)
{
    int cells[SLOTS_PER_DAY][DAYS_PER_WEEK][MAX_CELL_BLOCKS];
    int counts[SLOTS_PER_DAY][DAYS_PER_WEEK];
    int block_index;
    int slot;
    int day;
    int offset;
    int count;
    const Block *block;

    memset(counts, 0, sizeof(counts));
    for (block_index = 0; block_index < list->block_count; block_index++) {
        block = &list->blocks[block_index];
        if (dataset->courses[block->course].period != period) {
            continue;
        }
        for (slot = best->genes[block_index]; slot < best->genes[block_index] + block->length; slot++) {
            if (slot < 0 || slot >= SLOT_COUNT) {
                continue;
            }
            day = slot / SLOTS_PER_DAY;
            offset = slot % SLOTS_PER_DAY;
            if (day == SATURDAY_INDEX && offset >= SLOTS_PER_HALF) {
                continue;
            }
            count = counts[offset][day];
            if (count < MAX_CELL_BLOCKS) {
                cells[offset][day][count] = block_index;
                counts[offset][day] = count + 1;
            }
        }
    }

    fprintf(file, "<section class=\"slide\" data-period=\"%d\">\n", period);
    fprintf(file, "<h2>Per\xc3\xadodo %d de %d</h2>\n", period, periods);
    fputs("<div class=\"scroll\"><table>\n<thead><tr><th>Hor\xc3\xa1rio</th>", file);
    for (day = 0; day < DAYS_PER_WEEK; day++) {
        fprintf(file, "<th>%s</th>", DAY_NAMES[day]);
    }
    fputs("</tr></thead>\n<tbody>\n", file);
    for (offset = 0; offset < SLOTS_PER_DAY; offset++) {
        fprintf(file, "<tr><th scope=\"row\">%c%d <span class=\"time\">%s</span></th>",
                offset < SLOTS_PER_HALF ? 'M' : 'T', offset % SLOTS_PER_HALF + 1, START_TIMES[offset]);
        for (day = 0; day < DAYS_PER_WEEK; day++) {
            html_write_cell(file, cells[offset][day], counts[offset][day], list, dataset);
        }
        fputs("</tr>\n", file);
    }
    fputs("</tbody>\n</table></div>\n</section>\n", file);
}

int report_write_html(const char *path, const Individual *best, const BlockList *list, const Dataset *dataset, int periods)
{
    FILE *file;
    int period;

    file = fopen(path, "w");
    if (file == NULL) {
        return -1;
    }
    fputs(HTML_HEAD, file);
    for (period = 1; period <= periods; period++) {
        html_write_period(file, period, periods, best, list, dataset);
    }
    fputs(HTML_TAIL, file);
    return fclose(file) == 0 ? 0 : -1;
}

int report_write_history(const char *path, const GeneticResult *result)
{
    FILE *file;
    int generation;

    file = fopen(path, "w");
    if (file == NULL) {
        return -1;
    }
    fprintf(file, "generation,best_penalty\n");
    for (generation = 0; generation < result->generations; generation++) {
        fprintf(file, "%d,%d\n", generation + 1, result->best_penalty_history[generation]);
    }
    return fclose(file) == 0 ? 0 : -1;
}

int report_write_summary(const char *path, const RunSummary *summary)
{
    FILE *file;

    file = fopen(path, "w");
    if (file == NULL) {
        return -1;
    }
    fprintf(file, "runs,best_penalty,mean_penalty,standard_deviation,conflict_free_rate\n");
    fprintf(file, "%d,%d,%.4f,%.4f,%.4f\n", summary->runs, summary->best_penalty, summary->mean_penalty,
            summary->standard_deviation, summary->conflict_free_rate);
    return fclose(file) == 0 ? 0 : -1;
}

int report_ensure_directory(const char *path)
{
    struct stat info;

    if (mkdir(path, 0755) == 0) {
        return 0;
    }
    if (errno == EEXIST && stat(path, &info) == 0 && S_ISDIR(info.st_mode)) {
        return 0;
    }
    return -1;
}
