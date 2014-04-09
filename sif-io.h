/**
 * \internal
 * File:             sif-io.h
 * Date:             December 17, 2004
 * Author:           Damian Eads
 * Description:      A C library for manipulating Sparse Image Format (SIF) files.
 *
 * Copyright (C) 2004-2006 The Regents of the University of California.
 *
 * Copyright (C) 2006-2008 Los Alamos National Security, LLC.
 *
 * This material was produced under U.S. Government contract
 * DE-AC52-06NA25396 for Los Alamos National Laboratory (LANL), which is
 * operated by Los Alamos National Security, LLC for the U.S.
 * Department of Energy. The U.S. Government has rights to use,
 * reproduce, and distribute this software.  NEITHER THE
 * GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
 * EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS
 * SOFTWARE.  If software is modified to produce derivative works, such
 * modified software should be clearly marked, so as not to confuse it
 * with the version available from LANL.
 *
 * Additionally, this library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version. Accordingly, this
 * library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * Los Alamos Computer Code LA-CC-06-105
 */

/**
 * \file sif-io.h
 * @brief The only header file to include for using sif-io library functions.
 */

#ifndef __SIF_IO_H_
#define __SIF_IO_H_

#include "SIFExport.h"
#if defined(_MSC_VER)
#define HAVE_LONG_LONG
#include <windows.h>
#endif

#define LONGLONG long long

#include <sys/types.h>
#include <stdio.h>

/**
 * \defgroup sif_ec SIF Error Codes
 */

/**
 * \def SIF_ERROR_NONE
 * \ingroup sif_ec
 *
 * @brief A status code indicating no error has been detected for the processing of
 * the target file.
 */

#define SIF_ERROR_NONE 0

/**
 * \def SIF_ERROR_MEM
 * \ingroup sif_ec
 *
 * @brief A status code indicating an error occurred while allocating or freeing
 * memory.
 */

#define SIF_ERROR_MEM 1

/**
 * \def SIF_ERROR_NULL_FP
 * \ingroup sif_ec
 *
 * @brief A status code indicating a file could not be processed because the
 * file pointer is null. Admittedly, there is no way to store this
 * in the sif_file struct passed since it is null. However, setting a static
 * variable for the caller to check is under consideration for a future version.
 */

#define SIF_ERROR_NULL_FP 2

/**
 * \def SIF_ERROR_NULL_HDR
 * \ingroup sif_ec
 *
 * @brief A status code indicating a file could not be processed because the
 * header pointer is null.
 */

#define SIF_ERROR_NULL_HDR 3

/**
 * \def SIF_ERROR_INVALID_BN
 * \ingroup sif_ec
 *
 * @brief A status code indicating a block number passed to a sif-io function
 * was invalid (i.e. negative or out-of-bounds).
 */

#define SIF_ERROR_INVALID_BN 4

/**
 * \def SIF_ERROR_INVALID_TN
 * \ingroup sif_ec
 *
 * @brief A status code indicating a tile number passed to a sif-io function
 * was invalid (i.e. negative or out-of-bounds).
 */

#define SIF_ERROR_INVALID_TN 5

/**
 * \def SIF_ERROR_READ
 * \ingroup sif_ec
 *
 * @brief A status code indicating an error occurred when reading from the
 * file.
 */

#define SIF_ERROR_READ 6

/**
 * \def SIF_ERROR_WRITE
 * \ingroup sif_ec
 *
 * @brief A status code indicating an error occurred when writing to the
 * file.
 */

#define SIF_ERROR_WRITE 7

/**
 * \def SIF_ERROR_SEEK
 * \ingroup sif_ec
 *
 * @brief A status code indicating an error occurred when seeking in the
 * file.
 */

#define SIF_ERROR_SEEK 8

/**
 * \def SIF_ERROR_TRUNCATE
 * \ingroup sif_ec
 *
 * @brief A status code indicating an error occurred when truncating the file.
 */

#define SIF_ERROR_TRUNCATE 9

/**
 * \def SIF_ERROR_INVALID_FILE_MODE
 * \ingroup sif_ec
 *
 * @brief A status code indicating that the file mode chosen is invalid. This
 * usually occurs when a file is opened for update that is read-only or a
 * opened when the permissions do not permit reading.
 */

#define SIF_ERROR_INVALID_FILE_MODE 10

/**
 * \def SIF_ERROR_INCOMPATIBLE_VERSION
 * \ingroup sif_ec
 *
 * @brief A status code indicating that the currently loaded sif-io library
 * is not capable of processing the version of a file. This is usually due
 * to the fact that the file was written with a later version of the SIF
 * format than the loaded library.
 */


#define SIF_ERROR_INCOMPATIBLE_VERSION 11

/**
 * \def SIF_ERROR_META_DATA_KEY
 * \ingroup sif_ec
 *
 * @brief Returned when a call is made that expects a key to be present
 * when the key cannot be found.
 */

#define SIF_ERROR_META_DATA_KEY 12

/**
 * \def SIF_ERROR_META_DATA_VALUE
 * \ingroup sif_ec
 *
 * @brief Returned by <code>sif_get_meta_data</code> when the meta-data does not contain
 * a null-terminated string.
 */

#define SIF_ERROR_META_DATA_VALUE 13

/**
 * \def SIF_ERROR_CANNOT_WRITE_VERSION
 * \ingroup sif_ec
 *
 * @brief Returned by <code>sif_use_file_version</code> when the library is not capable
 * of writing the file in the requested version.
 */

#define SIF_ERROR_CANNOT_WRITE_VERSION 14

/**
 * \def SIF_ERROR_INVALID_BAND
 * \ingroup sif_ec
 *
 * @brief Returned if a band argument passed is invalid.
 */

#define SIF_ERROR_INVALID_BAND 15

/**
 * \def SIF_ERROR_INVALID_COORD
 * \ingroup sif_ec
 *
 * @brief Returned if a coordinate argument (e.g. <code>x</code> or
 * <code>y</code>) is invalid.
 */

#define SIF_ERROR_INVALID_COORD 16

/**
 * \def SIF_ERROR_INVALID_TILE_SIZE
 * \ingroup sif_ec
 *
 * @brief Returned if a tile size argument (e.g. <code>tile_width</code> or
 * <code>tile_height</code>) is invalid.
 */

#define SIF_ERROR_INVALID_TILE_SIZE 17

/**
 * \def SIF_ERROR_INVALID_REGION_SIZE
 * \ingroup sif_ec
 *
 * @brief Returned if a region size argument (e.g. <code>width</code> or
 * <code>height</code>) is invalid.
 */

#define SIF_ERROR_INVALID_REGION_SIZE 18


/**
 * \def SIF_ERROR_INVALID_BUFFER
 * \ingroup sif_ec
 *
 * @brief Returned if a tile size argument (e.g. <code>width</code> or
 * <code>height</code>) is invalid.
 */

#define SIF_ERROR_INVALID_BUFFER 19

/**
 * \def SIF_ERROR_PNM_INCOMPATIBLE_TYPE_CODE
 * \ingroup sif_ec
 *
 * @brief Returned if the type code is not supported for PNM output.
 */

#define SIF_ERROR_PNM_INCOMPATIBLE_TYPE_CODE 20

/**
 * \def SIF_ERROR_PGM_INVALID_BAND_COUNT
 *
 * @brief Returned if the number of bands is not equal to one, which is required
 * for PGM output.
 */

#define SIF_ERROR_PGM_INVALID_BAND_COUNT 21

/**
 * \def SIF_ERROR_PPM_INVALID_BAND_COUNT
 *
 * @brief Returned if the number of bands is not equal to three, which is required
 * for PPM output.
 */

#define SIF_ERROR_PPM_INVALID_BAND_COUNT 22

/**
 * \def SIF_ERROR_PNM_INCOMPATIBLE_DT_CONVENTION
 * \ingroup sif_ec
 *
 * @brief Returned if the data type convention is not "simple".
 */

#define SIF_ERROR_PNM_INCOMPATIBLE_DT_CONVENTION 23

/**
 * \defgroup simpdecs Simple Data Type Convention Macro Definitions
 */

