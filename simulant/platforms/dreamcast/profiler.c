#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <kos/version.h>
#include <dc/fs_dcload.h>

#include <kos/irq.h>
#include <arch/irq.h>
#include <arch/timer.h>

static char OUTPUT_FILENAME[128];
static volatile bool PROFILER_RECORDING = false;

#define BASE_ADDRESS 0x8c010000

/* Power-of-two bucket count so the hash lookup is a mask rather than a
 * (45+ cycle) integer division inside the interrupt handler. */
#define BUCKET_SIZE 8192
#define BUCKET_MASK (BUCKET_SIZE - 1)

/* Sampling rate. 1 kHz gives ~1 sample per 200k cycles - fine enough to see
 * individual hot functions without meaningful overhead. */
#define SAMPLE_HZ 1000

/* Maximum number of distinct (pc, pr) pairs recorded. Static pool: the
 * profiler runs from an interrupt handler, so it must never call malloc(). */
#define MAX_ARC_COUNT 8192

/* Simple hash table of samples. An array of Samples
 * but, each sample in that array can be the head of
 * a linked list of other samples */
typedef struct Arc {
    uint32_t pc;
    uint32_t pr; // Caller return address
    uint32_t count;
    struct Arc* next;
} Arc;

static Arc ARCS[BUCKET_SIZE];
static Arc ARC_POOL[MAX_ARC_COUNT];
static size_t ARC_POOL_NEXT = 0;
static size_t ARC_COUNT = 0;

/* Hashing function for two uint32_ts */
#define HASH_PAIR(x, y) (((x) * 0x1f1f1f1f) ^ (y))

static bool WRITE_TO_STDOUT = false;

static bool write_samples(const char* path);
static bool write_samples_to_stdout();
static void clear_samples();

static Arc* new_arc(uint32_t PC, uint32_t PR) {
    if(ARC_POOL_NEXT >= MAX_ARC_COUNT) {
        return NULL; /* Pool exhausted: drop this sample. */
    }

    Arc* s = &ARC_POOL[ARC_POOL_NEXT++];
    s->count = 1;
    s->pc = PC;
    s->pr = PR;
    s->next = NULL;

    ++ARC_COUNT;

    return s;
}

/* Called from the TMU1 interrupt handler. Must be short and non-blocking:
 * no allocation, no I/O. Only the interrupt writes the table, so no locking
 * is required as long as the timer is stopped before it is read. */
static void record_thread(uint32_t PC, uint32_t PR) {
    uint32_t bucket = HASH_PAIR(PC, PR) & BUCKET_MASK;

    Arc* s = &ARCS[bucket];

    if(s->pc) {
        /* Initialized sample in this bucket, does it match though? */
        while(s->pc != PC || s->pr != PR) {
            if(s->next) {
                s = s->next;
            } else {
                s->next = new_arc(PC, PR);
                return; // We're done
            }
        }

        s->count++;
    } else {
        /* Initialize this sample */
        s->count = 1;
        s->pc = PC;
        s->pr = PR;
        s->next = NULL;
        ++ARC_COUNT;
    }
}

/* TMU1 underflow handler: record the interrupted PC, giving an instantaneous
 * sample of the code that was running. */
static void sampler_handler(irq_t source, irq_context_t* context, void* data) {
    (void) source;
    (void) data;

    record_thread(context->pc, context->pr);

    /* Clear the underflow bit so the next interrupt can fire. */
    timer_clear(TMU1);
}

/* Declared in KOS in fs_dcload.c */
#if KOS_VERSION_ABOVE(2, 1, 0)
int syscall_dcload_detected();
#else
int fs_dcload_detected();
#endif
extern int dcload_type;


#define GMON_COOKIE "gmon"
#define GMON_VERSION 1

typedef struct {
    char cookie[4];  // 'g','m','o','n'
    int32_t version; // 1
    char spare[3 * 4]; // Padding
} GmonHeader;

typedef struct {
    uint32_t low_pc;
    uint32_t high_pc;
    uint32_t hist_size;
    uint32_t prof_rate;
    char dimen[15];			/* phys. dim., usually "seconds" */
    char dimen_abbrev;			/* usually 's' for "seconds" */
} GmonHistHeader;

