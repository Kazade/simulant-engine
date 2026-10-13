/*
 * adxtract - v1.0
 *
 * Public domain ADX decoder. No warranty implied - use at your own risk.
 *
 * This stb-style single file library is designed for all platforms but is optimised
 * for the Dreamcast SH4 CPU. I wrote it to learn how the ADX file decompression
 * worked and to add ADX support to [Simulant](simulant.dev)
 *
 * If you'd like a library that decodes in a background thread consider
 * [libadx](https://github.com/sega-dreamcast/libadx/)
 *
 * USAGE
 *
 * #define ADXTRACT_IMPLEMENTATION  # in a single source file
 * #include "adxtract.h"
 *
 * LICENSE
 *
 * See end of file for license information
 *
 * AI DISCLAIMER
 *
 * The core of this library was written by a human (API + initial implementation)
 * but then iteratively optimised with the use of an LLM. In particular the SH4 assembly is
 * written entirely by an LLM.
 *
 * REVISION HISTORY
 *
 * - 1.0 (2026-10-13) - initial release
 */

#ifndef _ADXTRACT_H
#define _ADXTRACT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _adxtract_stream adxtract_stream_t;

// Streams from a file, which is closed by adx_stream_close
adxtract_stream_t* adx_load_stream(FILE* file);

// Streams from a file that is already in memory. The buffer isn't copied, so it
// must stay valid until adx_stream_close (which won't free it)
adxtract_stream_t* adx_read_buffer(const uint8_t* buffer, size_t size);

size_t adx_stream_sample_rate(const adxtract_stream_t* stream);
size_t adx_stream_channel_count(const adxtract_stream_t* stream);

// Looping is off by default. When it's enabled on a stream that has a loop,
// adx_stream_read_samples will never reach the end of the stream
bool adx_stream_has_loop(const adxtract_stream_t* stream);
void adx_stream_set_looping(adxtract_stream_t* stream, bool enabled);

// Restarts playback from the beginning of the stream
void adx_stream_rewind(adxtract_stream_t* stream);

// Samples are signed 16-bit PCM in host byte order. Returns the number of samples
// read per channel, which is zero at the end of the stream
size_t adx_stream_read_samples(adxtract_stream_t* source, bool interleaved, size_t max_samples, int16_t* buffer);

void adx_stream_close(adxtract_stream_t* source);

#ifdef __cplusplus
}
#endif

#endif

#ifdef ADXTRACT_IMPLEMENTATION

#include <math.h>
#include <stddef.h>

#define ADXTRACT_MAX_CHANNELS 2

// Tells the compiler which way a branch normally goes, so that the common
// path doesn't have to jump
#if defined(__GNUC__) || defined(__clang__)
#define ADXTRACT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define ADXTRACT_UNLIKELY(x) (x)
#endif

// On the Dreamcast the inner decoding loop is written in SH4 assembly. Define
// ADXTRACT_NO_ASM to use the C version instead
#if defined(__DREAMCAST__) && !defined(ADXTRACT_NO_ASM)
#define ADXTRACT_SH4_ASM
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _adx_header_v3 {
    uint16_t loop_alignment_samples;
    uint16_t loop_enabled1;
    uint32_t loop_enabled2;
    uint32_t loop_begin_sample_index;
    uint32_t loop_begin_byte_index;
    uint32_t loop_end_sample_index;
    uint32_t loop_end_byte_index;
} adx_header_v3_t;

typedef struct _adx_header_v4 {
    uint16_t unused0;
    uint16_t unused1;
    // The previous two samples of each channel, to start decoding from
    int16_t history[ADXTRACT_MAX_CHANNELS][2];
    uint32_t unused4;
    uint32_t loop_enabled;
    uint32_t loop_begin_sample_index;
    uint32_t loop_begin_byte_index;
    uint32_t loop_end_sample_index;
    uint32_t loop_end_byte_index;
} adx_header_v4_t;

