/**
 * File:             sif-io.c
 * Date:             December 17, 2004
 * Author:           Damian Eads
 * Description:      A C library for manipulating Sparse Image Format (SIF) files.
 *
 * Copyright (C) 2004-2006 The Regents of the University of California.
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

#include "sif-io.h"

#ifndef WIN32
#include <errno.h>
#include <strings.h>
#include <unistd.h>
#endif

/**#define SIF_ASSERT assert(0)**/  /** used for debugging.**/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/** By defining the macro definition below, as opposed to the one above,
    SIF does not return when an error occurs when executing a function
    in the SIF API. */

#define SIF_ASSERT 

#ifdef WIN32
#define SIF_RECORD file->sys_error_no = GetLastError();
#else
#define SIF_RECORD file->sys_error_no = errno;
#endif

#define SIF_ERROR_CHECK(cond, code) if (cond) { file->error = code; file->error_line_no = __LINE__; SIF_RECORD; SIF_ASSERT; }
#define SIF_ERROR_CHECK_RETURN(cond, code, retcode) if (cond) { file->error = code; file->error_line_no = __LINE__; SIF_RECORD; SIF_ASSERT; return retcode; }
#define SIF_ERROR_CHECK_RETURN_V(cond, code) if (cond) { file->error = code; file->error_line_no = __LINE__; SIF_RECORD; SIF_ASSERT; return; }
#define SIF_ERROR_RETURN(code) if (file->error != 0) { SIF_RECORD; SIF_ASSERT; return code; }
#define SIF_ERROR_RETURN_V() if (file->error != 0) { SIF_RECORD; SIF_ASSERT; return; }

#define SIF_SIZE_FLAG_ARRAY(num_bits) (CEIL_DIV(num_bits, 8))
#define SIF_GET_BIT(uca, i) ((uca[i / 8] >> (7-(i % 8))) & 0x1)
#define SIF_SET_BIT(uca, i) (uca[i / 8] |= ((0x1) << (7-(i % 8))))
#define SIF_CLEAR_BIT(uca, i) (uca[i / 8] &= ~((0x1) << (7-(i % 8))))

#define SIF_HASH_TABLE_SIZE 128

/** The three below are function versions of the three macros above. **/

/**
 * Return a non-zero integer if i'th bit in the character array is set.
 *
 * @param v    The character array to retrieve the bit.
 * @param i    The bit number of the bit to retrieve.
 *
 * @return A non-zero integer if the bit is set.
 */
u_int _sif_get_bit(const u_char *v, u_int i) {
  return SIF_GET_BIT(v, i);
}

/**
 * Sets the i'th bit in the character array.
 *
 * @param v    The character array to retrieve the bit.
 * @param i    The bit number of the bit to retrieve.
 *
 * @return The first character in the array.
 */
int _sif_set_bit(u_char *v, u_int i) {
  SIF_SET_BIT(v, i);
  return (int)*v;
}

/**
 * Clears the i'th bit in the character array.
 *
 * @param v    The character array to retrieve the bit.
 * @param i    The bit number of the bit to retrieve.
 *
 * @return The first character in the array.
 */
int _sif_clear_bit(u_char *v, u_int i) {
  SIF_CLEAR_BIT(v, i);
  return (int)*v;
}

/** Dan Bernstein's hash. */

unsigned long _sif_hash(const unsigned char *str) {
  unsigned long hash = 5381;
  int c = *str;
  while (c != 0) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */  
    str++;
    c = *str;
  }
  return hash;
}

/**
 * Swaps the data elements in a buffer to change the byte order.
 *
 * @param n_bytes    The number of bytes in the buffer on which to change the byte order.
 * @param elem_size  The number of bytes per element, assumed to be even.
 */

void _sif_swap_bytes(unsigned char *buffer,
			      int n_bytes, int elem_size) {
  int i, j, k, m;
  const int esz_h = elem_size / 2;   /** Half the number of bytes per element. */
  unsigned char tmp;
  for (i = 0; i < esz_h; i++) {
    m = esz_h - i - 1;  /* The byte index to byte swapped. */
    /** For each element, the i'th byte of the element is going to be swapped with
        the m'th.

        We loop over the bytes in the buffer with two looper variables, j and k.

          j: the byte index (in the buffer!) of the byte of the element to swap
          k: the byte index (in the buffer!) of the byte of the element to swap

    **/
    for (j = i, k = m; k < n_bytes; j += elem_size, k += elem_size) {
      tmp = buffer[k];
      buffer[k] = buffer[i];
      buffer[i] = tmp;
    }
  }
}


void _sif_buffer_host_to_big(unsigned char *buffer, int n_bytes, int elem_size) {
#if defined(WIN32) || __BYTE_ORDER == __LITTLE_ENDIAN
  _sif_swap_bytes(buffer, n_bytes, elem_size);
#elif __BYTE_ORDER == __BIG_ENDIAN
  return;
#endif  
}

void _sif_buffer_host_to_little(unsigned char *buffer, int n_bytes, int elem_size) {
#if defined(WIN32) || __BYTE_ORDER == __LITTLE_ENDIAN
  return;
#elif __BYTE_ORDER == __BIG_ENDIAN
  _sif_swap_bytes(buffer, n_bytes, elem_size);
#endif  
}

void _sif_buffer_host_to_code(unsigned char *buffer, int n_bytes, int elem_size, int simple_endian_code) {
  if (simple_endian_code == SIF_SIMPLE_BIG_ENDIAN) {
    _sif_buffer_host_to_big(buffer, n_bytes, elem_size);
  }
  if (simple_endian_code == SIF_SIMPLE_LITTLE_ENDIAN) {
    _sif_buffer_host_to_little(buffer, n_bytes, elem_size);
  }
}

void _sif_buffer_little_to_host(unsigned char *buffer, int n_bytes, int elem_size) {
#if defined(WIN32) || __BYTE_ORDER == __LITTLE_ENDIAN
  return;
#elif __BYTE_ORDER == __BIG_ENDIAN
  _sif_swap_bytes(buffer, n_bytes, elem_size);
#endif  
}

void _sif_buffer_big_to_host(unsigned char *buffer, int n_bytes, int elem_size) {
#if defined(WIN32) || __BYTE_ORDER == __LITTLE_ENDIAN
  _sif_swap_bytes(buffer, n_bytes, elem_size);
#elif __BYTE_ORDER == __BIG_ENDIAN
  return;
#endif  
}

void _sif_buffer_code_to_host(unsigned char *buffer, int n_bytes, int elem_size, int simple_endian_code) {
  if (simple_endian_code == SIF_SIMPLE_BIG_ENDIAN) {
    _sif_buffer_big_to_host(buffer, n_bytes, elem_size);
  }
  if (simple_endian_code == SIF_SIMPLE_LITTLE_ENDIAN) {
    _sif_buffer_little_to_host(buffer, n_bytes, elem_size);
  }
}

void _sif_fwrite64_with_swap(sif_file *file, size_t size, size_t nmemb, const void *buffer) {
  SIF_ERROR_CHECK_RETURN_V(_sif_simple_alloc_region_buffer(file, size * nmemb) == 0, SIF_ERROR_MEM);
  memcpy(file->simple_region_buffer, buffer, size * nmemb);
  _sif_swap_bytes(file->simple_region_buffer, size * nmemb, size);
  FWRITE64V(file->simple_region_buffer, size, nmemb, file->fp);
}

void _sif_fwrite64_without_swap(sif_file *file, size_t size, size_t nmemb, const void *buffer) {
  FWRITE64V(buffer, size, nmemb, file->fp);
}

void _sif_fread64_with_swap(sif_file *file, size_t size, size_t nmemb, void *buffer) {
  FREAD64V(buffer, size, nmemb, file->fp);
  _sif_swap_bytes(buffer, size * nmemb, size);
}

void _sif_fread64_without_swap(sif_file *file, size_t size, size_t nmemb, void *buffer) {
  FREAD64V(buffer, size, nmemb, file->fp);
}

typedef void (*sif_buffer_preprocessor) (sif_file *file, size_t size, size_t nmemb, void *buffer);

/** These two macros check if the file pointer is null, the header is not null, and if the file version
    is compatible with this library version. */

#define SIF_CHECK_FILE(fp) if (fp == 0) { \
                                       return 0; \
                                   } else if (fp->header == 0) { \
                                       fp->error = SIF_ERROR_NULL_HDR;	  \
				       return 0; \
                                   } else if (fp->header->version > SIF_VERSION) { \
				       fp->error = SIF_ERROR_INCOMPATIBLE_VERSION; \
                                       return 0; \
                                   }

#define SIF_CHECK_FILE_V(fp) if (fp == 0) { \
                                       return; \
                                     } else if (fp->header == 0) { \
                                       fp->error = SIF_ERROR_NULL_HDR; \
				       return; \
                                     } else if (fp->header->version > SIF_VERSION) { \
       			               fp->error = SIF_ERROR_INCOMPATIBLE_VERSION; \
                                       return; \
                                     }

#ifdef WIN32
#define FILE_IS_OKAY(fp) (fp != INVALID_HANDLE_VALUE)
#define bzero(buf, n) memset(buf, 0, n);
#define FCLOSE64(fp) _sif_close_win(fp);
#define REWIND64(fp) SIF_ERROR_CHECK_RETURN(_sif_fseek_win(fp, 0, FILE_BEGIN) == -1, SIF_ERROR_SEEK, 0)
#define REWIND64V(fp) SIF_ERROR_CHECK_RETURN_V(_sif_fseek_win(fp, 0, FILE_BEGIN) == -1, SIF_ERROR_SEEK)
#define REWIND64NEC(fp) _sif_fseek_win(fp, 0, FILE_BEGIN)
#define FSEEK64(fp, loc, typ) SIF_ERROR_CHECK_RETURN(_sif_fseek_win(fp, loc, _sif_linux_to_win32_seek_type(typ)) == -1, SIF_ERROR_SEEK, 0)
#define FSEEK64V(fp, loc, typ) SIF_ERROR_CHECK_RETURN_V(_sif_fseek_win(fp, loc, _sif_linux_to_win32_seek_type(typ)) == -1, SIF_ERROR_SEEK)
#define FSEEK64NEC(fp, loc, typ) _sif_fseek_win(fp, loc, _sif_linux_to_win32_seek_type(typ))
#define FWRITE64(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN(_sif_fwrite_win(fp, buf, els * nel) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64V(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN_V(_sif_fwrite_win(fp, buf, els * nel) == 0, SIF_ERROR_WRITE)
#define FWRITE64CNT(buf, cnt, fp) cnt += sizeof(buf); SIF_ERROR_CHECK_RETURN(_sif_fwrite_win(fp, &(buf), sizeof(buf)) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64INT32CNT(vv, cnt, fp) cnt += 4; SIF_ERROR_CHECK_RETURN(_sif_write_int32(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64INT32(vv, fp) SIF_ERROR_CHECK_RETURN(_sif_write_int32(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64DOUBLE64CNT(vv, cnt, fp) cnt += 8; SIF_ERROR_CHECK_RETURN(_sif_write_double64(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64DOUBLE64(vv, fp) SIF_ERROR_CHECK_RETURN(_sif_write_double64(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64NEC(buf, els, nel, fp) (_sif_fwrite_win(fp, buf, els * nel) / els)
#define FREAD64(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN(_sif_fread_win(fp, buf, els * nel) == 0, SIF_ERROR_READ, 0)
#define FREAD64V(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN_V(_sif_fread_win(fp, buf, els * nel) == 0, SIF_ERROR_READ)
#define FREAD64CNT(buf, cnt, fp) cnt += sizeof(buf); SIF_ERROR_CHECK_RETURN(_sif_fread_win(fp, &(buf), sizeof(buf)) == 0, SIF_ERROR_READ, 0)
#define FREAD64INT32CNT(vv, cnt, fp) cnt += 4; SIF_ERROR_CHECK_RETURN(_sif_read_int32(fp, &(vv)) == 0, SIF_ERROR_READ, 0)
#define FREAD64INT32(vv, fp) SIF_ERROR_CHECK_RETURN(_sif_read_int32(fp, &(vv)) == 0, SIF_ERROR_READ, 0)
#define FREAD64INT32NEC(vv, fp) _sif_read_int32(fp, &(vv))
/** NEC -- No Error Checking. */
#define FREAD64NEC(buf, els, nel, fp) (_sif_fread_win(fp, buf, els * nel) / els)
#else
#define FILE_IS_OKAY(fp) (fp)
#define FCLOSE64(fp) fclose(fp);
#define REWIND64(fp) SIF_ERROR_CHECK_RETURN(fseek(fp, ((off64_t)0), SEEK_SET) != 0,  SIF_ERROR_SEEK, 0)
#define REWIND64V(fp) SIF_ERROR_CHECK_RETURN_V(fseek(fp, ((off64_t)0), SEEK_SET) != 0, SIF_ERROR_SEEK)
#define REWIND64NEC(fp) fseek(fp, ((off64_t)0), SEEK_SET)
#define FSEEK64(fp, loc, typ) SIF_ERROR_CHECK_RETURN(fseek(fp, ((off64_t)loc), (typ)) != 0, SIF_ERROR_SEEK, 0)
#define FSEEK64V(fp, loc, typ) SIF_ERROR_CHECK_RETURN_V(fseek(fp, ((off64_t)loc), (typ)) != 0, SIF_ERROR_SEEK)
#define FSEEK64NEC(fp, loc, typ) fseek(fp, ((off64_t)loc), (typ))
#define FWRITE64(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN(fwrite(buf, els, nel, fp) != nel, SIF_ERROR_WRITE, 0)
#define FWRITE64V(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN_V(fwrite(buf, els, nel, fp) != nel, SIF_ERROR_WRITE)
#define FWRITE64CNT(buf, cnt, fp) cnt += sizeof(buf); SIF_ERROR_CHECK_RETURN(fwrite(&(buf), sizeof(buf), 1, fp) != 1, SIF_ERROR_WRITE, 0)
#define FWRITE64INT32(vv, fp) SIF_ERROR_CHECK_RETURN(_sif_write_int32(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64INT32CNT(vv, cnt, fp) cnt += 4; SIF_ERROR_CHECK_RETURN(_sif_write_int32(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64DOUBLE64(vv, fp) SIF_ERROR_CHECK_RETURN(_sif_write_double64(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64DOUBLE64CNT(vv, cnt, fp) cnt += 8; SIF_ERROR_CHECK_RETURN(_sif_write_double64(fp, vv) == 0, SIF_ERROR_WRITE, 0)
#define FWRITE64NEC(buf, els, nel, fp) fwrite(buf, els, nel, fp)
#define FREAD64(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN(fread(buf, els, nel, fp) != nel, SIF_ERROR_READ, 0)
#define FREAD64V(buf, els, nel, fp) SIF_ERROR_CHECK_RETURN_V(fread(buf, els, nel, fp) != nel, SIF_ERROR_READ)
#define FREAD64CNT(buf, cnt, fp) cnt += sizeof(buf); SIF_ERROR_CHECK_RETURN(fread(&(buf), sizeof(buf), 1, fp) != 1, SIF_ERROR_READ, 0)
#define FREAD64INT32CNT(vv, cnt, fp) cnt += 4; SIF_ERROR_CHECK_RETURN(_sif_read_int32(fp, &(vv)) == 0, SIF_ERROR_READ, 0)
#define FREAD64INT32(vv, fp) SIF_ERROR_CHECK_RETURN(_sif_read_int32(fp, &(vv)) == 0, SIF_ERROR_READ, 0)
#define FREAD64DOUBLE64CNT(vv, cnt, fp) cnt += 8; SIF_ERROR_CHECK_RETURN(_sif_read_double64(fp, &(vv)) == 0, SIF_ERROR_READ, 0)
#define FREAD64DOUBLE64(vv, fp) SIF_ERROR_CHECK_RETURN(_sif_read_double64(fp, &(vv)) == 0, SIF_ERROR_READ, 0)
#define FREAD64INT32NEC(vv, fp) _sif_read_int32(fp, &(vv))
/** NEC -- No Error Checking. */
#define FREAD64NEC(buf, els, nel, fp) fread(buf, els, nel, fp)
#endif

