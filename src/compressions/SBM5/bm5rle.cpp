#include "bm5rle.h"

#include <cstdlib>
#include <cstring>

namespace bm5rle {

namespace {

const size_t BUFFER_SIZE = 0x10000;
const int MAX_MATCH_LENGTH = 32;
const int MAX_RAW_LENGTH = 32;
const int DISTANCE_COUNT = 6;
const size_t DISTANCES[DISTANCE_COUNT] = {1, 2, 4, 16, 32, 64};

struct backref_t {
    int length;
    size_t distance;
};

backref_t search_backref(const unsigned char *buffer, size_t inputsize, size_t pos)
{
    backref_t best;
    best.length = 0;
    best.distance = 0;

    for (int idx = 0; idx < DISTANCE_COUNT; idx++) {
        size_t dist = DISTANCES[idx];
        if (dist > pos)
            continue;
        size_t searchPosition = pos - dist;
        if (buffer[searchPosition] != buffer[pos])
            continue;
        int match = 1;
        while (pos + (size_t)match < inputsize
               && match < MAX_MATCH_LENGTH
               && buffer[searchPosition + (size_t)match] == buffer[pos + (size_t)match])
            match++;
        if (match > best.length) {
            best.length = match;
            best.distance = dist;
        }
    }
    return best;
}

backref_t search_incr(const unsigned char *buffer, size_t inputsize, size_t pos)
{
    backref_t best;
    best.length = 0;
    best.distance = 0;

    if (pos == 0)
        return best;
    size_t searchPosition = pos - 1;
    int n = buffer[searchPosition] + 1;
    if (n > 0xFF || buffer[pos] != n)
        return best;

    int match = 1;
    while (pos + (size_t)match < inputsize
           && match < MAX_MATCH_LENGTH
           && buffer[pos + (size_t)match] == n + match)
        match++;
    best.length = match;
    return best;
}

void write_raw(unsigned char *out, size_t *outPos, const unsigned char *in, int insize)
{
    out[(*outPos)++] = (unsigned char)(insize - 1);
    for (int i = 0; i < insize; i++)
        out[(*outPos)++] = in[i];
}

void write_backref(unsigned char *out, size_t *outPos, backref_t backref)
{
    unsigned char base;
    switch (backref.distance) {
        case 1:  base = 0x40; break;
        case 2:  base = 0x60; break;
        case 4:  base = 0x80; break;
        case 16: base = 0xA0; break;
        case 32: base = 0xC0; break;
        case 64: base = 0xE0; break;
        default: return;
    }
    out[(*outPos)++] = (unsigned char)(base | (backref.length - 1));
}

void write_incrref(unsigned char *out, size_t *outPos, int length)
{
    out[(*outPos)++] = (unsigned char)(0x20 | (length - 1));
}

}  // namespace

char *decompress(const char *data, size_t length, unsigned int *outSize, unsigned int *consumed)
{
    unsigned char *o = (unsigned char *)malloc(BUFFER_SIZE);
    if (o == NULL)
        return NULL;

    const unsigned char *r = (const unsigned char *)data;
    size_t inPos = 0;
    size_t outPos = 0;

    while (inPos < length && outPos < BUFFER_SIZE) {
        unsigned char ctrl = r[inPos++];
        unsigned char e = ctrl >> 5;
        int cnt = (ctrl & 0x1f) + 1;

        if (e == 0) {  // raw literals
            for (int i = 0; i < cnt && inPos < length && outPos < BUFFER_SIZE; i++)
                o[outPos++] = r[inPos++];
        } else if (e == 1) {  // increment run from the previous output byte
            unsigned char chr = (outPos > 0) ? o[outPos - 1] : 0;
            for (int i = 0; i < cnt && outPos < BUFFER_SIZE; i++) {
                chr = (unsigned char)(chr + 1);
                o[outPos++] = chr;
            }
        } else if (e == 2) {  // back-reference, distance 1
            if (outPos == 0) {
                o[outPos++] = 0;
                size_t rlePos = 0;
                for (int i = 0; i < cnt - 1 && outPos < BUFFER_SIZE; i++)
                    o[outPos++] = o[rlePos++];
            } else {
                size_t rlePos = outPos - 1;
                for (int i = 0; i < cnt && outPos < BUFFER_SIZE; i++)
                    o[outPos++] = o[rlePos++];
            }
        } else {  // back-references, distances 2/4/16/32/64
            size_t dist = (e == 3) ? 2 : (e == 4) ? 4 : (e == 5) ? 0x10 : (e == 6) ? 0x20 : 0x40;
            // Quirk kept from the reference port: a window that starts at
            // or before offset 0 emits zeros instead of copying.
            if (outPos > dist) {
                size_t rlePos = outPos - dist;
                for (int i = 0; i < cnt && outPos < BUFFER_SIZE; i++)
                    o[outPos++] = o[rlePos++];
            } else {
                for (int i = 0; i < cnt && outPos < BUFFER_SIZE; i++)
                    o[outPos++] = 0;
            }
        }
    }

    if (outSize)
        *outSize = (unsigned int)outPos;
    if (consumed)
        *consumed = (unsigned int)inPos;
    return (char *)o;
}

char *compress(const char *data, size_t length, unsigned int *outSize)
{
    // Worst case: every byte becomes part of a raw run, each run costing
    // one control byte, with a tail guard for the final partial run.
    size_t bound = length + (length / MAX_RAW_LENGTH + 2) + 16;
    unsigned char *out = (unsigned char *)malloc(bound);
    if (out == NULL)
        return NULL;

    const unsigned char *in = (const unsigned char *)data;
    unsigned char raw[MAX_RAW_LENGTH];
    int rawLength = 0;
    size_t inPos = 0;
    size_t outPos = 0;

    while (inPos < length) {
        backref_t backref = search_backref(in, length, inPos);
        backref_t incrref = search_incr(in, length, inPos);

        if (incrref.length > backref.length
            || (incrref.length >= rawLength && incrref.length >= 1)) {
            if (rawLength > 0) {
                write_raw(out, &outPos, raw, rawLength);
                rawLength = 0;
            }
            write_incrref(out, &outPos, incrref.length);
            inPos += (size_t)incrref.length;
        } else if (backref.length > rawLength
                   || backref.length > 1
                   || (backref.length < rawLength && backref.length > 1)) {
            if (rawLength > 0) {
                write_raw(out, &outPos, raw, rawLength);
                rawLength = 0;
            }
            write_backref(out, &outPos, backref);
            inPos += (size_t)backref.length;
        } else {
            raw[rawLength++] = in[inPos++];
            if (rawLength == MAX_RAW_LENGTH) {
                write_raw(out, &outPos, raw, rawLength);
                rawLength = 0;
            }
        }
    }
    if (rawLength > 0)
        write_raw(out, &outPos, raw, rawLength);

    if (outSize)
        *outSize = (unsigned int)outPos;
    return (char *)out;
}

}  // namespace bm5rle