typedef struct _adx_header {
    uint16_t id;
    uint16_t copyright_offset;
    uint8_t encoding_type;
    uint8_t block_size;
    uint8_t sample_depth;
    uint8_t channel_count;
    uint32_t sample_rate;
    uint32_t total_samples;
    uint16_t highpass_freq;
    uint8_t version;
    uint8_t flags;

    union {
        adx_header_v3_t v3;
        adx_header_v4_t v4;
    };
} adx_header_t;

typedef struct _adxtract_block {
    uint8_t predictor;
    uint8_t scale;

    // 32x 4-bit samples
    uint8_t samples[16];
} adxtract_block_t;

struct _adxtract_stream {
    // The data comes from either a file, or a buffer in memory (if file is NULL)
    FILE* file;
    const uint8_t* buffer;
    size_t buffer_size;
    size_t buffer_offset;

    adx_header_t header;
    char copyright[6];

    size_t audio_start;
    size_t current_block;
    size_t current_sample;

    // The number of samples played so far, which goes backwards when looping
    size_t position;

    // The blocks for each channel at current_block are read once and kept here.
    // frame_block is the block they were read from, and file_block is where the
    // file position is, so that we only seek when not reading sequentially
    adxtract_block_t frame[ADXTRACT_MAX_CHANNELS];
    size_t frame_block;
    size_t file_block;

    // Prediction filter coefficients, and the previous two decoded samples
    // of each channel. These are floats so that the decode loop stays in the FPU
    float coef1;
    float coef2;
    float history[ADXTRACT_MAX_CHANNELS][2];

    // The loop points in samples (the end is exclusive), and the history as it
    // was at the start of the loop so that it can be restored when jumping back
    bool has_loop;
    bool looping;
    size_t loop_begin;
    size_t loop_end;
    float loop_history[ADXTRACT_MAX_CHANNELS][2];
};

size_t adx_stream_channel_count(const adxtract_stream_t* stream) {
    return stream->header.channel_count;
}

size_t adx_stream_sample_rate(const adxtract_stream_t* stream) {
    return stream->header.sample_rate;
}

bool adx_stream_has_loop(const adxtract_stream_t* stream) {
    return stream->has_loop;
}

void adx_stream_set_looping(adxtract_stream_t* stream, bool enabled) {
    stream->looping = enabled && stream->has_loop;
}

void adx_stream_rewind(adxtract_stream_t* stream) {
    stream->current_block = 0;
    stream->current_sample = 0;
    stream->position = 0;

    // Version 4 files store the history to start decoding with, otherwise it's zero
    memset(stream->history, 0, sizeof(stream->history));

    if(stream->header.version == 4) {
        for(int i = 0; i < ADXTRACT_MAX_CHANNELS; ++i) {
            stream->history[i][0] = stream->header.v4.history[i][0];
            stream->history[i][1] = stream->header.v4.history[i][1];
        }
    }
}

void adx_stream_close(adxtract_stream_t* stream) {
    if(!stream) {
        return;
    }

    if(stream->file) {
        fclose(stream->file);
    }

    free(stream);
}

// These behave like fseek and fread, but work on whichever of the file or
// the memory buffer that the stream is using. Seeking returns false if the
// file can't seek (a pipe for example)
static bool _adx_seek(adxtract_stream_t* stream, size_t offset) {
    if(stream->file) {
        return fseek(stream->file, (long) offset, SEEK_SET) == 0;
    }

    stream->buffer_offset = offset < stream->buffer_size ? offset : stream->buffer_size;
    return true;
}

static size_t _adx_read(adxtract_stream_t* stream, void* out, size_t size, size_t count) {
    if(stream->file) {
        return fread(out, size, count, stream->file);
    }

    // Like fread, only whole items are read
    size_t available = (stream->buffer_size - stream->buffer_offset) / size;
    if(count > available) {
        count = available;
    }

    memcpy(out, stream->buffer + stream->buffer_offset, size * count);
    stream->buffer_offset += size * count;

    return count;
}

