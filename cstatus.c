/* cstatus – Claude Code status line renderer
 * Usage: cat status.json | cstatus
 *        streaming-daemon | cstatus        (one JSON object per render)
 * Build: cc -O2 -std=c99 -o cstatus cstatus.c -lcjson
 *
 * Template: ~/.claude/statusline.template (or --template <path>)
 *   Each non-comment line becomes one output line.
 *   Syntax: free text with $field_name or $field_name,COLOR  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <cjson/cJSON.h>

/* ── forward declarations ─────────────────────────────────── */
static cJSON *jnav(cJSON *root, const char *path);
static char *random_metric(cJSON *json, const char *metric);

/* ── ANSI ─────────────────────────────────────────────────── */
#define RESET    "\033[0m"
#define BOLD     "\033[1m"
#define DIM      "\033[2m"
#define CYAN     "\033[36m"
#define GREEN    "\033[32m"
#define YELLOW   "\033[33m"
#define RED      "\033[31m"
#define MAGENTA  "\033[35m"
#define BLUE     "\033[34m"
#define WHITE    "\033[97m"

/* ── random output generator ──────────────────────────────── *\n * Generates a random string for the $random field.            *\n * Uses a simple LCG random number generator.                  */
static unsigned int seed = 0;

static void random_seed(void)
{
    if (seed == 0) {
        seed = (unsigned int)time(NULL);
    }
}

static double random_double(void)
{
    random_seed();
    seed = seed * 1103515245 + 12345;
    return (double)(seed / 65536) / 65536.0; /* 0.0 to 1.0 */
}

/* Available metrics that $random can pick from */
static const char *random_metrics[] = {
    "exceeds_200k_tokens",
    "context_window.remaining_percentage",
    "context_window.used_percentage",
    "cost.total_cost_usd",
    "cost.total_duration_ms",
    "cost.total_api_duration_ms",
    "cost.total_lines_added",
    "cost.total_lines_removed",
    "context_window.total_input_tokens",
    "context_window.total_output_tokens",
    "context_window.context_window_size",
    "rate_limits.five_hour.used_percentage",
    "rate_limits.seven_day.used_percentage",
    "rate_limits.five_hour.resets_at",
    "rate_limits.seven_day.resets_at",
    NULL
};

/* Track which metrics have been used in current render */
#define MAX_USED_METRICS 32
static char used_metrics[MAX_USED_METRICS][128];
static int used_count = 0;

static int is_metric_used(const char *metric)
{
    for (int i = 0; i < used_count; i++) {
        if (!strcmp(used_metrics[i], metric)) return 1;
    }
    return 0;
}

static void mark_metric_used(const char *metric)
{
    if (used_count < MAX_USED_METRICS) {
        strncpy(used_metrics[used_count], metric, 127);
        used_metrics[used_count][127] = '\0';
        used_count++;
    }
}

static const char *get_random_metric(void)
{
    /* Count available metrics */
    int count = 0;
    for (int i = 0; random_metrics[i]; i++) count++;

    /* Find available metrics (not yet used) */
    int available[MAX_USED_METRICS];
    int avail_count = 0;
    for (int i = 0; i < count; i++) {
        if (!is_metric_used(random_metrics[i])) {
            available[avail_count++] = i;
        }
    }

    /* If all metrics used, reset and pick again */
    if (avail_count == 0) {
        used_count = 0;
        for (int i = 0; i < MAX_USED_METRICS; i++) used_metrics[i][0] = '\0';
        for (int i = 0; i < count; i++) {
            if (!is_metric_used(random_metrics[i])) {
                available[avail_count++] = i;
            }
        }
    }

    /* Pick random available metric */
    int idx = available[(int)(random_double() * avail_count)];
    const char *metric = random_metrics[idx];
    mark_metric_used(metric);
    return metric;
}

