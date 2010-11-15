/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007-2008 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <proto/graphics.h>
#include <graphics/gfxbase.h>

#include "Chunky2Bitmap.h"

#include "private.h"

struct BitMap *Chunky2Bitmap(APTR chunky, ULONG width, ULONG height, ULONG depth)
{
  struct BitMap *bm = NULL;

  ENTER();

  if(chunky != NULL && width > 0 && height > 0)
  {
    if((bm = AllocBitMap(width, height, min(8, depth), BMF_CLEAR|BMF_MINPLANES, NULL)) != NULL)
    {
      struct BitMap *tempBM;

      if((tempBM = AllocBitMap(width, 1, min(8, depth), BMF_CLEAR, NULL)) != NULL)
      {
        struct RastPort remapRP;
        struct RastPort tempRP;
        ULONG y;
        char *chunkyPtr = chunky;
        #if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
        // WritePixelLine8() is broken on plain OS3.1 systems, don't use it!
        BOOL useWPL8 = (GfxBase->LibNode.lib_Version > 40);
        #endif // !__amigaos4 && !__MORPHOS__ && !__AROS__

        InitRastPort(&remapRP);
        remapRP.BitMap = bm;

        InitRastPort(&tempRP);
        tempRP.BitMap = tempBM;

        for(y = 0; y < height; y++)
        {
          #if defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
          WritePixelLine8(&remapRP, 0, y, width, chunkyPtr, &tempRP);
          #else // __amigaos4__ || __MORPHOS__ || __AROS__
          if(useWPL8 == TRUE)
          {
            WritePixelLine8(&remapRP, 0, y, width, chunkyPtr, &tempRP);
          }
          else
          {
            ULONG x;

            for(x = 0; x < width; x++)
            {
              SetAPen(&remapRP, chunkyPtr[x]);
              WritePixel(&remapRP, x, y);
            }
          }
          #endif // __amigaos4__ || __MORPHOS__ || __AROS__

          chunkyPtr += width;
        }

        FreeBitMap(tempBM);
      }
      else
      {
        FreeBitMap(bm);
        bm = NULL;
      }
    }
  }

  RETURN(bm);
  return bm;
}