// Moves forwards by reading, so that it works on files that can't seek
static void _adx_skip(adxtract_stream_t* stream, size_t count) {
    if(!stream->file) {
        _adx_seek(stream, stream->buffer_offset + count);
        return;
    }

    uint8_t discard[64];
    while(count) {
        size_t chunk = count < sizeof(discard) ? count : sizeof(discard);
        if(_adx_read(stream, discard, 1, chunk) != chunk) {
            break;
        }

        count -= chunk;
    }
}

// ADX files are big-endian. These take a value as it was read from the file
// and return it in host byte order
static uint16_t _adx_be16(uint16_t v) {
    const uint8_t* b = (const uint8_t*) &v;
    return (uint16_t) ((b[0] << 8) | b[1]);
}

static uint32_t _adx_be32(uint32_t v) {
    const uint8_t* b = (const uint8_t*) &v;
    return ((uint32_t) b[0] << 24) | ((uint32_t) b[1] << 16) | ((uint32_t) b[2] << 8) | b[3];
}

static void _adx_header_to_host(adx_header_t* header) {
    header->id = _adx_be16(header->id);
    header->copyright_offset = _adx_be16(header->copyright_offset);
    header->sample_rate = _adx_be32(header->sample_rate);
    header->total_samples = _adx_be32(header->total_samples);
    header->highpass_freq = _adx_be16(header->highpass_freq);

    if(header->version == 3) {
        header->v3.loop_alignment_samples = _adx_be16(header->v3.loop_alignment_samples);
        header->v3.loop_enabled1 = _adx_be16(header->v3.loop_enabled1);
        header->v3.loop_enabled2 = _adx_be32(header->v3.loop_enabled2);
        header->v3.loop_begin_sample_index = _adx_be32(header->v3.loop_begin_sample_index);
        header->v3.loop_begin_byte_index = _adx_be32(header->v3.loop_begin_byte_index);
        header->v3.loop_end_sample_index = _adx_be32(header->v3.loop_end_sample_index);
        header->v3.loop_end_byte_index = _adx_be32(header->v3.loop_end_byte_index);
    } else if(header->version == 4) {
        for(int i = 0; i < ADXTRACT_MAX_CHANNELS; ++i) {
            header->v4.history[i][0] = (int16_t) _adx_be16((uint16_t) header->v4.history[i][0]);
            header->v4.history[i][1] = (int16_t) _adx_be16((uint16_t) header->v4.history[i][1]);
        }

        header->v4.loop_enabled = _adx_be32(header->v4.loop_enabled);
        header->v4.loop_begin_sample_index = _adx_be32(header->v4.loop_begin_sample_index);
        header->v4.loop_begin_byte_index = _adx_be32(header->v4.loop_begin_byte_index);
        header->v4.loop_end_sample_index = _adx_be32(header->v4.loop_end_sample_index);
        header->v4.loop_end_byte_index = _adx_be32(header->v4.loop_end_byte_index);
    }
}

