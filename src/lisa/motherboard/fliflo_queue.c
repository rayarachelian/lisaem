/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.10.15                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                Copyright (C) MCMXCVIII, MMXX Ray A. Arachelian                       *
*                                All Rights Reserved                                   *
*                                                                                      *
*           This program is free software; you can redistribute it and/or              *
*           modify it under the terms of the GNU General Public License                *
*           as published by the Free Software Foundation; either version 2             *
*           of the License, or (at your option) any later version.                     *
*                                                                                      *
*           This program is distributed in the hope that it will be useful,            *
*           but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*           MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*           GNU General Public License for more details.                               *
*                                                                                      *
*           You should have received a copy of the GNU General Public License          *
*           along with this program;  if not, write to the Free Software               *
*           Foundation, Inc., 59 Temple Place #330, Boston, MA 02111-1307, USA.        *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
*                                                                                      *
*  FLIFLO Queue Structures for various LisaEm usage. The name is a merging of FIFO     *
*  and LIFO queues, and is implemented as a circular buffer. If you use push/pop, it   *
*  acts as a LIFO or stack. If you use add/get, it acts as a FILE.                     *
*                                                                                      *
*  FIFO=a First In, First Out queue. LIFO=Last In, First Out (also known as a stack)   *
*                                                                                      *
\**************************************************************************************/

// This sure would work better as a C++ class. :)  But we're not writing in C++, so there.
// thththththpt!
//
// FIFO's are FIRST IN FIRST OUT i.e. circular buffers
// LIFO's are LAST IN, FIRST OUT queues, i.e. stacks
//
// FLIFLO's are both.  You can add data at the end of the queue,
// you can pop data off the end like a stack, or get it from the begining,
// like a fifo.

#define IN_FLIFLO_QUEUE_C
#include <vars.h>

// typedef struct
//{
//   uint32 size;     // this is the total size of entries in the queue
//   uint8 *buffer;   // buffer calloc'ed with size
//   uint32 start;    // start/end indeces for add/remove/push/pop
//   uint32 end;
// } FLIFLO_QUEUE_t;

// todo - add item size here and move mouse to fliflo but need backwards compatibility with keyboard/serial

// int fliflo_buff_is_full(FLIFLO_QUEUE_t *b);
// int fliflo_buff_has_data(FLIFLO_QUEUE_t *b);
// int fliflo_buff_is_empty(FLIFLO_QUEUE_t *b);
// uint32 fliflo_buff_size(FLIFLO_QUEUE_t *b);
// uint32 fliflo_buff_percent_full(FLIFLO_QUEUE_t *b);
// int fliflo_buff_add(FLIFLO_QUEUE_t *b,uint8 data);
// uint8 fliflo_buff_pop(FLIFLO_QUEUE_t *b);
// uint8 fliflo_buff_get(FLIFLO_QUEUE_t *b);
// uint8 fliflo_buff_peek(FLIFLO_QUEUE_t *b);
// uint8 fliflo_buff_peek_end(FLIFLO_QUEUE_t *b);
// int fliflo_buff_create(FLIFLO_QUEUE_t *b, uint32 size);
// void fliflo_buff_destroy(FLIFLO_QUEUE_t *b);

static inline int next_idx(FLIFLO_QUEUE_t *b, int index)
{
  if (!b)
    return 0;
  if (!b->size)
    return 0;
  return (index + 1) % b->size;
}

static inline int previous_idx(FLIFLO_QUEUE_t *b, int index)
{
  if (!b)
    return 0;
  if (!b->size)
    return 0;
  if (!index)
    return b->size - 1;
  return (index - 1) % b->size;
}

int fliflo_buff_is_full(FLIFLO_QUEUE_t *b)
{
  if (!b)
    return -1;
  return b->size ? (((b->end + 1) % b->size) == b->start) : 0;
}

int fliflo_buff_has_data(FLIFLO_QUEUE_t *b)
{
  if (!b)
    return 0;
  if (!b->buffer)
    return 0;
  if (!b->size)
    return 0;
  return (b->start != b->end);
}

