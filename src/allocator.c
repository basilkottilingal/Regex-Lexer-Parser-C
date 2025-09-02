#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "allocator.h"

typedef struct Block {
  struct Block * next, * prev;
  size_t size, allocsize;
  char * head;
} Block ;

static Block  * blockhead = NULL; 
static void  ** bins      = NULL;
static unsigned bits      = 0;

static Block * block_new ( void ) {
  size_t s = BLOCK_SIZE;
  void * mem = NULL;
  while ( !(mem = malloc (s)) && (s > 2 * PAGE_SIZE) ) {
    s = s >> 1;
  }
  assert (mem);
  Block * block = (Block *) mem;
  /* fixme : may use "max_align_t" for safer alignment*/
  size_t bsize = (sizeof (Block) + 7) & ~((size_t) 7);
  *block = (Block) {
    .next = blockhead,
    .prev = NULL,
    .size = s - bsize,
    .allocsize = s,
    .head = (char *) mem + bsize
  };
  if (!bins) {
    bins = (void **) block->head;
    int binsize = 16* sizeof (void *); 
    block->head += binsize;
    block->size -= binsize;
    memset (bins, 0, binsize);
  }
  return (blockhead = block);
}

static int block_available (size_t s) {
  assert ( s <= PAGE_SIZE );
  return ( blockhead && ( blockhead->size > s ) );
}

void destroy () {
  while ( blockhead ){
    Block * next = blockhead->next;
    free ( blockhead );
    blockhead = next;
  }
}

void * allocate ( size_t size ) {
  size = (size + 7) & ~((size_t) 7);
  if (!block_available (size)) {
    Block * oldb = blockhead, * newb = block_new();
    if (oldb) { oldb->prev = newb; }
  }
  char * mem = blockhead->head;
  memset (mem, 0, size);
  blockhead->head += size;
  blockhead->size -= size;
  return (void *) mem;
}

char * allocate_str ( const char * s ) {
  size_t size = strlen (s) + 1;
  char * str  = allocate (size);
  memcpy (str, s, size);
  return str;
}

#if 0

#include <stdint.h>
#include <stddef.h>

/* ===== Tunables ===== */
#if SIZE_MAX == 0xFFFFFFFFu
#  define MAX_BINS 32u   /* 2^0 .. 2^31 */
#else
#  define MAX_BINS 64u   /* 2^0 .. 2^63 */
#endif

#define WORD_BITS 64u
#define BITWORDS  ((MAX_BINS + WORD_BITS - 1u) / WORD_BITS)

/* ===== Free-list node ===== */
typedef struct Chunk {
    struct Chunk *next;
} Chunk;

/* ===== State ===== */
static Chunk *bins[MAX_BINS];               /* free lists */
static uint64_t avail_bits[BITWORDS];       /* bitset: bin i nonempty -> 1 */

/* ===== Bitset helpers ===== */
static inline void bitset_set(unsigned i)   { avail_bits[i/64] |=  (1ull << (i%64)); }
static inline void bitset_clr(unsigned i)   { avail_bits[i/64] &= ~(1ull << (i%64)); }
static inline int  bitset_tst(unsigned i)   { return (int)((avail_bits[i/64] >> (i%64)) & 1u); }

/* find first set bit index >= k, or return MAX_BINS if none */
static unsigned bitset_find_ge(unsigned k) {
    unsigned wi = k / 64, off = k % 64;
    if (wi >= BITWORDS) return MAX_BINS;

    uint64_t w = avail_bits[wi] & (~0ull << off);
    while (1) {
        if (w) {
#if defined(__GNUC__) || defined(__clang__)
            unsigned tz = (unsigned)__builtin_ctzll(w);
#else
            /* portable ctzll */
            unsigned tz = 0; uint64_t t = w;
            while ((t & 1u) == 0u) { t >>= 1; tz++; }
#endif
            unsigned idx = wi * 64 + tz;
            return idx < MAX_BINS ? idx : MAX_BINS;
        }
        if (++wi >= BITWORDS) break;
        w = avail_bits[wi];
    }
    return MAX_BINS;
}