// Reads the header and prepares the stream for decoding. The stream must already
// have its file or buffer set. If the data isn't supported the stream is freed
// and NULL is returned
static adxtract_stream_t* _adx_init_stream(adxtract_stream_t* stream) {
    // Start from the beginning if we can. A file that can't seek is read from
    // where it is, and nothing below goes backwards
    _adx_seek(stream, 0);

    // The id and the copyright offset come first
    const size_t prefix_size = offsetof(adx_header_t, encoding_type);
    if(_adx_read(stream, &stream->header, prefix_size, 1) != 1) {
        free(stream);
        return NULL;
    }

    // All ADX files start with 0x8000, and need at least the 20 byte header
    // that is common to all versions before the copyright string
    if(_adx_be16(stream->header.id) != 0x8000 || _adx_be16(stream->header.copyright_offset) < 20 + 2) {
        free(stream);
        return NULL;
    }

    // The header might not be a full header, or it might be padded beyond the
    // size of ours. The copyright offset is also 2 bytes after the copyright
    // begins weirdly
    size_t copyright_start = _adx_be16(stream->header.copyright_offset) - 2;
    size_t header_size = copyright_start < sizeof(adx_header_t) ? copyright_start : sizeof(adx_header_t);

    _adx_read(stream, (uint8_t*) &stream->header + prefix_size, header_size - prefix_size, 1);
    _adx_skip(stream, copyright_start - header_size);
    _adx_read(stream, &stream->copyright, 6, 1);

    _adx_header_to_host(&stream->header);

    if(memcmp(stream->copyright, "(c)CRI", 6) != 0) {
        free(stream);
        return NULL;
    }

    // Only standard ADX with 18 byte blocks is supported (the other encoding
    // types are fixed coefficient ADX, exponential scale ADX and AHX)
    if(stream->header.encoding_type != 3 || stream->header.block_size != sizeof(adxtract_block_t)) {
        free(stream);
        return NULL;
    }

    if(!stream->header.channel_count || !stream->header.sample_rate) {
        free(stream);
        return NULL;
    }

    // The highpass frequency has to be below Nyquist, which also keeps the sine
    // approximation for the coefficients (see below) within the range it's accurate for
    if((uint32_t) stream->header.highpass_freq * 2 >= stream->header.sample_rate) {
        free(stream);
        return NULL;
    }

    if(stream->header.channel_count > ADXTRACT_MAX_CHANNELS) {
        free(stream);
        return NULL;
    }

    // Version 4 files with flags set are encrypted, which isn't supported
    if(stream->header.version != 3 && (stream->header.version != 4 || stream->header.flags)) {
        free(stream);
        return NULL;
    }

    // Files that don't loop often have a header that's too short to hold the loop data
    bool loop_enabled = false;
    size_t loop_data_end = 0;

    if(stream->header.version == 4) {
        // An "AINF" chunk can sit where the loop data would otherwise be
        loop_enabled = stream->header.v4.loop_enabled && stream->header.v4.loop_enabled != 0x41494E46;
        loop_data_end = offsetof(adx_header_t, v4) + sizeof(stream->header.v4);
        stream->loop_begin = stream->header.v4.loop_begin_sample_index;
        stream->loop_end = stream->header.v4.loop_end_sample_index;
    } else {
        loop_enabled = stream->header.v3.loop_enabled2;
        loop_data_end = offsetof(adx_header_t, v3) + sizeof(stream->header.v3);
        stream->loop_begin = stream->header.v3.loop_begin_sample_index;
        stream->loop_end = stream->header.v3.loop_end_sample_index;
    }

    // Ignore loops that don't make sense, otherwise we'd never make progress
    stream->has_loop = loop_enabled && copyright_start >= loop_data_end && stream->loop_begin < stream->loop_end && stream->loop_end <= stream->header.total_samples;

    // The coefficients come from c = (a - sqrt((a + b) * (a - b))) / b, where
    // a = sqrt(2) - cos(2 * pi * highpass / sample_rate) and b = sqrt(2) - 1.
    //
    // We don't use cosf because when fast-math is enabled on SH4 that converts to fsca which isn't
    // accurate enough (introduces drift)
    const double half_angle = 3.14159265358979323846 * stream->header.highpass_freq / stream->header.sample_rate;
    double sine = half_angle;
    double term = half_angle;
    for(int i = 1; i < 16; ++i) {
        term *= -(half_angle * half_angle) / ((2 * i) * (2 * i + 1));
        sine += term;
    }

    const double b = 1.41421356237309504880 - 1.0;
    const double a_minus_b = 2.0 * sine * sine;
    const double c = (b + a_minus_b - sqrt((b + b + a_minus_b) * a_minus_b)) / b;

    // The coefficients are quantised to 12 bits to match what the encoder used
    stream->coef1 = (float) (int32_t) (c * 8192.0) / 4096.0f;
    stream->coef2 = (float) (int32_t) (c * c * -4096.0) / 4096.0f;

    // The audio follows the copyright string, which is where we've just read up to
    stream->audio_start = copyright_start + 6;

    // Nothing has been read yet
    stream->frame_block = (size_t) -1;

    adx_stream_rewind(stream);

    return stream;
}

