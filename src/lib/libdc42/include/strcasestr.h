/**************************************************************************************\
*                                                                                      *
*             strcasestr - a shitty naive implementation for just for windows          *
*              because, you know, that's a perfect fit for windows, right?             *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                          Copyright (C) 2021 Ray A. Arachelian                        *
*                                   All Rights Reserved                                *
*                                                                                      *
\**************************************************************************************/

#ifndef strcasestr
#ifndef __STRCASESTR__
#define __STRCASESTR__
// tolower requires this
#include <ctype.h>

char *strcasestr(const char *haystack, const char *needle)
{

  const char *s, // s=search pointer starts at r goes to the end
      *n,        // pointer to needle - starts at needle and goes to its end.
      *r;        // return pointer, also main ptr, starts at s, gets returned on match

  long count = 0;

  r = haystack;

  while (*r)
  {
    s = r;
    count = 0;
    n = needle; // start a new search at new location r
    while (tolower(*s) == tolower(*n) && *s && *n)
    {
      s++;
      n++;
      count++;
    }
    if (*n == 0 && count)
    {
      return (char *)(void *)r;
      // if we hit the end of the needle and found at least 1
      // have to do this idiotic cast through void * or else will
      // get an invalid conversion error when stripping const
    }
    r++; // keep going until the end of the haystack
  }

  return NULL; // no needle for you, one year!
}
#endif
#endif