/**
 * \def SIF_AGREEMENT_SIMPLE
 *
 * @brief A value to set the data-type convention agreement (i.e. "_sif_agree")
 * meta-data field to indicate that the <code>simple</code> data-type convention
 * is used.
 */

#define SIF_AGREEMENT_SIMPLE "simple"

/**
 * \def SIF_AGREEMENT_GDAL
 *
 * @brief A value to set the data-type convention agreement (i.e. "_sif_agree")
 * meta-data field to indicate that the <code>gdal</code> data-type convention
 * is used.
 */

#define SIF_AGREEMENT_GDAL "gdal"

/**
 * \def SIF_MAGIC_NUMBER
 *
 * @brief A string representing the magic number. The obscure string used to easily
 * identify a file as a file in SIF format.
 */

#define SIF_MAGIC_NUMBER "!**SIF**"

/**
 * \def SIF_MAGIC_NUMBER_SIZE
 *
 * @brief The number of bytes needed to store the magic number. The obscure string is
 * used to easily identify that a file is likely to be in SIF format.
 */

#define SIF_MAGIC_NUMBER_SIZE 8

/**
 * @brief A type of function pointer, instances of which are stored internally in a sif_file object.
 *
 * It is used for preprocessing a buffer and then writing it to disk. Either no preprocessing is
 * done or bytes are properly swapped to return a buffer with native byte ordering. This function
 * only serves a purpose with the "simple" data type interface.
 *
 * The \ref sif_file::simple_region_buffer in the \sif_file object may be used or resized,
 * as needed.
 *
 * @param file    The SIF file to which the preprocessed buffer will be written.
 * @param buffer  The buffer to preprocess.
 * @param size    The size of an element (in bytes).
 * @param nmemb   The number of elements to write.
 *
 * @return The number of bytes written.
 */

typedef int (*sif_buffer_preprocessor) (sif_file *file, size_t size, size_t nmemb, const void *buffer);

/**
 * @brief A type of function pointer, instances of which are stored internally in a sif_file object.
 * It is used for postprocessing a buffer after reading it from disk. Either no preprocessing is
 * done or bytes are properly swapped so the buffer is stored on disk using a particular byte
 * order that may be different from the host. This function only serves a purpose with the "simple" data
 * type interface.
 *
 * The \ref sif_file::simple_region_buffer in the \sif_file object may be used or resized,
 * as needed.
 *
 * @param file    The SIF file from which to read.
 * @param buffer  The buffer to read.
 * @param size    The size of an element (in bytes).
 * @param nmemb   The number of elements to read.
 *
 * @return The number of bytes read.
 */

typedef int (*sif_buffer_postprocessor) (sif_file *file, size_t size, size_t nmemb, void *buffer);

/**
 * \struct sif_header
 * @brief A struct for storing a SIF file header in memory.
 *
 * @warning Changing its fields does not result in an immediate change to the header
 * stored in the file to which it points. The file must be flushed with
 * \ref sif_flush or closed with \ref sif_close. Integers are stored
 * with a sign bit in big-endian form.
 */

typedef struct SIF_EXPORT {

  /**
   * @brief This field identifies whether the header read from a file is likely
   * to correspond to a SIF file.
   *
   * These bytes must equal the string "!**SIF**" or an error will occur
   * when the header is processed by a SIF function. The byte offset of this
   * field is 0.
   *
   * @warning Do not edit this field directly.
   */

  char                   magic_number[SIF_MAGIC_NUMBER_SIZE];

  /**
   * @brief The minimum version of the SIF library needed to read this file.
   *
   * @warning Do not edit this field directly. Changing the value of this field
   * without a corresponding change to the organization of the file may make
   * it unreadable.
   */

  long                   version;

  /**
   * @brief The width of the image in pixels.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   width;

  /**
   * @brief The height of the image in pixels.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   height;

  /**
   * @brief The number of bands of the image.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   bands;

  /**
   * @brief The number of keys stored in the meta-data.
   *
   * @warning Do not edit this field directly.
   * Instead use the \ref sif_get_meta_data, \ref sif_get_meta_data_binary,
   * \ref sif_set_meta_data, and \ref sif_get_meta_data_binary functions.
   */

  long                   n_keys;

  /**
   * @brief The number of tiles that comprise this image.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   n_tiles;

  /**
   * @brief The width of each tile in pixels.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   *
   * @warning Non-square tiles have not been tested.
   */

  long                   tile_width;

  /**
   * @brief The height of each tile in pixels.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   *
   * @warning Non-square tiles have not been tested.
   */

  long                   tile_height;

  /**
   * @brief The number of bytes required to store a single tile raster. This is equal
   * to tile_width * tile_height * n_bands * data_unit_size.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   tile_bytes;

  /**
   * @brief The number of tiles across the width of an image.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   n_tiles_across;

  /**
   * @brief The number of bytes required to store each pixel.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   *
   * @warning Non-square tiles have not been tested.
   */

  long                   data_unit_size;

  /**
   * @brief A number that is only read from and written to the SIF file header. It has no
   * meaning to the sif-io functions since sif-io processes images without regard to the
   * data type of the pixels. The caller to the library function can use the field to store
   * an integer that represents the data type of the pixels in the image.
   *
   * @warning Do not edit this field directly. Instead use the \ref sif_set_user_data_type function.
   */

  long                   user_data_type;

  /**
   * @brief A field that, when nonzero, indicates the file should be defragmented when its closed.
   *
   * @warning Do not edit this field directly. Instead use the \ref sif_set_defragment or
   * \ref sif_unset_defragment functions.
   */

  long                   defragment;

  /**
   * @brief A field, that when nonzero, indicates that the file should be consolidated when its closed.
   *
   * This involves performing pixel uniformity checks on each dirty tile during close.
   *
   * @warning Do not edit this field directly. Instead use the \ref sif_set_defragment or
   * \ref sif_unset_defragment functions.
   */

  long                   consolidate;

  /**
   * @brief A field, that when nonzero, indicates that when each tile is written, a uniformity check
   * should be performed.
   *
   * @warning Do not edit this field directly. Instead use the \ref sif_set_intrinsic_write
   * or \ref sif_unset_intrinsic_write functions.
   */

  long                   intrinsic_write;

  /**
   * @brief The number of bytes needed to store the header for each tile.
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   tile_header_bytes;

  /**
   * @brief The number of bytes to store the uniformity flags, i.e.
   * Ceil(number_of_flags / 8).
   *
   * @warning Do not edit this field directly. Changing its value without changing
   * the image layout on disk will make the file unreadable.
   */

  long                   n_uniform_flags;

  /**
   * @brief Six doubles representing the affine georeferencing transform parameters.
   *
   * The georeferenced coordinates of the pixel coordinate (Xpixel, Yline)
   * are computed as follows (from GDAL documentation):
   * \code
   *  const double *GT = &(hd->affine_geo_transform);
   *  Xgeo = GT[0] + Xpixel * GT[1] + Yline * GT[2];
   *  Ygeo = GT[3] + Xpixel * GT[4] + Yline * GT[5];
   * \endcode
   *
   * The transform is set to {0.0, 1.0, 0.0, 0.0, 0.0, 1.0} by default
   * by sif_create so that x and y are just mapped to themselves.
   *
   * @warning Do not edit this field directly. Instead use the
   * \ref sif_set_affine_geo_transform function.
   */

  double                 affine_geo_transform[6];
} sif_header;

/**
 * \struct sif_tile
 * @brief A struct for storing a SIF tile header in memory. It
 * stores important information related to a tile, including
 * which of its bands are uniform, and the uniform pixel values
 * of the bands.
 */

typedef struct SIF_EXPORT {
  //  long                   block_num;
  //  char                   uniform_pixel_value[16];

  /**
   * @brief A byte sequence where the i'th bit in the sequence
   * indicates whether the i'th band in the tile is uniform. The
   * number of bytes is Ceil(n_bands / 8).
   */
  u_char                 *uniform_flags;

  /**
   * @brief A sequence of pixel data units. The i'th data unit
   * represents the uniform pixel value for the i'th band. The
   * number of bytes is n_bands * data_unit_size.
   */

  u_char                 *uniform_pixel_values;

  /**
   * @brief The block location of the file where the tile is stored.
   *
   * This number is -1 if the tile is completely uniform, i.e. each
   * band in the tile is completely uniform. Note that the bands
   * of a tile (i.e. slices) may have different uniform pixel values.
   * A tile or block is uniform iff each of its slices is uniform.
   */

  long                   block_num;

} sif_tile;