adxtract_stream_t* adx_load_stream(FILE* file) {
    adxtract_stream_t* stream = (adxtract_stream_t*) calloc(1, sizeof(adxtract_stream_t));
    if(!stream) {
        return NULL;
    }

    stream->file = file;

    return _adx_init_stream(stream);
}

adxtract_stream_t* adx_read_buffer(const uint8_t* buffer, size_t size) {
    adxtract_stream_t* stream = (adxtract_stream_t*) calloc(1, sizeof(adxtract_stream_t));
    if(!stream) {
        return NULL;
    }

    stream->buffer = buffer;
    stream->buffer_size = size;

    return _adx_init_stream(stream);
}


// Decodes a single sample. Delta is the nibble's value already multiplied by the
// block's scale, and the history is biased by 32768 (see _adx_read_block)
static inline void _adx_decode_sample(float delta, float coef1, float coef2, float* hist1, float* hist2, int16_t** buffer, size_t stride, bool odd) {
#ifdef ADXTRACT_SH4_ASM
    // This has to round in exactly the same way as the assembly loop in _adx_read_block
    // does, so that the output doesn't depend on how the reads are split up. That means
    // using fipr, with the history in the same order as the loop has it for this sample
    float vectors[8] = {
        1.0f, odd ? coef2 : coef1, odd ? coef1 : coef2, 0.0f,
        delta, odd ? *hist2 : *hist1, odd ? *hist1 : *hist2, 0.0f
    };

    register float* r_vectors __asm__("r1") = vectors;
    register int32_t r_sample __asm__("r0");

    __asm__ volatile(
        "fmov.s @r1+,fr0\n\t"
        "fmov.s @r1+,fr1\n\t"
        "fmov.s @r1+,fr2\n\t"
        "fmov.s @r1+,fr3\n\t"
        "fmov.s @r1+,fr4\n\t"
        "fmov.s @r1+,fr5\n\t"
        "fmov.s @r1+,fr6\n\t"
        "fmov.s @r1+,fr7\n\t"
        "fipr fv0,fv4\n\t"
        "ftrc fr7,fpul\n\t"
        "sts fpul,r0\n\t"
        : "=r"(r_sample), "+r"(r_vectors)
        :
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fpul", "memory"
    );

    int32_t out = r_sample;
#else
    (void) odd;

    float sample = delta;
    sample += coef2 * *hist2;
    sample += coef1 * *hist1;

    // The integer decoders round the prediction down, and without matching that the
    // error builds up in the filter. Float to int conversion truncates towards zero,
    // but the bias keeps the sum positive which makes the truncation a floor. Anything
    // that is still negative is about to be clamped anyway
    int32_t out = (int32_t) sample;
#endif
    float hist = (float) out;

    // Both ends of the clamp are covered by a single rarely taken branch
    if(ADXTRACT_UNLIKELY((uint32_t) out > 65535)) {
        out = ~(out >> 31) & 65535;
        hist = (float) out;
    }

    *hist2 = *hist1;
    *hist1 = hist;

    // Remove the bias
    **buffer = (int16_t) (out - 32768);
    *buffer += stride;
}

