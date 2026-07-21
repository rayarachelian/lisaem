/**************************************************************************************\
*                                                                                      *
*                machine.h -  detected integer types for this host                     *
*                              http://lisaem.sunder.net                                *
*                                                                                      *
*                   Copyright (C) 1998, 2020 Ray A. Arachelian                         *
*                                All Rights Reserved                                   *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef IN_MACHINE_H
#define IN_MACHINE_H 1

/*---------------------------------------------
  uname -msr: Linux 5.8.0-41-generic x86_64

 *---------------------------------------------*/
/* sizeof(char     )= 1 */
/* sizeof(int      )= 4 */
/* sizeof(short    )= 2 */
/* sizeof(long     )= 8 */
/* sizeof(long long)= 8 */
/* sizeof(float    )= 4 */
/* sizeof(double   )= 8 */
/*---------------------------------------------*/

typedef char int8;
typedef char sint8;
typedef unsigned char uint8;
typedef short int16;
typedef short sint16;
typedef unsigned short uint16;
typedef int int32;
typedef int sint32;
typedef unsigned int uint32;
typedef long long int64;
typedef long long sint64;
typedef unsigned long long uint64;
#endif
