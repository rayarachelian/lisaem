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

#include <stdio.h>

char *strcasestr(const char *haystack, const char *needle)
{

  char *s, // s=search pointer starts at r goes to the end
      *n,  // pointer to needle - starts at needle and goes to its end.
      *r;  // return pointer, also main pointer starts at s, gets returned if it matches string.
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
    if (*n == NULL && count)
      return r; // if we hit the end of the needle and found at least 1
    r++;        // keep going until the end of the haystack
  }

  return NULL; // no needle for you, one year!
}

int main(int argc, char *argv[])
{
  printf("should fail, return %p\n", strcasestr("Meow", "Arf"));
  printf("should pass, return %p\n", strcasestr("Meow", "meow"));
  printf("should pass, return %p\n", strcasestr("Meow meow meow", "meow"));
  printf("should pass, return %p\n", strcasestr("Arf Meow meow", "meow"));
  printf("should fail, return %p\n", strcasestr("Arf Meow meow", "meow arf"));
  printf("should pass, return %p\n", strcasestr("Arf Meow meow", "arf meow"));
  printf("should pass, return %p\n", strcasestr("Arf Meow meow", " meow "));
}