int fliflo_buff_is_empty(FLIFLO_QUEUE_t *b)
{
  if (!b)
  {
    ALERT_LOG(0, "NULL queue");
    return -1;
  }
  if (!b->buffer)
  {
    ALERT_LOG(0, "QUEUE has no buffer");
    return -2;
  }
  if (!b->size)
  {
    ALERT_LOG(0, "QUEUE is empty");
    return -3;
  }
  return (b->start == b->end);
}

uint32 fliflo_buff_size(FLIFLO_QUEUE_t *b)
{
  if (!b)
    return -1;
  if (!b->size)
    return -1;

  if (b->end == b->start)
    return 0;

  if (b->end > b->start)
    return (b->end) - (b->start);
  else
    return (b->end + b->size) - (b->start);
}

uint32 fliflo_buff_percent_full(FLIFLO_QUEUE_t *b)
{
  int t = fliflo_buff_size(b);
  if (t < 0)
    return 0;
  return 100 * (b->size - t);
}

int fliflo_buff_add(FLIFLO_QUEUE_t *b, uint8 data) // checked
{
  if (!b->size)
    return -4;

  if (!b)
  {
    ALERT_LOG(0, "null queue!");
    return -2;
  }
  if (!b->buffer)
  {
    ALERT_LOG(0, "buffer is missing!");
    return -3;
  }
  if (fliflo_buff_is_full(b))
  {
    ALERT_LOG(0, "buffer is full");
    return -1;
  }

#ifdef DEBUG
  if ((b->end) > (b->size))
  {
    EXITR(178, 0, "ERROR! end:%d pointer>size! %d", b->end, b->size);
  }
  if ((b->start) > (b->size))
  {
    EXITR(179, 0, "ERROR! start:%d pointer>size! %d", b->start, b->size);
  }
#endif

  DEBUG_LOG(0, "adding %d to buffer at index:%d", data, b->end);
  // BUG://b->buffer[b->start]=data; // this will be replaced because start will not change! should add to the end?
  b->buffer[b->end] = data; // this will be replaced because start will not change! should add to the end?

  b->end = next_idx(b, b->end);
  return 0;
}

extern char niceascii(char c); // in ./src/lib/libGenerator/generator/reg68k.c
/* code for converting binary into readable ascii, converts high bit and control chars back to text.
char niceascii(char c)
{ c &=127;
 if (c<31) c|=32;
 if (c==127) c='.';
 return c;
}
*/

void fliflo_dump(FILE *log, FLIFLO_QUEUE_t *b, char *s)
{
  uint32 i;
  fprintf(log, "FLIFLO queue dump of %s @%p start-end:size:%d-%d:%d::\n  ", s, b, b->start, b->end, b->size);

  i = b->start;
  while (i != b->end)
  {
    fprintf(log, "%02x ", b->buffer[i]);
    i = next_idx(b, i);
  }
  fprintf(log, " | ");

  i = b->start;
  while (i != b->end)
  {
    fprintf(log, "%c", niceascii(b->buffer[i]));
    i = next_idx(b, i);
  }

  fprintf(log, "\n\n");
}

uint8 fliflo_buff_pop(FLIFLO_QUEUE_t *b) // checked
{
  uint8 data;
  if (!b)
    return 0;
  if (fliflo_buff_is_empty(b))
    return 0;
  if (!b->buffer)
    return 0;
  b->end = previous_idx(b, b->end);
  data = b->buffer[b->end];
  b->buffer[b->end] = 0;
  return data;
}

uint8 fliflo_buff_get(FLIFLO_QUEUE_t *b) // checked.
{
  uint8 data;
  if (!b)
  {
    ALERT_LOG(0, "NULL queue");
    return 0;
  }
  if (fliflo_buff_is_empty(b))
  {
    ALERT_LOG(0, "empty");
    return 0;
  }
  if (!b->buffer)
  {
    ALERT_LOG(0, "QUEUE has no buffer");
    return 0;
  }
  data = b->buffer[b->start]; // read a value off the start;
  ALERT_LOG(0, "Read %02x at start index:%d", data, b->start);
  b->buffer[b->start] = 0; // clobber it to make sure
  b->start = next_idx(b, b->start);
  ALERT_LOG(0, "next start index is now: %d", b->start);
  return data;
}

