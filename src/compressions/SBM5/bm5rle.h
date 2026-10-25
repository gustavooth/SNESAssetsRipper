#ifndef BM5RLE_H
#define BM5RLE_H

#include <stddef.h>

// Super Bomberman 5 (Hudson Soft) tile compression: an RLE /
// back-reference scheme driven by one control byte per command
// (top 3 bits = command, low 5 bits = count-1):
//
//   0x00-0x1F  raw: copy (n+1) literal bytes
//   0x20-0x3F  increment run: write (n+1) bytes, each the previous
//              output byte + 1
//   0x40-0x5F  copy (n+1) bytes from output-1
//   0x60-0x7F  copy (n+1) bytes from output-2
//   0x80-0x9F  copy (n+1) bytes from output-4
//   0xA0-0xBF  copy (n+1) bytes from output-16
//   0xC0-0xDF  copy (n+1) bytes from output-32
//   0xE0-0xFF  copy (n+1) bytes from output-64
//
// The game's own decompressor stops on a target output length it gets
// from its pointer table, not from the stream; the stream itself carries
// no size. The ROM-side decode here therefore runs input-driven: it
// consumes every byte of the buffer it is given. Callers that know the
// stream's exact end (the pointer table at PC 0x05E22C) should size the
// input buffer to the stream, not to the whole ROM.
//
// Ported from Proton's decompressor (via DackR's C# in MBuild), keeping
// its quirks — notably, a back-reference whose start lands at or before
// offset 0 emits zeros, matching the game's behaviour.
namespace bm5rle {

// Decodes `length` bytes at `data` into a freshly malloc'd buffer
// (free() it when done). `outSize` receives the number of bytes written,
// `consumed` the number of input bytes read. Returns NULL on allocation
// failure.
char *decompress(const char *data, size_t length, unsigned int *outSize, unsigned int *consumed);

// Compresses `length` bytes at `data` into a freshly malloc'd buffer
// (free() it when done). `outSize` receives the compressed size. The
// greedy matcher mirrors Proton's compressor, so re-compressing a
// decoded stream stays loadable by the game.
char *compress(const char *data, size_t length, unsigned int *outSize);

}  // namespace bm5rle

#endif