/**
 * Computes the minimum of two primitive values.
 */

#ifndef MAX
#define MAX(x, y) ((x > y) ? (x) : (y))
#endif

/**
 * Computes the minimum of two primitive values.
 */

#ifndef MIN
#define MIN(x, y) ((x < y) ? (x) : (y))
#endif

/**
 * Divides two integers (int or long) and takes the ceiling of the result as
 * if they were divided as real numbers.
 */

#ifndef CEIL_DIV
#define CEIL_DIV(x, y) ((((double)x)/(double)y) == ((double)((x)/(y))) ? ((x)/(y)) : ((x)/(y) + 1))
#endif

#define SIF_VERSION 2

/**
 * Returns the latest version of the SIF file format that the
 * currently loaded SIF library can process.
 *
 * @return The version number.
 */

long sif_get_version() {
  return SIF_VERSION;
}

/** Function prototype. The function definition has more specific
documentation.*/

static int _sif_is_uniform(sif_file *file, const void *data, int extentX, int extentY);
#ifdef WIN32

/**
 * A POSIX-like wrapper function around the Windows API for closing
 * files. The FCLOSE64 macro uses this function instead of fclose when
 * Visual Studio is used as the compiler. This ensures consistency,
 * i.e., FCLOSE64 always evaluates to an integer.
 *
 * @param hFile The windows handle of the file to close.
 *
 * @return Always returns 0.
 */

static int _sif_close_win(HANDLE hFile) {
  /** Flushing should be unnecessary, but I've noticed strange behavior
      for files accessed over the network without it. */
  FlushFileBuffers(hFile);

  /** Finally, close the handle. */
  CloseHandle(hFile);
  return 0;
}

/**
 * A POSIX-like wrapper function for seeking using the Windows API for
 * seeking within potentially large (i.e. 64-bit) byte-addressable
 * files.
 *
 * @param hf         The file to seek.
 * @param distance   The distance from the starting point to seek.
 * @param MoveMethod Defines how the starting point is
 *                   calculated, e.g. FILE_BEGIN means the starting
 *                   point is the begining of the file. Pass
 *                   a POSIX-whence field to
 *                   _sif_linux_to_win32_seek_type, and it will
 *                   return the POSIX equivalent.
 *
 * @return 0 if successful, -1 if unsuccessful.
 * @see _sif_linux_to_win32_seek_type
 */

static long long _sif_fseek_win(HANDLE hf, long long distance, DWORD MoveMethod) {
	 unsigned long dwMoveHigh, nMoveLow;
   LARGE_INTEGER li;

   int rc = 0;

   /** Calculate the windows high and low seek offsets. */
   li.QuadPart = distance;
   nMoveLow = li.LowPart;
   dwMoveHigh = li.HighPart;
   /**rc = SetFilePointerEx(hf, m, &p, MoveMethod);**/

   /** Set the error status to no error.. */
   SetLastError(0);

   /** Set the location in the file accordingly. */
   rc = SetFilePointer(hf, (LONG)nMoveLow, (PLONG)&dwMoveHigh, MoveMethod);

   /** See if windows gets an error... */
   if (GetLastError() != NO_ERROR) {
     /** ... if so, return a negative status code. */
     return -1;
   }

   /** if not, return zero. */
   return 0;
}

/**
 * A POSIX-like wrapper to the Windows API function for writing to
 * large files. Writing starts at the current position in the
 * file.
 *
 * @param hFile     The file to write.
 * @param buffer    The buffer containing the bytes to be written to
 *                  the file.
 * @param num_bytes The number of bytes to write to the file.
 *
 * @return The number of bytes written.
 */

static DWORD _sif_fwrite_win(HANDLE hFile, void *buffer, DWORD num_bytes) {
   DWORD bytes_written;
   int rc;
   rc = WriteFile(hFile, buffer, num_bytes, &bytes_written, NULL);
   /** If an error occurred, return 0, giving the caller the
       indication that no bytes were written.*/
   if (rc == 0) {
      return 0;	
   }
   /** Otherwise, return the number of bytes written by the Windows
       API. */
   return bytes_written;
}

/**
 * A wrapper to the Windows API function for reading from large
 * files. Read starts at the current position in the file.
 *
 * @param hFile     The file to read.
 * @param buffer    The buffer to store the read bytes.
 * @param num_bytes The number of bytes to read from the file.
 *
 * @return The number of bytes read.
 */

static DWORD _sif_fread_win(HANDLE hFile, void *buffer, DWORD num_bytes) {
   DWORD bytes_read;
   int rc;
   rc = ReadFile(hFile, buffer, num_bytes, &bytes_read, NULL);
   if (rc == 0) {
      return 0;	
   }
   return bytes_read;
}

/**
 * A POSIX-like wrapper to the Windows API function for reading from
 * large files. Read starts at the current position in the
 * file.
 *
 * @param hFile     The file to read.
 * @param size      The size of each member.
 * @param nmemb     The number of members to write.
 * @param stream    The buffer to store the bytes read.
 *
 * @return The number of bytes read. If an error occurs or the end of
 * file is reached, this value may not necessarily equal size * nmemb.
 */

static DWORD _sif_fread(HANDLE ptr, size_t size, size_t nmemb, void *stream) {
	return (_sif_fread_win(ptr, stream, size * nmemb) == size * nmemb) ? nmemb : 0;
}

/**
 * Translates a POSIX fseek/fseek64 code into a WIN32 API seek code. More
 * specifically, SEEK_SET returns FILE_BEGIN, SEEK_CUR returns FILE_CURRENT,
 * and SEEK_END returns FILE_END.<p>
 *
 * This function is useful because it allows SIF I/O library functions
 * to use the macros FXXX macros and pass the POSIX seek-type
 * equivalents. When Visual Studio is used as the compiler, the macro
 * converts the POSIX equivalents to windows equivalents using this function.
 *
 * @param whence The POSIX fseek code.
 *
 * @return The Windows API equivalent code.
 */

static DWORD _sif_linux_to_win32_seek_type(int whence) {
  DWORD retval = FILE_BEGIN;
  switch (whence) {
  case SEEK_SET:
    retval = FILE_BEGIN;
    break;
  case SEEK_CUR:
    retval = FILE_CURRENT;
    break;
  case SEEK_END:
    retval = FILE_END;
    break;
  }
  return retval;
}

#else

/** If Visual Studio is not the compiler, wrapping fread is easy. We don't
 even need a function. */ 
#define _sif_fread(ptr, size, nmemb, stream) fread((FILE*)ptr, size, nmemb, stream)
#endif

/**
 * To ensure portability across platforms, the byte ordering must be
 * consistent for all 32-bit integers written to a file. This function
 * takes in a non-portable long and writes the 32 most-significant
 * bits to the buffer in big-endian network byte order.
 *
 * @param val   The value to write to the buffer.
 * @param ptr   A buffer containing at least 4 bytes (32-bits). This
 *              function guarantees that only the first 4 bytes will
 *              be written.
 */

static void _sif_int32_to_packed_bytes(long val, u_char *ptr) {
  ptr[0] = (val >> 24) & 0xFF;
  ptr[1] = (val >> 16) & 0xFF;
  ptr[2] = (val >> 8) & 0xFF;
  ptr[3] = val & 0xFF;
}

/**
 * To ensure portability across platforms, the byte ordering must be
 * consistent for all 64-bit integers written to a file. This function
 * takes in a non-portable long and writes the 64 most-significant
 * bits to the buffer in big-endian network byte order.
 *
 * @param val   The value to write to the buffer.
 * @param ptr   A buffer containing at least 8 bytes (64-bits). This
 *              function guarantees that only the first 8 bytes will
 *              be written.
 */

static void _sif_int64_to_packed_bytes(long long val, u_char *ptr) {
  ptr[0] = (val >> 56) & 0xFF;
  ptr[1] = (val >> 48) & 0xFF;
  ptr[2] = (val >> 40) & 0xFF;
  ptr[2] = (val >> 32) & 0xFF;
  ptr[4] = (val >> 24) & 0xFF;
  ptr[5] = (val >> 16) & 0xFF;
  ptr[6] = (val >> 8) & 0xFF;
  ptr[7] = val & 0xFF;
}

/**
 * This function takes in a buffer containing at least 4 bytes and
 * reads a 32-bit integer value from it. It assumes the integer value
 * is stored in big-endian network byte order.
 *
 * @param ptr   A buffer containing at least 4 bytes (32-bits) from
 *              which to read the 32-bit integer value stored as a
 *              big-endian.
 *
 * @return The long value.
 */

static long _sif_packed_bytes_to_int32(const u_char* ptr) {
 // MSB first
 return ((long)ptr[0] << 24) | ((long)ptr[1] << 16)
   | ((long)ptr[2] << 8) | (long)ptr[3];
} 


/**
 * This function takes in a buffer containing at least 4 bytes and
 * reads a 64-bit integer value from it. It assumes the integer value
 * is stored in big-endian network byte order.
 *
 * @param ptr   A buffer containing at least 8 bytes (64-bits) from
 *              which to read the 64-bit integer value stored as a
 *              big-endian.
 *
 * @return The long value.
 */

static long long sif_packed_bytes_to_int64(const u_char* ptr) {
 // MSB first
 return ((long long)ptr[0] << 56) | ((long long)ptr[1] << 48)
   | ((long long)ptr[2] << 40) | ((long long)ptr[3] << 32)
   | ((long long)ptr[4] << 24) | ((long long)ptr[5] << 16)
   | ((long long)ptr[6] << 8) | (long long)ptr[7];
} 

static double _sif_swap_double64(const double val) {
  double retval;
  unsigned char *out = (unsigned char *)&retval;
  const unsigned char *in = (unsigned char *)&val;
  for (int i = 0; i < 8; i++) {
    out[i] = in[7-i];
  }
  return retval;
}

static double _sif_swap_float32(const float val) {
  float retval;
  unsigned char *out = (unsigned char *)&retval;
  const unsigned char *in = (unsigned char *)&val;
  for (int i = 0; i < 4; i++) {
    out[i] = in[4-i];
  }
  return retval;
}

static double _sif_hton_double64(const double val) {
  double retval;
  /** If we're on a windows machine, assume 'val' is little endian (even
      Windows on DEC-Alpha requires little-endian.) Otherwise, assume
      big-endian.*/
#if defined(WIN32) || __BYTE_ORDER == __LITTLE_ENDIAN
  retval = _sif_swap_double64(val);
#elif __BYTE_ORDER == __BIG_ENDIAN
  retval = val;
#endif
  return retval;
}

static double  _sif_ntoh_double64(const double val) {
  double retval;
#if defined(WIN32) || __BYTE_ORDER == __LITTLE_ENDIAN
  retval = _sif_swap_double64(val);
#elif __BYTE_ORDER == __BIG_ENDIAN
  retval = val;
#endif
  return retval;
}

static int _sif_write_double64(sif_file *file, const double val) {
  double net = _sif_hton_double64(val);
  if (FWRITE64NEC(&net, sizeof(u_char), 8, file->fp) != 8) {
    return 0;
  }
  return 1;
}

static int _sif_read_double64(sif_file *file, double *val) {
  double *net = (double*)file->ubuf;
  if (FREAD64NEC(file->ubuf, sizeof(u_char), 8, file->fp) != 8) {
    return 0;
  }
  *val = _sif_ntoh_double64(*net);
  return 1;
}

/**
 * Write an integer to a file in big endian network byte order. Only
 * the first 32 significant bits of an integer are considered.
 *
 * @param file   The file on which to write an integer.
 * @param long   The integer to write.
 *
 * @return 1 if successful, 0 if an error occurred.
 */

static int _sif_write_int32(sif_file *file, long val) {
  _sif_int32_to_packed_bytes(val, (u_char*)&(file->ubuf));
  if (FWRITE64NEC(&(file->ubuf), sizeof(u_char), 4, file->fp) != 4) {
    return 0;
  }
  return 1;
}


/**
 * Read an integer from a file in big endian network byte order. Four
 * bytes are read, and the integer is assumed to be stored as a big
 * endian.
 *
 * @param file   The file on which to write an integer.
 * @param long   The integer to write.
 *
 * @return 1 if successful, 0 if an error occurred.
 */

