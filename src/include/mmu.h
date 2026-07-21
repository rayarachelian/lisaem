/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.6      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2007 Ray A. Arachelian                          *
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
*     New version of MMU code designed for faster access with generator core           *
*                                                                                      *
\**************************************************************************************/

/**************************************************************************************\
*  isHex and gethex are Used by the dtc_read_rom functions to read the assembled h rom *
\**************************************************************************************/

#ifndef MMUHINCLUDED
#define MMUHINCLUDED 1
int8 ishex(char c);
int8 gethex(char c);
int16 read_dtc_rom(char *filename, char *ROM);
int16 read_split_rom(char *filename, char *ROMMX);
int16 read_rom(char *filename, char *ROMMX);
void mmu_refresh_context(void);

#ifdef DEBUG
void checkcontext(uint8 c, char *text)
#else
nline checkcontext(uint8 c, char *text) { ; }
#endif

#endif MMUHINCLUDED
