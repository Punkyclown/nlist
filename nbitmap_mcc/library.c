/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id: library.c 125 2006-12-17 00:15:23Z damato $

***************************************************************************/

/* system includes */
#include <proto/exec.h>
#include <proto/intuition.h>

/* local includes */
#include "Debug.h"
#include "private.h"
#include "rev.h"

/* mcc initialisation */
#define VERSION       LIB_VERSION
#define REVISION      LIB_REVISION

#define CLASS         MUIC_NBitmap
#define SUPERCLASS    MUIC_Area

#define INSTDATA      InstData

#define UserLibID     "$VER: " MUIC_NBitmap " " LIB_REV_STRING CPU " (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define USEDCLASSESP used_classesP
static const char *used_classesP[] = { "NBitmap.mcp", NULL };

#define ClassInit
#define ClassExit

#define USE_UTILITYBASE

// libraries
struct Library *DataTypesBase = NULL;
struct Library *P96Base = NULL;

struct DataTypesIFace *IDataTypes = NULL;
struct P96IFace *IP96 = NULL;

// BOOL ClassInitFunc()
BOOL ClassInitFunc(UNUSED struct Library *base)
{
  BOOL res = FALSE;

  ENTER();

  // open library interfaces
  if((DataTypesBase = OpenLibrary("datatypes.library", 39L)) != NULL &&
     GETINTERFACE(IDataTypes, DataTypesBase))
  {

    // Picasso96API.library is not necessary but
    // highly recommened
    if((P96Base = OpenLibrary("Picasso96API.library", 2L)) != NULL &&
       GETINTERFACE(IP96, P96Base))
    {
    }

    res = TRUE;
  }

  RETURN(res);
  return res;
}

/* VOID ClassExitFunc() */
VOID ClassExitFunc(UNUSED struct Library *base)
{
  ENTER();

  /* close libraries */
  if(P96Base != NULL)
  {
    DROPINTERFACE(IP96);
    CloseLibrary(P96Base);
    P96Base = NULL;
  }

  if(DataTypesBase != NULL)
  {
    DROPINTERFACE(IDataTypes);
    CloseLibrary(DataTypesBase);
    DataTypesBase = NULL;
  }

  LEAVE();
}

/* library startup code */
#include "mccheader.c"

