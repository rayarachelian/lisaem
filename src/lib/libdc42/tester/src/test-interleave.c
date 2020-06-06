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

long interleave5(long sector)
{
  static const int offset[]={0,5,10,15,4,9,14,3,8,13,2,7,12,1,6,11,16,21,26,31,20,25,30,19,24,29,18,23,28,17,22,27};
  return offset[sector%32] + sector-(sector%32);
}

long deinterleave5(long sector)
{
  static const int offset[]={0,13,10,7,4,1,14,11,8,5,2,15,12,9,6,3,16,29,26,23,20,17,30,27,24,21,18,31,28,25,22,19};
  return offset[sector%32] + sector-(sector%32);
}


int main(int argc, char *argv[])
{
  long i, inter,deinter=0,error=0;

  for (i=0; i<65535000; i++)
  {
     inter   = interleave5(i);
     deinter = deinterleave5(inter);
     if (i!=deinter) {fprintf(stdout,"interleave failure. deinterleaved5(interleaved(%ld)=%ld)=%ld != %ld\n",i,inter,deinter,i); error++;}
  }

 fprintf(stderr,"testing complete. %ld errors\n",error);
}