/**
 * \struct sif_meta_data
 * @brief A struct for storing meta-data in memory. It stores a node
 * in a linked list of meta-data.
 *
 * @warning Do not modify this data structure directly. Instead use
 * the \ref sif_set_meta_data and \ref sif_set_meta_data_binary
 * functions.
 */

typedef struct SIF_EXPORT sif_meta_data {

  /**
   * @brief The key identifier of this meta-data field.
   */

  char*                  key;

  /**
   * @brief The value of this meta-data field.
   */

  char*                  value;

  /**
   * @brief The number of bytes to store the key and its null terminator.
   */

  unsigned long          key_length;

  /**
   * @brief The number of bytes to store the value. If the value is binary,
   * the null terminator is included in this count.
   */

  unsigned long          value_length;

  /**
   * @brief A pointer to the next meta-data field. The value is NULL if
   * there is no next meta-data field.
   */

  struct sif_meta_data*  next;

} sif_meta_data;

/** \internal
   We need the CFile class for 64-bit file support in windows. Microsoft does
   not conform to the LFS standard.
*/

/**
 * \struct sif_file
 * @brief A struct for storing necessary data for the processing of an
 * open file.
 *
 * @warning Do not modify this data structure directly.
 */


typedef struct SIF_EXPORT {
#ifdef WIN32 
  /** @brief The handle to the internal file pointer. */
  HANDLE                   fp;
#else
  /** @brief The handle to the internal file pointer. */
  FILE*                    fp;
#endif
  /**
   * @brief The header corresponding to the target file.
   */
 
  sif_header*              header;

  /**
   * @brief An array of tiles to store.
   */

  sif_tile*                tiles;

  /**
   * @brief The meta-data for the file. This structure is a linked list
   * of (key, value) pairs. Meta-data in SIF can be null-terminated strings
   * or binary data blocks.   
   */
  sif_meta_data**          meta_data;

  /**
   * @brief A flag indicating whether the file is open in
   * read-only mode.
   */
  int                      read_only;

  /**
   * @brief An array where the i'th value is the tile index of the
   * tile stored in data block i.

   * If no tile is stored in data block i, the block is unused,
   * and the corresponding value in this array is set to -1. Unused
   * blocks can be reclaimed for use or truncated when the file is
   * consolidated or defragmented.
   */

  long*                    blocks_to_tiles;

  /**
   * @brief An array of booleans where the i'th value is one iff the
   * i'th tile has been written and no uniformity check was made
   * during the write. In this future, the type of the values contained
   * in the array will be changed to char*, pending confirmation that
   * the change does not break regression tests.
   */

  long*                    dirty_tiles;

  /**
   * @brief Two buffers with enough memory to each store one block. The
   * number of bytes for one block is computed by,
   * \code
   *    tile_width * tile_height * n_bands * data_unit_size .
   * \endcode
   */

  void*                    buffer[2];

  /**
   * \internal @brief Stores the projection string, which is expected
   * to be empty ("") or in  OpenGIS WKT format.
   *
   * @warning Do not modify this value directly. Instead use the
   * \ref sif_set_projection function.
   */

  /**  char*                  proj;**/

  /**
   * @brief Stores the byte offset of the first block in the file.
   */

  LONGLONG                 base_location;

  /**
   * @brief An error code for the last error that occurred. The value
   * is non-zero if an error occurred during the last sif-io call.
   */

  int                      error;

  /**
   * @brief When the C standary library is used, it represents
   * the last errno encountered when executing a libc function in
   * a sif-io function. Otherwise, it represents the WIN32 error code
   * returned by the GetLastErr function.
   */

  long                     sys_error_no;

  /**
   * @brief The number of pixels per band in a tile (slice). This value
   * is simply tile_width * tile_height. The term slice differs slightly
   * from the term band, it is a band within a tile.
   */
  long                     units_per_slice;

  /**
   * @brief The number of pixels per tile. This value is simply the number
   * of units_per_slice times the number of bands in the image. This number
   * is also the number of units in a block.
   */

  long                     units_per_tile;

  /**
   * @brief The number of bytes to store the header.
   */

  long                     header_bytes;

  /**
   * @brief A character buffer with enough bytes to store a 64-bit integer.
   */

  u_char                   ubuf[8];

  /**
   * @brief A integer representing the SIF file version to use when
   * writing the file. This is used to ensure that the file is written
   * with an earlier version so that it can be read by previous versions
   * of this library. 
   */

  long                     use_file_version;

  /**
   * @brief A buffered only used by the SIF "simple" interface for byte
   * swapping prior to writing to a file. It is initially holds the number
   * of bytes needed to store a tile slice but grows as larger regions are
   * written.
   */

  void*                    simple_region_buffer;

  /**
   * @brief The size of the simple region buffer (in bytes).
   */

  long                     simple_region_bytes;

  /**
   * @brief The line number of sif-io.c where the last SIF error occurred.
   */

  int                      error_line_no;

  /**
   * @brief The function to call to write image buffers or uniform pixel value
   * buffers to a disk.
   */

  sif_buffer_preprocessor  preprocessor;

  /**
   * @brief The function to call to read image buffers or uniform pixel value
   * buffers to a disk.
   */

  sif_buffer_postprocessor postprocessor;

} sif_file;

/**
 * @brief Return the latest version of the SIF file format that the
 * currently loaded SIF library can process.
 *
 * @return The latest version number of a SIF file this library can
 *         process.
 */

SIF_EXPORT long             sif_get_version();

/**
 * @brief Open a Sparse Image File (SIF) format file for reading or update.
 *
 * @param filename  The filename of the SIF file to open.
 * @param read_only A flag indicating whether to open as read-only (1)
 *                  or update (0).
 *
 * @return A file structure containing all the constructs needed to
 *         manipulate the opened SIF file is returned. NULL is returned
 *         if an error occured during open.
 */

SIF_EXPORT sif_file*        sif_open(const char* filename, int read_only);

/**
 * @brief Create a new Sparse Image Format (SIF) file with a given filename
 * and attributes. The file's header and tile headers are written. No
 * space is preallocated for data blocks.
 *
 * @param filename              The filename of the new file.
 * @param width                 The width of the image to store in the file to create.
 * @param height                The height of the image to store in the file to create.
 * @param bands                 The number of bands of the image to store in the file to create.
 * @param data_unit_size        The size of a single pixel in bytes, e.g. <code>sizeof(pixel_data_type)</code>.
 * @param user_data_type        A user-defined data type. The SIF I/O functions do not
 *                              look at this value. This is strictly for the user's reference
 *                              when opening a pre-existing file.
 * @param consolidate_on_close  Defines whether an intrinsic uniformity check should be applied to
 *                              dirty tiles during each close.
 * @param defragment_on_close   Defines whether the file should be defragmented during each close.
 * @param tile_width            The width of a single tile.
 * @param tile_height           The height of a single tile.
 * @param intrinsic_write       Defines whether intrinsic uniformity checks should be performed
 *                              when rasters are written to a file.
 *
 * @return  A file structure containing the constructs needed to manipulate the file created
 *          by this function. This function returns NULL if an error occurs during creation.
 */

SIF_EXPORT sif_file*        sif_create(const char *filename, long width, long height,
				       long bands, int data_unit_size,
				       int user_data_type, int consolidate_on_close,
				       int defragment_on_close,
				       long tile_width, long tile_height,
				       int intrinsic_write);

/**
 * @brief Create a copy of a SIF file.
 *
 * @warning Note that this function has neither been tested nor ported for use with WIN32+MSVS.
 *
 * @param file      The file structure pointing to the file to copy. This
 *                  file is flushed before its contents are read.
 * @param filename  The filename of the file to store the copy.
 *
 * @return  A file structure containing the constructs needed to manipulate the file copied
 *          by this function. This function returns NULL if an error occurs during file
 *          creation.
 */