static char *random_metric(cJSON *json, const char *metric) {
    static char buf[256];
    buf[0] = '\0';
    double v = 0;

    random_seed();

    /* Boolean metrics */
    if (!strcmp(metric, "exceeds_200k_tokens")) {
        cJSON *node = jnav(json, "exceeds_200k_tokens");
        if (!node || !cJSON_IsTrue(node)) return NULL;
        if (random_double() > 0.5) {
            strncpy(buf, "⚠️", sizeof buf - 1);
            return buf;
        }
        return NULL;
    }

    /* Percentage metrics (0-100) */
    if (!strcmp(metric, "context_window.remaining_percentage") ||
        !strcmp(metric, "context_window.used_percentage") ||
        !strcmp(metric, "rate_limits.five_hour.used_percentage") ||
        !strcmp(metric, "rate_limits.seven_day.used_percentage")) {
        double pct = random_double() * 100.0;
        snprintf(buf, sizeof buf, "%.0f%%", pct);
        return buf;
    }

    /* Cost metric */
    if (!strcmp(metric, "cost.total_cost_usd")) {
        double cost = random_double() * 10.0; /* $0.00 to $10.00 */
        snprintf(buf, sizeof buf, "$%.2f", cost);
        return buf;
    }

    /* Duration metric */
    if (!strcmp(metric, "cost.total_duration_ms")) {
        double dur = random_double() * 300000.0; /* 0 to 5 min */
        long mins = (long)(dur / 60000), secs = (long)(dur / 1000) % 60;
        if (mins > 0) snprintf(buf, sizeof buf, "%ldm %lds", mins, secs);
        else          snprintf(buf, sizeof buf, "%lds", secs);
        return buf;
    }

    /* Token count metrics */
    if (!strcmp(metric, "context_window.total_input_tokens") ||
        !strcmp(metric, "context_window.total_output_tokens") ||
        !strcmp(metric, "context_window.context_window_size")) {
        double tokens = random_double() * 200000.0;
        snprintf(buf, sizeof buf, "%ld", (long)tokens);
        return buf;
    }

    /* Lines metrics */
    if (!strcmp(metric, "cost.total_lines_added") ||
        !strcmp(metric, "cost.total_lines_removed")) {
        double lines = random_double() * 500.0;
        snprintf(buf, sizeof buf, "%+ld", (long)lines);
        return buf;
    }

    /* Rate limit resets (random future time) */
    if (!strcmp(metric, "rate_limits.five_hour.resets_at") ||
        !strcmp(metric, "rate_limits.seven_day.resets_at")) {
        time_t now = time(NULL);
        time_t offset = (time_t)(random_double() * 86400); /* 0 to 24 hours */
        time_t reset = now + offset;
        struct tm *tm = localtime(&reset);
        strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", tm);
        return buf;
    }

    return NULL;
}

/* ── read one complete JSON object from stdin ─────────────── */
static char *read_object(void)
{
    size_t cap = 8192, n = 0;
    char *b = malloc(cap);
    if (!b) return NULL;

    int depth = 0, in_str = 0, escape = 0, started = 0;
    int c;

    while ((c = getchar()) != EOF) {
        if (n + 2 >= cap) {
            cap *= 2;
            char *t = realloc(b, cap);
            if (!t) { free(b); return NULL; }
            b = t;
        }
        b[n++] = (char)c;

        if (escape)              { escape = 0; continue; }
        if (c == '\\' && in_str) { escape = 1; continue; }
        if (c == '"')            { in_str = !in_str; continue; }
        if (in_str)              continue;

        if      (c == '{') { depth++; started = 1; }
        else if (c == '}') { depth--; }

        if (started && depth == 0) break;
    }

    if (!started) { free(b); return NULL; }
    b[n] = '\0';
    return b;
}

/* ── JSON helpers (dot-path navigation via cJSON) ─────────── */
static cJSON *jnav(cJSON *root, const char *path)
{
    cJSON *node = root;
    char seg[128];
    while (*path) {
        const char *dot = strchr(path, '.');
        size_t sl = dot ? (size_t)(dot - path) : strlen(path);
        if (sl >= sizeof seg) return NULL;
        memcpy(seg, path, sl); seg[sl] = '\0';
        path = dot ? dot + 1 : path + sl;
        node = cJSON_GetObjectItemCaseSensitive(node, seg);
        if (!node) return NULL;
    }
    return node;
}

