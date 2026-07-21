/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2019.09.19                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2019 Ray A. Arachelian                          *
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
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  FILE *in;
  unsigned char c;
  int i, z = 0;
  char name[128], *s;
  for (i = 1; i < argc; i++)
  {
    strncpy(name, argv[i], 127);
    s = strstr(name, ".");
    if (s)
      s[0] = 0;

    in = fopen(argv[i], "rb");
    if (!in)
    {
      fprintf(stderr, "Error open %s\n", argv[i]);
      perror("Could not open file");
      exit(1);
    }
    z = 0;
    fprintf(stdout, "uint8 %s={\n"
                    "    /*           +0   +1   +2   +3   +4   +5   +6   +7   +8   +9   +a   +b   +c   +d   +e   +f */\n"
                    "    /* 0000 */ ",
            name);
    while (!feof(in))
    {
      c = fgetc(in);
      fprintf(stdout, "0x%02x", c);
      z++;
      if (!feof(in))
        fprintf(stdout, ",");
      if ((z & 15) == 0)
        fprintf(stdout, "\n    /* %04x */ ", z);
    }
    fprintf(stdout, "};\n\n");
  }
}
