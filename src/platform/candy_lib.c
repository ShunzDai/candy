#include "candy_memory.h"

candy_hash_t candy_time33(char *name){
  candy_hash_t hash = 5381;
  while (*name)
    hash += (hash << 5) + (*name++);
  return (hash & 0x7FFFFFFF);
}

void *candy_memset(void *dst, uint8_t val, size_t size){
#define LBLOCKSIZE      (sizeof(long))
#define UNALIGNED(X)    ((long)X & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN)  ((LEN) < LBLOCKSIZE)

  unsigned int i;
  char *m = (char *)dst;
  unsigned long buffer;
  unsigned long *aligned_addr;
  unsigned int d = val & 0xff;  /* To avoid sign extension, copy C to an unsigned variable.  */

  if (!TOO_SMALL(size) && !UNALIGNED(dst)){
    /* If we get this far, we know that count is large and s is word-aligned. */
    aligned_addr = (unsigned long *)dst;

    /* Store d into each char sized location in buffer so that we can set large blocks quickly. */
    if (LBLOCKSIZE == 4){
      buffer = (d << 8) | d;
      buffer |= (buffer << 16);
    }
    else{
      buffer = 0;
      for (i = 0; i < LBLOCKSIZE; i ++)
        buffer = (buffer << 8) | d;
    }

    while (size >= LBLOCKSIZE * 4){
      *aligned_addr++ = buffer;
      *aligned_addr++ = buffer;
      *aligned_addr++ = buffer;
      *aligned_addr++ = buffer;
      size -= 4 * LBLOCKSIZE;
    }

    while (size >= LBLOCKSIZE){
      *aligned_addr++ = buffer;
      size -= LBLOCKSIZE;
    }

    /* Pick up the remainder with a bytewise loop. */
    m = (char *)aligned_addr;
  }

  while (size--)
    *m++ = (char)d;

  return dst;

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL
}

void *candy_memcpy(void *dst, const void *src, size_t size){
#define UNALIGNED(X, Y) (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))
#define BIGBLOCKSIZE    (sizeof (long) << 2)
#define LITTLEBLOCKSIZE (sizeof (long))
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

  char *dst_ptr = (char *)dst;
  char *src_ptr = (char *)src;
  long *aligned_dst;
  long *aligned_src;
  uint32_t len = size;

  /* If the size is small, or either SRC or DST is unaligned, then punt into the byte copy loop.  This should be rare. */
  if (!TOO_SMALL(len) && !UNALIGNED(src_ptr, dst_ptr)){
    aligned_dst = (long *)dst_ptr;
    aligned_src = (long *)src_ptr;

    /* Copy 4X long words at a time if possible. */
    while (len >= BIGBLOCKSIZE){
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      len -= BIGBLOCKSIZE;
    }

    /* Copy one long word at a time if possible. */
    while (len >= LITTLEBLOCKSIZE){
      *aligned_dst++ = *aligned_src++;
      len -= LITTLEBLOCKSIZE;
    }

    /* Pick up any residual with a byte copier. */
    dst_ptr = (char *)aligned_dst;
    src_ptr = (char *)aligned_src;
  }

  while (len--)
    *dst_ptr++ = *src_ptr++;

  return dst;
#undef UNALIGNED
#undef BIGBLOCKSIZE
#undef LITTLEBLOCKSIZE
#undef TOO_SMALL
}