static int jstr(cJSON *root, const char *path, char *buf, size_t blen)
{
    cJSON *node = jnav(root, path);
    if (!node || !cJSON_IsString(node)) { buf[0] = '\0'; return 0; }
    strncpy(buf, node->valuestring, blen - 1);
    buf[blen - 1] = '\0';
    return 1;
}

static int jnum(cJSON *root, const char *path, double *out)
{
    cJSON *node = jnav(root, path);
    if (!node || !cJSON_IsNumber(node)) return 0;
    *out = node->valuedouble;
    return 1;
}

/* ── helpers ──────────────────────────────────────────────── */
static const char *bname(const char *path)
{
    const char *s = strrchr(path, '/');
    return (s && s[1]) ? s + 1 : path;
}

static void draw_bar(int filled)
{
    for (int i = 0; i < 10; i++)
        fputs(i < filled ? "\xe2\x96\x88" : "\xe2\x96\x91", stdout); /* █ ░ */
}

/* ── template: color lookup ───────────────────────────────── */
static const char *lookup_color(const char *name)
{
    if (!strcmp(name, "RESET"))   return RESET;
    if (!strcmp(name, "BOLD"))    return BOLD;
    if (!strcmp(name, "DIM"))     return DIM;
    if (!strcmp(name, "CYAN"))    return CYAN;
    if (!strcmp(name, "GREEN"))   return GREEN;
    if (!strcmp(name, "YELLOW"))  return YELLOW;
    if (!strcmp(name, "RED"))     return RED;
    if (!strcmp(name, "MAGENTA")) return MAGENTA;
    if (!strcmp(name, "BLUE"))    return BLUE;
    if (!strcmp(name, "WHITE"))   return WHITE;
    return NULL;
}

/* ── template: field resolution ───────────────────────────── *
 * Fills buf with a display-ready string for the named field.  *
 * Returns 1 if the field has a value, 0 if absent/empty.      */