SIF_EXPORT sif_file*        sif_create_copy(sif_file *file, const char *filename);

/**
 * @brief Close a SIF file.

 * If the file is open for reading and writing, defragmentation
 * and consolidation occur only if the defragment and consolidate
 * flags are set in the file's header. The file header, tile headers, and
 * meta data are written upon close.
 *
 * @param file   The SIF file to close.
 *
 * @return       The status of the close.
 *
 * @see sif_header::consolidate
 * @see sif_header::defragment
 * @see sif_set_consolidate
 * @see sif_unset_consolidate
 * @see sif_is_consolidate_set
 */

SIF_EXPORT int              sif_close(sif_file* file);

/**
 * @brief Check all tiles in a file for intrinsic uniformity.
 *
 * If a tile is found to be intrinsically uniform, its common pixel values
 * for each slice is stored in its header and the physical storage block it
 * is using is freed. If the consolidation flag in the file's header is
 * turned off or the file is read only, this method does nothing.
 *
 * @param file   The file to mark for uniformity.
 */

SIF_EXPORT void             sif_consolidate(sif_file* file);

/**
 * @brief Defragment the file.
 *
 * This results in a sort of the storage blocks
 * so they are in the order of their corresponding tile indices. This
 * enables faster reading/writing of continguous blocks. No unused
 * storage blocks remain in the file (i.e. the used blocks are shifted
 * so that they write over the unused blocks). The file is
 * truncated at the position of the last used storage block byte. Meta-data
 * and the file's header are rewritten.
 *
 * @param file   The file to defragment.
 */

SIF_EXPORT void             sif_defragment(sif_file* file);

/**
 * @brief Writes a rectangular image region to a file.
 *
 * The tiles changed by this write are not checked for pixel uniformity.
 * This results in the dirty flags in their respective tile headers being
 * set to true. This results in a uniformity check during the file's close unless
 * the uniformity check flag is set to false in the file's header. Also, any
 * fragmentation caused by this function is not resolved until the file is closed.
 *
 * @warning This function has not been tested.
 *
 * @param file   The file on which to write the raster plane.
 * @param data   The buffer containing the raster to write.
 * @param x      The starting horizontal pixel offset (0..N-1 indexed) to write.
 * @param y      The starting vertical pixel offset (0..N-1 indexed) to write.
 * @param w      The width of the region.
 * @param h      The height of the region.
 * @param band   The band offset (0..N-1 indexed).
 *
 * @see sif_get_raster
 * @see sif_get_tile_slice
 * @see sif_set_tile_slice
 */

SIF_EXPORT void             sif_set_raster(sif_file* file, const void *data,
                                long x, long y, long w, long h, long band);

/**
 * @brief Reads a rectangular raster region from a file. It may overlap
 * multiple tiles in the file.
 *
 * @warning This function has not been tested.
 *
 * @param file   The file on which to read the raster plane out.
 * @param data   The buffer to store the raster plane.
 * @param x      The starting horizontal pixel offset (0..N-1 indexed) to read.
 * @param y      The starting vertical pixel offset (0..N-1 indexed) to read.
 * @param w      The width of the region.
 * @param h      The height of the region.
 * @param band   The band offset (0..N-1 indexed).
 *
 * @see sif_set_raster
 * @see sif_get_tile_slice
 * @see sif_set_tile_slice
 */


SIF_EXPORT void             sif_get_raster(sif_file* file, void *data,
                                long x, long y, long w, long h, long band);

/**
 * @brief Fill all tiles of a particular band with a constant value.
 *
 * If uniformity results as a result of this fill, the corresponding
 * tiles are marked appropriately and the block space they use is
 * freed.
 *
 * @param file  The file on which to perform the fill.
 * @param band  The band index of the tile to retrieve (0..N-1 indexed).
 * @param value The value to fill all values of the slice. It
 *              must be data_unit_size in bytes.
 */

SIF_EXPORT void             sif_fill_tiles(sif_file *file, long band, const void *value);

/**
 * @brief Retrieve a tile slice.
 *
 * If the tile is uniform, no access to the disk is made; instead,
 * the uniform pixel value for the band in the tile's header is
 * used to fill the buffer. The buffer must contain enough bytes
 * to hold a slice.
 *
 * @param file   The file on which to perform the fill.
 * @param tx     The horizontal index of the slice to retrieve (0..N-1 indexed).
 * @param ty     The vertical index of the slice to retrieve (0..N-1 indexed).
 * @param band   The band index of the slice to retrieve (0..N-1 indexed).
 * @param buffer The buffer to store the tile slice. It must be data_unit_size in
 *               bytes.
 *
 * @see sif_fill_tile_slice
 */

SIF_EXPORT void             sif_get_tile_slice(sif_file *file, void *buffer, long tx, long ty, long band);

/**
 * @brief Store a tile slice.
 *
 * A check is not made to determine pixel uniformity. The tile's dirty flag
 * is set to true. This results in a uniformity check during the file's close,
 * unless uniformity check flag is disabled in the file's header. Also, any
 * fragmentation caused by this function is not resolved until the file is closed.
 *
 * @param file    The file on which to perform the write.
 * @param tx      The horizontal index (0..N-1 indexed) of the slice to write.
 * @param ty      The vertical index (0..N-1 indexed) of the slice to write.
 * @param band    The band index (0..N-1 indexed) of the slice to write.
 * @param buffer  The buffer to write. It must have enough bytes for an entire
 *                tile slice, e.g. \ref sif_header::tile_width *
 *                \ref sif_header::tile_height * \ref sif_header::data_unit_size.
 */

SIF_EXPORT void             sif_set_tile_slice(sif_file *file, const void *buffer, long tx, long ty, long band);

/**
 * @brief Fill a tile slice with a constant value.
 * 
 * If all bands become uniform as a result of this fill, the block for this
 * slice's corresponding cube will be freed.
 *
 * @warning This function has not been tested.
 *
 * @param file  The file on which to perform the fill.
 * @param tx    The horizontal index of the tile to fill (0..N-1 indexed).
 * @param ty    The vertical index of the tile to fill (0..N-1 indexed).
 * @param band  The band index of the tile to fill (0..N-1 indexed).
 * @param value The value to fill all values of the slice. It
 *              must be \ref sif_header::data_unit_size bytes in size.
 *
 * @see sif_fill_tiles
 */

SIF_EXPORT void             sif_fill_tile_slice(sif_file *file, long tx, long ty, long band, const void *value);

/**
 * @brief Set a meta-data field with a given key to a value defined by
 * a null-terminated character string.
 *
 * @param file      The file to set the meta-data.
 * @param key       The key of the field to set.
 * @param value     The value to set the field.
 *
 * @see sif_set_meta_data_binary
 * @see sif_get_meta_data
 * @see sif_get_meta_data_binary
 */

SIF_EXPORT void             sif_set_meta_data(sif_file *file, const char *key, const char *value);

/**
 * @brief Set a meta-data field with a given key to a given
 * sequence of bytes.
 *
 * The length of the value is passed here, thereby allowing for
 * binary, non-null-terminated, meta-data.
 *
 * @param file      The file on which to set the meta-data field.
 * @param key       The key of the field to set.
 * @param buffer    The value to set the field.
 * @param n_bytes   The length of the value (in bytes).
 *
 * @warning The meta-data is not written to the file until the file
 * is closed or flushed.
 *
 * @see sif_set_meta_data
 * @see sif_get_meta_data
 * @see sif_get_meta_data_binary
 */

SIF_EXPORT void             sif_set_meta_data_binary(sif_file *file, const char *key, const void *buffer, int n_bytes);

/**
 * @brief Get a string meta-data field with a given key. This function
 * returns 0 and sets the error field in the file's header if the buffer
 * stored for this meta-data is not a null-terminated string or if the
 * field with the given key string could not be found.
 *
 * @param file      The file on which to set the meta-data field.
 * @param key       The key of the field to set.
 *
 * @return          The value of the field.
 *
 * @see sif_get_meta_data_binary
 * @see sif_set_meta_data
 * @see sif_set_meta_data_binary
 */

SIF_EXPORT const char*      sif_get_meta_data(sif_file *file, const char *key);

