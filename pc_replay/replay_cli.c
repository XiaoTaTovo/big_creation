#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "pd_replay_model.h"

/* The firmware header is the sole owner of event/state definitions and reducer. */
#define MAX_EVENTS 4096U
#define EVENT_HEADER "message,sender,id,t8,crc_ok,len_ok,ndo,data0,data1,data2,data3,data4"
static PdReplayEvent events[MAX_EVENTS];
static unsigned event_count;
static const char *messages[] = {"Source_Capabilities", "GoodCRC", "Request", "Accept", "PS_RDY"};
static const char *phases[] = {"WAITING", "HAVE_CAPS", "REQUESTED", "ACCEPTED", "READY"};

static int number(const char *s, int base, unsigned long limit, unsigned long *value)
{
    char *end;
    if(!*s || *s == '-' || *s == '+' || *s == ' ' || *s == '\t') return 0;
    errno = 0;
    *value = strtoul(s, &end, base);
    return errno == 0 && *end == '\0' && *value <= limit;
}

/* v1 CSV intentionally has no quoting/empty cells; reject ambiguity, never guess. */
static int read_events(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[512];
    unsigned line_no = 0;
    if(!f) { perror(path); return 0; }
    event_count = 0;
    while(fgets(line, sizeof(line), f)) {
        line_no++;
        if(!strchr(line, '\n') && !feof(f)) goto invalid;
        line[strcspn(line, "\r\n")] = '\0';
        if(line_no == 1) {
            if(strcmp(line, EVENT_HEADER)) goto invalid;
            continue;
        }
        if(event_count == MAX_EVENTS) goto invalid;
        char *cells[12];
        char *p = line;
        for(unsigned i = 0; i < 12; i++) {
            cells[i] = p;
            char *comma = strchr(p, ',');
            if(i < 11) {
                if(!comma) goto invalid;
                *comma = '\0';
                p = comma + 1;
            }
            else if(comma) goto invalid;
            if(!*cells[i]) goto invalid;
        }
        PdReplayEvent e = {0};
        unsigned m;
        for(m = 0; m < 5 && strcmp(cells[0], messages[m]); m++) {}
        if(m == 5) goto invalid;
        e.message = (PdMessage)m;
        if(!strcmp(cells[1], "SRC")) e.sender = PD_SRC;
        else if(!strcmp(cells[1], "SNK")) e.sender = PD_SNK;
        else goto invalid;
        unsigned long v[10];
        const unsigned long limits[] = {7,255,1,1,5,0xffffffffUL,0xffffffffUL,0xffffffffUL,0xffffffffUL,0xffffffffUL};
        for(unsigned i = 0; i < 10; i++)
            if(!number(cells[i+2], i < 5 ? 10 : 16, limits[i], &v[i])) goto invalid;
        e.id = (uint8_t)v[0]; e.t8 = (uint8_t)v[1];
        e.crc_ok = (uint8_t)v[2]; e.len_ok = (uint8_t)v[3]; e.ndo = (uint8_t)v[4];
        for(unsigned i = 0; i < 5; i++) {
            e.data[i] = (uint32_t)v[i+5];
            if(i >= e.ndo && e.data[i] != 0) goto invalid;
        }
        events[event_count++] = e;
    }
    if(ferror(f) || line_no == 0 || event_count == 0) goto invalid;
    fclose(f);
    return 1;
invalid:
    fprintf(stderr, "Invalid event CSV at line %u (v1, 12 columns, max %u events).\n", line_no, MAX_EVENTS);
    fclose(f);
    return 0;
}

static int save_events(const char *path)
{
    /* Exclusive creation protects previous exports and user input files. */
    FILE *f = fopen(path, "wx");
    if(!f) { perror(path); return 0; }
    fprintf(f, "%s\n", EVENT_HEADER);
    for(unsigned n = 0; n < event_count; n++) {
        const PdReplayEvent *e = &events[n];
        fprintf(f, "%s,%s,%u,%u,%u,%u,%u", messages[e->message],
                e->sender == PD_SRC ? "SRC" : "SNK", (unsigned)e->id,
                (unsigned)e->t8, (unsigned)e->crc_ok, (unsigned)e->len_ok, (unsigned)e->ndo);
        for(unsigned i = 0; i < 5; i++) fprintf(f, ",%08lX", (unsigned long)e->data[i]);
        fputc('\n', f);
    }
    int ok = !ferror(f);
    return fclose(f) == 0 && ok;
}

static void show_state(const PdDisplayState *s)
{
    printf("Phase: %s\n", phases[s->phase]);
    if(s->request_valid)
        printf("Request: PDO %u, %u mV, operating %u mA, maximum %u mA\n",
               (unsigned)s->pdo_index, (unsigned)s->voltage_mv,
               (unsigned)s->operating_ma, (unsigned)s->maximum_ma);
    else puts("Request: --");
    printf("Frames: %u, CRC/length bad: %u\n", (unsigned)s->frames, (unsigned)s->bad_frames);
}