static int resolve_field(cJSON *json, const char *name, const char *arg, char *buf, size_t blen)
{
    buf[0] = '\0';
    double v = 0;

    /* string fields */
    if (!strcmp(name, "current_dir")) {
        if (!jstr(json, "workspace.current_dir", buf, blen))
            jstr(json, "cwd", buf, blen);
        if (buf[0]) { const char *b = bname(buf); memmove(buf, b, strlen(b) + 1); }
        return buf[0] != '\0';
    }
    if (!strcmp(name, "project_dir")) {
        jstr(json, "workspace.project_dir", buf, blen);
        if (buf[0]) { const char *b = bname(buf); memmove(buf, b, strlen(b) + 1); }
        return buf[0] != '\0';
    }
    if (!strcmp(name, "git_worktree"))  return jstr(json, "workspace.git_worktree", buf, blen);
    if (!strcmp(name, "model_name"))    return jstr(json, "model.display_name",     buf, blen);
    if (!strcmp(name, "model_id"))      return jstr(json, "model.id",               buf, blen);
    if (!strcmp(name, "session_id"))    return jstr(json, "session_id",             buf, blen);
    if (!strcmp(name, "session_name"))  return jstr(json, "session_name",           buf, blen);
    if (!strcmp(name, "version"))       return jstr(json, "version",                buf, blen);
    if (!strcmp(name, "vim_mode"))      return jstr(json, "vim.mode",               buf, blen);
    if (!strcmp(name, "agent"))         return jstr(json, "agent.name",             buf, blen);
    if (!strcmp(name, "worktree"))      return jstr(json, "worktree.name",          buf, blen);
    if (!strcmp(name, "branch")) {
        if (!jstr(json, "worktree.branch", buf, blen))
            jstr(json, "workspace.git_worktree", buf, blen);
        return buf[0] != '\0';
    }
    if (!strcmp(name, "worktree_path")) return jstr(json, "worktree.path",          buf, blen);
    if (!strcmp(name, "worktree_orig_cwd")) return jstr(json, "worktree.original_cwd", buf, blen);
    if (!strcmp(name, "worktree_orig_branch")) return jstr(json, "worktree.original_branch", buf, blen);
    if (!strcmp(name, "transcript_path")) return jstr(json, "transcript_path",      buf, blen);
    if (!strcmp(name, "repo_host"))     return jstr(json, "workspace.repo.host",   buf, blen);
    if (!strcmp(name, "repo_owner"))    return jstr(json, "workspace.repo.owner",  buf, blen);
    if (!strcmp(name, "repo_name"))     return jstr(json, "workspace.repo.name",   buf, blen);
    if (!strcmp(name, "output_style"))  return jstr(json, "output_style.name",     buf, blen);
    if (!strcmp(name, "effort_level"))  return jstr(json, "effort.level",          buf, blen);
    if (!strcmp(name, "thinking_enabled")) {
        cJSON *node = jnav(json, "thinking.enabled");
        if (!node || !cJSON_IsTrue(node)) return 0;
        strncpy(buf, "🧠", blen - 1);
        buf[blen - 1] = '\0';
        return 1;
    }
    if (!strcmp(name, "exceeds_200k")) {
        cJSON *node = jnav(json, "exceeds_200k_tokens");
        if (!node || !cJSON_IsTrue(node)) return 0;
        strncpy(buf, "⚠️", blen - 1);
        buf[blen - 1] = '\0';
        return 1;
    }
    if (!strcmp(name, "random")) {
        /* $random[metric] - generate random value for a specific metric */
        if (arg[0]) {
            char *result = random_metric(json, arg);
            if (result) {
                strncpy(buf, result, blen - 1);
                buf[blen - 1] = '\0';
                return 1;
            }
            return 0;
        }
        /* $random without args - pick a random metric from the pool */
        const char *metric = get_random_metric();
        if (metric) {
            char *result = random_metric(json, metric);
            if (result) {
                strncpy(buf, result, blen - 1);
                buf[blen - 1] = '\0';
                return 1;
            }
        }
        return 0;
    }
    if (!strcmp(name, "api_duration")) {
        if (!jnum(json, "cost.total_api_duration_ms", &v) || v <= 0) return 0;
        long mins = (long)(v / 60000), secs = (long)(v / 1000) % 60;
        if (mins > 0) snprintf(buf, blen, "%ldm %lds", mins, secs);
        else          snprintf(buf, blen, "%lds", secs);
        return 1;
    }
    if (!strcmp(name, "input_tokens")) {
        if (!jnum(json, "context_window.total_input_tokens", &v)) return 0;
        snprintf(buf, blen, "%ld", (long)v); return 1;
    }
    if (!strcmp(name, "output_tokens")) {
        if (!jnum(json, "context_window.total_output_tokens", &v)) return 0;
        snprintf(buf, blen, "%ld", (long)v); return 1;
    }
    if (!strcmp(name, "context_size")) {
        if (!jnum(json, "context_window.context_window_size", &v)) return 0;
        snprintf(buf, blen, "%ld", (long)v); return 1;
    }
    if (!strcmp(name, "input_tokens_current")) {
        if (!jnum(json, "context_window.current_usage.input_tokens", &v)) return 0;
        snprintf(buf, blen, "%ld", (long)v); return 1;
    }
    if (!strcmp(name, "output_tokens_current")) {
        if (!jnum(json, "context_window.current_usage.output_tokens", &v)) return 0;
        snprintf(buf, blen, "%ld", (long)v); return 1;
    }
    if (!strcmp(name, "cache_creation_tokens")) {
        if (!jnum(json, "context_window.current_usage.cache_creation_input_tokens", &v)) return 0;
        snprintf(buf, blen, "%ld", (long)v); return 1;
    }
    if (!strcmp(name, "cache_read_tokens")) {
        if (!jnum(json, "context_window.current_usage.cache_read_input_tokens", &v)) return 0;
        snprintf(buf, blen, "%ld", (long)v); return 1;
    }
    if (!strcmp(name, "rate_5h_resets_at")) {
        if (!jnum(json, "rate_limits.five_hour.resets_at", &v)) return 0;
        time_t t = (time_t)v;
        struct tm *tm = localtime(&t);
        strftime(buf, blen, "%Y-%m-%d %H:%M", tm);
        return buf[0] != '\0';
    }
    if (!strcmp(name, "rate_7d_resets_at")) {
        if (!jnum(json, "rate_limits.seven_day.resets_at", &v)) return 0;
        time_t t = (time_t)v;
        struct tm *tm = localtime(&t);
        strftime(buf, blen, "%Y-%m-%d %H:%M", tm);
        return buf[0] != '\0';
    }

    /* numeric fields – formatted for display */
    if (!strcmp(name, "cost")) {
        if (!jnum(json, "cost.total_cost_usd", &v) || v <= 0.0001) return 0;
        snprintf(buf, blen, "$%.2f", v);
        return 1;
    }
    if (!strcmp(name, "duration")) {
        if (!jnum(json, "cost.total_duration_ms", &v) || v <= 0) return 0;
        long mins = (long)(v / 60000), secs = (long)(v / 1000) % 60;
        if (mins > 0) snprintf(buf, blen, "%ldm %lds", mins, secs);
        else          snprintf(buf, blen, "%lds", secs);
        return 1;
    }
    if (!strcmp(name, "lines_added")) {
        if (!jnum(json, "cost.total_lines_added", &v)) return 0;
        snprintf(buf, blen, "+%d", (int)v); return 1;
    }
    if (!strcmp(name, "lines_removed")) {
        if (!jnum(json, "cost.total_lines_removed", &v)) return 0;
        snprintf(buf, blen, "-%d", (int)v); return 1;
    }
    if (!strcmp(name, "context_pct")) {
        if (!jnum(json, "context_window.used_percentage", &v)) return 0;
        snprintf(buf, blen, "%d%%", (int)(v + 0.5)); return 1;
    }
    if (!strcmp(name, "context_bar")) {
        if (!jnum(json, "context_window.used_percentage", &v)) return 0;
        int pct = (int)(v + 0.5);
        int filled = pct > 100 ? 10 : pct * 10 / 100;
        size_t i = 0;
        buf[i++] = '[';
        for (int k = 0; k < 10 && i + 4 < blen; k++) {
            /* █ = \xe2\x96\x88  ░ = \xe2\x96\x91 */
            buf[i++] = '\xe2';
            buf[i++] = k < filled ? '\x96' : '\x96';
            buf[i++] = k < filled ? '\x88' : '\x91';
        }
        buf[i++] = ']'; buf[i] = '\0';
        snprintf(buf + i, blen - i, " %d%%", pct);
        return 1;
    }
    if (!strcmp(name, "context_remaining")) {
        if (!jnum(json, "context_window.remaining_percentage", &v)) return 0;
        snprintf(buf, blen, "%d%%", (int)(v + 0.5)); return 1;
    }
    if (!strcmp(name, "rate_5h")) {
        if (!jnum(json, "rate_limits.five_hour.used_percentage", &v)) return 0;
        snprintf(buf, blen, "%.0f%%", v); return 1;
    }
    if (!strcmp(name, "rate_7d")) {
        if (!jnum(json, "rate_limits.seven_day.used_percentage", &v)) return 0;
        snprintf(buf, blen, "%.0f%%", v); return 1;
    }

    return 0;
}