static int _sif_read_int32(sif_file *file, long *val) {
  if (FREAD64NEC(&(file->ubuf), sizeof(u_char), 4, file->fp) != 4) {
    return 0;
  }
  *val = _sif_packed_bytes_to_int32((const u_char *)&(file->ubuf));
  return 1;
}

/**
 * A shallow check for complete uniformity. Each flag in the tiles
 * header is examined however the raster is not scanned for
 * uniformity.
 *
 * @param tile_no The tile to check for complete uniformity.
 *
 * @return Returns true iff all bands in a tile are uniform.
 */

static int _sif_completely_uniform_shallow(sif_file *file, long i) {
  sif_tile *tile = file->tiles + i;
  sif_header *hd = file->header;
  int j, retval = 0xFF;
  u_char *last_flag_byte = tile->uniform_flags + (hd->n_uniform_flags - 1);
  *last_flag_byte = (u_char)(0xFF >> (8 - (hd->bands % 8))) | *last_flag_byte;
  for (j = 0; j < hd->n_uniform_flags && retval == 0xFF; j++) {
    retval = retval & tile->uniform_flags[j];
  }
  return retval == 0xFF;
}

/**
 * Returns true if a band in a tile is uniform.
 */

static int _sif_band_of_tile_is_uniform_shallow(sif_file *file, long i, long b) {
  sif_tile *tile = file->tiles + i;
  return SIF_GET_BIT(tile->uniform_flags, b);
}

/**
 * Computes the starting offset for a specific data block in the file. This
 * offset is computed by multiplying the block size in bytes by the block
 * index passed and adding the location where the block data starts.
 *
 * @param file      A pointer to a sif file of interest.
 * @param block_num The index of the block in the file passed.
 *
 * @return          The offset where the data starts.
 */

static LONGLONG          _sif_get_block_location(const sif_file *file, long block_num) {
  assert(block_num >= 0LL);
  return file->base_location
    + ((LONGLONG)file->header->tile_bytes * (LONGLONG)block_num);
}

/**
 * Allocates enough space for the meta-data table.
 *
 * @return The newly allocated meta-data table.
 */

static sif_meta_data   **_sif_alloc_meta_data_table() {
  sif_meta_data **result = (sif_meta_data**)malloc(sizeof(sif_meta_data*) * SIF_HASH_TABLE_SIZE);
  if (result != 0) {
    bzero(result, sizeof(sif_meta_data*) * SIF_HASH_TABLE_SIZE);
  }
  return result;
}

/**
 * Frees the space for the meta-data table and the items inside of it.
 */

static void            _sif_free_meta_data(sif_file *file) {
  int j;
  sif_meta_data *cur, *next = 0;
  for (j = 0; j < SIF_HASH_TABLE_SIZE; j++) {
    for (cur = file->meta_data[j]; cur != 0; cur = next) {
      next = cur->next;
      free(cur->value);
      free(cur->key);
    }
  }
  free(file->meta_data);
}

/**
 * Allocates enough space for a header struct. Sets all header values to their
 * defaults (usually zero).
 *
 * @return          A pointer to a newly allocated header struct.
 *                  Returns zero if an error occurred during allocation.
 */

static sif_header       *_sif_alloc_header() {
  sif_header *retval = 0;
  retval = (sif_header*)malloc(sizeof(sif_header));
  if (retval != 0) {
    bzero(retval, sizeof(sif_header));
  }
  return retval;
}

/**
 * Allocates enough space for a SIF file pointer struct. Sets all values to
 * their defaults (usually zero).
 *
 * @return          A pointer to a newly allocated file pointer struct.
 *                  Returns zero if an error occurred during allocation.
 */

static sif_file         *_sif_alloc_fp() {
  sif_file *retval = 0;
  retval = (sif_file*)malloc(sizeof(sif_file));
  if (retval != 0) {
    bzero(retval, sizeof(sif_file));
  }
  return retval;
}

/**
 * Allocates enough space for all the SIF tile header structures.
 *
 * @param           n_tiles The number of tiles to allocate.
 *
 * @return          A pointer to a newly allocated file pointer struct.
 *                  Returns zero if an error occurred during allocation.
 */

static sif_tile         *_sif_alloc_tile_headers(sif_file *file) {
  sif_tile *retval = 0, *tile;
  sif_header *hd = file->header;
  int i, s = SIF_SIZE_FLAG_ARRAY(hd->bands);

  /** Allocate enough space (in bytes) to hold the "band" number of
      flags for all tiles. */
  u_char *master_uf = (u_char*)malloc(hd->n_tiles * s);

  /** Allocate enough space to hold the uniform pixel values for each
      tile and for each band.*/
  u_char *master_upv = (u_char*)malloc(hd->n_tiles * hd->bands * hd->data_unit_size);
  retval = (sif_tile*)malloc(sizeof(sif_tile) * hd->n_tiles);

  /** If there was an error allocating any block, free all allocated blocks
      and exit. */
  if (retval == 0 || master_uf == 0 || master_upv == 0) {
    free(master_uf);
    free(master_upv);
    free(retval);
    return 0;
  }

  /** Set everything to be initially uniform. */
  bzero(master_upv, hd->n_tiles * hd->bands * hd->data_unit_size);
  memset(master_uf, 0xFF, s * hd->n_tiles);
  if (retval != 0) {
    for (i = 0; i < hd->n_tiles; i++) {
      tile = retval + i;
      /** The flags for ALL the tiles are stored in one big array. Do
       some point arithmetic so the tile's header points to its
       flags.*/
      tile->uniform_flags = master_uf + (i * s);

      /** Do the same kind of thing for the uniform pixel values. */
      tile->uniform_pixel_values = master_upv + (i * hd->bands * hd->data_unit_size);

      /** Initially, no raster block is allocated for the tiles. */
      tile->block_num = -1;
    }
  }

  /** We need enough space to hold uniform pixel values,
      the uniformity flags, and the block number (32-bits).*/
  hd->tile_header_bytes = hd->bands * hd->data_unit_size + s + 4;

  /**
   * The number of bytes to store the uniformity flags. Thus,
   * s = Ceil(number_of_flags / 8).
   */
  hd->n_uniform_flags = s;
  return retval;
}

/**
 * Free the tile headers for a file.
 *
 * @param file  The file to free the headers.
 */

static void            _sif_free_tile_headers(sif_file *file) {
  /** Since the fields uniform flags and uniform_pixel_values
      are both located in a continguous blocks across all
      tile headers, we can free the fields of the first tile,
      thereby free all the data for the other tiles. */
  free(file->tiles->uniform_flags);
  free(file->tiles->uniform_pixel_values);
  free(file->tiles);
  file->tiles = 0;
}

/**
 * Writes the header for the file passed.
 *
 * @param           file The file pointer corresponding to the file
 *                       to write the header.
 */

static int             _sif_write_header(sif_file *file) {
  sif_header *hd = file->header;
  int cnt = 0, dummy = 0, i;
  REWIND64(file->fp);
  FWRITE64INT32CNT(dummy, cnt, file);
  FWRITE64CNT(hd->magic_number, cnt, file->fp);
  FWRITE64INT32CNT(file->use_file_version, cnt, file);
  hd->version = file->use_file_version;
  FWRITE64INT32CNT(hd->width, cnt, file);
  FWRITE64INT32CNT(hd->height, cnt, file);
  FWRITE64INT32CNT(hd->bands, cnt, file);
  FWRITE64INT32CNT(hd->n_keys, cnt, file);
  FWRITE64INT32CNT(hd->n_tiles, cnt, file);
  FWRITE64INT32CNT(hd->tile_width, cnt, file);
  FWRITE64INT32CNT(hd->tile_height, cnt, file);
  FWRITE64INT32CNT(hd->tile_bytes, cnt, file);
  FWRITE64INT32CNT(hd->n_tiles_across, cnt, file);
  FWRITE64INT32CNT(hd->data_unit_size, cnt, file);
  FWRITE64INT32CNT(hd->user_data_type, cnt, file);
  FWRITE64INT32CNT(hd->defragment, cnt, file);
  FWRITE64INT32CNT(hd->consolidate, cnt, file);
  FWRITE64INT32CNT(hd->intrinsic_write, cnt, file);
  FWRITE64INT32CNT(hd->tile_header_bytes, cnt, file);
  FWRITE64INT32CNT(hd->n_uniform_flags, cnt, file);
  /** For SIF file versions 2 and higher, floats/doubles are big-endian.
      SIF file version 1 has an anomaly where the float is written as
      little-endian while the ints are written as big-endian. */

  /** Version 1 logic. **/
  if (hd->version < 2 || file->use_file_version < 2) {
    for (i = 0; i < 6; i++) {
      FWRITE64CNT(hd->affine_geo_transform[i], cnt, file->fp);
    }
  }
  /** Version 2 and higher logic. **/
  else {
    for (i = 0; i < 6; i++) {
      FWRITE64DOUBLE64CNT(hd->affine_geo_transform[i], cnt, file);
    }
  }
  REWIND64(file->fp);
  FWRITE64INT32(cnt, file);
  file->header_bytes = cnt;
  return 1;
}

/**
 * Reads the header from the file passed.
 *
 * @param           file The file pointer corresponding to the file
 *                       to write the header.
 */

static int             _sif_read_header(sif_file *file) {
  sif_header *hd = file->header;
  int cnt = 0, i;
  REWIND64(file->fp);
  FREAD64INT32CNT(file->header_bytes, cnt, file);
  FREAD64CNT(hd->magic_number, cnt, file->fp);
  FREAD64INT32CNT(hd->version, cnt, file);
  file->use_file_version = hd->version;
  FREAD64INT32CNT(hd->width, cnt, file);
  FREAD64INT32CNT(hd->height, cnt, file);
  FREAD64INT32CNT(hd->bands, cnt, file);
  FREAD64INT32CNT(hd->n_keys, cnt, file);
  FREAD64INT32CNT(hd->n_tiles, cnt, file);
  FREAD64INT32CNT(hd->tile_width, cnt, file);
  FREAD64INT32CNT(hd->tile_height, cnt, file);
  FREAD64INT32CNT(hd->tile_bytes, cnt, file);
  FREAD64INT32CNT(hd->n_tiles_across, cnt, file);
  FREAD64INT32CNT(hd->data_unit_size, cnt, file);
  FREAD64INT32CNT(hd->user_data_type, cnt, file);
  FREAD64INT32CNT(hd->defragment, cnt, file);
  FREAD64INT32CNT(hd->consolidate, cnt, file);
  FREAD64INT32CNT(hd->intrinsic_write, cnt, file);
  FREAD64INT32CNT(hd->tile_header_bytes, cnt, file);
  FREAD64INT32CNT(hd->n_uniform_flags, cnt, file);

  if (hd->version < 2) {
    for (i = 0; i < 6; i++) {
      FREAD64CNT(hd->affine_geo_transform[i], cnt, file->fp);
    }
  }
  else if (hd->version >= 2) {
    for (i = 0; i < 6; i++) {
      FREAD64DOUBLE64CNT(hd->affine_geo_transform[i], cnt, file);
    }
  }
  return 1;
}

/**
 * Writes tile headers for the file passed.
 *
 * @param           file The file pointer corresponding to the file
 *                       to write the header.
 */

static int             _sif_write_tile_headers(sif_file *file) {
  long i = 0;
  long long base = file->header_bytes;
  sif_tile *tile = 0;
  sif_header *hd = file->header;
  FSEEK64(file->fp, base, SEEK_SET);
  for (; i < hd->n_tiles; i++, base += hd->tile_header_bytes) {
    tile = file->tiles + i;
    FWRITE64(tile->uniform_pixel_values, hd->data_unit_size, hd->bands, file->fp);
    FWRITE64(tile->uniform_flags, 1, hd->n_uniform_flags, file->fp);
    FWRITE64INT32(tile->block_num, file);
  }
  return 0;
}

/**
 * Reads tile headers for the file passed.
 *
 * @param           file The file pointer corresponding to the file
 *                       to write the header.
 */

static int             _sif_read_tile_headers(sif_file *file) {
  long i = 0, j = 1;
  long long base = file->header_bytes;
  sif_header *hd = file->header;
  sif_tile *tile = 0;
  FSEEK64(file->fp, base, SEEK_SET);
  for (; i < hd->n_tiles; i++, base += hd->tile_header_bytes) {
    tile = file->tiles + i;
    FREAD64(tile->uniform_pixel_values, hd->data_unit_size, hd->bands, file->fp);
    FREAD64(tile->uniform_flags, 1, hd->n_uniform_flags, file->fp);
    FREAD64INT32(tile->block_num, file);
  }
  return j;
}

/**
 * Writes a specific tile header for the file passed.
 *
 * @param           file     The file pointer corresponding to the file
 *                           to write the header.
 * @param           tile     The tile to write.
 * @param           tile_num The number of the tile to write.
 */

static int               _sif_write_tile_header(sif_file *file, sif_tile *tile, long tile_num) {
  LONGLONG loc;
  sif_header *hd = file->header;
  assert(file);
  assert(tile_num >= 0L);
  assert(tile_num < file->header->n_tiles);
  /** In theory, our tile header block would never be longer than the size of a long long.*/
  loc = (LONGLONG)(file->header_bytes + tile_num * file->header->tile_header_bytes);
  /** Set the location.*/
  FSEEK64(file->fp, loc, SEEK_SET);
  /** Write to the file. */
  FWRITE64(tile->uniform_pixel_values, hd->data_unit_size, hd->bands, file->fp);
  FWRITE64(tile->uniform_flags, 1, hd->n_uniform_flags, file->fp);
  FWRITE64INT32(tile->block_num, file);
  return 1;
}

/**
 * Retrieves a meta-data pair by its key so that its value may be inspected
 * or modified. Returns 0 if the meta-data pair corresponding to the key
 * could not be found.
 *
 * @param file     The file on which to perform the operation.
 * @param key      The key of the meta-data pair to retrieve.
 *
 * @return A pointer to the meta-data pair, 0 if not found.
 */

