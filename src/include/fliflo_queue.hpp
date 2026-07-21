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

// C++ include for fliflo functions so they can be used in terminalwx.cpp
extern "C"
{

  typedef struct
  {
    uint32 size;
    uint8 *buffer;
    uint32 start;
    uint32 end;
  } FLIFLO_QUEUE_t;

  int fliflo_buff_is_full(FLIFLO_QUEUE_t *b);
  int fliflo_buff_has_data(FLIFLO_QUEUE_t *b);
  int fliflo_buff_is_empty(FLIFLO_QUEUE_t *b);
  uint32 fliflo_buff_size(FLIFLO_QUEUE_t *b);
  uint32 fliflo_buff_percent_full(FLIFLO_QUEUE_t *b);
  int fliflo_buff_add(FLIFLO_QUEUE_t *b, uint8 data);
  uint8 fliflo_buff_pop(FLIFLO_QUEUE_t *b);
  uint8 fliflo_buff_get(FLIFLO_QUEUE_t *b);
  uint8 fliflo_buff_peek(FLIFLO_QUEUE_t *b);
  uint8 fliflo_buff_peek_end(FLIFLO_QUEUE_t *b);
  int fliflo_buff_create(FLIFLO_QUEUE_t *b, uint32 size);
  void fliflo_buff_destroy(FLIFLO_QUEUE_t *b);
}