/**
 * @brief Get a string meta-data field with a given key. This function
 * returns 0 and sets the error field in the file's header.
 *
 * @param file      The file to set the meta-data.
 * @param key       The key of the field to set.
 * @param n_bytes   A pointer to an integer value. This
 *                  value is set to the size of the buffer
 *                  returned.
 *
 * @return          The value of the field.
 */


SIF_EXPORT const void*      sif_get_meta_data_binary(sif_file *file, const char *key, int *n_bytes);

/**
 * @brief Determine if the tiles comprising a region are shallow uniform.
 *
 * @param file          The file to perform the check.
 * @param x             The starting horizontal pixel offset (0..N-1 indexed)
 *                      of the region to check.
 * @param y             The starting vertical pixel offset (0..N-1 indexed)
 *                      of the region to check.
 * @param w             The width of the region.to check.
 * @param h             The height of the region to check.
 * @param band          The band offset (0..N-1 indexed).
 * @param uniform_value This value is only meaningful when this function
 *                      returns true (non-zero). It is expected that the
 *                      pointer passed point to at least data_unit_size bytes.
 *                      When the region is completely uniform, the uniform
 *                      pixel value is stored here.
 *
 * @return 0 if the tiles are not shallow uniform or a memory allocation error
 *           occurred, otherwise a non-zero value.
 */

SIF_EXPORT int              sif_is_shallow_uniform(sif_file *file, long x, long y, long w, long h, long band, void *uniform_value);

/**
 * @brief Determine if a tile has shallow uniformity.
 *
 * @param file          The file to perform the check.
 * @param tx            The horizontal tile index (0..N-1 indexed)
 *                      of the region to check.
 * @param ty            The vertical tile index (0..N-1 indexed)
 *                      of the region to check.
 * @param band          The band offset (0..N-1 indexed).
 * @param uniform_value This value is only meaningful when this function
 *                      returns true (non-zero). It is expected that the
 *                      pointer passed point to at least data_unit_size bytes.
 *                      When the region is completely uniform, the uniform
 *                      pixel value is stored here.
 *
 * @return 0 if the tiles are non-uniform or a memory allocation error
 *           occurred, otherwise a non-zero value.
 */

SIF_EXPORT int              sif_is_slice_shallow_uniform(sif_file *file, long tx, long ty, long band, void *uniform_value);

/**
 * @brief Flush all remaining unwritten data to the file.
 *
 * This function immediately returns if the file passed is read-only.
 *
 * @param file   The SIF file to flush.
 *
 * @return A non-zero value if no error occurred during the flush.
 */

SIF_EXPORT int              sif_flush(sif_file* file);

/**
 * @brief Set the user data type for the file.
 *
 * This value does not change the behavior of any sif-io functions. The user
 * may use it to store an integer representing the data type of the pixel
 * values in the image.
 *
 * @param file            The file to change the data type flag.
 * @param user_data_type  The value of the new user-defined data type flag.
 *
 */

SIF_EXPORT void             sif_set_user_data_type(sif_file *file, long user_data_type);

/**
 * @brief Get the user data type integer for the file.
 *
 * This value does not change the behavior of any \ref sif-io.h functions. The user
 * may use it to store an integer representing the data type of the pixel
 * values in the image.
 *
 * @param file            The file on which to get the user-defined data type flag.
 *
 * @return                The user-defined data type of the data units in the
 *                        file.
 */

SIF_EXPORT long             sif_get_user_data_type(sif_file *file);

/**
 * @brief Set the uniformity flag. This results in a pixel uniformity check on
 * all dirty tiles.
 *
 * @param file            The file to change.
 * @see sif_is_intrinsic_write_set
 * @see sif_unset_intrinsic_write
 * @see sif_is_shallow_uniform
 * @see sif_is_slice_shallow_uniform
 */

void                         sif_set_intrinsic_write(sif_file *file);

/**
 * @brief Return the value of the uniformity flag. When true, a uniformity
 * check is perfomed on all dirty tiles during close.
 *
 * @param file            The file to check.
 * @see sif_unset_intrinsic_write
 * @see sif_is_intrinsic_write_set
 * @see sif_is_shallow_uniform
 * @see sif_is_slice_shallow_uniform
 *
 * @return The value of the flag.
 */

SIF_EXPORT int              sif_is_intrinsic_write_set(sif_file *file);

/**
 * @brief Unset the uniformity flag.
 *
 * This cancels pixel uniformity checks during close.
 *
 * @param file            The file to change.
 * @see sif_unset_intrinsic_write
 * @see sif_set_intrinsic_write
 * @see sif_is_shallow_uniform
 * @see sif_is_slice_shallow_uniform
 */

SIF_EXPORT void             sif_unset_intrinsic_write(sif_file *file);

/**
 * @brief Set the defragmentation flag.
 *
 * Defragmentation is then performed during the file's close. Data blocks
 * are rearranged in the order they appear in the image.
 *
 * @param file            The file to change.
 * @see sif_unset_defragment
 * @see sif_is_defragment_set
 */

SIF_EXPORT void             sif_set_defragment(sif_file *file);

/**
 * @brief Unsets the defragmentation flag.
 *
 * This cancels defragmentation when the file is closed.
 *
 * @param file            The file to change.
 *
 * @return The value of the defragmentation flag.
 * @see sif_set_defragment
 * @see sif_unset_defragment
 * @see sif_defragment
 * @see sif_close
 * @see sif_header::defragment
 */

SIF_EXPORT int               sif_is_defragment_set(sif_file *file);

/**
 * @brief Set the defragmentation flag.
 *
 * Defragmentation on the file's close is cancelled.
 *
 * @param file            The file to change.
 * @see sif_set_defragment
 * @see sif_is_defragment_set
 * @see sif_defragment
 * @see sif_close
 * @see sif_header::defragment
 */

SIF_EXPORT void              sif_unset_defragment(sif_file *file);

/**
 * @brief Set the consolidation flag.
 *
 * Consolidation is then performed during the files close.
 *
 * @param file            The file to change.
 * @see sif_unset_consolidate
 * @see sif_is_consolidate_set
 * @see sif_consolidate
 * @see sif_close
 * @see sif_header::consolidate
 */

SIF_EXPORT void              sif_set_consolidate(sif_file *file);

/**
 * @brief Return whether the file will be scheduled for consolidation
 * on its close. Used blocks are moved toward the begining of the
 * file, taking up the space of unused blocks before them. If there
 * are no unused blocks or all of the unused blocks are at the end of
 * the file, this file is simply truncated at the location of the first
 * byte of the unused block and the meta-data is rewritten.
 *
 * @param file            The file to check.
 *
 * @return The value of the consolidation flag.
 * @see sif_unset_consolidate
 * @see sif_set_consolidate
 * @see sif_consolidate
 * @see sif_close
 * @see sif_header::consolidate
 */

SIF_EXPORT int               sif_is_consolidate_set(sif_file *file);

/**
 * @brief Unset the consolidation flag.
 *
 * Consolidation is then performed during the files close.
 *
 * @param file            The file to change.
 * @see sif_set_consolidate
 * @see sif_is_consolidate_set
 * @see sif_consolidate
 * @see sif_close
 * @see sif_header::consolidate
 */

SIF_EXPORT void              sif_unset_consolidate(sif_file *file);

/**
 * @brief Set the affine georeferencing transform of an open file.
 *
 * @param file            The file to set the affine georeferencing transform.
 * @param trans           A double array of size 6 with the new value of the
 *                        georeferencing transform.
 *
 * @see sif_get_affine_geo_transform
 * @see sif_header::affine_geo_transform
 */

SIF_EXPORT void              sif_set_affine_geo_transform(sif_file *file,
							  const double *trans);

/**
 * @brief Get the affine georeferencing transform of an open file.
 *
 * @param file            The file to get the transform.
 * @return                An array of six doubles representing the transform.
 *
 * @see sif_set_affine_geo_transform
 * @see sif_header::affine_geo_transform
 */

SIF_EXPORT const double *    sif_get_affine_geo_transform(sif_file *file);