static void step(PdDisplayState *s, unsigned index, FILE *trace)
{
    const PdReplayEvent *e = &events[index];
    printf("\n#%u %s sender=%s id=%u t8=%u (unit unknown)\n", index+1,
           messages[e->message], e->sender == PD_SRC ? "SRC" : "SNK",
           (unsigned)e->id, (unsigned)e->t8);
    printf("Logger checks: CRC=%s, length=%s\n", e->crc_ok ? "OK" : "BAD", e->len_ok ? "OK" : "BAD");
    for(unsigned i = 0; i < e->ndo; i++) printf("  Object %u: 0x%08lX\n", i+1, (unsigned long)e->data[i]);
    pd_display_apply(s, e);
    show_state(s);
    if(trace) {
        fprintf(trace, "%u,%s,%s,%u,%u,%u,%u,%s,%u,", index+1, messages[e->message],
                e->sender == PD_SRC ? "SRC" : "SNK", (unsigned)e->id, (unsigned)e->t8,
                (unsigned)e->crc_ok, (unsigned)e->len_ok, phases[s->phase], (unsigned)s->request_valid);
        if(s->request_valid)
            fprintf(trace, "%u,%u,%u,%u", (unsigned)s->pdo_index, (unsigned)s->voltage_mv,
                    (unsigned)s->operating_ma, (unsigned)s->maximum_ma);
        else fprintf(trace, ",,,");
        fprintf(trace, ",%u,%u\n", (unsigned)s->frames, (unsigned)s->bad_frames);
    }
}

int main(int argc, char **argv)
{
    const char *input = NULL, *save = NULL, *export = NULL;
    int automatic = 0, bad_request = 0;
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--auto")) automatic = 1;
        else if(!strcmp(argv[i], "--bad-request")) bad_request = 1;
        else if(!strcmp(argv[i], "--input") && i+1 < argc) input = argv[++i];
        else if(!strcmp(argv[i], "--save-events") && i+1 < argc) save = argv[++i];
        else if(!strcmp(argv[i], "--export") && i+1 < argc) export = argv[++i];
        else {
            puts("Usage: replay_cli [--auto] [--bad-request] [--input events.csv] [--save-events new.csv] [--export new-trace.csv]");
            return !strcmp(argv[i], "--help") ? 0 : 2;
        }
    }
    if(input) { if(!read_events(input)) return 2; }
    else {
        event_count = (unsigned)PD_REPLAY_COUNT;
        memcpy(events, pd_replay_events, sizeof(pd_replay_events));
    }
    puts("PD replay schema v1 | CC1/SOP fixture | t8 unit UNKNOWN | request values, NOT measurements");
    printf("Source: %s\n", input ? input : "1022 transcribed built-in events");
    if(bad_request) {
        unsigned i;
        for(i = 0; i < event_count; i++) {
            if(events[i].message == PD_REQUEST) { events[i].crc_ok = 0; break; }
        }
        if(i == event_count) { fputs("No Request available for injection.\n", stderr); return 2; }
        puts("SIMULATED FAULT: first Request CRC forced BAD; original capture unchanged.");
    }
    if(save && !save_events(save)) return 2;
    FILE *trace = NULL;
    if(export) {
        trace = fopen(export, "wx");
        if(!trace) { perror(export); return 2; }
        fputs("sequence,message,sender,id,t8_raw,crc_ok,len_ok,phase,request_valid,pdo_index,voltage_mv,operating_ma,maximum_ma,frames,bad_frames\n", trace);
        automatic = 1;
    }
    PdDisplayState state;
    pd_display_reset(&state);
    show_state(&state);
    unsigned next = 0;
    if(automatic) {
        while(next < event_count) { step(&state, next, trace); next++; }
    }
    else {
        char command[64];
        puts("Enter/n: next | a: remaining frames | r: reset | q: quit");
        while(1) {
            printf("\n> "); fflush(stdout);
            if(!fgets(command, sizeof(command), stdin) || command[0] == 'q') break;
            if(command[0] == 'r') { next = 0; pd_display_reset(&state); show_state(&state); continue; }
            if(command[0] == 'a') {
                while(next < event_count) { step(&state, next, NULL); next++; }
            }
            else if(command[0] == '\n' || command[0] == 'n') {
                if(next < event_count) { step(&state, next, NULL); next++; }
                else puts("End of replay. Use r to restart.");
            }
            else puts("Use Enter/n, a, r or q.");
        }
    }
    if(trace) {
        int ok = !ferror(trace);
        if(fclose(trace) || !ok) { fputs("Export failed.\n", stderr); return 2; }
    }
    return 0;
}
