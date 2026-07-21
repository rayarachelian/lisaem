/************************************************************************
 IDEFile to Disk Copy 4.2 image converter
 ------------------------------------------------------------------------

 Filename        : IDEFileToDC42.c
 Version         : 1.0
 Author(s)       : Natalia Portillo

 Component       : Main program loop.

 --[ Description ]-------------------------------------------------------

 Converts an IDEFile disk image to Disk Copy 4.2 format.

 --[ License ] ----------------------------------------------------------

 Redistribution and use in source and binary forms, with or without modification, are
 permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of
 conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list
 of conditions and the following disclaimer in the documentation and/or other materials
 provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY NATALIA PORTILLO ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ------------------------------------------------------------------------
 Copyright (C) 2011 Natalia Portillo
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libdc42.h"

// #define DEBUG 1

void usage(void)
{
	printf("IDEFile to Disk Copy 4.2 image converter, version 1.0.\n"
		   "(C) 2011 Natalia Portillo, All Rights Reserved.\n"
		   "libdc32 (C) 2011 Ray Arachelian, All Rights Reserved.\n"
		   "Under BSD and GPL licenses.\n\n"

		   "Usage: IDEFileToDC42 IDEFile_Image.bin\n\n"

		   "This utility will take an IDEFile image and convert it to a Disk Copy 4.2\n"
		   "image suitable to be use on emulators.\n"
		   "Filename will be \"idefile.dc42\".\n");
}

long get_idefile_offset(int sector)
{
	int offset_delta[] = {0, 4, 8, 12, 0, 4, 8, -4, 0, 4, -8, -4, 0, -12, -8, -4};

	return (sector + offset_delta[(sector % 16)]) * 532;
}

int main(int argc, char *argv[])
{

	uint32 i;
	int dc42_errno;
	int fd;
	uint32 sectors;
	struct stat file_stat;
	DC42ImageType f_dc42;
	FILE *f_idefile;
	char *image = NULL;
	uint8 *data, *tags;
	uint32 datasize, tagsize;

	for (i = 1; i < (uint32)argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			usage();
			return 0;
		}
		else
		{
			if (!image)
				image = argv[i];
			else
			{
				fprintf(stderr, "Only one file supported at a time.\n");
				return 1;
			}
		}
	}

	if (!image)
	{
		usage();
		return 0;
	}

	f_idefile = fopen(image, "rb");
	fd = open(image, O_RDONLY);

	if (f_idefile == NULL || fd == -1)
	{
		fprintf(stderr, "Error %d opening \"%s\".\n", errno, image);
		return 2;
	}

	dc42_errno = fstat(fd, &file_stat);

	close(fd);

	if (dc42_errno == -1)
	{
		fprintf(stderr, "Error %d stating \"%s\".\n", errno, image);
		return 3;
	}

	if ((file_stat.st_size % 532) != 0)
	{
		fprintf(stderr, "Input file size is not correct. Not an IDEFile image?\n");
		fclose(f_idefile);
		return 4;
	}

	sectors = file_stat.st_size / 532;
	datasize = sectors * 512;
	tagsize = sectors * 20;

	dc42_errno = dc42_create("idefile.dc42", "IDEFile", datasize, tagsize);

	if (dc42_errno != 0)
	{
		fprintf(stderr, "Error creating output file.\n");
		fclose(f_idefile);
		return 5;
	}

	dc42_errno = dc42_open(&f_dc42, "idefile.dc42", "wb");

	if (dc42_errno)
	{
		fprintf(stderr, "Unable to create image \"idefile.dc42\".\n");
		fprintf(stderr, "Error %d: %s.\n", f_dc42.retval, f_dc42.errormsg);
		fclose(f_idefile);
		return 6;
	}

	// Allocate memory
	data = (uint8 *)malloc(sizeof(uint8) * 512);
	tags = (uint8 *)malloc(sizeof(uint8) * 20);

	if (data == NULL || tags == NULL)
	{
		fprintf(stderr, "Error %d allocating memory.\n", errno);
		fclose(f_idefile);
		return 7;
	}

#if !DEBUG
	fprintf(stdout, "Converting image \"%s\" to \"idefile.dc42\".\n", image);
#endif

	for (i = 0; i < sectors; i++)
	{
#if !DEBUG
		fprintf(stdout, "\r\e[0K");
		fprintf(stdout, "Converting sector %d.\n", i);
#endif

#if DEBUG
		fprintf(stderr, "Seeking to sector %d, %ld inside IDEFile.\n", i, get_idefile_offset(i) / 532);
#endif
		dc42_errno = fseek(f_idefile, get_idefile_offset(i), SEEK_SET);
		if (dc42_errno != 0)
		{
			fprintf(stderr, "Error seeking to sector %d (%ld).\n", i, get_idefile_offset(i) / 532);
			free(data);
			free(tags);
			f_dc42.close_image(&f_dc42);
			fclose(f_idefile);
			return 8;
		}

#if DEBUG
		fprintf(stderr, "Reading tags for sector %d.\n", i);
#endif
		dc42_errno = fread(tags, 1, 20, f_idefile);
		if (dc42_errno != 20)
		{
			fprintf(stderr, "Error reading tags for sector %d.\n", i);
			free(data);
			free(tags);
			f_dc42.close_image(&f_dc42);
			fclose(f_idefile);
			return 9;
		}

#if DEBUG
		fprintf(stderr, "Reading sector %d.\n", i);
#endif
		dc42_errno = fread(data, 1, 512, f_idefile);
		if (dc42_errno != 512)
		{
			fprintf(stderr, "Error reading sector %d.\n", i);
			free(data);
			free(tags);
			f_dc42.close_image(&f_dc42);
			fclose(f_idefile);
			return 10;
		}

#if DEBUG
		fprintf(stderr, "Writing sector %d.\n", i);
#endif
		dc42_errno = f_dc42.write_sector_data(&f_dc42, i, data);
		if (dc42_errno != 0)
		{
			fprintf(stderr, "Error writing sector %d.\n", i);
			fprintf(stderr, "Error %d: %s.\n", f_dc42.retval, f_dc42.errormsg);
			free(data);
			free(tags);
			f_dc42.close_image(&f_dc42);
			fclose(f_idefile);
			return 11;
		}

#if DEBUG
		fprintf(stderr, "Writing tags for sector %d.\n", i);
#endif
		dc42_errno = f_dc42.write_sector_tags(&f_dc42, i, tags);
		if (dc42_errno != 0)
		{
			fprintf(stderr, "Error writing tags for sector %d.\n", i);
			fprintf(stderr, "Error %d: %s.\n", f_dc42.retval, f_dc42.errormsg);
			free(data);
			free(tags);
			f_dc42.close_image(&f_dc42);
			fclose(f_idefile);
			return 12;
		}
	}

	fprintf(stdout, "Conversion successfully done.\n");

	free(data);
	free(tags);
	f_dc42.close_image(&f_dc42);
	fclose(f_idefile);
}