sif_meta_data*   _sif_get_meta_data_pair(sif_file *file, const char *key) {
  sif_header *hd = 0;
  sif_meta_data *q = 0, *r = 0, *result = 0;
  unsigned int hash;
  SIF_CHECK_FILE(file);
  hash = _sif_hash(key) % SIF_HASH_TABLE_SIZE;
  hd = file->header;
  q = file->meta_data[hash];
  for (r = q; r != 0; r = r->next) {
    if (strcmp(key, r->key) == 0) {
      result = r;
      break;
    }
  }
  result = r;
  return result;
}

/**
 * Unlink a meta-data pair by its key. The function simply unlinks it from
 * the collision chain but does not delocate the pair or its key and value
 * fields. Returns the pair unlinked so it may be deallocated by the
 * caller.
 *
 * This function is guaranteed to modify only the next field of any
 * meta-data pair in the file. The next field of the meta-data pair unlinked
 * is set to NULL for safety.
 *
 * @param     file       The file on which to perform the operation.
 * @param     key        The key of the meta-data pair to remove.
 *
 * @return The unlinked meta-data pair.
 */
sif_meta_data           *_sif_unlink_meta_data_pair(sif_file *file, const char *key) {
  sif_meta_data *q = 0, *r = 0, *result = 0, *prev = 0;
  unsigned int hash;
  SIF_CHECK_FILE(file);
  hash = _sif_hash(key) % SIF_HASH_TABLE_SIZE;
  q = file->meta_data[hash];
  for (r = q; r != 0; r = r->next) {
    if (strcmp(key, r->key) == 0) {
      result = r;
      /** If the previous is NULL, then the pair we're unlinking is certainly
	  the first element. Thus, the new first element becomes the result's
          next.*/
      if (prev == 0) {
	file->meta_data[hash] = result->next;
      }
      else {
	prev->next = result->next;
      }
      result->next = 0;
      break;
    }
    prev = r;
  }
  return r;
}

void           _sif_insert_meta_data_pair(sif_file *file, const char *key, sif_meta_data *to_insert) {
  sif_meta_data *q = 0, *result = 0;
  unsigned int hash;
  SIF_CHECK_FILE_V(file);
  hash = _sif_hash(key) % SIF_HASH_TABLE_SIZE;
  to_insert->next = file->meta_data[hash];
  file->meta_data[hash] = to_insert;
  (file->header->n_keys)++;
}

const int        _sif_null_terminator_check(const char *v, int n) {
  int i;
  for (i = 0; i < n; i++) {
    if (v[0] == 0x00) { return 1; }
  }
  return 0;
}


/* See sif-io.h for detailed documentation of public functions. */
const void*      sif_get_meta_data_binary(sif_file *file, const char *key, int *n_bytes) {
  const void *retval = 0; /** By default, null is returned (not found).*/
  sif_meta_data *q = 0;
  SIF_CHECK_FILE(file);
  q = _sif_get_meta_data_pair(file, key);
  if (q == 0) {
    file->error = SIF_ERROR_META_DATA_KEY;
    *n_bytes = 0;
  }
  else {
    *n_bytes = q->value_length;
    retval = q->value;
  }
  return retval;
}


/* See sif-io.h for detailed documentation of public functions. */
const char       *sif_get_meta_data(sif_file *file, const char *key) {
  const char *retval = 0; /** By default, null is returned (not found).*/
  sif_meta_data *q = 0;
  SIF_CHECK_FILE(file);
  q = _sif_get_meta_data_pair(file, key);
  if (q == 0) {
    file->error = SIF_ERROR_META_DATA_KEY;
    retval = 0;
  }
  else {
    retval = (char*)q->value;
    if (!_sif_null_terminator_check(retval, q->value_length)) {
      file->error = SIF_ERROR_META_DATA_VALUE;
    }
  }
  return retval;
}

/**
 * Sets a meta-data field with a given key to a value. The
 * length of the value is specified, thereby allowing for
 * binary, non-null-terminated, meta-data.
 *
 * @param file      The file to set the meta-data.
 * @param key       The key of the field to set.
 * @param value     The value for which to set the field.
 * @param value_len The length of the value.
 */

static void             _sif_set_meta_data_len(sif_file *file, const char *key, const char *value, int value_len) {
  int success = 0, key_len;       /** We need room for the null terminator. */
  sif_meta_data *i = 0;

  assert(file);
  i = _sif_get_meta_data_pair(file, key);
  if (i == 0) {
    key_len = strlen(key) + 1;
    SIF_ERROR_CHECK_RETURN_V((i = (sif_meta_data*)malloc(sizeof(sif_meta_data))) == 0, SIF_ERROR_MEM);
    SIF_ERROR_CHECK_RETURN_V((i->value = (char*)malloc(value_len * sizeof(char))) == 0, SIF_ERROR_MEM);
    SIF_ERROR_CHECK_RETURN_V((i->key = (char*)malloc(key_len * sizeof(char))) == 0, SIF_ERROR_MEM);
    i->key_length = key_len;
    i->value_length = value_len;
    memcpy(i->key, key, sizeof(char) * key_len);
    memcpy(i->value, value, sizeof(char) * value_len);
    _sif_insert_meta_data_pair(file, key, i);
  }
  else {
    if (value_len != i->value_length) {
      SIF_ERROR_CHECK_RETURN_V((i->value = (char*)realloc(i->value, value_len * sizeof(char))) == 0, SIF_ERROR_MEM);
      i->value_length = value_len;
      memcpy(i->value, value, sizeof(char) * value_len);
    }
  }

}

/* See sif-io.h for detailed documentation of public functions. */
void             sif_set_meta_data(sif_file *file, const char *key, const char *value) {
  SIF_CHECK_FILE_V(file);
  _sif_set_meta_data_len(file, key, value, strlen(value) + 1);
}

/* See sif-io.h for detailed documentation of public functions. */
void             sif_set_meta_data_binary(sif_file *file, const char *key, const void *buffer, int n_bytes) {
  SIF_CHECK_FILE_V(file);
  _sif_set_meta_data_len(file, key, (const char*)buffer, n_bytes);
}

/**
 * Return the last used block index in the file.
 *
 * @param file The file to compute the last used block index.
 *
 * @return The last used block index. -1 is returned
 *         if no blocks are in use.
 */

static long         _sif_get_last_used_block_index(sif_file *file) {
  long i = 0;
  long last_known_used_block = -1;
  for (; i < file->header->n_tiles; i++) {
    if (file->blocks_to_tiles[i] != -1) {
      last_known_used_block = i;
    }
  }
  return last_known_used_block;
}

/**
 * Truncate the file at a particular position.
 *
 * @param file   The file to truncate.
 * @param pos    The position of truncation.
 */

static void             _sif_truncate(sif_file *file, LONGLONG pos) {
#ifdef WIN32
  FSEEK64V(file->fp, pos, SEEK_SET);
  SIF_ERROR_CHECK_RETURN_V(SetEndOfFile(file->fp) == 0, SIF_ERROR_TRUNCATE)
#else
  SIF_ERROR_CHECK_RETURN_V(ftruncate(fileno(file->fp), pos) != 0, SIF_ERROR_TRUNCATE);
#endif
}

/**
 * Read the meta-data from the disk, storing the contents in
 * the file structure for easy access.
 *
 * @param file   The file to read the meta data.
 */
static void             _sif_read_meta_data(sif_file *file) {
  sif_header *header = 0;
  LONGLONG loc = 0;
  unsigned int i = 0;
  long val = 0;
  int n_keys;

  /**  sif_meta_data *md = 0;**/
  sif_meta_data md;
  header = file->header;

  n_keys = header->n_keys;
  header->n_keys = 0;
  /** move the file pointer to one after the last used block. **/
  loc = _sif_get_block_location(file, _sif_get_last_used_block_index(file) + 1);
  FSEEK64V(file->fp, loc, SEEK_SET);

  /**
   * This code is usually executed during an open. As such, the file structure
   * has not been completely initialized. FREAD64NEC must be used instead of FREAD64
   * because we must free the memory before returning upon an error.
   */
  for (i = 0; i < n_keys; i++) {
    if (FREAD64INT32NEC(val, file) == 0) {
      _sif_free_meta_data(file);
      SIF_ERROR_CHECK_RETURN_V(1, SIF_ERROR_READ);
    }
    md.key_length = (unsigned long)val;
    if ((md.key = malloc(sizeof(char) * md.key_length)) == 0) {
      _sif_free_meta_data(file);
      SIF_ERROR_CHECK_RETURN_V(1, SIF_ERROR_MEM);
    }
    if (md.key_length != 0) {
      (md.key)[md.key_length - 1] = 0;
    }
    if (FREAD64NEC(md.key, 1, md.key_length, file->fp) != md.key_length) {
      free(md.key);
      _sif_free_meta_data(file);
      SIF_ERROR_CHECK_RETURN_V(1, SIF_ERROR_READ);
    }
    if (FREAD64INT32NEC(val, file) == 0) {
      free(md.key);
      _sif_free_meta_data(file);
      SIF_ERROR_CHECK_RETURN_V(1, SIF_ERROR_READ);
    }
    md.value_length = val;
    if ((md.value = malloc(sizeof(char) * md.value_length)) == 0) {
      free(md.key);
      _sif_free_meta_data(file);
      SIF_ERROR_CHECK_RETURN_V(1, SIF_ERROR_MEM);
    }
    /**    (md.value)[md.value_length] = 0;**/
    if (FREAD64NEC(md.value, 1, md.value_length, file->fp) == 0) {
      free(md.value);
      free(md.key);
      _sif_free_meta_data(file);
      SIF_ERROR_CHECK_RETURN_V(1, SIF_ERROR_READ);
    }
    sif_set_meta_data_binary(file, md.key, md.value, md.value_length);
    free(md.value);
    free(md.key);
    if (file->error) {
      return;
    }
  }
}

/**
 * Write the meta data from the file structure to the disk.
 *
 * @param file        The file containing the meta-data structure to write.
 */

static int             _sif_write_meta_data(sif_file *file) {
  sif_header *header = 0;
  LONGLONG loc = 0, eofpos = 0;
  sif_meta_data *i = 0;
  int j = 0;
  header = file->header;
  loc = _sif_get_block_location(file, _sif_get_last_used_block_index(file) + 1);
  eofpos = loc;
  FSEEK64(file->fp, loc, SEEK_SET);
  for (j = 0; j < SIF_HASH_TABLE_SIZE; j++) {
    for (i = file->meta_data[j]; i != 0; i = i->next) {
      FWRITE64INT32(i->key_length, file); eofpos += 4;
      FWRITE64(i->key, i->key_length, 1, file->fp); eofpos += i->key_length;
      FWRITE64INT32(i->value_length, file); eofpos += 4;
      FWRITE64(i->value, i->value_length, 1, file->fp); eofpos += i->value_length;
    }
  }
  eofpos = eofpos + 1;
  _sif_truncate(file, eofpos);
  return 1;
}

/* See sif-io.h for detailed documentation of public functions. */
void             sif_get_tile_slice(sif_file *file, void *buffer, long tx, long ty, long band) {
  sif_tile *tile = 0;
  sif_header *hd = 0;
  long i = 0, tile_num = 0;
  LONGLONG pos = 0;
  u_char *data = buffer, *upv;
  //  printf("get x: %d y: %d b: %d\n", tx, ty, band);
  SIF_CHECK_FILE_V(file);
  hd = file->header;
  if (tx < 0 || ty < 0 || tx >= hd->n_tiles_across) {
    file->error = SIF_ERROR_INVALID_TN;
    return;
  }
  if (band < 0 || band >= hd->bands) {
    file->error = SIF_ERROR_INVALID_BAND;
    return;
  }
  if (buffer == 0) {
    file->error = SIF_ERROR_INVALID_BUFFER;
    return;
  }
  tile_num = (hd->n_tiles_across * ty) + tx;
  tile = file->tiles + tile_num;
  bzero(buffer, hd->data_unit_size * file->units_per_slice);
  if (_sif_band_of_tile_is_uniform_shallow(file, tile_num, band)) {
	  upv = tile->uniform_pixel_values + (hd->data_unit_size * band);
	  if (hd->data_unit_size > 1) {
		for (i = 0, data = buffer; i < file->units_per_slice; i++, data += hd->data_unit_size) {
			memcpy(data, upv, hd->data_unit_size);
		}
	  }
	  else {
		memset(data, upv[0], file->units_per_slice);
	  }
  }
  else {
    pos = _sif_get_block_location(file, tile->block_num) + (hd->data_unit_size * file->units_per_slice) * band;
    FSEEK64V(file->fp, pos, SEEK_SET);
    FREAD64V(buffer, hd->data_unit_size, file->units_per_slice, file->fp);
  }
  return;
}

/* See sif-io.h for detailed documentation of public functions. */
void            sif_fill_tile_slice(sif_file *file, long tx, long ty, long band, const void *value) {
  sif_tile *tile;
  sif_header *hd = 0;
  long tile_num;
  SIF_CHECK_FILE_V(file);
  hd = file->header;
  if (tx < 0 || ty < 0 || tx >= hd->n_tiles_across) {
    file->error = SIF_ERROR_INVALID_TN;
    return;
  }
  if (band < 0 || band >= hd->bands) {
    file->error = SIF_ERROR_INVALID_BAND;
    return;
  }
  if (value == 0) {
    file->error = SIF_ERROR_INVALID_BUFFER;
    return;
  }
  /** Compute the tile number using the stride stored in the header. */
  tile_num = (hd->n_tiles_across * ty) + tx;
  tile = file->tiles + tile_num;
  //  printf("set x: %d y: %d b: %d\n", tx, ty, band);

  /** We should not be changing tiles for read-only files. Return an error. */
  if (file->read_only) {
    file->error = SIF_ERROR_INVALID_FILE_MODE;
    return;
  }

  memcpy(tile->uniform_pixel_values + (hd->data_unit_size * band), value, hd->data_unit_size);
  SIF_SET_BIT(tile->uniform_flags, band);
  if (_sif_completely_uniform_shallow(file, tile_num) && tile->block_num != -1) {
    file->blocks_to_tiles[file->tiles[tile_num].block_num] = -1;
    file->tiles[tile_num].block_num = -1;
  }
  _sif_write_tile_header(file, tile, tile_num);
  return;
}