/**
 * @brief Return the projection string of an open file. It is expected
 * to be in OpenGIS WKT format. The string is stored in the "_sif_proj"
 * field in the meta-data region of the file. If the projection string
 * cannot be found, the empty string is returned.
 *
 * @param file             The file from which to get the projection string.
 *
 * @return The projection string.
 *
 * @see sif_set_projection
 */

SIF_EXPORT const char *      sif_get_projection(sif_file *file);

/**
 * @brief Set the projection string of an open file. This is expected
 * to be empty ("") or in OpenGIS WKT format. The string is stored in
 * the "_sif_proj" field in the meta-data region of the file.
 *
 * @param file             The file to set the projection string.
 * @param proj             The new projection string value.
 *
 * @see sif_get_projection
 */

SIF_EXPORT void              sif_set_projection(sif_file *file, const char *proj);

/**
 * @brief Return a string indicating the data type convention used in
 *        this file. If the string is <code>"gdal"</code> then the GDT type codes in
 *        the GDAL library are used. If the string is <code>"simple"</code> then
 *        the convention presented earlier in this document is used.
 *
 * @param file             The file from which to get the projection string.
 *
 * @return The convention agreement string.
 *
 * @see sif_set_agreement
 */

SIF_EXPORT const char *      sif_get_agreement(sif_file *file);

/**
 * @brief Set a string indicating the data type convention used in this
 *        file. If the string is "gdal" then the GDT type codes in the
 *        GDAL library are used. If the string is "simple" then the
 *        convention presented earlier in this document is used.
 *
 * @param file             The file to set the projection string.
 * @param agree            The new projection string value.
 *
 * @see sif_get_agreement
 */

SIF_EXPORT void              sif_set_agreement(sif_file *file, const char *agree);

/**
 * @brief Get the number of meta data (key, value) pairs in this file.
 *
 * @param file             The file from which to get the number of meta-data items.
 */

SIF_EXPORT int               sif_get_meta_data_num_items(sif_file *file);

/**
 * @brief Retrieve the keys of the meta data stored in the file. It is the
 * responsibility of the caller to free the memory to which *key_strs points
 * but not (*key_strs)[i] for any i, non-negative i < \ref sif_header::n_keys.
 *
 * @param file             The file from which to retrieve the meta-data keys.
 * @param key_strs         A pointer pointing to the pointer to set to the location
 *                         of the array of strings. The last value of the array of
 *                         strings is set to 0 (sentinel).
 * @param num_keys         A pointer to the int to store the number of keys retrieved
 *                         by this function.
 */

SIF_EXPORT void              sif_get_meta_data_keys(sif_file *file, const char *** key_strs, int *num_keys);

/**
 * @brief Removes a meta-data item by its key string.
 *
 * @param file             The file from which to remove a meta-data item.
 * @param key              The key of the meta-data item to remove.
 */

SIF_EXPORT void              sif_remove_meta_data_item(sif_file *file, const char *key);

/**
 * @brief Specifies that when the file is written, the file should be
 * written with using the SIF file format version specified. If the version
 * is not supported for write, a SIF_ERROR_CANNOT_WRITE_VERSION error
 * is set in the header's error code field.
 */
SIF_EXPORT void              sif_use_file_format_version(sif_file *file, long version);

/**
 * @brief Returns a positive value if the file referred to by <code>filename</code>
 * could possibly be a SIF file.
 *
 * @param filename         The filename of the file to check.
 *
 * @return A boolean indicating the result of the check.
 */

SIF_EXPORT int               sif_is_possibly_sif_file(const char *filename);

/**
 * @brief Returns a description of a SIF error code.
 *
 * @param code             The code of the error.
 *
 * @return A description of the error as a string. Returns -1 if the file could not
 * be opened. Returns 0 if the open was successful but the file is not a SIF file.
 */

SIF_EXPORT const char *      sif_get_error_description(int code);

/**
 * \defgroup simpfuncs Simple Data-Type Convention Declarations
 */

/**
 * \def SIF_SIMPLE_ERROR_UNDEFINED_DT
 *
 * @brief An error indicating that the data type is not recognized as a data type
 * in the simple data-type convention.
 */

#define SIF_SIMPLE_ERROR_UNDEFINED_DT 100

/**
 * \def SIF_SIMPLE_ERROR_INCORRECT_DT
 *
 * @brief An error to indicate the data type of the image does not correspond to
 * the data type requested.
 */

#define SIF_SIMPLE_ERROR_INCORRECT_DT 101

/**
 * \def SIF_SIMPLE_ERROR_UNDEFINED_ENDIAN
 *
 * @brief An error to indicate an endian code is invalid.
 */

#define SIF_SIMPLE_ERROR_UNDEFINED_ENDIAN 102

/**
 * \def SIF_SIMPLE_UINT8
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing unsigned 8-bit
 * integers.
 */

#define SIF_SIMPLE_UINT8 0

/**
 * \def SIF_SIMPLE_INT8
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing signed 8-bit
 * integers.
 */

#define SIF_SIMPLE_INT8 1

/**
 * \def SIF_SIMPLE_UINT16
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing unsigned 16-bit
 * integers.
 */

#define SIF_SIMPLE_UINT16 2

/**
 * \def SIF_SIMPLE_INT16
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing signed 16-bit
 * integers.
 */

#define SIF_SIMPLE_INT16 3

/**
 * \def SIF_SIMPLE_UINT32
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing unsigned 32-bit
 * integers.
 */

#define SIF_SIMPLE_UINT32 4

/**
 * \def SIF_SIMPLE_INT32
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing signed 32-bit
 * integers.
 */

#define SIF_SIMPLE_INT32 5

/**
 * \def SIF_SIMPLE_UINT64
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing unsigned 64-bit
 * integers.
 */

#define SIF_SIMPLE_UINT64 6

/**
 * \def SIF_SIMPLE_INT64
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing signed 64-bit
 * integers.
 */

#define SIF_SIMPLE_INT64 7

/**
 * \def SIF_SIMPLE_FLOAT32
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing IEEE-754 standard
 * 32-bit floats.
 */

#define SIF_SIMPLE_FLOAT32 8

/**
 * \def SIF_SIMPLE_FLOAT64
 * \ingroup simpdecs
 *
 * @brief The base type code (i.e. <code>user_data_type mod 10</code>) for storing IEEE-754 standard
 * 64-bit floats.
 */

#define SIF_SIMPLE_FLOAT64 9

/**
 * \def SIF_SIMPLE_LITTLE_ENDIAN
 * \ingroup simpdecs
 *
 * @brief The endian code for little endian.
 */

#define SIF_SIMPLE_LITTLE_ENDIAN 0

/**
 * \def SIF_SIMPLE_BIG_ENDIAN
 * \ingroup simpdecs
 *
 * @brief The endian code for little endian.
 */

#define SIF_SIMPLE_BIG_ENDIAN 1

/**
 * \def SIF_SIMPLE_NATIVE_ENDIAN
 * \ingroup simpdecs
 *
 * @brief The endian code for the byte order of the native machine on which this library runs.
 */

#if defined(WIN32) || __BYTE_ORDER == __LITTLE_ENDIAN
#define SIF_SIMPLE_NATIVE_ENDIAN SIF_SIMPLE_LITTLE_ENDIAN 
#elif __BYTE_ORDER == __BIG_ENDIAN
#define SIF_SIMPLE_NATIVE_ENDIAN SIF_SIMPLE_BIG_ENDIAN
#endif  

/**
 * \def SIF_SIMPLE_ENDIAN(t)
 * \ingroup simpdecs
 *
 * @brief A function macro that returns the endian code for a simple type code \a t.
 */

#define SIF_SIMPLE_ENDIAN(t) (((int)t)/10)

/**
 * \def SIF_SIMPLE_TYPE_CODE(bt, ec)
 * \ingroup simpdecs
 *
 * @brief A function macro that computes the compound type code from the base type code \a bt
 * and endian code \a ec.
 */

#define SIF_SIMPLE_TYPE_CODE(bt, ec) ((bt)+(ec))

/**
 * \def SIF_SIMPLE_BASE_TYPE_CODE(bt, ec)
 * \ingroup simpdecs
 *
 * @brief A function macro that computes the base type code from the compound type code.
 */

