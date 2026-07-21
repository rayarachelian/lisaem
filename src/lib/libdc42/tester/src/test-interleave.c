/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) 2020  Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
*                              interleave tester                                       *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

/**
For sector number  0: interleave(0)=0 
For sector number  1: interleave(1)=5
For sector number  2: interleave(2)=10 
For sector number  3: interleave(3)=15
For sector number  4: interleave(4)=4
For sector number  5: interleave(5)=9 
For sector number  6: interleave(6)=14
For sector number  7: interleave(7)=3 
For sector number  8: interleave(8)=8 
For sector number  9: interleave(9)=13 
For sector number 10: interleave(10)=2 
For sector number 11: interleave(11)=7 
For sector number 12: interleave(12)=12 
For sector number 13: interleave(13)=1 
For sector number 14: interleave(14)=6 
For sector number 15: interleave(15)=11 
For sector number 16: interleave(16)=16 
For sector number 17: interleave(17)=21 
For sector number 18: interleave(18)=26 
For sector number 19: interleave(19)=31 
For sector number 20: interleave(20)=20
... etc ...
 */
long interleave5(long sector)
{
  static const int offset_delta[] = {0, 4, 8, 12, 0, 4, 8, -4, 0, 4, -8, -4, 0, -12, -8, -4};
  return sector + offset_delta[(sector & 15)]; // "sector & 15" is an optimized version of "sector % 16"
}

long interleave5_suboptimal(long sector)
{
  static const int offset[] = {0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11, 16, 21, 26, 31, 20, 25, 30, 19, 24, 29, 18, 23, 28, 17, 22, 27};
  return offset[sector % 32] + sector - (sector % 32);
}

long deinterleave5(long sector)
{
  static const int offset[] = {0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 16, 29, 26, 23, 20, 17, 30, 27, 24, 21, 18, 31, 28, 25, 22, 19};
  return offset[sector % 32] + sector - (sector % 32);
}

int main(int argc, char *argv[])
{
  long i, inter, deinter = 0, error = 0;

  for (i = 0; i < 65535000; i++)
  {
    inter = interleave5(i);
    deinter = deinterleave5(inter);
    fprintf(stdout, "For sector number %ld: interleave5=%ld ; deinterleave5=%ld\n", i, inter, inter, deinter);
    if (i != deinter)
    {
      fprintf(stdout, "interleave failure. deinterleaved5(interleaved(%ld)=%ld)=%ld != %ld\n", i, inter, deinter, i);
      error++;
    }
  }

  fprintf(stderr, "testing complete. %ld errors\n", error);
}