/* See sif-io.h for detailed documentation of public functions. */
void            sif_fill_tiles(sif_file *file, long band, const void *value) {
  sif_tile *tile;
  sif_header *hd = 0;
  long tile_num;

  SIF_CHECK_FILE_V(file);
  hd = file->header;
  if (band < 0 || band >= hd->bands) {
    file->error = SIF_ERROR_INVALID_BAND;
    return;
  }
  if (value == 0) {
    file->error = SIF_ERROR_INVALID_BUFFER;
    return;
  }
  /** We should not be changing tiles for read-only files. Return an error. */
  if (file->read_only) {
     file->error = SIF_ERROR_INVALID_FILE_MODE;
     return;
  }

  for (tile_num = 0; tile_num < hd->n_tiles; tile_num++) {
     /** Compute the tile number using the stride stored in the header. */
     tile = file->tiles + tile_num;
     /**  printf("set x: %d y: %d b: %d\n", tx, ty, band);**/
  
     memcpy(tile->uniform_pixel_values + (hd->data_unit_size * band), value, hd->data_unit_size);
     SIF_SET_BIT(tile->uniform_flags, band);
     if (_sif_completely_uniform_shallow(file, tile_num) && tile->block_num != -1) {
        file->blocks_to_tiles[file->tiles[tile_num].block_num] = -1;
        file->tiles[tile_num].block_num = -1;
     }
  }
  _sif_write_tile_headers(file);
}

/* See sif-io.h for detailed documentation of public functions. */
void            sif_set_tile_slice(sif_file *file, const void *buffer, long tx, long ty, long band) {
  sif_tile *tile = 0;
  sif_header *hd = 0;
  long i = 0, free_b = 0, extentX = 0, extentY = 0;             ;
  LONGLONG loc, tile_num;
  SIF_CHECK_FILE_V(file);
  hd = file->header;

  if (tx < 0 || ty < 0 || tx >= hd->n_tiles_across) {
    file->error = SIF_ERROR_INVALID_TN;
    return;
  }
  if (band < 0 || band >= hd->bands) {
    file->error = SIF_ERROR_INVALID_BAND;
    return;
  }
  if (buffer == 0) {
    file->error = SIF_ERROR_INVALID_BUFFER;
    return;
  }
  extentX = hd->tile_width;
  extentY = hd->tile_height;
  //  printf("set x: %d y: %d b: %d\n", tx, ty, band);
  /** We should not be changing tiles for read-only files. Return an error. */
  if (file->read_only) {
    file->error = SIF_ERROR_INVALID_FILE_MODE;
    return;
  }

  extentX = MIN(hd->tile_width, hd->width - tx * hd->tile_width);
  extentY = MIN(hd->tile_height, hd->height - ty * hd->tile_height);

  /** Compute the tile number using the stride stored in the header. */
  tile_num = (hd->n_tiles_across * ty) + tx;
  tile = file->tiles + tile_num;
  /** If the flag intrinsic_write is set, that means we check for pixel
      uniformity on a write. */
  if (hd->intrinsic_write && _sif_is_uniform(file, buffer, extentX, extentY)) {
    memcpy(tile->uniform_pixel_values + (hd->data_unit_size * band), buffer, hd->data_unit_size);
    SIF_SET_BIT(tile->uniform_flags, band);
    if (_sif_completely_uniform_shallow(file, tile_num) && tile->block_num != -1) {
      file->blocks_to_tiles[file->tiles[tile_num].block_num] = -1;
      file->tiles[tile_num].block_num = -1;
    }
    _sif_write_tile_header(file, tile, tile_num);
    return;
  }
  /** If we've gotten here then the tile is non-uniform or we're presuming that
      it is. If each slice of the tile cube was uniform before, we need to find
      a free spot on disk to put the tile cube. */
  if (tile->block_num == -1) {
    /** Look for the first free tile block. */
    for (i = 0; i < hd->n_tiles; i++) {
      if (file->blocks_to_tiles[i] == -1) {
	free_b = i;
	break;
      }
    }
    tile->block_num = free_b;
    file->blocks_to_tiles[free_b] = tile_num;
    loc = _sif_get_block_location(file, tile->block_num);
    FSEEK64V(file->fp, loc, SEEK_SET);
    /** Write the buffer out n times where n is the number of bands. Raster
        data stored for uniform bands will be ignored.*/
    for (i = 0; i < hd->bands; i++) {
      FWRITE64V((u_char*)buffer, hd->data_unit_size, file->units_per_slice, file->fp);
    }
 
  }
  /** If we already checked for pixel uniformity, we don't need to do
      it again. */
  if (hd->intrinsic_write == 0) {
    file->dirty_tiles[tile_num] = 1;
  }
  /** Compute the location for the non-uniform slice and go there. */
  loc = _sif_get_block_location(file, tile->block_num) + hd->data_unit_size * file->units_per_slice * band;
  FSEEK64V(file->fp, loc, SEEK_SET);
  /** Write the non-uniform slice to disk. */
  FWRITE64V((u_char*)buffer, hd->data_unit_size, file->units_per_slice, file->fp);

  /** Set the uniformity flag for this band to false. */
  SIF_CLEAR_BIT(tile->uniform_flags, band);

  /** Write the tile header out to disk. */
  _sif_write_tile_header(file, tile, tile_num);
}

/* See sif-io.h for detailed documentation of public functions. */
static void             _sif_swap_blocks(sif_file *file, long a, long b, void *da, void *db, int assign) {
  LONGLONG pos_a = 0, pos_b = 0, tb = file->header->tile_bytes;
  if (a == b) {
    return;
  }
  pos_a = _sif_get_block_location(file, a);
  pos_b = _sif_get_block_location(file, b);
  if (a != -1 && !assign) {
    FSEEK64V(file->fp, pos_a, SEEK_SET);
    FREAD64V((unsigned char*)da, 1, tb, file->fp);
  }
  if (b != -1) {
    FSEEK64V(file->fp, pos_b, SEEK_SET);
    FREAD64V((unsigned char*)db, 1, tb, file->fp);
  }
  if (a != -1 && !assign) {
    FSEEK64V(file->fp, pos_b, SEEK_SET);
    FWRITE64V((unsigned char*)da, 1, tb, file->fp);
  }
  if (b != -1) {
    FSEEK64V(file->fp, pos_a, SEEK_SET);
    FWRITE64V((unsigned char*)db, 1, tb, file->fp);
  }
}

/* See sif-io.h for detailed documentation of public functions. */
void             sif_set_raster(sif_file* file, const void *data,
                                long x, long y, long w, long h, long band) {
  const unsigned char *datav = data; /** makes VC++ happy. can't do
                                         pointer arithmetic on void*'s! */
  long tnx1, tny1, tnx2, tny2; /** the starting and ending tile indices. */
  long sxt, syt, ext, eyt;     /** the starting and ending coordinates on the tile raster. */
  long sxd, syd;               /** the starting coordinates on the data raster. */
  long cyd, cyt;               /** the current ordinates for the data and tile rasters. */
  long tx, ty;                 /** the current working tile indices. */
  long tw, th, trs, dus, wdus; /** the tile width, height, data unit size, scan line byte size. */
  sif_header *hd;                  /** header */
  unsigned char *buffer;                     /** buffer */
  SIF_CHECK_FILE_V(file);
  if (file->read_only) {
    return;
  }
  hd = file->header;
  if (x < 0 || y < 0) {
    file->error = SIF_ERROR_INVALID_COORD;
    return;
  }
  if (w < 1 || h < 1 || x + w > hd->width || y + h > hd->height) {
    file->error = SIF_ERROR_INVALID_REGION_SIZE;
    return;
  }
  if (band < 0 || band >= hd->bands) {
    file->error = SIF_ERROR_INVALID_BAND;
    return;
  }
  buffer = file->buffer[0];
  tw = hd->tile_width;
  th = hd->tile_height;
  trs = hd->n_tiles_across;
  dus = hd->data_unit_size;/** size of a single pixel in bytes. */
  wdus = dus * w;          /** width of a single scan line. */
  tnx1 = x / tw;           /** the starting tile horizontal index. */
  tny1 = y / th;           /** the starting tile vertical index. */
  tnx2 = (x + w - 1) / tw; /** the end tile horizontal index. */
  tny2 = (y + h - 1) / th; /** the end tile vertical index. */
  for (ty = tny1; ty <= tny2; ty++) {
    for (tx = tnx1; tx <= tnx2; tx++) {
      /** grab the tile. */
      sif_get_tile_slice(file, buffer, tx, ty, band);
      sxt = MAX(0, x - tx * tw);                  /** starting x pixel on tile raster. */
      syt = MAX(0, y - ty * th);                  /** starting y pixel on tile raster. */
      ext = MIN(tw - 1, x + w - 1 - (tx * tw));   /** ending x pixel on tile raster. */
      eyt = MIN(th - 1, y + h - 1 - (ty * th));   /** ending y pixel on tile raster. */
      sxd = (tx * tw + sxt) - x;                  /** starting x pixel on data raster. */
      syd = (ty * th + syt) - y;                  /** starting y pixel on data raster. */

      /** copy the window from the raster to the tile.*/
      for (cyd = syd, cyt = syt; cyt <= eyt; cyd++, cyt++) {
	memcpy(buffer + (cyt * trs) + (sxt * dus), datav + (cyd * wdus) + (sxd * dus), (ext - sxt + 1) * dus);
      }
      /** put the tile back with modifications. */
      sif_set_tile_slice(file, buffer, tx, ty, band);
      if (file->error) {
	return;
      }
    }
  }
}

/**
 * Retrieves an entire tile (all bands).
 *
 * @param file    The file where the tile is stored.
 * @param tile_no The index of the tile.
 * @param data    The buffer to store the result of the read.
 */

void              _sif_get_tile(sif_file *file, LONGLONG tile_no, unsigned char *data) {
  sif_tile *tile = file->tiles + tile_no;
  sif_header *hd = file->header;
  u_char *buffer = 0;
  u_char *upv = 0;
  long i = 0, j = 0;
  LONGLONG pos = 0;
  bzero(data, hd->tile_bytes);
  buffer = data;
  for (i = 0; i < hd->bands; i++) {
    buffer = ((u_char*)data) + (file->units_per_slice * hd->data_unit_size * i);
	if (_sif_completely_uniform_shallow(file, tile_no)) {
		upv = tile->uniform_pixel_values + i * hd->data_unit_size;
		if (hd->data_unit_size == 1) {
			for (j = 0; j < file->units_per_slice; j++, buffer += hd->data_unit_size) {
				memcpy(buffer, upv, hd->data_unit_size);
			}
		}
		else {
			memset(buffer, upv[0], file->units_per_slice);
		}
	}
	else {
		pos = _sif_get_block_location(file, tile->block_num) + file->units_per_slice * hd->data_unit_size * i;
		FSEEK64V(file->fp, pos, SEEK_SET);
		FREAD64V(buffer, 1, hd->tile_bytes, file->fp);
	}
  }
  return;
}

/* See sif-io.h for detailed documentation of public functions. */
void             sif_get_raster(sif_file* file, void *data,
				long x, long y, long w, long h, long band) {
  unsigned char *datav = data;
  long tnx1, tny1, tnx2, tny2; /** the starting and ending tile indices. */
  long sxt, syt, ext, eyt;     /** the starting and ending coordinates on the tile raster. */
  long sxd, syd;               /** the starting coordinates on the data raster. */
  long cyd, cyt;               /** the current ordinates for the data and tile rasters. */
  long tx, ty;                 /** the current working tile indices. */
  long tw, th, trs, dus, wdus; /** the tile width, height, data unit size, scan line byte size. */
  sif_header *hd;                  /** header */
  unsigned char *buffer;                    /** buffer */
  SIF_CHECK_FILE_V(file);
  hd = file->header;
  if (x < 0 || y < 0) {
    file->error = SIF_ERROR_INVALID_COORD;
    return;
  }
  if (w < 1 || h < 1 || x + w > hd->width || y + h > hd->height) {
    file->error = SIF_ERROR_INVALID_REGION_SIZE;
    return;
  }
  if (band < 0 || band >= hd->bands) {
    file->error = SIF_ERROR_INVALID_BAND;
    return;
  }
  buffer = file->buffer[0];
  tw = hd->tile_width;
  th = hd->tile_height;
  trs = hd->n_tiles_across;
  dus = hd->data_unit_size;/** size of a single pixel in bytes. */
  wdus = dus * w;          /** width of a single scan line. */
  tnx1 = x / tw;           /** the starting tile horizontal index. */
  tny1 = y / th;           /** the starting tile vertical index. */
  tnx2 = (x + w - 1) / tw; /** the end tile horizontal index. */
  tny2 = (y + h - 1) / th; /** the end tile vertical index. */
  for (ty = tny1; ty <= tny2; ty++) {
    for (tx = tnx1; tx <= tnx2; tx++) {

      sxt = MAX(0, x - tx * tw);                  /** starting x pixel on tile raster. */
      syt = MAX(0, y - ty * th);                  /** starting y pixel on tile raster. */
      ext = MIN(tw - 1, x + w - 1 - (tx * tw));   /** ending x pixel on tile raster. */
      eyt = MIN(th - 1, y + h - 1 - (ty * th));   /** ending y pixel on tile raster. */
      sxd = (tx * tw + sxt) - x;                  /** starting x pixel on data raster. */
      syd = (ty * th + syt) - y;                  /** starting y pixel on data raster. */

      /** grab the tile. */
      sif_get_tile_slice(file, buffer, tx, ty, band);

      /** copy the window from the tile to the input raster.*/
      for (cyd = syd, cyt = syt; cyt <= eyt; cyd++, cyt++) {
	memcpy(datav + (cyd * wdus) + (sxd * dus), buffer + (cyt * trs) + (sxt * dus), (ext - sxt + 1) * dus);
      }
    }
  }
}