/* ===== log2 helpers ===== */
static inline unsigned floor_log2_sz(size_t x) {
#if defined(__GNUC__) || defined(__clang__)
# if SIZE_MAX == 0xFFFFFFFFu
    return 31u - (unsigned)__builtin_clz((unsigned)x);
# else
    return 63u - (unsigned)__builtin_clzll((unsigned long long)x);
# endif
#else
    unsigned n = 0; while (x >>= 1) n++; return n;
#endif
}
static inline unsigned ceil_log2_sz(size_t x) {
    if (x <= 1) return 0;
    unsigned f = floor_log2_sz(x - 1);
    return f + 1;
}

/* ===== Bin push/pop (maintain bitset) ===== */
static inline void bin_push(void *ptr, unsigned n) {
    Chunk *c = (Chunk*)ptr;
    c->next = bins[n];
    bins[n]  = c;
    bitset_set(n);
}
static inline void *bin_pop(unsigned n) {
    Chunk *c = bins[n];
    if (!c) return NULL;
    bins[n] = c->next;
    if (!bins[n]) bitset_clr(n);
    return (void*)c;
}

/* ===== Public API ===== */

/* 1) Partial recycle of a large freed block into powers of two bins */
void bins_partial_free(void *ptr, size_t k) {
    uint8_t *p = (uint8_t*)ptr;
    while (k) {
        unsigned n = floor_log2_sz(k);
        size_t sz  = (size_t)1u << n;
        bin_push(p, n);
        p += sz;
        k -= sz;
    }
}

/* 2) Allocate a chunk of at least 'size'. If larger bin used, split down. */
/* You can supply 'refill' callback that asks your arena for a fresh block
   of size (1<<i) when no bin >= N is available. Return NULL to signal OOM. */
typedef void* (*refill_fn)(unsigned i); /* request 2^i bytes */

void *bins_alloc(size_t size, refill_fn refill) {
    unsigned N = ceil_log2_sz(size);
    if (N >= MAX_BINS) return NULL;

retry:
    /* Find first available bin >= N */
    unsigned i = bitset_find_ge(N);
    if (i == MAX_BINS) {
        /* Try to refill from arena with some strategy, e.g., i = max(N, PREF_BIN) */
        if (!refill) return NULL;
        /* Choose a refill size; simple pick: N (exact) */
        void *blk = refill(N);
        if (!blk) return NULL;
        bin_push(blk, N);
        goto retry;
    }

    /* Take one chunk from bin i; split down to N if necessary. */
    uint8_t *p = (uint8_t*)bin_pop(i);
    while (i > N) {
        i--;
        size_t half = (size_t)1u << i;
        /* split the block into two halves: keep first, push second */
        bin_push(p + half, i);
        /* keep 'p' as left half and continue splitting */
    }
    return p; /* size == 2^N */
}

/* 3) Simple query: is any bin >= k available? */
int bins_any_ge(unsigned k) {
    return bitset_find_ge(k) != MAX_BINS;
}

static inline hbit ( size_t bytes ) {  /* Highest bit position*/
  unsigned n = 
  #if defined(__GNUC__) || defined(__clang__)
  #if SIZE_MAX == 0xFFFFFFFFu
    31 - __builtin_clz ((unsigned) bytes);
  #else
    63 - __builtin_clzll ((unsigned long long) bytes);
  #endif
  #else
    0; while (bytes >>= 1) n++;
  #endif
  return n;  
}

static void * bin_alloc ( size_t bytes ) {
  bytes = bytes + 7 & ~(size_t)7;
  if ( bytes > 256 ) {
    unsigned b = hbit ( bytes );
    if ( ~((size_t)1 << b) & bytes ) return allocate (bytes);
  }
  if ( (bytes >> 3) & ~(size_t) 15 ) {
    unsigned b = hbit ( bytes );
  }
  size_t s = bytes >> 3;
  unsigned b = (bytes >> 3;
  if ( b & ~(size_t) 15 == 0 )
  hbit (bytes);
  void * m = bins [b];
  if (m) {
    bins [b] = * (void **)m;
    return memset (m, 0, bytes);
  }
  return  
}
#endif
