/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) 2020  Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
*                           Low Level libdc42 tester                                   *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>

long interleave5(long sector)
{
  static const int offset[] = {0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11, 16, 21, 26, 31, 20, 25, 30, 19, 24, 29, 18, 23, 28, 17, 22, 27};
  return offset[sector % 32] + sector - (sector % 32);
}

long deinterleave5(long sector)
{
  static const int offset[] = {0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 16, 29, 26, 23, 20, 17, 30, 27, 24, 21, 18, 31, 28, 25, 22, 19};
  return offset[sector % 32] + sector - (sector % 32);
}

void verify_sector(DC42ImageType *F, int i, int n)
{
  int j = 0, x = 0, a = 0, b = 0, e = 0;
  uint8 *data = NULL;
  uint8 *tags = NULL;

  data = F->read_sector_data(F, i);

  if (!data)
  {
    fprintf(stderr, "Could not read sector data %i\n");
    exit(1);
  }

  for (j = 0; j < 512; j += 8)
  {
    if ((a = data[j + 0]) != (b = (n ^ ((i & 0xff000000) >> 24))))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 0, i);
    }
    if ((a = data[j + 1]) != (b = (n ^ ((i & 0x00ff0000) >> 16))))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 1, i);
    }
    if ((a = data[j + 2]) != (b = (n ^ ((i & 0x0000ff00) >> 8))))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 2, i);
    }
    if ((a = data[j + 3]) != (b = (n ^ ((i & 0x000000ff)))))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 3, i);
    }

    if ((a = data[j + 4]) != (b = data[j + 0] ^ 0xa5))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 4, i);
    }
    if ((a = data[j + 5]) != (b = data[j + 1] ^ 0x55))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 5, i);
    }
    if ((a = data[j + 6]) != (b = data[j + 2] ^ 0x5a))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 6, i);
    }
    if ((a = data[j + 7]) != (b = data[j + 3] ^ 0xaa))
    {
      e = 1;
      fprintf(stderr, "unexpected data %x!=%x at location %d in sector %d\n", a, b, j + 7, i);
    }
  }

  tags = F->read_sector_tags(F, i);
  if (!tags)
  {
    fprintf(stderr, "Could not read sector tags %i\n");
    exit(1);
  }

  for (j = 0; j < 20; j += 4)
  {
    if (j & 4)
      x = 0xff;
    else
      x = 0;
    if ((a = tags[j + 0]) != (b = (n ^ x ^ ((i & 0xff000000) >> 24))))
    {
      e = 1;
      fprintf(stderr, "unexpected tags %x!=%x at location %d in sector %d\n", a, b, j + 0, i);
    }
    if ((a = tags[j + 1]) != (b = (n ^ x ^ ((i & 0x00ff0000) >> 16))))
    {
      e = 1;
      fprintf(stderr, "unexpected tags %x!=%x at location %d in sector %d\n", a, b, j + 1, i);
    }
    if ((a = tags[j + 2]) != (b = (n ^ x ^ ((i & 0x0000ff00) >> 8))))
    {
      e = 1;
      fprintf(stderr, "unexpected tags %x!=%x at location %d in sector %d\n", a, b, j + 2, i);
    }
    if ((a = tags[j + 3]) != (b = (n ^ x ^ ((i & 0x000000ff)))))
    {
      e = 1;
      fprintf(stderr, "unexpected tags %x!=%x at location %d in sector %d\n", a, b, j + 3, i);
    }
  }

  if (e)
  {
    F->close_image(&F);
    exit(1);
  }
}