// Decompress the block for a single channel into the buffer. Buffer should point
// at the first output sample for this channel, and stride is the distance between
// its samples (the channel count if interleaving, otherwise 1)
static size_t _adx_read_block(adxtract_stream_t* source, const adxtract_block_t* block, size_t channel, size_t start_sample, size_t max_samples, int16_t* buffer, size_t stride) {
    // Sign-extended 4-bit values as floats
    static const float nibbles[16] = {
        0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
        -8.0f, -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f
    };

    // Everything the loop needs is copied into locals so that it lives in registers
    // and the loop only touches the block, the table below and the output
    const float coef1 = source->coef1;
    const float coef2 = source->coef2;
    const float bias = 32768.0f;
    // CRI's tools add one to the scale, but ffmpeg doesn't and that's what
    // files are most likely to have been encoded with
    const float scale = (float) ((block->predictor << 8) | block->scale);

    // While decoding, samples are kept biased by 32768 (so 0..65535) which lets the
    // truncated result go straight back into the filter. The filter needs a constant
    // correction for that, which is folded into this table of every value that a
    // nibble can decode to in this block. At 64 bytes it is only 2 cache lines
    const float correction = bias * (1.0f - coef1 - coef2);
    float deltas[16];
    for(int i = 0; i < 16; ++i) {
        deltas[i] = nibbles[i] * scale + correction;
    }

    float* history = source->history[channel];
    float hist1 = history[0] + bias;
    float hist2 = history[1] + bias;

    // Each byte holds two samples with the high nibble first. Whole bytes are
    // decoded in pairs, with an odd sample either side if the read doesn't
    // start or end on a byte boundary
    const uint8_t* in = block->samples + (start_sample >> 1);
    size_t remaining = max_samples;

    if((start_sample & 1) && remaining) {
        _adx_decode_sample(deltas[*in++ & 0xF], coef1, coef2, &hist1, &hist2, &buffer, stride, true);
        remaining--;
    }

    size_t pairs = remaining >> 1;

#ifdef ADXTRACT_SH4_ASM
    if(pairs) {
        /*
         * Each sample is one fipr (a 4 element dot product), which does the whole of
         * delta + (coef1 * hist1) + (coef2 * hist2) in a single instruction:
         *
         *   fv4 = {delta, history, history, previous result}
         *   fv0 = {1, coef1, coef2, 0}  for the first sample of each byte
         *   fv8 = {1, coef2, coef1, 0}  for the second sample of each byte
         *
         * The result goes into fr7, is truncated, and is then converted back to a float
         * directly over the older of the two history values. That swaps which register
         * holds the newest sample, which is why there are two coefficient vectors.
         *
         * The SH4 executes in order, so the instructions that don't depend on the fipr
         * (looking up the next delta, storing the previous sample) are placed straight
         * after it to run while it completes.
         *
         * r0 = scratch          r1 = deltas          r2 = input
         * r3 = output           r4 = output stride   r5 = sample
         * r6 = 65535            r7 = 0x8000          r8 = current byte
         * r9 = pairs remaining  r10 = state
         */
        float state[4] = {coef1, coef2, hist1, hist2};

        register const float* r_deltas __asm__("r1") = deltas;
        register const uint8_t* r_in __asm__("r2") = in;
        register int16_t* r_out __asm__("r3") = buffer;
        register size_t r_stride __asm__("r4") = stride * sizeof(int16_t);
        register uint32_t r_max __asm__("r6") = 65535;
        register uint32_t r_flip __asm__("r7") = 0x8000;
        register size_t r_pairs __asm__("r9") = pairs;
        register float* r_state __asm__("r10") = state;

        __asm__ volatile(
            "fldi1   fr0\n\t"
            "fmov.s  @r10+,fr1\n\t"
            "fmov.s  @r10+,fr2\n\t"
            "fldi0   fr3\n\t"
            "fldi1   fr8\n\t"
            "fmov    fr2,fr9\n\t"
            "fmov    fr1,fr10\n\t"
            "fldi0   fr11\n\t"
            "fmov.s  @r10+,fr5\n\t"
            "fmov.s  @r10,fr6\n\t"
            "fldi0   fr7\n\t"

            // Look up the delta for the high nibble of the first byte
            "mov.b   @r2+,r0\n\t"
            "extu.b  r0,r0\n\t"
            "mov     r0,r8\n\t"
            "shlr2   r0\n\t"
            "and     #60,r0\n\t"
            "fmov.s  @(r0,r1),fr4\n\t"
            "fipr    fv0,fv4\n"

            // First sample of the byte. While the fipr completes, look up the delta
            // for the low nibble
            "1:\n\t"
            "mov     r8,r0\n\t"
            "and     #15,r0\n\t"
            "shll2   r0\n\t"
            "fmov.s  @(r0,r1),fr4\n\t"
            "ftrc    fr7,fpul\n\t"
            "float   fpul,fr6\n\t"
            "sts     fpul,r5\n\t"
            "cmp/hi  r6,r5\n\t"
            "bt      3f\n"

            // Second sample of the byte. While the fipr completes, store the first
            // sample and look up the delta for the high nibble of the next byte
            "2:\n\t"
            "fipr    fv8,fv4\n\t"
            "xor     r7,r5\n\t"
            "mov.w   r5,@r3\n\t"
            "add     r4,r3\n\t"
            "mov.b   @r2+,r0\n\t"
            "extu.b  r0,r0\n\t"
            "mov     r0,r8\n\t"
            "shlr2   r0\n\t"
            "and     #60,r0\n\t"
            "fmov.s  @(r0,r1),fr4\n\t"
            "ftrc    fr7,fpul\n\t"
            "float   fpul,fr5\n\t"
            "sts     fpul,r5\n\t"
            "cmp/hi  r6,r5\n\t"
            "bt      5f\n"

            // Start the first sample of the next byte, and store the second sample
            // of this one while that completes
            "4:\n\t"
            "dt      r9\n\t"
            "bt      6f\n\t"
            "fipr    fv0,fv4\n\t"
            "xor     r7,r5\n\t"
            "mov.w   r5,@r3\n\t"
            "bra     1b\n\t"
            "add     r4,r3\n"

            // The rarely taken clamps. The sample becomes 0 if it was negative or 65535
            // if it was too large, and the history is converted again from that
            "3:\n\t"
            "cmp/pz  r5\n\t"
            "bt/s    7f\n\t"
            "mov     r6,r5\n\t"
            "mov     #0,r5\n"
            "7:\n\t"
            "lds     r5,fpul\n\t"
            "bra     2b\n\t"
            "float   fpul,fr6\n"

            "5:\n\t"
            "cmp/pz  r5\n\t"
            "bt/s    8f\n\t"
            "mov     r6,r5\n\t"
            "mov     #0,r5\n"
            "8:\n\t"
            "lds     r5,fpul\n\t"
            "bra     4b\n\t"
            "float   fpul,fr5\n"

            // Store the final sample, then the history (the newest is in fr5)
            "6:\n\t"
            "xor     r7,r5\n\t"
            "mov.w   r5,@r3\n\t"
            "add     r4,r3\n\t"
            "fmov.s  fr6,@r10\n\t"
            "fmov.s  fr5,@-r10\n\t"
            : "+r"(r_in), "+r"(r_out), "+r"(r_pairs), "+r"(r_state)
            : "r"(r_deltas), "r"(r_stride), "r"(r_max), "r"(r_flip)
            : "r0", "r5", "r8", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fpul", "t", "memory"
        );

        // The loop always looks up the delta for the byte after the ones it decodes,
        // so it finishes one byte further on than it got to
        in = r_in - 1;
        buffer = r_out;
        hist1 = state[2];
        hist2 = state[3];
    }
#else
    for(; pairs; --pairs) {
        const uint32_t byte = *in++;
        _adx_decode_sample(deltas[byte >> 4], coef1, coef2, &hist1, &hist2, &buffer, stride, false);
        _adx_decode_sample(deltas[byte & 0xF], coef1, coef2, &hist1, &hist2, &buffer, stride, true);
    }
#endif

    if(remaining & 1) {
        _adx_decode_sample(deltas[*in >> 4], coef1, coef2, &hist1, &hist2, &buffer, stride, false);
    }

    history[0] = hist1 - bias;
    history[1] = hist2 - bias;

    return max_samples;
}

