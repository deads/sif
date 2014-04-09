  	                  SIF C Library README

                                  by

                      Damian Eads <eads@lanl.gov>

1. Introduction
---------------

The Sparse Image Format (SIF) is a file format for storing sparse raster 
images. It works by breaking an image down into tiles. Space is saved 
by only storing tiles that are non-uniform, i.e. tiles with two 
different pixel values. If a tile is completely uniform, its common 
pixel value is stored instead of the complete tile raster.

A C library is provided to manipulate files in SIF format. Detailed 
documentation is provided for each function in sif-io.c.

2. Building the SIF C library
-----------------------------

You will need gcc version 3.2.3 and gmake, and your kernel must support 
64-bit files. Type in the following at the bash prompt to build:

   make

This will create a library file called sif-io.so.

3. SIF Functions
----------------

Detailed documentation is given for each function in the library. This 
brief tutorial demonstrates the basics of how the SIF I/O library is
used.
 
  1. The header sif-io.h must be included.

     #include <sif-io.h>

  2. The following code snippet creates a byte image in SIF format of 
  size 1000x1000x1 with 64 by 64 tiles, storing it in the file test.sif.

    sif_file *file = sif_create("test.sif", 10000, 10000, 1, sizeof(u_char),
                                0, 1, 1, 64, 64);

  3. Next enough space is allocated to write a tile. The buffer is made
  uniform by setting each pixel value to 55. The buffer is then written
  to the file.

    u_char *chunk = malloc(sizeof(u_char) * 64 * 64);
    memset(chunk, 0x55, 64 * 64);
    sif_set_tile_xyb(file, 0, 0, 0, chunk);

  4. We write a non-uniform tile of random values to the tile at 
  position 0, 1 at band 0, thereby increasing the space taken
  up by the file.

    u_char *rand = malloc(sizeof(u_char) * 64 * 64);
    int i = 0;
    for (i = 0; i < 64 * 64; i++) {
      rand[i] = rand() % 256;
    }
    sif_set_tile_xyb(file, 0, 1, 0, rand);

  5. If we overwrite a non-uniform tile with a uniform tile,
  the space used by the tile will be reclaimed.

    sif_set_tile_xyb(file, 0, 1, 0, chunk);

  6. The syntax for reading a tile is similar. The following
  reads the tile at position 0, 0, 0 from a SIF file.

    sif_get_tile_xyb(file, 0, 0, 0, chunk);

  7. To close the SIF file, use sif_close.

    sif_close(file)

  8. To read back the SIF file just created, use sif_open.

    sif_file *fp = sif_open("test.sif", 1)

  Note that the second argument is a boolean flag. Pass true to open 
  the file for reading only, and false otherwise.

4. License
----------

 Copyright (C) 2004-2006 The Regents of the University of California.
 Copyright (C) 2007 Los Alamos National Security, LLC.
 
 This material was produced under U.S. Government contract
 DE-AC52-06NA25396 for Los Alamos National Laboratory (LANL), which is
 operated by Los Alamos National Security, LLC for the U.S.
 Department of Energy. The U.S. Government has rights to use,
 reproduce, and distribute this software.  NEITHER THE
 GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
 EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS
 SOFTWARE.  If software is modified to produce derivative works, such
 modified software should be clearly marked, so as not to confuse it
 with the version available from LANL.

 Additionally, this library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either version 2.1
 of the License, or (at your option) any later version. Accordingly, this
 library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 License for more details.

 Los Alamos Computer Code LA-CC-06-105