typedef struct {
    unsigned char tag; // GMON_TAG_TIME_HIST = 0, GMON_TAG_CG_ARC = 1, GMON_TAG_BB_COUNT = 2
    size_t ncounts; // Number of address/count pairs in this sequence
} GmonBBHeader;

typedef struct {
    uint32_t from_pc;	/* address within caller's body */
    uint32_t self_pc;	/* address within callee's body */
    uint32_t count;			/* number of arc traversals */
} GmonArc;

static bool init_sample_file(const char* path) {
    printf("Detecting dcload... ");

#if KOS_VERSION_ABOVE(2, 1, 0)
    if(!syscall_dcload_detected() || dcload_type == DCLOAD_TYPE_NONE) {
#else
    if(!fs_dcload_detected() || dcload_type == DCLOAD_TYPE_NONE) {
#endif
        printf("[Not Found]\n");
        WRITE_TO_STDOUT = true;
        return false;
    } else {
        printf("[Found]\n");
    }

    FILE* out = fopen(path, "w");
    if(!out) {
        WRITE_TO_STDOUT = true;
        return false;
    }

    /* Write the GMON header */

    GmonHeader header;
    memcpy(&header.cookie[0], GMON_COOKIE, sizeof(header.cookie));
    header.version = 1;
    memset(header.spare, '\0', sizeof(header.spare));

    fwrite(&header, sizeof(header), 1, out);

    fclose(out);
    return true;
}

#define ROUNDDOWN(x,y) (((x)/(y))*(y))
#define ROUNDUP(x,y) ((((x)+(y)-1)/(y))*(y))

static bool write_samples(const char* path) {
    /* Appends samples to the output file in gmon format.
     * gmon requires: GmonHeader (written at init) → histogram (tag=0) → arcs (tag=1).
     * We build the histogram bins first, write the histogram record, then write arcs. */

    if(WRITE_TO_STDOUT) {
        write_samples_to_stdout();
        return true;
    }

    extern char _etext;

    const uint32_t HISTFRACTION = 8;
    uint32_t lowest_address = ROUNDDOWN(BASE_ADDRESS, HISTFRACTION);
    uint32_t highest_address = ROUNDUP((uint32_t) &_etext, HISTFRACTION);

    const int BIN_COUNT = ((highest_address - lowest_address) / HISTFRACTION);
    uint32_t bin_size = (highest_address - lowest_address) / BIN_COUNT;

    uint16_t* bins = (uint16_t*) malloc(BIN_COUNT * sizeof(uint16_t));
    memset(bins, 0, sizeof(uint16_t) * BIN_COUNT);

    FILE* out = fopen(path, "a");  /* Append — GmonHeader already written by init_sample_file */
    if(!out) {
        fprintf(stderr, "-- Error writing samples to output file\n");
        free(bins);
        return false;
    }

    printf("-- Writing %d arcs\n", (int) ARC_COUNT);

    /* Pass 1: build histogram bins from arc PCs, weighted by sample count.
     * Samples can legitimately fall outside [lowest, highest) (e.g. inside the
     * KOS kernel at 0x8c000000, below the app's base address), so clamp rather
     * than indexing out of bounds. */
    Arc* root = ARCS;
    for(int i = 0; i < BUCKET_SIZE; ++i) {
        if(root->pc) {
            if(root->pc >= lowest_address && root->pc < highest_address) {
                bins[(root->pc - lowest_address) / bin_size] +=
                    (uint16_t) root->count;
            }
            Arc* s = root->next;
            while(s) {
                assert(s->pc);
                if(s->pc >= lowest_address && s->pc < highest_address) {
                    bins[(s->pc - lowest_address) / bin_size] +=
                        (uint16_t) s->count;
                }
                s = s->next;
            }
        }
        root++;
    }

    /* Write histogram record (tag=0) — must precede all arc records */
    GmonHistHeader hist_header;
    hist_header.low_pc = lowest_address;
    hist_header.high_pc = highest_address;
    hist_header.hist_size = BIN_COUNT;
    hist_header.prof_rate = SAMPLE_HZ;  /* Hz, not ms */
    strcpy(hist_header.dimen, "seconds");
    hist_header.dimen_abbrev = 's';

    unsigned char hist_tag = 0;
    fwrite(&hist_tag, sizeof(hist_tag), 1, out);
    fwrite(&hist_header, sizeof(hist_header), 1, out);
    fwrite(bins, sizeof(uint16_t), BIN_COUNT, out);
    free(bins);

    /* Pass 2: write arc records (tag=1). */
    uint8_t arc_tag = 1;

#ifndef NDEBUG
    size_t written = 0;
#endif

    root = ARCS;
    for(int i = 0; i < BUCKET_SIZE; ++i) {
        if(root->pc) {
            GmonArc arc;
            arc.from_pc = root->pr;
            arc.self_pc = root->pc;
            arc.count = root->count;

            fwrite(&arc_tag, sizeof(arc_tag), 1, out);
            fwrite(&arc, sizeof(GmonArc), 1, out);

#ifndef NDEBUG
            ++written;
#endif

            Arc* s = root->next;
            while(s) {
                arc.from_pc = s->pr;
                arc.self_pc = s->pc;
                arc.count = s->count;

                fwrite(&arc_tag, sizeof(arc_tag), 1, out);
                fwrite(&arc, sizeof(GmonArc), 1, out);

#ifndef NDEBUG
                ++written;
#endif

                s = s->next;
            }
        }
        root++;
    }

    fclose(out);

    assert(written == ARC_COUNT);

    clear_samples();

    return true;
}

static bool write_samples_to_stdout() {
    /* Write samples to stdout as a CSV file
     * for processing */

    printf("--------------\n");
    printf("\"PC\", \"PR\", \"COUNT\"\n");

    Arc* root = ARCS;
    for(int i = 0; i < BUCKET_SIZE; ++i) {
        Arc* s = root;
        while(s) {
            if(s->pc) {
                printf("\"%x\", \"%x\", \"%d\"\n", (unsigned int) s->pc, (unsigned int) s->pr, (unsigned int) s->count);
            }
            s = s->next;
        }

        root++;
    }

    printf("--------------\n");

    return true;
}

void profiler_init(const char* output) {
    /* Store the filename */
    strncpy(OUTPUT_FILENAME, output, sizeof(OUTPUT_FILENAME));

    /* Initialize the file */
    printf("Creating samples file...\n");
    if(!init_sample_file(OUTPUT_FILENAME)) {
        printf("Read-only filesytem. Writing samples to stdout\n");
    }

    /* Initialize the samples to zero */
    memset(ARCS, 0, sizeof(ARCS));
    ARC_POOL_NEXT = 0;
    ARC_COUNT = 0;
}

/* Turn the TMU1 sampling interrupt on. TMU1 is not used by KOS (TMU0 drives
 * the scheduler, TMU2 the wall clock), so it is safe to claim for profiling. */
static void sampler_enable(void) {
    irq_set_handler(EXC_TMU1_TUNI1, sampler_handler, NULL);
    timer_prime(TMU1, SAMPLE_HZ, 1);
    timer_clear(TMU1);
    timer_start(TMU1);
}

static void sampler_disable(void) {
    timer_stop(TMU1);
    timer_disable_ints(TMU1);
    timer_clear(TMU1);
    irq_set_handler(EXC_TMU1_TUNI1, NULL, NULL);
}

void profiler_start() {
    if(PROFILER_RECORDING) {
        return;
    }

    PROFILER_RECORDING = true;
    printf("Starting profiling...\n");
    sampler_enable();
}

static void clear_samples() {
    /* No heap allocation is used for arcs, so just reset the table and pool. */
    memset(ARCS, 0, sizeof(ARCS));
    ARC_POOL_NEXT = 0;
    ARC_COUNT = 0;
}

/* Drop all samples collected so far without writing them out. The sampler is
 * briefly disabled so it can't fire while the table is being cleared. */
void profiler_reset() {
    if(!PROFILER_RECORDING) {
        clear_samples();
        return;
    }

    sampler_disable();
    clear_samples();
    sampler_enable();
}

bool profiler_stop() {
    if(!PROFILER_RECORDING) {
        return false;
    }

    printf("Stopping profiling...\n");

    /* Stop the interrupt *before* reading/writing the table. */
    sampler_disable();
    PROFILER_RECORDING = false;

    if(!write_samples(OUTPUT_FILENAME)) {
        printf("ERROR WRITING SAMPLES (RO filesystem?)! Outputting to stdout\n");
        return false;
    }

    return true;
}

void profiler_clean_up() {
    profiler_stop(); // Make sure everything is stopped
}