/* See sif-io.h for detailed documentation of public functions. */
int              sif_is_shallow_uniform(sif_file *file, long x, long y, long w, long h, long band, void *uniform_value) {
  sif_header *hd = file->header;
  long sx = 0;            /** starting tile x index **/
  long sy = 0;            /** starting tile y index **/
  long ex = 0;            /** ending tile x index **/
  long ey = 0;            /** ending tile y index **/
  long ix, iy;            /** current tile index. */
  int uniform = 1;
  sif_tile *first_tile = 0;
  u_char *upv = 0;
  SIF_CHECK_FILE(file);
  hd = file->header;
  sx = x / hd->tile_width;
  sy = y / hd->tile_height;
  ex = (x + w - 1) / hd->tile_width;
  ey = (y + h - 1) / hd->tile_height;
  first_tile = file->tiles + ((hd->n_tiles_across * sy) + sx);
  upv = ((u_char*)first_tile->uniform_pixel_values) + hd->data_unit_size * band;

  /** Scan through each tile in the region. If we reach a tile that is
      uncompressed, stop, return false.  If we reach a tile that is
      compressed but whose data differs from the first tile, stop,
      return false.  If we scan through every tile, and each one is
      compressed and has a uniform pixel value that is identical to
      the first tile, return true. Note that we incure the cost,
      albeit minimal, by comparing the first tile with itself. This
      approach has the advantage of avoiding an extra if in the for
      statement.*/

  for (ix = sx; (ix <= ex) && uniform; ix++) {
    for (iy = sy; (iy <= ey) && uniform; iy++) {
      uniform =
	uniform
	&& sif_is_slice_shallow_uniform(file, ix, iy, band, uniform_value)
	&& !memcmp(upv, uniform_value, hd->data_unit_size);
    }
  }
  return uniform;
}

/* See sif-io.h for detailed documentation of public functions. */
int              sif_is_slice_shallow_uniform(sif_file *file, long tx, long ty, long band,
					      void *uniform_value) {
  sif_header *hd = 0;
  long tile_num = 0;
  sif_tile *tile = 0;
  u_char *upv = 0;
  SIF_CHECK_FILE(file);

  hd = file->header;
  tile_num = (hd->n_tiles_across * ty) + tx;
  tile = file->tiles + tile_num;
  upv = ((u_char*)tile->uniform_pixel_values) + band * hd->data_unit_size;

  /** Is the bit for the band set? If so, the tile is compressed. Copy the
      uniform pixel value and return true. */
  if (SIF_GET_BIT(tile->uniform_flags, band)) {
    memcpy(uniform_value, upv, hd->data_unit_size);
    return 1;
  }
  /** The band is not compressed. Return false. */
  return 0;
}

/**
 * Check whether a tile (all bands) is uniform. If the tile is found to be
 * uniform (i.e., each data unit in the tile is represented by an identical
 * sequence of bytes), this common data unit sequence is stored in the
 * tile's header, and the physical block is freed. If the tile is already
 * uniform, no change is made to the file or tile header. The tile's dirty flag
 * is set to FALSE (not dirty) upon completion.
 *
 * @param file    The file containing the tile to check.
 * @param tile_no The index of the tile to check.
 * @param data    A buffer which contains enough bytes to store the tile.
 */

static int             _sif_uniform_check(sif_file *file, long tile_no, void *data) {
  long i = 0;  
  int uniform = 1;
  sif_tile *tiles = file->tiles;
  sif_tile *tile = tiles + tile_no;
  sif_header *hd = file->header;
  u_char *datau = data, *upv = 0;
  long row = tile_no / hd->n_tiles_across, col = tile_no % hd->n_tiles_across, extentX = hd->tile_width, extentY = hd->tile_height;

  if (tile->block_num == -1) {
    return 1;
  }
  _sif_get_tile(file, tile_no, data);
  if (file->error != 0) {
    return -1;
  }

  extentX = MIN(hd->tile_width, hd->width - col * hd->tile_width);
  extentY = MIN(hd->tile_height, hd->height - row * hd->tile_height);

  for (i = 0; i < hd->bands; i++) {
    datau = ((u_char*)data) + (i * file->units_per_slice * hd->data_unit_size);
    if (!SIF_GET_BIT(tile->uniform_flags, i)
		&& _sif_is_uniform(file, datau, extentX, extentY)
		&& tile->block_num != -1) {
      upv = tile->uniform_pixel_values + (i * hd->data_unit_size);
      memcpy(upv, datau, hd->data_unit_size);
      SIF_SET_BIT(tile->uniform_flags, i);
    }
  }
  if (_sif_completely_uniform_shallow(file, tile_no) && tile->block_num != -1) {
    file->blocks_to_tiles[tiles[tile_no].block_num] = -1;
    tile->block_num = -1;
  }
  _sif_write_tile_header(file, tiles + tile_no, tile_no);
  return uniform;
}

int             _sif_is_uniform(sif_file *file, const void *data, int extentX, int extentY) {
  int uniform = 1;
  long i = 0, j = 0, k = 0;
  const unsigned char *datav = data;
  unsigned char first, second;
  sif_header *hd = file->header;
  long data_unit_size = file->header->data_unit_size;
  long num_units = 0;
  num_units = extentY * hd->tile_width;
  if (data_unit_size == 1) {
    first = *datav;

	/** In the case of border tiles (right-most and bottom-most), we don't want to evaluate pixels that
	    are outside the image, thus, we have two for loops. */
	for (i = 0; i < num_units && uniform; i += hd->tile_width) {
		for (j = i, k = 0; k < extentX && uniform; j++, k++) {
		   uniform = uniform && (first == datav[j]);
		}
	}
  }
  /** If our data units are words, things are a bit more complicated. We
      still want to avoid using memcmp. */
  else if (data_unit_size == 2) {
    first = datav[0];
    second = datav[1];
    for (i = 0; i < num_units && uniform; i += hd->tile_width) {
	  for (j = i, k = 0; k < extentX && uniform; j++, k++) {
        uniform = uniform && (first == datav[j * 2]) && (second == datav[j * 2 + 1]);
	  }
    }
  }
  else {
	for (i = 0; i < num_units && uniform; i += hd->tile_width) {
	  for (j = i, k = 0; k < extentX && uniform; j++, k++) {
        uniform = uniform && (memcmp(data,
                                     datav + (data_unit_size * j),
									 data_unit_size) == 0);
	  }
    }
  }
  return uniform;
}

/**
 * Check all tiles in a file for pixel uniformity. If any tiles are found
 * to be uniform (i.e., each data unit in a tile is represented by an
 * identical sequence of bytes), the common data units are stored
 * in the tile headers, and the physical storage blocks are freed. If
 * the consolidation flag in the file's header is turned off or
 * the file is read only, this method does nothing.
 *
 * @param file   The file to mark for uniformity.
 */

static void             _sif_mark_uniform_tiles(sif_file *file, void *buffer) {
  long i = 0;
  sif_header *hd = file->header;
  if (file->read_only || !file->header->consolidate) {
    return;
  }
  for (i = 0; i < hd->n_tiles; i++) {
    if (file->tiles[i].block_num != -1 && file->dirty_tiles[i]) {
      _sif_uniform_check(file, i, buffer);
      if (file->error != 0) {
	return;
      }
      file->dirty_tiles[i] = 0;
    }
  }
}

/* See sif-io.h for detailed documentation of public functions. */
void             sif_defragment(sif_file *file) {
  void *buf1, *buf2;
  sif_header *hd;
  LONGLONG i = 0, tn1 = 0, tn2 = 0, bn1 = 0, bn2 = 0;
  SIF_CHECK_FILE_V(file);
  if (file->read_only || !file->header->defragment) {
    return;
  }
  hd = file->header;
  buf1 = file->buffer[0];
  buf2 = file->buffer[1];
  for (i = 0, bn1 = 0; i < hd->n_tiles; i++) {
    if (file->tiles[i].block_num != -1) {
      bn2 = file->tiles[i].block_num;
      tn1 = file->blocks_to_tiles[bn1];
      tn2 = i;

      /** Swap the block nums for bookkeeping purposes. **/
      file->tiles[tn2].block_num = bn1;
      file->blocks_to_tiles[bn1] = tn2;

      /** It's possible that the other tile is uniform... */
      if (tn1 != -1) {
	file->tiles[tn1].block_num = bn2;
	file->blocks_to_tiles[bn2] = tn1;
	_sif_write_tile_header(file, file->tiles + tn1, tn1);
	if (file->error != 0) {
	  return;
	}
      }
      else {
	file->blocks_to_tiles[bn2] = -1;
      }

      /** Swap the disk blocks. **/
      _sif_swap_blocks(file, bn1, bn2, buf1, buf2, tn1 == -1);
      if (file->error != 0) {
	return;
      }

      _sif_write_tile_header(file, file->tiles + tn2, tn2);
      if (file->error != 0) {
	return;
      }
      bn1++;
    }
  }

  /** Truncate the file. */
  /**_sif_truncate(file, _sif_get_block_location(file, _sif_get_last_used_block_index(file) + 1));**/
  if (file->error != 0) {
    return;
  }

  /** We lost the meta data, write it out again. */
  _sif_write_meta_data(file);
  if (file->error != 0) {
    return;
  }
}