/* ── template: render one format line ────────────────────── *
 * Walks the format string char by char.                       *
 *   $field_name       → field value, no color                 *
 *   $field_name,COLOR → field value wrapped in COLOR…RESET    *
 * Literal text and unknown $names pass through unchanged.     */
static void render_template_line(const char *fmt, cJSON *json)
{
    const char *p = fmt;
    while (*p) {
        if (*p != '$') { putchar(*p++); continue; }

        const char *dollar = p++;          /* save '$', advance past it */
        const char *fs = p;
        while (*p && (isalnum((unsigned char)*p) || *p == '_')) p++;
        size_t flen = (size_t)(p - fs);

        if (!flen) { putchar('$'); continue; } /* lone '$' */

        char fname[64] = "";
        if (flen < sizeof fname) { memcpy(fname, fs, flen); fname[flen] = '\0'; }

        /* optional [arg] for fields like $random[metric] */
        char arg[256] = "";
        const char *arg_start = NULL;
        if (*p == '[') {
            const char *ae = p + 1;
            while (*ae && *ae != ']') ae++;
            size_t alen = (size_t)(ae - p - 1);
            if (alen > 0 && alen < sizeof arg) {
                memcpy(arg, p + 1, alen);
                arg[alen] = '\0';
                p = ae + 1; /* skip past ] */
            }
        }

        /* optional ,COLOR immediately after the field name */
        const char *color = NULL;
        const char *after = p;
        if (*p == ',') {
            const char *cs = p + 1, *ce = cs;
            while (*ce && isalpha((unsigned char)*ce)) ce++;
            size_t clen = (size_t)(ce - cs);
            if (clen && clen < 32) {
                char cname[32];
                memcpy(cname, cs, clen); cname[clen] = '\0';
                color = lookup_color(cname);
                if (color) after = ce;
            }
        }

        char val[512];
        if (resolve_field(json, fname, arg, val, sizeof val) && val[0]) {
            if (color) printf("%s%s" RESET, color, val);
            else       fputs(val, stdout);
        }
        p = after;
    }
    putchar('\n');
    fflush(stdout);
}