#define SIF_SIMPLE_BASE_TYPE_CODE(x) (((int)x)%10)


/**
 * @brief Create a new Sparse Image Format (SIF) file with a given filename
 * and attributes. The file's header and tile headers are written. No
 * space is preallocated for data blocks. The simple data-type convention
 * is used. When reading, writing, or filling the image file created by
 * this function, the <code>sif_simple_*</code> functions must be used
 * to ensure the image data elements are written with the proper byte
 * order.
 *
 * Unless \ref sif_simple_set_endian is called prior to reading or writing
 * any image raster, the pixels will be stored in native byte order.
 *
 * @param filename              The filename of the new file.
 * @param width                 The width of the image to store in the file to create.
 * @param height                The height of the image to store in the file to create.
 * @param bands                 The number of bands of the image to store in the file to create.
 * @param simple_data_type      The data type code.
 * @param tile_width            The width of a single tile.
 * @param tile_height           The height of a single tile.
 * @param consolidate_on_close  Defines whether a pixel uniformity check should be applied to
 *                              dirty tiles during each close.
 * @param defragment_on_close   Defines whether the file should be defragmented during each close.
 * @param intrinsic_write       Defines whether intrinsic uniformity checks should be performed
 *                              when rasters are written to a file.
 *
 * @return  A file structure containing the constructs needed to manipulate the file created
 *          by this function. This function returns NULL if an error occurs during creation.
 */

SIF_EXPORT sif_file*        sif_simple_create(const char *filename,
					      long width, long height,
					      long bands,
					      int simple_data_type,
					      int consolidate_on_close,
					      int defragment_on_close,
					      long tile_width, long tile_height,
					      int intrinsic_write);

/**
 * @brief Create a new Sparse Image Format (SIF) file with a given filename
 * and attributes. The file's header and tile headers are written. No
 * space is preallocated for data blocks. The simple data-type convention
 * is used.
 *
 * When reading, writing, or filling the image file created by
 * this function, the <code>sif_simple_*</code> functions must be used
 * to ensure the image data elements are written with the proper byte
 * order.
 *
 * Unless \ref sif_simple_set_endian is called prior to reading or writing
 * any image raster, the pixels will be stored in native byte order.
 *
 * The sif_header::consolidate, sif_header::defragment and the
 * sif_header::intrisic_write flags are all set to true. The sif_header::tile_width
 * and sif_header::tile_height are both set to 64.
 *
 * @param filename              The filename of the new file.
 * @param width                 The width of the image to store in the file to create.
 * @param height                The height of the image to store in the file to create.
 * @param bands                 The number of bands of the image to store in the file to create.
 * @param simple_data_type      The data type code.
 *
 * @return  A file structure containing the constructs needed to manipulate the file created
 *          by this function. This function returns NULL if an error occurs during creation.
 */

SIF_EXPORT sif_file*        sif_simple_create_defaults(const char *filename,
						       long width, long height,
						       long bands,
						       int simple_data_type);

/**
 * @brief Set the network byte order for the pixel. Note that this field
 * should never be set once a raster is written to a file.
 * This field must be set to one of \ref SIF_SIMPLE_LITTLE_ENDIAN or
 * \ref SIF_SIMPLE_BIG_ENDIAN.
 *
 * @param  file       The file on which to perform the operation.
 * @param  endian     The endian code to set the file.
 */

SIF_EXPORT void               sif_simple_set_endian(sif_file *file, int endian);

/**
 * @brief Get the network byte order of the pixel values in the image
 * of this file.
 *
 * @param  file       The file on which to perform the operation.
 *
 * @return            The endian code of this file.
 */

SIF_EXPORT int               sif_simple_get_endian(sif_file *file);

/**
 * @brief Set the simple data type code for the pixel. Note that this field
 * should never be set once a raster is written to a file.
 *
 * @param  file       The file on which to perform the operation.
 * @param  code       The simple data type code of the pixel values.
 * @see simpdecs
 */

SIF_EXPORT void               sif_simple_set_data_type(sif_file *file, int code);

/**
 * @brief Get the simple data type code of the pixel values in the image
 * of this file.
 *
 * @param  file       The file on which to perform the operation.
 *
 * @return            The simple data type code of the pixel values.
 * @see simpdecs
 */

SIF_EXPORT int               sif_simple_get_data_type(sif_file *file);

/**
 * @brief Write a rectangular region to a file. The byte order of the
 * data values is automatically converted from host order to the the
 * byte order of the file.
 *
 * @param file The file on which to perform the operation.
 * @param data The buffer containing the data to write.
 * @param x    The horizontal starting index of the file.
 * @param y    The vertical starting index of the file.
 * @param w    The width of the region.
 * @param h    The height of the region.
 * @param band The band of the region.
 */

SIF_EXPORT void              sif_simple_set_raster(sif_file* file,
						   const void *data,
						   long x, long y,
						   long w, long h,
						   long band);

/**
 * @brief Read a rectangular region from a file. The byte order of
 * the data values in the buffer is automatically converted to the
 * byte order of the file.
 *
 * @param file The file on which to perform the operation.
 * @param data The buffer into which the data will be read.
 * @param x    The horizontal starting index of the file.
 * @param y    The vertical starting index of the file.
 * @param w    The width of the region.
 * @param h    The height of the region.
 * @param band The band of the region.
 */

SIF_EXPORT void              sif_simple_get_raster(sif_file* file,
						   void *data,
						   long x, long y,
						   long w, long h,
						   long band);
/**
 * @brief Fill a band with a constant value. The byte order of the
 * value is converted to the byte order of the file's image.
 *
 * @param file   The file on which to perform the operation.
 * @param band   The band to fill.
 * @param value  The pointer to the value with which to fill the band.
 */

SIF_EXPORT void              sif_simple_fill_tiles(sif_file *file,
						   long band, const void *value);

/**
 * @brief Retrieve a tile slice. The byte order of the data values
 * in the buffer are in host byte order.
 *
 * @param file   The file on which to perform the operation.
 * @param buffer The buffer to store the slice.
 * @param tx     The horizontal index of the tile slice.
 * @param ty     The vertical index of the tile slice.
 * @param band   The band of the tile to which the slice corresponds.
 */

SIF_EXPORT void              sif_simple_get_tile_slice(sif_file *file, void *buffer,
						       long tx, long ty, long band);
/**
 * @brief Store a tile slice. The byte order of the data values
 * in the buffer are converted to the byte order of the file.
 *
 * @param file   The file on which to perform the operation.
 * @param buffer The buffer to store the slice.
 * @param tx     The horizontal index of the tile slice.
 * @param ty     The vertical index of the tile slice.
 * @param band   The band of the tile to which the slice corresponds.
 */

SIF_EXPORT void              sif_simple_set_tile_slice(sif_file *file,
						       const void *buffer,
						       long tx, long ty, long band);

/**
 * @brief Fill a tile slice with a constant value. The value is
 * converted from host order to the byte order in the file.
 *
 * @param file   The file on which to perform the operation.
 * @param tx     The horizontal index of the tile slice.
 * @param ty     The vertical index of the tile slice.
 * @param band   The band of the tile to which the slice corresponds.
 * @param value  The pointer to the value to fill the tile slice.
 */

SIF_EXPORT void              sif_simple_fill_tile_slice(sif_file *file,
							long tx, long ty,
							long band, const void *value);

/**
 * @brief Open a Sparse Image File (SIF) format file for reading or update.
 * The file is expected to use the Simple data type convention.
 *
 * When reading, writing, or filling the image file opened by
 * this function, the <code>sif_simple_*</code> functions must be used
 * to ensure the image data elements are written with the proper byte
 * order.
 *
 * If the file does not use the simple data-type convention, 0 is returned.
 *
 * @param filename  The filename of the SIF file to open.
 * @param read_only A flag indicating whether to open as read-only (1)
 *                  or update (0).
 *
 * @return A file structure containing all the constructs needed to
 *         manipulate the opened SIF file is returned. NULL is returned
 *         if an error occured during open.
 */

SIF_EXPORT sif_file*        sif_simple_open(const char* filename, int read_only);

