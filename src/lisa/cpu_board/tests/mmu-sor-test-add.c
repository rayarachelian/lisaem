#include <stdio.h>
#include "../src/include/machine.h"

int main(int argc, char *argv[])
{
  uint32 segment, sor, offset = 0, seg17, sor9;
  uint32 addr, addrend, phys, physend, wantaddr, wantaddrend;
  int64 ea; // epage,         // page and effective page (sor+/-page), and diff address ea
  for (sor = 0; sor < 0xfff; sor++)
    for (segment = 0; segment < 128; segment++)
      for (offset = 0; offset < 0x20000; offset++)
      {

        // ea=((int32)sor<<9)-(int32)(segment<<17); // what we have now.
        // 76543210
        // 00SGxxxx
        // 0001ffff

        sor9 = sor << 9;
        seg17 = segment << 17;

        addr = seg17 + offset;    // input address
        wantaddr = sor9 + offset; // output address

        // ea               phys
        // addr-(seg<<17) = (sor*512)
        //
        // phys=(addr) + (ea)

        ea = (int64)(-((int32)(seg17)) + ((int32)(sor9))); // delete the segment offset then add the SOR offset.

        phys = (uint32)((int32)(addr) + ea);

        if (phys != wantaddr)
        {
          fprintf(stdout, "want error:    seg:%02x seg17:%08x sor:%03x sor9:%08x ea:%08x offset:%08x    inaddr: %08x got phys:%08x want:%08x diff:%x\n", segment, segment << 17, sor, sor9, (uint32)ea, offset, addr, phys, wantaddr, wantaddr - phys);
        }
      }
}