/* ── template: load file, skip comment/blank lines ────────── */
#define MAX_TMPL_LINES 32
#define MAX_LINE_LEN   512

typedef struct { char lines[MAX_TMPL_LINES][MAX_LINE_LEN]; int count; } Template;

static int load_template(const char *path, Template *t)
{
    t->count = 0;
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof line, f) && t->count < MAX_TMPL_LINES) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '#') continue;               /* skip comments & blanks */
        strncpy(t->lines[t->count], p, MAX_LINE_LEN - 1);
        t->lines[t->count++][MAX_LINE_LEN - 1] = '\0';
    }
    fclose(f);
    return t->count > 0;
}

/* ── default render (no template) ────────────────────────── */
static void render(cJSON *json)
{
    char cwd[512]="", model[64]="", vim_mode[32]="";
    char agent[64]="", wt_name[64]="", wt_branch[64]="", git_wt[64]="";
    double used_pct=0, cost=0, dur_ms=0, rate5h=0, rate7d=0;

    if (!jstr(json, "workspace.current_dir", cwd, sizeof cwd))
        jstr(json, "cwd", cwd, sizeof cwd);
    jstr(json, "model.display_name",     model,     sizeof model);
    jstr(json, "vim.mode",               vim_mode,  sizeof vim_mode);
    jstr(json, "agent.name",             agent,     sizeof agent);
    jstr(json, "worktree.name",          wt_name,   sizeof wt_name);
    jstr(json, "worktree.branch",        wt_branch, sizeof wt_branch);
    jstr(json, "workspace.git_worktree", git_wt,    sizeof git_wt);

    int has_pct  = jnum(json, "context_window.used_percentage",        &used_pct);
    int has_cost = jnum(json, "cost.total_cost_usd",                   &cost);
    int has_dur  = jnum(json, "cost.total_duration_ms",                &dur_ms);
    jnum(json, "rate_limits.five_hour.used_percentage", &rate5h);
    jnum(json, "rate_limits.seven_day.used_percentage", &rate7d);

    const char *folder = cwd[0] ? bname(cwd) : "~";
    const char *branch = wt_branch[0] ? wt_branch : git_wt[0] ? git_wt : NULL;
    int    pct    = has_pct ? (int)(used_pct + 0.5) : 0;
    int    filled = pct > 100 ? 10 : pct * 10 / 100;
    long   mins   = has_dur ? (long)(dur_ms / 60000) : 0;
    long   secs   = has_dur ? (long)(dur_ms / 1000) % 60 : 0;
    double rate   = rate5h > rate7d ? rate5h : rate7d;

    const char *bar_col = pct >= 90 ? RED : pct >= 70 ? YELLOW : GREEN;
    const char *vim_col = vim_mode[0] == 'I' ? GREEN :
                          vim_mode[0] == 'V' ? YELLOW :
                          vim_mode[0] == 'R' ? RED : BLUE;

    int first = 1;
#define SEP do { if (!first) fputs("  ", stdout); first = 0; } while (0)

    if (vim_mode[0])
        { SEP; printf("%s%s%c%s", vim_col, BOLD, vim_mode[0], RESET); }
    if (model[0])
        { SEP; printf("%s%s%s%s", CYAN, BOLD, model, RESET); }
    { SEP; printf("%s%s%s%s", WHITE, BOLD, folder, RESET); }
    if (branch)
        { SEP; printf("%s%s%s", GREEN, branch, RESET); }
    if (agent[0])
        { SEP; printf("%s[%s]%s", MAGENTA, agent, RESET); }
    if (has_pct) {
        SEP; printf("%s", bar_col); draw_bar(filled);
        printf(" %s%d%%%s", BOLD, pct, RESET);
    }
    if (has_cost && cost > 0.0001)
        { SEP; printf("%s$%.2f%s", YELLOW, cost, RESET); }
    if (has_dur && dur_ms > 0) {
        SEP; printf("%s", DIM);
        if (mins > 0) printf("%ldm ", mins);
        printf("%lds%s", secs, RESET);
    }
    if (rate >= 50.0)
        { SEP; printf("%sRL:%.0f%%%s", rate >= 80.0 ? RED : YELLOW, rate, RESET); }
    if (wt_name[0])
        { SEP; printf("%s%s%s%s", DIM, BLUE, wt_name, RESET); }

    putchar('\n');
    fflush(stdout);
}