void fill_sector(DC42ImageType *F, int i, int n)
{
  int j = 0, x = 0;
  uint8 data[512];
  uint8 tags[20];

  for (j = 0; j < 512; j += 8)
  {
    data[j + 0] = n ^ ((i & 0xff000000) >> 24);
    data[j + 1] = n ^ ((i & 0x00ff0000) >> 16);
    data[j + 2] = n ^ ((i & 0x0000ff00) >> 8);
    data[j + 3] = n ^ ((i & 0x000000ff));

    data[j + 4] = data[j + 0] ^ 0xa5;
    data[j + 5] = data[j + 1] ^ 0x55;
    data[j + 6] = data[j + 2] ^ 0x5a;
    data[j + 7] = data[j + 3] ^ 0xaa;
  }
  j = F->write_sector_data(F, i, data);
  if (j)
  {
    fprintf(stderr, "Error writing sector data %d\n", i);
    exit(1);
  }

  for (j = 0; j < 20; j += 4)
  {
    if (j & 4)
      x = 0xff;
    else
      x = 0;
    tags[j + 0] = n ^ x ^ ((i & 0xff000000) >> 24);
    tags[j + 1] = n ^ x ^ ((i & 0x00ff0000) >> 16);
    tags[j + 2] = n ^ x ^ ((i & 0x0000ff00) >> 8);
    tags[j + 3] = n ^ x ^ ((i & 0x000000ff));
  }
  j = F->write_sector_tags(F, i, tags);
  if (j)
  {
    fprintf(stderr, "Error writing sector tags%d\n", i);
    exit(1);
  }
}

void fill_image(DC42ImageType *F)
{
  int i;
  fprintf(stderr, "filling disk with initial pattern\n");
  for (i = 0; i < F->numblocks; i++)
    fill_sector(F, i, 0);
}

void check_image(DC42ImageType *F)
{
  int i;
  fprintf(stderr, "checking disk for initial pattern\n");
  for (i = 0; i < F->numblocks; i++)
    verify_sector(F, i, 0);
}

void check_image_except_sector(DC42ImageType *F, int s, int n)
{
  int i;
  fill_sector(F, s, n);
  for (i = 0; i < F->numblocks; i++)
    if (i != s)
      verify_sector(F, i, 0);
    else
      verify_sector(F, i, n);
  fill_sector(F, s, 0);
}

void test(char *dc42filename, int flag)
{
  DC42ImageType F;
  int i;

  if (flag)
  {
    fprintf(stderr, "Could not create %s\n", dc42filename);
    return;
  }

  i = dc42_open(&F, dc42filename, "wm");
  if (i)
  {
    fprintf(stderr, "Could not open created DC42 :%s because %s\n", dc42filename, F.errormsg);
    exit(1);
  }

  fill_image(&F);
  check_image(&F);

  fprintf(stderr, "checking disk for single sector write consistency\n");
  for (i = 0; i < F.numblocks; i++)
    check_image_except_sector(&F, i, 0x22);

  F.close_image(&F);
}

int main(int argc, char *argv[])
{
  int i;
  long numblocks;

  char *s;

  puts("  ---------------------------------------------------------------------------");
  puts("    libdc42 library tester                          http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) 2020, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 3.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");
  puts("");
  puts("  This program is used to test the libdc42 library for valid data writing");
  puts("");

  numblocks = 9728 * 100;
  i = dc42_create(s = "test-500mb.dc42", "-lisaem.sunder.net hd-", numblocks * 512, numblocks * 20);
  test(s, i);
  exit(0);
  numblocks = 800;
  i = dc42_create(s = "test-floppy.dc42", "-not a Macintosh disk-", numblocks * 512, numblocks * 20);
  test(s, i);
  numblocks = 9728;
  i = dc42_create(s = "test-5mb.dc42", "-lisaem.sunder.net hd-", numblocks * 512, numblocks * 20);
  test(s, i);
  numblocks = 19456;
  i = dc42_create(s = "test-10mb.dc42", "-lisaem.sunder.net hd-", numblocks * 512, numblocks * 20);
  test(s, i);
  numblocks = 19456 * 3;
  i = dc42_create(s = "test-30mb.dc42", "-lisaem.sunder.net hd-", numblocks * 512, numblocks * 20);
  test(s, i);
  numblocks = 9728 * 10;
  i = dc42_create(s = "test-50mb.dc42", "-lisaem.sunder.net hd-", numblocks * 512, numblocks * 20);
  test(s, i);
  numblocks = 9728 * 100;
  i = dc42_create(s = "test-500mb.dc42", "-lisaem.sunder.net hd-", numblocks * 512, numblocks * 20);
  test(s, i);

  return 0;
}
