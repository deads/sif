/**
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

#ifndef SIFExport_h_
#define SIFExport_h_
#if defined(_MSC_VER)
#if defined(SIF_DLL)
#define SIF_EXPORT __declspec(dllexport)
#else
#define SIF_EXPORT __declspec(dllimport)
#endif
#pragma warning(disable : 4503)
#else
#define SIF_EXPORT
#endif
#endif