/**
 * @brief Determine if the tiles comprising a region are shallow uniform. The
 * "simple" data-type convention is assumed.
 *
 * @param file          The file to perform the check.
 * @param x             The starting horizontal pixel offset (0..N-1 indexed)
 *                      of the region to check.
 * @param y             The starting vertical pixel offset (0..N-1 indexed)
 *                      of the region to check.
 * @param w             The width of the region.to check.
 * @param h             The height of the region to check.
 * @param band          The band offset (0..N-1 indexed).
 * @param uniform_value This value is only meaningful when this function
 *                      returns true (non-zero). It is expected that the
 *                      pointer passed point to at least data_unit_size bytes.
 *                      When the region is completely uniform, the uniform
 *                      pixel value is stored here.
 *
 * @return 0 if the tiles are not shallow uniform or a memory allocation error
 *           occurred, otherwise a non-zero value.
 */

SIF_EXPORT int              sif_simple_is_shallow_uniform(sif_file *file, long x, long y, long w, long h, long band, void *uniform_value);

/**
 * @brief Determine if a tile slice has shallow uniformity. The simple user data
 * type convention is assumed.
 *
 * @param file          The file to perform the check.
 * @param tx            The horizontal tile index (0..N-1 indexed)
 *                      of the region to check.
 * @param ty            The vertical tile index (0..N-1 indexed)
 *                      of the region to check.
 * @param band          The band offset (0..N-1 indexed).
 * @param uniform_value This value is only meaningful when this function
 *                      returns true (non-zero). It is expected that the
 *                      pointer passed point to at least data_unit_size bytes.
 *                      When the region is completely uniform, the uniform
 *                      pixel value is stored here.
 *
 * @return 0 if the tiles are non-uniform or a memory allocation error
 *           occurred, otherwise a non-zero value.
 */

SIF_EXPORT int              sif_simple_is_slice_shallow_uniform(sif_file *file, long tx, long ty, long band, void *uniform_value);

/**
 * Return if the file conforms to the "simple" data type convention.
 *
 * @param file              The file on which to perform the operation.
 *
 * @return A positive value if the file conforms to the "simple" data type convention.
 */

SIF_EXPORT int              sif_is_simple(sif_file *file);

/**
 * Return a positive number if the file referred to by the filename conforms to the "simple"
 * data type convention.
 *
 * @param filename          The filename of the file on which to perform the operation.
 *
 * @return A positive value if the file referred to by the filename conforms to the
 *          "simple" data type convention. If the file does not conform to the "simple"
 *         data type convention, -2 is returned. If the file could not be opened, -1 is returned.
 *         If the file is not a SIF file, 0 is returned.
 */

SIF_EXPORT int              sif_is_simple_by_name(const char *filename);

/**
 * Exports a region of an open SIF file to PGM (Portable Grayscale Map) format. The file must
 * conform to the "simple" data type convention. uint8 and uint16 are the only
 * supported "simple" data types for PGM output.
 *
 * @param   file       The SIF file to export.
 * @param   filename   The filename to write the PGM output.
 * @param   x          The x pixel coordinate of the region to write (0..N-1 indexed).
 * @param   y          The y pixel coordinate of the region to write (0..N-1 indexed).
 * @param   width      The width of the region to write.
 * @param   height     The height of the region to write.
 * @param   band       The band of the region to write.
 *
 * @return A nonzero value if successful.
 */

SIF_EXPORT int              sif_export_region_to_pgm_file(sif_file *file, const char *filename,
							  int x, int y, int width, int height, int band);

/**
 * Exports a slice of a tile in an image of an open SIF file to PGM (Portable Grayscale Map) format.
 * The file must conform to the "simple" data type convention. uint8 and uint16 are the only
 * supported "simple" data types for PGM output.
 *
 * @param   file       The SIF file to export.
 * @param   filename   The filename to write the PGM output.
 * @param   tx         The x tile coordinate of the slice to write (0..N-1 indexed).
 * @param   ty         The y tile coordinate of the slice to write (0..N-1 indexed).
 * @param   band       The band number of the slice to write (0..N-1 indexed).
 *
 * @return A nonzero value if successful.
 */

SIF_EXPORT int              sif_export_slice_to_pgm_file(sif_file *file, const char *filename,
							 int tx, int ty, int band);

/**
 * Exports three bands of a region of an open SIF file to PPM (Portable Pixel Map) format. The file
 * must conform to the "simple" data type convention. The image in the file must contain 3 bands.
 * uint8 and uint16 are the only supported "simple" data types for PPM output.
 *
 * @param   file       The SIF file to export.
 * @param   filename   The filename to write the PGM output.
 * @param   x          The x pixel coordinate of the region to write (0..N-1 indexed).
 * @param   y          The y pixel coordinate of the region to write (0..N-1 indexed).
 * @param   width      The width of the region to write.
 * @param   height     The height of the region to write.
 * @param   band0      The band number of the SIF image corresponding to the red band in the PPM file.
 * @param   band1      The band number of the SIF image corresponding to the green band in the PPM file.
 * @param   band2      The band number of the SIF image corresponding to the blue band in the PPM file.
 *
 * @return A nonzero value if successful.
 */

SIF_EXPORT int              sif_export_region_to_ppm_file(sif_file *file, const char *filename,
							  int x, int y, int width, int height,
							  int band0, int band1, int band2);

/**
 * Exports three slices of a tile of an open SIF file to PPM (Portable Pixel Map) format. The file
 * must conform to the "simple" data type convention. The image in the file must contain at least 3 bands.
 * uint8 and uint16 are the only supported "simple" data types for PPM output.
 *
 * @param   file       The SIF file to export.
 * @param   filename   The filename to write the PGM output.
 * @param   tx         The x tile coordinate of the slice to write (0..N-1 indexed).
 * @param   ty         The y tile coordinate of the slice to write (0..N-1 indexed).
 * @param   band0      The band number of the tile slice corresponding to the red band in the PPM file.
 * @param   band1      The band number of the tile slice corresponding to the green band in the PPM file.
 * @param   band2      The band number of the tile slice corresponding to the blue band in the PPM file.
 *
 * @return A nonzero value if successful.
 */

SIF_EXPORT int              sif_export_slices_to_ppm_file(sif_file *file, const char *filename,
							  int tx, int ty,
							  int band0, int band1, int band2);

/**
 * Exports three bands of a region of an open SIF file to PAM (Portable Any Map) format. The file
 * must conform to the "simple" data type convention. 
 * uint8 and uint16 are the only supported "simple" data types for PAM output.
 *
 * @param   file       The SIF file to export.
 * @param   filename   The filename to write the PAM output.
 * @param   x          The x pixel coordinate of the region to write (0..N-1 indexed).
 * @param   y          The y pixel coordinate of the region to write (0..N-1 indexed).
 * @param   width      The width of the region to write.
 * @param   height     The height of the region to write.
 * @param   bands      The band numbers of the SIF image corresponding to the bands in the PAM file,
 *                     in ascending order.
 * @param   nbands     The total number of bands to write.
 *
 * @return A nonzero value if successful.
 */

SIF_EXPORT int              sif_export_region_to_ppm_file(sif_file *file, const char *filename,
							  int x, int y, int width, int height,
							  int *bands, int nbands);

/**
 * Exports three slices of a tile of an open SIF file to PAM (Portable Any Map) format. The file
 * must conform to the "simple" data type convention. The image in the file must contain at least 3 bands.
 * uint8 and uint16 are the only supported "simple" data types for PAM output.
 *
 * @param   file       The SIF file to export.
 * @param   filename   The filename to write the PAM output.
 * @param   tx         The x tile coordinate of the slice to write (0..N-1 indexed).
 * @param   ty         The y tile coordinate of the slice to write (0..N-1 indexed).
 * @param   bands      The band numbers of the slices of the tile in the SIF image corresponding
 *                     to the bands in the PAM file, in ascending order.
 * @param   nbands     The total number of bands to write.
 *
 * @return A nonzero value if successful.
 */

SIF_EXPORT int              sif_export_slices_to_ppm_file(sif_file *file, const char *filename,
							  int tx, int ty,
							  int *bands, int nbands);

#endif