/* See sif-io.h for detailed documentation of public functions. */
sif_file*        sif_open(const char *filename, int read_only) {
  sif_file *retval = 0;
#ifdef WIN32
  HANDLE fp = 0;
#else
  FILE *fp = 0;
#endif
  sif_header *header = 0;
  int i = 0;
#ifdef WIN32
  if (read_only) {
    fp = CreateFile(TEXT(filename), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
  }
  else {
    fp = CreateFile(TEXT(filename), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  }

#else
  if (read_only) {
    fp = fopen64(filename, "rb");
  }
  else {
    fp = fopen64(filename, "r+b");
  }
#endif
  if (FILE_IS_OKAY(fp)) {
    retval = _sif_alloc_fp();
    if (retval == 0) {
      return 0;
    }
    retval->fp = fp;
    retval->error = 0;
    header = (retval->header = _sif_alloc_header());
    if (header == 0) {
      free(retval);
      FCLOSE64(fp);
      return 0;
    }
    retval->tiles = 0;

    retval->meta_data = _sif_alloc_meta_data_table();
    retval->read_only = read_only;
    REWIND64NEC(fp);
    if (_sif_read_header(retval) != 1 ||
	header->version > SIF_VERSION ||
        strncmp(header->magic_number, SIF_MAGIC_NUMBER, SIF_MAGIC_NUMBER_SIZE) != 0 ||
	(retval->tiles = _sif_alloc_tile_headers(retval)) == 0) {
      free(header);
      free(retval);
      FCLOSE64(fp);
      return 0;
    }
    retval->base_location =  retval->header_bytes + (header->tile_header_bytes * header->n_tiles);
    retval->units_per_tile = header->tile_width * header->tile_height * header->bands;
    retval->units_per_slice = header->tile_width * header->tile_height;

    if (_sif_read_tile_headers(retval) != 1 ||
	(retval->blocks_to_tiles = (long*)malloc(header->n_tiles * sizeof(long))) == 0 ||
	(retval->dirty_tiles = (long*)malloc(header->n_tiles * sizeof(long))) == 0 ||
	(retval->buffer[0] = malloc(header->tile_bytes)) == 0 ||
	(retval->buffer[1] = malloc(header->tile_bytes)) == 0) {
      free(header);
      free(retval->tiles);
      free(retval->blocks_to_tiles);
      free(retval->dirty_tiles);
      free(retval->buffer[0]);
      free(retval->buffer[1]);
      free(retval);
      FCLOSE64(fp);
      return 0;
    }
    bzero(retval->dirty_tiles, sizeof(long) * header->n_tiles);
    for (i = 0; i < header->n_tiles; i++) {
      retval->blocks_to_tiles[i] = -1;
    }
    for (i = 0; i < header->n_tiles; i++) {
      if (retval->tiles[i].block_num != -1) {
	retval->blocks_to_tiles[retval->tiles[i].block_num] = i;
      }
    }
    _sif_read_meta_data(retval);
    if (retval->error != 0) {
      free(header);
      free(retval->tiles);
      free(retval->blocks_to_tiles);
      free(retval->dirty_tiles);
      free(retval->buffer[0]);
      free(retval->buffer[1]);
      free(retval);
      FCLOSE64(fp);
    }
  }
  return retval;
}

/* See sif-io.h for detailed documentation of public functions. */
int              sif_close(sif_file* file) {
  int status = 0;
  /** Flush whatever data has not been written to disk. */
  sif_flush(file);
  _sif_free_tile_headers(file);
  _sif_free_meta_data(file);
  free(file->header);
  free(file->blocks_to_tiles);
  free(file->dirty_tiles);
  free(file->buffer[0]);
  free(file->buffer[1]);
  free(file->simple_region_buffer);
  status = FCLOSE64(file->fp);
  if (file->error) { free(file); return -1; }
  free(file);
  return status;
}

/* See sif-io.h for detailed documentation of public functions. */
int             sif_flush(sif_file* file) {
  if (!file->read_only) {
    _sif_write_header(file);
    _sif_write_tile_headers(file);
    _sif_write_meta_data(file);
    /** Detect pixel uniformity in blocks. Any block that has pixel uniformity will be compressed. */
    if (file->header->consolidate) {
        sif_consolidate(file);
    }
    /** Defragment the block space. */
    if (file->header->defragment) {
        sif_defragment(file);
    }
#ifdef WIN32
    FlushFileBuffers(file->fp);
#else
    fflush(file->fp);
#endif
  }
  return 0;
}

/* See sif-io.h for detailed documentation of public functions. */
void             sif_consolidate(sif_file *file) {
  if (file->read_only || !file->header->consolidate) {
    return;
  }
  _sif_mark_uniform_tiles(file, file->buffer[0]);
  /**_sif_truncate(file, _sif_get_block_location(file, _sif_get_last_used_block_index(file) + 1));**/
  _sif_write_meta_data(file);
}

/* See sif-io.h for detailed documentation of public functions. */
sif_file*        sif_create(const char *filename, long width, long height,
			    long bands, int data_unit_size,
			    int user_data_type, int consolidate_on_close,
			    int defragment_on_close,
                            long tile_width, long tile_height,
			    int intrinsic_write) {
  sif_file *retval = 0;
  sif_header *hd = 0;
  long i = 0, tb = tile_width * tile_height * bands * data_unit_size;

  /** Check for basic sanity of the arguments. */
  if (bands < 1 || width < 1 || height < 1 || tile_width < 1 || tile_height < 1 || data_unit_size < 1
      || filename == 0) {
    return 0;
  }
#ifdef WIN32
  HANDLE fp = 0;
  fp = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
  FILE *fp = 0;
  fp = fopen64(filename, "wb+");
#endif
  if (!FILE_IS_OKAY(fp)) {
    return 0;
  }
  if ((retval = _sif_alloc_fp()) == 0
      || (hd = _sif_alloc_header()) == 0
      || (retval->buffer[0] = malloc(tb)) == 0
      || (retval->buffer[1] = malloc(tb)) == 0) {
    free(hd);
    free(retval->buffer[0]);
    free(retval);
    return 0;
  }
  retval->fp = fp;
  retval->header = hd;
  retval->read_only = 0;
  retval->simple_region_buffer = 0;
  retval->simple_region_bytes = 0;
  hd->consolidate = consolidate_on_close;
  hd->intrinsic_write = intrinsic_write;
  hd->defragment = defragment_on_close;
  hd->intrinsic_write = 1;
  hd->width = width;
  hd->height = height;
  hd->bands = bands;
  hd->tile_width = tile_width;
  hd->tile_height = tile_height;
  hd->version = SIF_VERSION;
  retval->units_per_tile = tile_width * tile_height * bands;
  retval->units_per_slice = tile_width * tile_height;
  hd->tile_bytes = tile_width * tile_height * bands * data_unit_size;
  hd->data_unit_size = data_unit_size;
  hd->user_data_type = user_data_type;
  hd->n_tiles_across = CEIL_DIV(hd->width, hd->tile_width);
  hd->n_tiles = hd->n_tiles_across * CEIL_DIV(hd->height, hd->tile_height);
  hd->n_keys = 0;
  retval->meta_data = _sif_alloc_meta_data_table();
  if ((retval->tiles = _sif_alloc_tile_headers(retval)) == 0 ||
      (retval->blocks_to_tiles = (long*)malloc(hd->n_tiles * sizeof(long))) == 0 ||
      (retval->dirty_tiles = (long*)malloc(hd->n_tiles * sizeof(long))) == 0) {
    _sif_free_tile_headers(retval);
    free(hd);
    free(retval->meta_data);
    free(retval->blocks_to_tiles);
    free(retval->dirty_tiles);
    free(retval->buffer[0]);
    free(retval->buffer[1]);
    free(retval);
    return 0;
  }
  bzero(retval->dirty_tiles, hd->n_tiles * sizeof(long));
  for (i = 0; i < hd->n_tiles; i++) {
    retval->blocks_to_tiles[i] = -1;
  }
  memcpy(&(hd->magic_number), SIF_MAGIC_NUMBER, SIF_MAGIC_NUMBER_SIZE);
  _sif_write_header(retval);
  retval->base_location = retval->header_bytes + (hd->tile_header_bytes * hd->n_tiles);
  if (retval->error != 0) {
    _sif_free_tile_headers(retval);
    free(hd);
    free(retval->blocks_to_tiles);
    free(retval->dirty_tiles);
    free(retval->buffer[0]);
    free(retval->buffer[1]);
    _sif_truncate(retval, 0);
    FCLOSE64(fp);
    free(retval);
    return 0;
  }
  _sif_write_tile_headers(retval);
  if (retval->error != 0) {
    _sif_free_tile_headers(retval);
    free(hd);
    free(retval->blocks_to_tiles);
    free(retval->dirty_tiles);
    free(retval->buffer[0]);
    free(retval->buffer[1]);
    _sif_truncate(retval, 0);
    FCLOSE64(fp);
    free(retval);
  }
  sif_use_file_format_version(retval, sif_get_version());
  return retval;
}

/* See sif-io.h for detailed documentation of public functions. */
sif_file         *sif_create_copy(sif_file *file, const char *filename) {
  sif_header *hd = file->header;
  long i;
  /**  retval = sif_create(filename, hd->width, hd->height, hd->bands, hd->data_unit_size,
		      hd->user_data_type, hd->consolidate, hd->defragment,
		      hd->tile_width, hd->tile_height);**/
#ifdef WIN32
  HANDLE fp = 0;
  LARGE_INTEGER lsz;
  LONGLONG j, k, bufsize, sz;
  fp = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!FILE_IS_OKAY(fp)) {
    return 0;
  }
  sif_flush(file);
  GetFileSizeEx(file->fp, &lsz);
  sz = lsz.QuadPart;
  REWIND64NEC(file->fp);
  bufsize = file->header->data_unit_size * file->units_per_tile;
  for (j = 0; j < sz; j += bufsize) {
    k = MIN(sz - j, bufsize);
    if (FREAD64NEC(file->buffer[0], 1, k, file->fp) == 0 ||
	FWRITE64NEC(file->buffer[0], 1, k, fp) == 0) {
      FCLOSE64(fp);
      return 0;
    }
  }
#else
  FILE *fp = 0;
  sif_flush(file);
  REWIND64NEC(file->fp);
  fp = fopen64(filename, "wb+");
  if (!FILE_IS_OKAY(fp)) {
    return 0;
  }
  while (!feof(file->fp)) {
    i = FREAD64NEC(file->buffer[0], 1, hd->data_unit_size * file->units_per_tile, file->fp);
    if (ferror(file->fp)
	|| FWRITE64NEC(file->buffer[0], 1, i, fp) != i) {
      fclose(fp);
      return 0;
    }
  }
#endif
  FCLOSE64(fp);
  return sif_open(filename, 0);
}

/* See sif-io.h for detailed documentation of public functions. */
void sif_set_user_data_type(sif_file *file, long user_data_type) {
  SIF_CHECK_FILE_V(file);
  file->header->user_data_type = user_data_type;
}

/* See sif-io.h for detailed documentation of public functions. */
long sif_get_user_data_type(sif_file *file) {
  SIF_CHECK_FILE(file);
  return file->header->user_data_type;
}

/* See sif-io.h for detailed documentation of public functions. */
void sif_set_intrinsic_write(sif_file *file) {
  SIF_CHECK_FILE_V(file);
  file->header->intrinsic_write = 1;
}

/* See sif-io.h for detailed documentation of public functions. */
int sif_is_intrinsic_write_set(sif_file *file) {
  SIF_CHECK_FILE(file);
  return file->header->intrinsic_write;
}


/* See sif-io.h for detailed documentation of public functions. */
void sif_unset_intrinsic_write(sif_file *file) {
  SIF_CHECK_FILE_V(file);
  file->header->intrinsic_write = 0;
}


/* See sif-io.h for detailed documentation of public functions. */
void sif_set_defragment(sif_file *file) {
  SIF_CHECK_FILE_V(file);
  file->header->defragment = 1;
}


/* See sif-io.h for detailed documentation of public functions. */
int sif_is_defragment_set(sif_file *file) {
  SIF_CHECK_FILE(file);
  return file->header->defragment;
}

/* See sif-io.h for detailed documentation of public functions. */
void sif_unset_defragment(sif_file *file) {
  SIF_CHECK_FILE_V(file);
  file->header->defragment = 0;
}


/* See sif-io.h for detailed documentation of public functions. */
void sif_set_consolidate(sif_file *file) {
  SIF_CHECK_FILE_V(file);
  file->header->consolidate = 1;
}

/* See sif-io.h for detailed documentation of public functions. */
int sif_is_consolidate_set(sif_file *file) {
  SIF_CHECK_FILE(file);
  return file->header->consolidate;
}

/* See sif-io.h for detailed documentation of public functions. */
void sif_unset_consolidate(sif_file *file) {
  SIF_CHECK_FILE_V(file);
  file->header->consolidate = 0;
}

/* See sif-io.h for detailed documentation of public functions. */
void sif_set_affine_geo_transform(sif_file *file, const double *trans) {
  int i;
  SIF_CHECK_FILE_V(file);
  for (i = 0; i < 6; i++) {
    file->header->affine_geo_transform[i] = trans[i];
  }
}

/* See sif-io.h for detailed documentation of public functions. */
const double *sif_get_affine_geo_transform(sif_file *file) {
  SIF_CHECK_FILE(file);
  return file->header->affine_geo_transform;
}

/* See sif-io.h for detailed documentation of public functions. */
const char *sif_get_projection(sif_file *file) {
  const char *result;
  SIF_CHECK_FILE(file);
  result = sif_get_meta_data(file, "_sif_proj");
  if (result == 0 && file->error == SIF_ERROR_META_DATA_KEY) {
    result = "";
    file->error = SIF_ERROR_NONE;
  }
  return result;
}

/* See sif-io.h for detailed documentation of public functions. */
void sif_set_projection(sif_file *file, const char *proj) {
  SIF_CHECK_FILE_V(file);
  sif_set_meta_data(file, "_sif_proj", proj);
}

/* See sif-io.h for detailed documentation of public functions. */
const char *sif_get_agreement(sif_file *file) {
  const char *result;
  SIF_CHECK_FILE(file);
  result = sif_get_meta_data(file, "_sif_agree");
  if (result == 0 && file->error == SIF_ERROR_META_DATA_KEY) {
    result = "";
    file->error = SIF_ERROR_NONE;
  }
  return result;
}

/* See sif-io.h for detailed documentation of public functions. */
void sif_set_agreement(sif_file *file, const char *proj) {
  SIF_CHECK_FILE_V(file);
  sif_set_meta_data(file, "_sif_agree", proj);
}

/* See sif-io.h for detailed documentation of public functions. */
int               sif_get_meta_data_num_items(sif_file *file) {
  return file->header->n_keys;
}

/* See sif-io.h for detailed documentation of public functions. */
void              sif_get_meta_data_keys(sif_file *file,
					 const char *** key_strs,
					 int *num_keys) {
  int n, i, j;
  sif_meta_data *cur;
  SIF_CHECK_FILE_V(file);
  n = sif_get_meta_data_num_items(file);
  *key_strs = (const char**)malloc(sizeof(char*) * (n + 1));
  if (*key_strs == 0) {
    file->error = SIF_ERROR_MEM;
  }
  for (j = 0, i = 0; j < SIF_HASH_TABLE_SIZE; j++) {
    for (cur = file->meta_data[j]; cur != 0; cur = cur->next) {
      (*key_strs)[i] = cur->key;
      i++;
    }
  }
  (*key_strs)[n] = 0;
  if (num_keys != 0) {
    *num_keys = n;
  }
}

/* See sif-io.h for detailed documentation of public functions. */
void              sif_remove_meta_data_item(sif_file *file, const char *key) {
  sif_meta_data *item;
  SIF_CHECK_FILE_V(file);
  item = _sif_unlink_meta_data_pair(file, key);
  if (item) {
    free(item->value);
    free(item->key);
    free(item);
  }
}

/* See sif-io.h for detailed documentation of public functions. */
void              sif_use_file_format_version(sif_file *file, long version) {
  if (version < 1) {
    file->error = SIF_ERROR_CANNOT_WRITE_VERSION;
    return;
  }
  else {
    file->use_file_version = version;
  }
}

/* See sif-io.h for detailed documentation of public functions. */
int               sif_is_possibly_sif_file(const char *filename) {
#ifdef WIN32
  HANDLE fp = 0;
#else
  FILE *fp = 0;
#endif
  sif_header oheader;
  sif_file ofile;
  sif_header *header;
  sif_file *file;
  int retval = 0;
  /** We're not going to return any structures to the user, so we can use the stack for the header. **/
  header = &oheader;
  file = &ofile;
#ifdef WIN32
  fp = CreateFile(TEXT(filename), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
#else
  fp = fopen64(filename, "rb");
#endif
  if (FILE_IS_OKAY(fp)) {
    file->fp = fp;
    file->error = 0;
    file->tiles = 0;
    
    file->meta_data = 0;
    file->read_only = 1;
    REWIND64NEC(fp);
    if (_sif_read_header(file) != 1 ||
	strncmp(header->magic_number, SIF_MAGIC_NUMBER, SIF_MAGIC_NUMBER_SIZE) != 0 ||
	(file->tiles = _sif_alloc_tile_headers(file)) == 0) {
      retval = 0;
    }
    else {
      retval = 1;
    }
    FCLOSE64(fp);
  }
  else {
    retval = -1;
  }
  return retval;
}

/* See sif-io.h for detailed documentation of public functions. */
const char *      sif_get_error_description(int code) {
  const char *str;
  switch(code) {
  case SIF_ERROR_NONE:
    str = "No error";
    break;
  case SIF_ERROR_MEM:
    str = "Memory error";
    break;
  case SIF_ERROR_NULL_FP:
    str = "Null file pointer";
    break;
  case SIF_ERROR_NULL_HDR:
    str = "Null header";
    break;
  case SIF_ERROR_INVALID_BN:
    str = "Invalid block number";
    break;
  case SIF_ERROR_INVALID_TN:
    str = "Invalid tile number";
    break;
  case SIF_ERROR_READ:
    str = "Error when reading";
    break;
  case SIF_ERROR_WRITE:
    str = "Error when writing";
    break;
  case SIF_ERROR_SEEK:
    str = "Error when seeking";
    break;
  case SIF_ERROR_TRUNCATE:
    str = "Error when truncating";
    break;
  case SIF_ERROR_INVALID_FILE_MODE:
    str = "Invalid file mode";
    break;
  case SIF_ERROR_INCOMPATIBLE_VERSION:
    str = "Cannot files of the version stored in the SIF file";
    break;
  case SIF_ERROR_META_DATA_KEY:
    str = "Cannot find a (key,value) pair with the specified key";
    break;
  case SIF_ERROR_META_DATA_VALUE:
    str = "The value of the meta-data item is invalid.";
    break;
  case SIF_ERROR_CANNOT_WRITE_VERSION:
    str = "Cannnot write files of the version requested.";
    break;
  case SIF_ERROR_INVALID_BAND:
    str = "Band index invalid (e.g. band argument).";
    break;
  case SIF_ERROR_INVALID_COORD:
    str = "Invalid coordinate (e.g. x or y).";
    break;
  case SIF_ERROR_INVALID_TILE_SIZE:
    str = "Invalid tile size (e.g. tile_width or tile_height).";
    break;
  case SIF_ERROR_INVALID_REGION_SIZE:
    str = "Invalid region size (e.g. width or height).";
    break;
  case SIF_ERROR_INVALID_BUFFER:
    str = "Invalid buffer passed (NULL?).";
    break;
  case SIF_ERROR_PNM_INCOMPATIBLE_TYPE_CODE:
    str = "Invalid type code for PNM output.";
    break;
  case SIF_ERROR_PGM_INVALID_BAND_COUNT:
    str = "Invalid band count for PGM output.";
    break;
  case SIF_ERROR_PPM_INVALID_BAND_COUNT:
    str = "Invalid band count for PPM output.";
    break;
  case SIF_ERROR_PNM_INCOMPATIBLE_DT_CONVENTION:
    str = "PNM output requires the 'simple' data type convention.";
    break;
  case SIF_SIMPLE_ERROR_UNDEFINED_DT:
    str = "Undefined data type code (simple).";
    break;
  case SIF_SIMPLE_ERROR_INCORRECT_DT:
    str = "Data type mismatch (simple).";
    break;
  case SIF_SIMPLE_ERROR_UNDEFINED_ENDIAN:
    str = "Endian code not understood (simple).";
    break;
  default:
    str = "Unknown error.";
    break;
  }
  return str;
}

void               sif_simple_set_endian(sif_file *file, int endian) {
  int simple_data_type;
  SIF_ERROR_CHECK_RETURN_V(endian < 0 || endian > 1, SIF_SIMPLE_ERROR_UNDEFINED_ENDIAN);
  SIF_CHECK_FILE_V(file);
  simple_data_type = SIF_SIMPLE_BASE_TYPE_CODE(file->header->user_data_type);
  file->header->user_data_type = simple_data_type + 10 * endian;
}

int               sif_simple_get_endian(sif_file *file) {
  SIF_CHECK_FILE(file);
  return SIF_SIMPLE_ENDIAN(file->header->user_data_type);
}

void               sif_simple_set_data_type(sif_file *file, int data_type_code) {
  int simple_endian;
  SIF_ERROR_CHECK_RETURN_V(data_type_code < 0 || data_type_code > 9, SIF_SIMPLE_ERROR_UNDEFINED_DT);
  SIF_CHECK_FILE_V(file);
  simple_endian = SIF_SIMPLE_ENDIAN(file->header->user_data_type);
  file->header->user_data_type = data_type_code + 10 * simple_endian;
}

int               sif_simple_get_data_type(sif_file *file) {
  SIF_CHECK_FILE(file);
  return SIF_SIMPLE_BASE_TYPE_CODE(file->header->user_data_type);
}

static const long _sif_simple_data_type_sizes_bits [] = { 8, 8, 16, 16, 32, 32, 64, 64, 32, 64};
static const long _sif_simple_data_type_sizes_bytes [] = { 1, 1,  2,  2,  4,  4,  8,  8,  4,  8};

int              _sif_simple_alloc_region_buffer(sif_file *file, long nbytes) {
  /** If the number of bytes requested for allocation is 0 or the
      native endian is the same as the byte order of the image rasters,
      do not allocate, return successful. */
  if (nbytes == 0 || sif_simple_get_endian(file) == SIF_SIMPLE_NATIVE_ENDIAN
      || nbytes <= file->simple_region_bytes) {
    return 1;
  }
  /** If the buffer is already allocated but not big enough, try expanding its size. */
  if (file->simple_region_buffer) {
    file->simple_region_buffer = (unsigned char*)realloc(file->simple_region_buffer, nbytes);
    file->simple_region_bytes = nbytes;
  }
  /** Otherwise, allocate a new buffer. */
  else {
    file->simple_region_buffer = (unsigned char*)malloc(nbytes);
    file->simple_region_bytes = nbytes;
  }
  /** Return successful if the buffer was allocated, unsuccessful otherwise.*/
  if (file->simple_region_buffer) {
    return 1;
  }
  else {
    file->simple_region_buffer = 0;
    file->simple_region_bytes = 0;
    return 0;
  }
}

SIF_EXPORT sif_file*        sif_simple_create(const char *filename,
					      long width, long height,
					      long bands,
					      int simple_data_type,
					      int consolidate_on_close,
					      int defragment_on_close,
					      long tile_width, long tile_height,
					      int intrinsic_write) {
  int user_data_type, data_unit_size;
  sif_file *retval;
  if (simple_data_type < 0 || simple_data_type > 9) {
    return 0;
  }

  user_data_type = SIF_SIMPLE_NATIVE_ENDIAN * 10 + simple_data_type;
  data_unit_size = _sif_simple_data_type_sizes_bytes[simple_data_type];

  /** Now create the file. */
  retval = sif_create(filename, width, height, bands, data_unit_size,
		      user_data_type, consolidate_on_close,
		      defragment_on_close, tile_width, tile_height, intrinsic_write);

  /** If successful, set the file's region buffer and bytes fields. **/
  if (retval != 0) {
    sif_set_agreement(retval, "simple");
  }
  return retval;
}

sif_file*                 sif_simple_create_defaults(const char *filename, long width, long height,
						     long bands, int simple_data_type) {
  return sif_simple_create(filename, width, height, bands, simple_data_type, 1, 1, 64, 64, 1);
}

void                     sif_simple_set_raster(sif_file* file, const void *data,
					       long x, long y, long w, long h,
					       long band) {
  long region_bytes;
  int file_endian;
  SIF_CHECK_FILE_V(file);
  if (file->error) { return; }
  region_bytes = file->header->data_unit_size * w * h;
  file_endian = sif_simple_get_endian(file);
  /** If the byte order of the data elements in the file is not the same as the
      byte order of the current architecture, we need to do some byte swapping. */
  if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
    /** See if the file's block buffer is big enough for the raster. If not, reallocate
	and anticipate more of the same big writes so keep the buffer at its increased
	size.*/
    SIF_ERROR_CHECK_RETURN_V(_sif_simple_alloc_region_buffer(file, region_bytes) == 0, SIF_ERROR_MEM);
    memcpy(file->simple_region_buffer, data, region_bytes);
    _sif_buffer_host_to_code(file->simple_region_buffer, region_bytes,
			     file->header->data_unit_size, file_endian);
    sif_set_raster(file, file->simple_region_buffer, x, y, w, h, band);
  }
  /** Otherwise, our task is much easier, just write the bytes to the file in native order. */
  else {
    sif_set_raster(file, data, x, y, w, h, band);
  }
}

void              sif_simple_get_raster(sif_file* file, void *data, long x, long y,
						   long w, long h, long band) {
  long region_bytes;
  int file_endian;
  SIF_CHECK_FILE_V(file);
  file_endian = sif_simple_get_endian(file);
  region_bytes = file->header->data_unit_size * w * h;
  /** Get the raster from the file, being ignorant about the byte order. */
  sif_get_raster(file, data, x, y, w, h, band);
  /** If an error occured during the read, just return. */
  if (file->error) { return; }
  /** Do a byte swap if the data elements stored in the file are in a different byte
      order than the native byte order. */
  if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
    _sif_buffer_code_to_host(file->simple_region_buffer, region_bytes,
			     file->header->data_unit_size, SIF_SIMPLE_NATIVE_ENDIAN);
  }
}