/* ── help ─────────────────────────────────────────────────── */
static void print_help(void)
{
    puts("cstatus – Claude Code status line renderer\n");
    puts("Usage:");
    puts("  cat status.json | cstatus [--template <path>]");
    puts("  streaming-daemon | cstatus [--template <path>]\n");
    puts("Template file (default: ~/.claude/statusline.template):");
    puts("  Lines starting with '#' are comments and are ignored.");
    puts("  Each non-comment line becomes one line of output.");
    puts("  Syntax: free text with $field_name or $field_name,COLOR\n");
    puts("Available fields:");
    puts("  $current_dir           workspace.current_dir (or cwd), basename only");
    puts("  $project_dir           workspace.project_dir, basename only");
    puts("  $git_worktree          workspace.git_worktree");
    puts("  $model_name            model.display_name  (e.g. \"Opus\")");
    puts("  $model_id              model.id            (e.g. \"claude-opus-4-6\")");
    puts("  $session_id            session_id");
    puts("  $session_name          session_name");
    puts("  $version               version");
    puts("  $transcript_path       transcript_path");
    puts("  $cost                  cost.total_cost_usd            e.g. $0.05");
    puts("  $duration              cost.total_duration_ms         e.g. 1m 30s");
    puts("  $api_duration          cost.total_api_duration_ms     e.g. 2s");
    puts("  $lines_added           cost.total_lines_added         e.g. +42");
    puts("  $lines_removed         cost.total_lines_removed       e.g. -7");
    puts("  $context_pct           context_window.used_percentage e.g. 34%");
    puts("  $context_bar           graphical bar + pct            e.g. [████░░░░░░] 34%");
    puts("  $context_remaining     context_window.remaining_percentage");
    puts("  $input_tokens          context_window.total_input_tokens");
    puts("  $output_tokens         context_window.total_output_tokens");
    puts("  $context_size          context_window.context_window_size");
    puts("  $input_tokens_current  context_window.current_usage.input_tokens");
    puts("  $output_tokens_current context_window.current_usage.output_tokens");
    puts("  $cache_creation_tokens context_window.current_usage.cache_creation_input_tokens");
    puts("  $cache_read_tokens     context_window.current_usage.cache_read_input_tokens");
    puts("  $rate_5h               rate_limits.five_hour.used_percentage");
    puts("  $rate_7d               rate_limits.seven_day.used_percentage");
    puts("  $rate_5h_resets_at     rate_limits.five_hour.resets_at     (date/time)");
    puts("  $rate_7d_resets_at     rate_limits.seven_day.resets_at     (date/time)");
    puts("  $vim_mode              vim.mode  (NORMAL / INSERT / VISUAL / REPLACE)");
    puts("  $agent                 agent.name");
    puts("  $worktree              worktree.name");
    puts("  $worktree_path         worktree.path");
    puts("  $worktree_orig_cwd     worktree.original_cwd");
    puts("  $worktree_orig_branch  worktree.original_branch");
    puts("  $branch                worktree.branch (falls back to git_worktree)");
    puts("  $repo_host             workspace.repo.host");
    puts("  $repo_owner            workspace.repo.owner");
    puts("  $repo_name             workspace.repo.name");
    puts("  $output_style          output_style.name");
    puts("  $effort_level          effort.level  (low/medium/high/xhigh)");
    puts("  $thinking_enabled      thinking.enabled  (🧠 or empty)");
    puts("  $exceeds_200k          exceeds_200k_tokens  (⚠️ or empty)");
    puts("  $random                random emoji + phrase (rotates each call)");
    puts("\nAvailable colors:");
    printf("  " RESET   "RESET"   RESET "\n");
    printf("  " BOLD    "BOLD"    RESET "\n");
    printf("  " DIM     "DIM"     RESET "\n");
    printf("  " CYAN    "CYAN"    RESET "\n");
    printf("  " GREEN   "GREEN"   RESET "\n");
    printf("  " YELLOW  "YELLOW"  RESET "\n");
    printf("  " RED     "RED"     RESET "\n");
    printf("  " MAGENTA "MAGENTA" RESET "\n");
    printf("  " BLUE    "BLUE"    RESET "\n");
    printf("  " WHITE   "WHITE"   RESET "\n");
    puts("\nExample template lines:");
    puts("  $vim_mode,BLUE  $model_name,CYAN  $current_dir,WHITE  $branch,GREEN");
    puts("  Cost: $cost,YELLOW  Context: $context_pct,GREEN  $duration,DIM");
}

/* ── main ─────────────────────────────────────────────────── */
int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    /* resolve template path */
    const char *tmpl_path = NULL;
    char default_path[512] = "";
    if (argc > 2 && strcmp(argv[1], "--template") == 0) {
        tmpl_path = argv[2];
    } else {
        const char *home = getenv("HOME");
        if (home) snprintf(default_path, sizeof default_path,
                           "%s/.claude/statusline.template", home);
        if (default_path[0]) tmpl_path = default_path;
    }

    Template tmpl = {{{0}}, 0};
    int use_template = tmpl_path && load_template(tmpl_path, &tmpl);

    char *text;
    while ((text = read_object()) != NULL) {
        cJSON *json = cJSON_Parse(text);
        free(text);
        if (!json) { fputs("cstatus: invalid JSON\n", stderr); continue; }

        if (use_template)
            for (int i = 0; i < tmpl.count; i++)
                render_template_line(tmpl.lines[i], json);
        else
            render(json);

        cJSON_Delete(json);
    }
    return 0;
}