uint8 fliflo_buff_peek(FLIFLO_QUEUE_t *b)
{
  uint8 data;
  if (!b)
  {
    ALERT_LOG(0, "NULL QUEUE");
    return 0;
  }
  if (fliflo_buff_is_empty(b))
  {
    ALERT_LOG(0, "QUEUE is empty");
    return 0;
  }
  if (!b->buffer)
  {
    ALERT_LOG(0, "QUEUE HAS NO BUFFER");
    return 0;
  }
  data = b->buffer[b->start];
  return data;
}

uint8 fliflo_buff_peek_end(FLIFLO_QUEUE_t *b)
{
  uint8 data;
  if (!b)
    return 0;
  if (fliflo_buff_is_empty(b))
    return 0;
  if (!b->buffer)
    return 0;
  data = b->buffer[previous_idx(b, b->end)];
  return data;
}

int fliflo_buff_create(FLIFLO_QUEUE_t *b, uint32 size)
{
  if (!b)
    return -2;
  if (size < 2)
    return -3;
  size++;
  b->buffer = calloc(1, size);
  if (!b->buffer)
    return -1;
  b->start = 0;
  b->end = 0;
  b->size = size;
  return 0;
}

void fliflo_buff_destroy(FLIFLO_QUEUE_t *b)
{

  if (!b)
    return;
  b->size = 0;
  b->start = 0;
  b->end = 0;
  if (b->buffer)
    free(b->buffer);
  b->buffer = NULL;
}

#ifdef NEWCODE

/* give a size value, find the next bitmask that contains this size.
I.e give it 2 returns 3, give it 4,5,6 reurns 7, etc. So can use val &
mask instead of modulo division.)  */

uint32 findfittingbitmask(uint32 size)
{
  uint32 count = 0;
  while (size >>= 1)
    count++;
  return (1 << (count + 1)) - 1;
  // check the need for +1 above
}

// S=start pointer. E=end pointer.

#define s (RB->start)
#define e (RB->end)
#define size (RB->rbsize)
#define data (RB->data)
#define CHKRB    \
  {              \
    if (!RB)     \
      return -2; \
  }

inline static int rb_is_empty(RB)
{
  CHKRB;
  return (s == e);
}
inline static int rb_is_full(RB)
{
  CHKRB;
  return (next(e) == s);
}
inline static int rb_has_data(RB)
{
  CHKRB;
  return (s != e);
}
inline static int rb_size(RB)
{
  CHKRB;
  if (e >= s)
    return e - s;
  return (RB->buffersize - (s - e));
}

inline static rb_remaining_space(RB)
{
  return RB->buffersize - rb_size(RB);
}

inline static int rb_percent_full(RB)
{
  if (!RB)
    return 100;
  return rb_size(RB) * 100 / RB->buffersize;
}

inline static int next(RB, int i)
{
  CHKRB;
  i++;
  if (i >= RB->buffersize)
    return 0;
  return i;
}

inline static prev next(RB, int i)
{
  CHKRB;
  i--;
  if (i < 0)
    return RB->buffersize - 1;
  return i;
}

int add(RB, void *x)
{
  CHKRB;
  if (is_full(RB))
    return -1;
  data[prev(RB, e)] = x;
  e = next(RB, e);
  size++;
  return 0;
}

void *get(RB)
{
  int sold;
  CHKRB;
  if (is_empty(RB))
    return NULL;
  sold = s;
  s = next(RB, s);
  return data[sold];
}

void *peek(RB)
{
  CHKRB;
  if (is_empty(RB))
    return NULL;
  return data[s];
}

void *peek_end(RB)
{
  if (!RB || is_empty(RB))
    return NULL;
  return data[prev(RB, e)];
}

void *get_end(RB)
{
  CHKRB;
  if (is_empty(RB))
    return -1;
  e = prev(RB, e);
  return data[e];
}

int add_start(RB, void *x)
{
  int ps = prev(RB, s);
  CHKRB;
  if (is_full(RB) || ps == e)
    return -1;
  s = ps;
  data[ps] = x;
  return 0;
}

#endif