void              sif_simple_fill_tiles(sif_file *file, long band, const void *value) {
  int file_endian;
  char v[8]; /** A char array with size=maximum size of any simple data type. */
  SIF_CHECK_FILE_V(file);
  if (file->read_only) { return; }
  file_endian = sif_simple_get_endian(file);
  if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
    memcpy(&v, value, file->header->data_unit_size);
    _sif_buffer_host_to_code((unsigned char *)&v, file->header->data_unit_size,
			     file->header->data_unit_size, file_endian);
    sif_fill_tiles(file, band, &v);
  }
}

void              sif_simple_get_tile_slice(sif_file *file, void *buffer, long tx, long ty,
						       long band) {
  long region_bytes;
  int file_endian;
  SIF_CHECK_FILE_V(file);
  file_endian = sif_simple_get_endian(file);
  region_bytes = file->header->tile_bytes / file->header->bands;
  /** Get the raster from the file, being ignorant about the byte order. */
  sif_get_tile_slice(file, buffer, tx, ty, band);
  /** If an error occured during the read, just return. */
  if (file->error) { return; }
  /** Do a byte swap if the data elements stored in the file are in a different byte
      order than the native byte order. */
  if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
    _sif_buffer_code_to_host((unsigned char *)file->simple_region_buffer, region_bytes,
			     file->header->data_unit_size, SIF_SIMPLE_NATIVE_ENDIAN);
  }
}

void              sif_simple_set_tile_slice(sif_file *file, const void *buffer,
						       long tx, long ty, long band) {
  long region_bytes;
  int file_endian;
  SIF_CHECK_FILE_V(file);
  if (file->read_only) { return; }
  region_bytes = file->header->tile_bytes / file->header->bands;
  file_endian = sif_simple_get_endian(file);
  /** If the byte order of the data elements in the file is not the same as the
      byte order of the current architecture, we need to do some byte swapping. */
  if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
    /** See if the file's block buffer is big enough for the raster. If not, reallocate
	and anticipate more of the same big writes so keep the buffer at its increased
	size.*/
    SIF_ERROR_CHECK_RETURN_V(_sif_simple_alloc_region_buffer(file, region_bytes) == 0, SIF_ERROR_MEM);
    memcpy(file->simple_region_buffer, buffer, region_bytes);
    _sif_buffer_host_to_code((unsigned char *)file->simple_region_buffer, region_bytes,
			     file->header->data_unit_size, file_endian);
    sif_set_tile_slice(file, file->simple_region_buffer, tx, ty, band);
  }
  /** Otherwise, our task is much easier, just write the bytes to the file in native order. */
  else {
    sif_set_tile_slice(file, buffer, tx, ty, band);
  }  
}

void              sif_simple_fill_tile_slice(sif_file *file, long tx, long ty,
							long band, const void *value) {
  int file_endian;
  char v[8];
  SIF_CHECK_FILE_V(file);
  if (file->read_only) {
    return;
  }
  file_endian = sif_simple_get_endian(file);
  if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
    memcpy(&v, value, file->header->data_unit_size);
    _sif_buffer_host_to_code((unsigned char *)&v, file->header->data_unit_size,
			     file->header->data_unit_size, file_endian);
    sif_fill_tile_slice(file, tx, ty, band, &v);
  }
}

sif_file*        sif_simple_open(const char* filename, int read_only) {
  sif_file *retval;
  long simple_region_bytes;
  retval = sif_open(filename, read_only);
  if (retval) {
    if (strcmp("simple", sif_get_agreement(retval)) != 0) {
      sif_close(retval);
      retval = 0;
    }
  }
  return retval;
}

int              sif_simple_is_shallow_uniform(sif_file *file, long x, long y, long w, long h, long band, void *uniform_value) {
  int retcode;
  int file_endian;
  char v[8];
  retcode = sif_is_shallow_uniform(file, x, y, w, h, band, uniform_value);
  if (retcode != 0) {
    file_endian = sif_simple_get_endian(file);
    if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
      _sif_buffer_code_to_host((unsigned char*)uniform_value, file->header->data_unit_size,
			       file->header->data_unit_size, file_endian);
    }
  }
  return retcode;
}

int              sif_simple_is_slice_shallow_uniform(sif_file *file, long tx, long ty, long band, void *uniform_value) {
  int retcode;
  int file_endian;
  char v[8];
  retcode = sif_is_slice_shallow_uniform(file, tx, ty, band, uniform_value);
  if (retcode != 0) {
    file_endian = sif_simple_get_endian(file);
    if (file_endian != SIF_SIMPLE_NATIVE_ENDIAN) {
      _sif_buffer_code_to_host((unsigned char*)uniform_value, file->header->data_unit_size,
			       file->header->data_unit_size, file_endian);
    }
  }
  return retcode;
}

int              sif_is_simple(sif_file *file) {
  int retval = 0;
  const char *agree;
  if (file) {
    agree = sif_get_agreement(file);
    if (agree) {
      if (strcmp(agree, "simple") == 0) {
	retval = 1;
      }
    }
  }
  return retval;
}

int              sif_is_simple_by_name(const char *filename) {
  int retval = 0;
  int retval2 = 0;
  const char *agree;
  sif_file *file;
  if (filename) {
    retval2 = sif_is_possibly_sif_file(filename);
    if (retval2 > 0) {
      /** Open the file. */
      file = sif_open(filename, 1);
      /** It may be the case that the file is no longer openable, for some odd reason.
	  Let's check. */
      if (file) {
	retval2 = sif_is_simple_file(file);
	/** If the file is a simple file, set the return value accordingly. */
	if (retval2 == 0) {
	  retval = -2; /** -2: file is a SIF file but does not conform to the simple data type convention. */
	}
	else {
	  retval = 1;  /** 1: file could be opened, is a SIF file, & conforms to the "simple" convention. */
	}
	sif_close(file);
      }
      else {
	retval = -1; /** -1: file could not be opened. */
      }
    }
    else {
      retval = retval2; /**  0: file exists and openable but not SIF file
			     -1: file could not be opened. */
    }
  }
  return retval;
}

typedef struct {
#ifdef WIN32
  HANDLE fp;
#else
  FILE *fp;
#endif
} _sif_file_ptr;

int           _sif_create_blank_file(const char *filename, _sif_file_ptr *ptr) {
#ifdef WIN32
  HANDLE fp = 0;
#else
  FILE *fp = 0;
#endif
  ptr->fp = 0;

#ifdef WIN32
  fp = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
  fp = fopen64(filename, "wb+");
#endif
  if (!FILE_IS_OKAY(fp)) {
    return 0;
  }
  ftr->fp = fp;
  return 1;
}

int          sif_export_to_pgm_file(sif_file *file, const char *filename) {
  _sif_file_ptr ptr;
  int user_data_type;
  if (file == 0) {
    return 0;
  }
  if (sif_is_simple(file) < 1) {
    file->error = SIF_ERROR_PNM_INCOMPATIBLE_DT_CONVENTION;
    return 0;
  }
  if (file->header->n_bands != 1) {
    file->error = SIF_ERROR_PGM_INVALID_BAND_COUNT;
    return 0;
  }
  if (!(user_data_type == 0 || user_data_type == 2)) {
    file->error = SIF_ERROR_PNM_INCOMPATIBLE_TYPE_CODE;
    return 0;
  }
  if (_sif_create_blank_file(filename, &ptr)) {

  }
}
