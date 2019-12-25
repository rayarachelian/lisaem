/**************************************************************************************\
*                                                                                      *
*                machine.h -  detected integer types for this host                     *
*                              http://lisaem.sunder.net                                *
*                                                                                      *
*                   Copyright (C) 1998, 2019 Ray A. Arachelian                         *
*                                All Rights Reserved                                   *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>


#ifndef IN_MACHINE_H
#define IN_MACHINE_H 1


/*---------------------------------------------
  uname -msr: Linux 4.15.0-72-generic x86_64

 *---------------------------------------------*/
/* sizeof(char     )= 1 */
/* sizeof(int      )= 4 */
/* sizeof(short    )= 2 */
/* sizeof(long     )= 8 */
/* sizeof(long long)= 8 */
/* sizeof(float    )= 4 */
/* sizeof(double   )= 8 */
/*---------------------------------------------*/

typedef          char         int8;
typedef unsigned char        uint8;
typedef          short       int16;
typedef unsigned short      uint16;
typedef          int         int32;
typedef unsigned int        uint32;
typedef          long        int64;
typedef unsigned long       uint64;

typedef          int8         sint8;
typedef          int16       sint16;
typedef          int32       sint32;
typedef          int64       sint64;


#ifndef int64_t 
   #define int64_t int64 
#endif 

#ifndef int32_t 
   #define int32_t int32 
#endif 
#ifndef int16_t 
   #define int16_t int16 
#endif 
#ifndef int8_t 
   #define int8_t int8 
#endif 
#ifndef uint64_t 
   #define uint64_t uint64 
#endif 

#ifndef uint32_t 
   #define uint32_t uint32 
#endif 
#ifndef uint16_t 
   #define uint16_t uint16 
#endif 
#ifndef uint8_t 
   #define uint8_t uint8 
#endif 
#ifndef uint_t 
   #define uint_t uint16 
#endif 
#ifndef uint 
   #define uint uint16 
#endif 

#endif