/*
 * Reads the next batch of samples from the stream. If interleave is true then channels will
 * be interleaved (e.g. left/right/left/right). If it's false then samples will be returned in
 * channel blocks, with each channel starting at a multiple of max_samples (even if fewer samples
 * than that were read).
 *
 * Buffer should be at least max_samples * channels in size
 */
size_t adx_stream_read_samples(adxtract_stream_t* stream, bool interleave, size_t max_samples, int16_t* buffer) {
    size_t samples_read = 0;

    // The only difference between the two layouts is where each channel starts
    // in the buffer, and how far apart its samples are
    const size_t stride = interleave ? stream->header.channel_count : 1;
    const size_t channel_offset = interleave ? 1 : max_samples;

    while(samples_read < max_samples) {
        // We might not need to read all samples from this block
        size_t samples_to_read = 32 - stream->current_sample;
        if(samples_to_read > max_samples - samples_read) {
            samples_to_read = max_samples - samples_read;
        }

        size_t position = stream->position;
        size_t end = stream->header.total_samples;

        if(stream->has_loop) {
            if(stream->looping) {
                end = stream->loop_end;

                if(position >= end) {
                    // Jump back to the start of the loop, which might be part way through a block
                    stream->current_block = (stream->loop_begin / 32) * stream->header.channel_count;
                    stream->current_sample = stream->loop_begin % 32;
                    stream->position = stream->loop_begin;
                    memcpy(stream->history, stream->loop_history, sizeof(stream->history));
                    continue;
                }
            }

            // Reads are split at the start of the loop so that the history can be saved there
            if(position == stream->loop_begin) {
                memcpy(stream->loop_history, stream->history, sizeof(stream->history));
            } else if(position < stream->loop_begin) {
                end = stream->loop_begin;
            }
        }

        // Don't read past the end of the stream, the final block is padded
        if(position >= end) {
            break;
        }

        if(samples_to_read > end - position) {
            samples_to_read = end - position;
        }

        size_t new_samples_read = 0;
        bool ended = false;

        // Read the blocks for every channel in one go, unless we already have them
        // because the last read finished part way through
        if(stream->frame_block != stream->current_block) {
            // Being unable to seek is treated as the end of the audio too
            if(stream->file_block != stream->current_block) {
                if(!_adx_seek(stream, stream->audio_start + (sizeof(adxtract_block_t) * stream->current_block))) {
                    stream->frame_block = (size_t) -1;
                    stream->file_block = (size_t) -1;
                    break;
                }
            }

            // Running out of file is treated as the end of the audio
            if(_adx_read(stream, stream->frame, sizeof(adxtract_block_t), stream->header.channel_count) != stream->header.channel_count) {
                stream->frame_block = (size_t) -1;
                stream->file_block = (size_t) -1;
                break;
            }

            stream->frame_block = stream->current_block;
            stream->file_block = stream->current_block + stream->header.channel_count;
        }

        for(int i = 0; i < stream->header.channel_count; ++i) {
            const adxtract_block_t* block = &stream->frame[i];

            // A scale of 0x8001 marks the end of the audio, which can be earlier than
            // the header says
            if(block->predictor == 0x80 && block->scale == 0x01) {
                ended = true;
                break;
            }

            int16_t* outp = buffer + (samples_read * stride) + (i * channel_offset);

            // Decompress the samples up until the number of samples we need
            new_samples_read = _adx_read_block(stream, block, i, stream->current_sample, samples_to_read, outp, stride);
        }

        if(ended) {
            break;
        }

        // If we read 32 samples from all channels, then the current sample in the new block is zero
        // and we can move to the next frame. If we didn't read all the samples, we set the current_sample
        // to the next start position and don't move the current block
        stream->current_sample = (stream->current_sample + new_samples_read) % 32;
        if(!stream->current_sample) {
            stream->current_block += stream->header.channel_count;
        }

        stream->position += new_samples_read;
        samples_read += new_samples_read;

        if(!new_samples_read) {
            break;
        }
    }

    return samples_read;
}


#ifdef __cplusplus
}
#endif

#endif

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2026 Luke Benstead
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
