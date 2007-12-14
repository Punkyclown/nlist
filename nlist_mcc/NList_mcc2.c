/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2005 by NList Open Source Team

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

#include <stdlib.h>

#include <clib/alib_protos.h>
#include <proto/muimaster.h>

#include "private.h"

#if !defined(__amigaos4__)
#include "newmouse.h"
#endif

#include "NList_func.h"
#include "NListviews_mcp.h"

#define MAX_INTUITICKS_WAIT 3

#define MinColWidth 2

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright(obj)) && _between(_mtop(obj),(y),_mbottom(obj)))
#define _isinobject2(x,y) (_between(_left(obj),(x),_right(obj)) && _between(_top(obj),(y),_bottom(obj)))

static LONG NL_TestKey(UNUSED Object *obj,struct NLData *data,LONG KeyTag,UWORD Code,UWORD Qualifier, BOOL force)
{
  UWORD qual;
  LONG i;
  Qualifier &= KBQUAL_MASK;

  if (data->NList_Keys && (!(Code & 0x80) || force))
  {
    for (i = 0 ; data->NList_Keys[i].kb_KeyTag > 0 ; i++)
    {
      if ((data->NList_Keys[i].kb_KeyTag == (ULONG)KeyTag) &&
          ((Code == data->NList_Keys[i].kb_Code) ||
           ((data->NList_Keys[i].kb_Code == (UWORD)~0) && data->NList_Keys[i].kb_Qualifier)))
      {
        qual = data->NList_Keys[i].kb_Qualifier;
        if (Qualifier == qual)
          return (TRUE);
        if (!Qualifier || !(qual & KBSYM_MASK))
          continue;

        if ((qual & KBSYM_SHIFT) && !(Qualifier & KBQUALIFIER_SHIFT))
          continue;
        else
          qual = qual & ~KBQUALIFIER_SHIFT;

        if ((qual & KBSYM_CAPS) && !(Qualifier & KBQUALIFIER_CAPS))
          continue;
        else
          qual = qual & ~KBQUALIFIER_CAPS;

        if ((qual & KBSYM_ALT) && !(Qualifier & KBQUALIFIER_ALT))
          continue;
        else
          qual = qual & ~KBQUALIFIER_ALT;

        if ((Qualifier & qual) == qual)
          return (TRUE);
      }
    }
  }
  return (FALSE);
}


static void NL_RequestIDCMP(Object *obj,struct NLData *data,LONG IDCMP_val)
{
  if (IDCMP_val & IDCMP_MOUSEMOVE)
    data->MOUSE_MOVE = TRUE;
  if (!(data->ehnode.ehn_Events & IDCMP_val))
  {
    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehnode);
    data->ehnode.ehn_Events |= IDCMP_val;
    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehnode);
  }
}


static void NL_RejectIDCMP(Object *obj,struct NLData *data,LONG IDCMP_val,BOOL really)
{

  if (IDCMP_val & IDCMP_MOUSEMOVE)
    data->MOUSE_MOVE = FALSE;
  if ((really || !(IDCMP_val & IDCMP_MOUSEMOVE)) &&
      (data->ehnode.ehn_Events & IDCMP_val))
  {
    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehnode);
    data->ehnode.ehn_Events &= (~IDCMP_val);
    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehnode);
  }
}


ULONG mNL_HandleEvent(struct IClass *cl,Object *obj,struct MUIP_HandleInput *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  ULONG retval = 0;
  ULONG NotNotify = data->DoNotify;
  LONG tempbutton;
  LONG tempbuttonline;
  LONG tempbuttoncol;
  LONG tempbuttonstate;
  LONG tempstorebutton;
/*
 *if (msg->imsg && (data->markdrawnum == MUIM_NList_Trigger))
 *{
 *LONG ab = (LONG) data->adjustbar;
 *LONG co = (LONG) msg->imsg->Code;
 *LONG qu = (LONG) msg->imsg->Qualifier;
 *LONG mx = (LONG) msg->imsg->MouseX;
 *LONG my = (LONG) msg->imsg->MouseY;
 *kprintf("%lx|1ab %ld stop IntuiMessage :\n",obj,ab);
 *if (msg->imsg->Class == IDCMP_INACTIVEWINDOW)
 *D(bug("  Class=INACTIVEWINDOW Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_ACTIVEWINDOW)
 *D(bug("  Class=ACTIVEWINDOW Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_MOUSEMOVE)
 *D(bug("  Class=MOUSEMOVE Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_INTUITICKS)
 *D(bug("  Class=INTUITICKS Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_MOUSEBUTTONS)
 *D(bug("  Class=MOUSEBUTTONS Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else
 *D(bug("  Class=%lx Code=%lx Qualifier=%lx mx=%ld my=%ld\n",msg->imsg->Class,co,qu,mx,my));
 *}
*/

  if (!data->SHOW || !data->DRAW)
    return (0);
  if (data->NList_First_Incr || data->NList_AffFirst_Incr)
    return (MUI_EventHandlerRC_Eat);
  if (data->NList_Disabled)
    return (0);

  if ((data->left != _left(obj)) || (data->top != _top(obj)) ||
      (data->width != _width(obj)) || (data->height != _height(obj)))
    NL_SetObjInfos(obj,data,FALSE);

  if ((msg->muikey != MUIKEY_NONE) && !data->NList_Quiet && !data->NList_Disabled)
  {
    data->ScrollBarsTime = SCROLLBARSTIME;

    SHOWVALUE(DBF_ALWAYS, msg->muikey);
    switch (msg->muikey)
    {
      case MUIKEY_UP:
      {
        BOOL changed;

        if(data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
          changed = NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->NList_List_Select,FALSE);
        else
          changed = NL_List_First(obj,data,MUIV_NList_First_Up,NULL);

        // if we have an object that we should make the new active object
        // of the window we do so in case this up key action didn't end up in
        // a real scrolling
        if(changed == FALSE && data->NList_KeyUpFocus != NULL)
          set(_win(obj), MUIA_Window_ActiveObject, data->NList_KeyUpFocus);

        retval = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_DOWN:
      {
        BOOL changed;

        if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
          changed = NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->NList_List_Select,FALSE);
        else
          changed = NL_List_First(obj,data,MUIV_NList_First_Down,NULL);

        // if we have an object that we should make the new active object
        // of the window we do so in case this down key action didn't end up in
        // a real scrolling
        if(changed == FALSE && data->NList_KeyDownFocus != NULL)
          set(_win(obj), MUIA_Window_ActiveObject, data->NList_KeyDownFocus);

        retval = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_PAGEUP   :
        if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
        {
          NL_List_Active(obj,data,MUIV_NList_Active_PageUp,NULL,data->NList_List_Select,FALSE);
          retval = MUI_EventHandlerRC_Eat;
        }
        else
        {
          NL_List_First(obj,data,MUIV_NList_First_PageUp,NULL);
          retval = MUI_EventHandlerRC_Eat;
        }
        break;
      case MUIKEY_PAGEDOWN :
        if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
        {
          NL_List_Active(obj,data,MUIV_NList_Active_PageDown,NULL,data->NList_List_Select,FALSE);
          retval = MUI_EventHandlerRC_Eat;
        }
        else
        {
          NL_List_First(obj,data,MUIV_NList_First_PageDown,NULL);
          retval = MUI_EventHandlerRC_Eat;
        }
        break;
      case MUIKEY_TOP      :
        if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
        {
          NL_List_Active(obj,data,MUIV_NList_Active_Top,NULL,data->NList_List_Select,FALSE);
          retval = MUI_EventHandlerRC_Eat;
        }
        else
        {
          NL_List_First(obj,data,MUIV_NList_First_Top,NULL);
          retval = MUI_EventHandlerRC_Eat;
        }
        break;
      case MUIKEY_BOTTOM   :
        if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
        {
          NL_List_Active(obj,data,MUIV_NList_Active_Bottom,NULL,data->NList_List_Select,FALSE);
          retval = MUI_EventHandlerRC_Eat;
        }
        else
        {
          NL_List_First(obj,data,MUIV_NList_First_Bottom,NULL);
          retval = MUI_EventHandlerRC_Eat;
        }
        break;
      case MUIKEY_PRESS   :
        if (data->NList_Input && !data->NList_TypeSelect && data->NList_Active >= 0)
        {
          data->click_x = data->hpos - data->NList_Horiz_First;
          if ((data->NList_DefClickColumn > 0) && (data->NList_DefClickColumn < data->numcols))
            data->click_x += data->cols[data->NList_DefClickColumn].c->minx;
          else
            data->click_x += data->cols[0].c->minx;
          if (data->click_x < data->hpos)
            data->click_x = data->hpos;
          if (data->click_x >= data->hpos + data->NList_Horiz_Visible)
            data->click_x = data->hpos + data->NList_Horiz_Visible - 1;
          data->click_y = data->vpos + ((data->NList_Active-data->NList_First) * data->vinc) + (data->vinc/2);
          data->click_line = data->NList_Active;

          DO_NOTIFY(NTF_Doubleclick | NTF_LV_Doubleclick);

          if (WANTED_NOTIFY(NTF_EntryClick) && !WANTED_NOTIFY(NTF_Doubleclick) && !WANTED_NOTIFY(NTF_LV_Doubleclick))
          {
            DO_NOTIFY(NTF_EntryClick);
          }

          retval = MUI_EventHandlerRC_Eat;
        }
        break;
      case MUIKEY_TOGGLE   :
        if (data->multiselect && data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
        {
          MOREQUIET;
          NL_List_Select(obj,data,MUIV_NList_Select_Active,MUIV_NList_Active_Off,MUIV_NList_Select_Toggle,NULL);
          NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->NList_List_Select,FALSE);
          LESSQUIET;

          retval = MUI_EventHandlerRC_Eat;
        }
        break;

      case MUIKEY_LEFT:
      {
        BOOL scrolled = NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left,NULL);

        // if we have an object that we should make the new active object
        // of the window we do so in case this left key action didn't end up in
        // a real scrolling
        if(scrolled == FALSE && data->NList_KeyLeftFocus != NULL)
          set(_win(obj), MUIA_Window_ActiveObject, data->NList_KeyLeftFocus);

        retval = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_RIGHT:
      {
        BOOL scrolled = NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right,NULL);

        // if we have an object that we should make the new active object
        // of the window we do so in case this right key action didn't end up in
        // a real scrolling
        if(scrolled == FALSE && data->NList_KeyRightFocus != NULL)
          set(_win(obj), MUIA_Window_ActiveObject, data->NList_KeyRightFocus);

        retval = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_WORDLEFT :
      {
        NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_PageLeft,NULL);
        retval = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_WORDRIGHT:
      {
        NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_PageRight,NULL);
        retval = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_LINESTART:
      {
        NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Start,NULL);
        retval = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_LINEEND  :
      {
        NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_End,NULL);
        retval = MUI_EventHandlerRC_Eat;
      }
      break;
    }

    // return immediately if a key has been "used"
    if(retval != 0)
      return retval;
  }

  /*D(bug("NL: mNL_HandleEvent() /before \n"));*/

  if (msg->imsg && !data->NList_Quiet && !data->NList_Disabled)
  { LONG tagval,tagval2;
    LONG drag_ok = FALSE;
    WORD hfirst = data->NList_Horiz_AffFirst & ~1;
    WORD hfirsthpos = hfirst - data->hpos;
    WORD do_draw = FALSE;
    LONG WheelFastQual = NL_TestKey(obj,data,KEYTAG_QUALIFIER_WHEEL_FAST,msg->imsg->Code,msg->imsg->Qualifier,TRUE);
    LONG WheelHorizQual = NL_TestKey(obj,data,KEYTAG_QUALIFIER_WHEEL_HORIZ,msg->imsg->Code,msg->imsg->Qualifier,TRUE);
    LONG BalanceQual = NL_TestKey(obj,data,KEYTAG_QUALIFIER_BALANCE,msg->imsg->Code,msg->imsg->Qualifier,TRUE);
    LONG DragQual = NL_TestKey(obj,data,KEYTAG_QUALIFIER_DRAG,msg->imsg->Code,msg->imsg->Qualifier,TRUE);
    LONG MultQual = NL_TestKey(obj,data,KEYTAG_QUALIFIER_MULTISELECT,msg->imsg->Code,msg->imsg->Qualifier,TRUE);
    LONG Title2Qual = NL_TestKey(obj,data,KEYTAG_QUALIFIER_TITLECLICK2,msg->imsg->Code,msg->imsg->Qualifier,TRUE);
    if (data->multisel_qualifier && ((msg->imsg->Qualifier & data->multisel_qualifier) == data->multisel_qualifier))
      MultQual = TRUE;
    if (data->NList_WheelMMB && (msg->imsg->Qualifier & IEQUALIFIER_MIDBUTTON))
      WheelFastQual = TRUE;
    retval = 0;

    if ((msg->imsg->Class == IDCMP_INTUITICKS) ||
        (msg->imsg->Class == IDCMP_MOUSEMOVE) ||
        (msg->imsg->Class == IDCMP_MOUSEBUTTONS))
    { if ((data->multiclickalone > 0) &&
          !DoubleClick(data->secs,data->micros,msg->imsg->Seconds,msg->imsg->Micros))
      { DO_NOTIFY(NTF_MulticlickAlone);
      }
    }
    switch (msg->imsg->Class)
    {
      case IDCMP_INACTIVEWINDOW:
      case IDCMP_ACTIVEWINDOW:
        {
/*
 *{
 *LONG ab = (LONG) data->adjustbar;
 *LONG co = (LONG) msg->imsg->Code;
 *LONG qu = (LONG) msg->imsg->Qualifier;
 *LONG mx = (LONG) msg->imsg->MouseX;
 *LONG my = (LONG) msg->imsg->MouseY;
 *D(bug("%lx|1ab %ld stop IntuiMessage :\n",obj,ab));
 *if (msg->imsg->Class == IDCMP_INACTIVEWINDOW)
 *D(bug("  Class=INACTIVEWINDOW Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_ACTIVEWINDOW)
 *D(bug("  Class=ACTIVEWINDOW Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else
 *D(bug("  Class=%lx Code=%lx Qualifier=%lx mx=%ld my=%ld\n",msg->imsg->Class,co,qu,mx,my));
 *}
*/
          NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
          data->selectskiped = FALSE;
          data->moves = FALSE;

          if (data->adjustbar != -1)
          { data->adjustbar = -1;
            do_draw = TRUE;
            data->click_line = -2;
          }
/*
** Reset the ShortHelp to the Default for Not Being Over Button
*/
          if (data->affover>=0)
          {
             data->affover = -1;
             nnset(obj,MUIA_ShortHelp,data->NList_ShortHelp);
          }
          if (data->affbutton >= 0)
          {
            NL_Changed(data,data->affbuttonline);
            do_draw = TRUE;
            data->affbutton = -1;
            data->affbuttonline = -1;
            data->affbuttoncol = -1;
            data->affbuttonstate = 0;
            data->storebutton = FALSE;
          }
        }
        mNL_Trigger(cl,obj,NULL);
        return (MUI_EventHandlerRC_Eat);
      break;

      case IDCMP_RAWKEY:

        data->ScrollBarsTime = SCROLLBARSTIME;

        #if !defined(__amigaos4__)
        // check for wheelmouse events first
        if(_isinobject(msg->imsg->MouseX,msg->imsg->MouseY))
        {
          if(msg->imsg->Code == NM_WHEEL_UP)  /* MOUSE_WHEEL_UP */
          {
            if(WheelHorizQual && WheelFastQual)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left4,NULL);
            else if(WheelHorizQual)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left,NULL);
            else if(WheelFastQual)
            {
              int i;
              for(i=0; i < data->NList_WheelFast; i++)
                NL_List_First(obj,data,MUIV_NList_First_Up,NULL);
            }
            else
            {
              int i;
              for(i=0; i < data->NList_WheelStep; i++)
                NL_List_First(obj,data,MUIV_NList_First_Up,NULL);
            }

            retval = MUI_EventHandlerRC_Eat;
          }
          else if(msg->imsg->Code == NM_WHEEL_DOWN)  /* MOUSE_WHEEL_DOWN */
          {
            if(WheelHorizQual && WheelFastQual)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right4,NULL);
            else if(WheelHorizQual)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right,NULL);
            else if(WheelFastQual)
            {
              int i;
              for(i=0; i < data->NList_WheelFast; i++)
                NL_List_First(obj,data,MUIV_NList_First_Down,NULL);
            }
            else
            {
              int i;
              for(i=0; i < data->NList_WheelStep; i++)
                NL_List_First(obj,data,MUIV_NList_First_Down,NULL);
            }

            retval = MUI_EventHandlerRC_Eat;
          }
          else if(msg->imsg->Code == NM_WHEEL_LEFT)  /* MOUSE_WHEEL_LEFT */
          {
            NL_List_Horiz_First(obj, data, WheelFastQual ? MUIV_NList_Horiz_First_Left4 : MUIV_NList_Horiz_First_Left, NULL);

            retval = MUI_EventHandlerRC_Eat;
          }
          else if(msg->imsg->Code == NM_WHEEL_RIGHT)  /* MOUSE_WHEEL_RIGHT */
          {
            NL_List_Horiz_First(obj, data, WheelFastQual ? MUIV_NList_Horiz_First_Right4 : MUIV_NList_Horiz_First_Right, NULL);

            retval = MUI_EventHandlerRC_Eat;
          }
        }
        #endif

        if((tagval = xget(_win(obj), MUIA_Window_ActiveObject)) &&
           ((tagval == (LONG)obj) ||
             (tagval && (tagval2 = xget((Object *)tagval, MUIA_Listview_List)) && (tagval2 == (LONG)obj)) ||
             (!tagval && (tagval2 = xget(_win(obj), MUIA_Window_DefaultObject)) && (tagval2 == (LONG)obj)))
           )
        {
          if (data->NList_AutoCopyToClip)
          {
            if (NL_TestKey(obj,data,KEYTAG_COPY_TO_CLIPBOARD,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
            {
              NL_CopyTo(obj,data,MUIV_NList_CopyToClip_Selected,NULL,0L,NULL,NULL);
              if (data->NList_TypeSelect == MUIV_NList_TypeSelect_Char)
                SelectFirstPoint(obj,data,data->click_x,data->click_y);
              retval = MUI_EventHandlerRC_Eat;
            }
          }
          if (NL_TestKey(obj,data,KEYTAG_SELECT_TO_TOP,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          {
            if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
            {
            	LONG newactsel = MUIV_NList_Select_On;
            	if (data->NList_Active >= 0 && data->EntriesArray[data->NList_Active]->Select == TE_Select_None)
            		newactsel = MUIV_NList_Select_Off;
              NL_List_Active(obj,data,MUIV_NList_Active_UntilTop,NULL,newactsel,FALSE);
            }
            retval = MUI_EventHandlerRC_Eat;
          }
          else if (NL_TestKey(obj,data,KEYTAG_SELECT_TO_BOTTOM,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          {
            if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
            {
            	LONG newactsel = MUIV_NList_Select_On;
            	if (data->NList_Active >= 0 && data->EntriesArray[data->NList_Active]->Select == TE_Select_None)
            		newactsel = MUIV_NList_Select_Off;
              NL_List_Active(obj,data,MUIV_NList_Active_UntilBottom,NULL,newactsel,FALSE);
            }
            retval = MUI_EventHandlerRC_Eat;
          }
          else if (NL_TestKey(obj,data,KEYTAG_SELECT_TO_PAGE_UP,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          {
            /* Page up key pressed */
            if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
            {
            	LONG newactsel = MUIV_NList_Select_On;
            	if (data->NList_Active >= 0 && data->EntriesArray[data->NList_Active]->Select == TE_Select_None)
            		newactsel = MUIV_NList_Select_Off;
              NL_List_Active(obj,data,MUIV_NList_Active_UntilPageUp,NULL,newactsel,FALSE);
            }
            retval = MUI_EventHandlerRC_Eat;
          }
          else if (NL_TestKey(obj,data,KEYTAG_SELECT_TO_PAGE_DOWN,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          {
          	/* Page down key pressed */
            if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
            {
            	LONG newactsel = MUIV_NList_Select_On;
            	if (data->NList_Active >= 0 && data->EntriesArray[data->NList_Active]->Select == TE_Select_None)
            		newactsel = MUIV_NList_Select_Off;
              NL_List_Active(obj,data,MUIV_NList_Active_UntilPageDown,NULL,newactsel,FALSE);
            }
            retval = MUI_EventHandlerRC_Eat;
          }
          else if (NL_TestKey(obj,data,KEYTAG_SELECT_UP,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          {
          	/* Up key pressed */
            if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
            {
            	LONG newactsel = MUIV_NList_Select_On;
            	if (data->NList_Active >= 0 && data->EntriesArray[data->NList_Active]->Select == TE_Select_None)
            		newactsel = MUIV_NList_Select_Off;
             	NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,newactsel,FALSE);
            }
            retval = MUI_EventHandlerRC_Eat;
          }
          else if (NL_TestKey(obj,data,KEYTAG_SELECT_DOWN,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          {
          	/* Down key pressed */
            if (data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
            {
            	LONG newactsel = MUIV_NList_Select_On;
            	if (data->NList_Active >= 0 && data->EntriesArray[data->NList_Active]->Select == TE_Select_None)
            		newactsel = MUIV_NList_Select_Off;
             	NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,newactsel,FALSE);
            }
            retval = MUI_EventHandlerRC_Eat;
          }
          else if (NL_TestKey(obj,data,KEYTAG_TOGGLE_ACTIVE,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          {
            if (data->multiselect && data->NList_Input && !data->NList_TypeSelect && data->EntriesArray)
              NL_List_Select(obj,data,MUIV_NList_Select_Active,MUIV_NList_Active_Off,MUIV_NList_Select_Toggle,NULL);
            retval = MUI_EventHandlerRC_Eat;
          }
          else if (NL_TestKey(obj,data,KEYTAG_DEFAULT_WIDTH_COLUMN,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          { struct MUI_NList_TestPos_Result res;
            res.char_number = -2;
            NL_List_TestPos(obj,data,MUI_MAXMAX,0,&res);
            if ((res.column >= 0) && (res.column < data->numcols) && !(res.flags & MUI_NLPR_BAR))
            {
              NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
              data->selectskiped = FALSE;
              data->moves = FALSE;
              if (data->adjustbar != -1)
              { data->adjustbar = -1;
                do_draw = TRUE;
                data->click_line = -2;
              }
              NL_ColWidth(obj,data,NL_ColumnToCol(obj,data,res.column),MUIV_NList_ColWidth_Default);
            }
          }
          else if (NL_TestKey(obj,data,KEYTAG_DEFAULT_WIDTH_ALL_COLUMNS,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
            NL_ColWidth(obj,data,MUIV_NList_ColWidth_All,MUIV_NList_ColWidth_Default);
          else if (NL_TestKey(obj,data,KEYTAG_DEFAULT_ORDER_COLUMN,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
          { struct MUI_NList_TestPos_Result res;
            res.char_number = -2;
            NL_List_TestPos(obj,data,MUI_MAXMAX,0,&res);
            if ((res.column >= 0) && (res.column < data->numcols) && !(res.flags & MUI_NLPR_BAR))
            {
              NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
              data->selectskiped = FALSE;
              data->moves = FALSE;
              if (data->adjustbar != -1)
              { data->adjustbar = -1;
                do_draw = TRUE;
                data->click_line = -2;
              }
              NL_SetCol(obj,data,res.column,MUIV_NList_SetColumnCol_Default);
            }
          }
          else if (NL_TestKey(obj,data,KEYTAG_DEFAULT_ORDER_ALL_COLUMNS,msg->imsg->Code,msg->imsg->Qualifier,FALSE))
            NL_SetCol(obj,data,MUIV_NList_SetColumnCol_Default,MUIV_NList_SetColumnCol_Default);

/*
 *           else
 *           { int posraw;
 *             posraw = DeadKeyConvert(data,msg->imsg,data->rawtext,MAXRAWBUF,0L);
 *             if (posraw > 0)
 *             {
 *   D(bug("RAWKEY BUF=%s \n",data->rawtext));
 *             }
 *           }
 */
        }
        break;

      #if defined(__amigaos4__)
      case IDCMP_EXTENDEDMOUSE:
      {
        if(msg->imsg->Code & IMSGCODE_INTUIWHEELDATA &&
           _isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
        {
      		struct IntuiWheelData *iwd = (struct IntuiWheelData *)msg->imsg->IAddress;

          if(iwd->WheelY < 0) // WHEEL_UP
          {
            if(WheelHorizQual && WheelFastQual)
              NL_List_Horiz_First(obj, data, MUIV_NList_Horiz_First_Left4, NULL);
            else if(WheelHorizQual)
              NL_List_Horiz_First(obj, data, MUIV_NList_Horiz_First_Left, NULL);
            else if(WheelFastQual)
            {
              int i;
              for(i=0; i < data->NList_WheelFast; i++)
                NL_List_First(obj, data, MUIV_NList_First_Up, NULL);
            }
            else
            {
              int i;
              for(i=0; i < data->NList_WheelStep; i++)
                NL_List_First(obj, data, MUIV_NList_First_Up, NULL);
            }

            retval = MUI_EventHandlerRC_Eat;
          }
          else if(iwd->WheelY > 0) // WHEEL_DOWN
          {
            if(WheelHorizQual && WheelFastQual)
              NL_List_Horiz_First(obj, data, MUIV_NList_Horiz_First_Right4, NULL);
            else if(WheelHorizQual)
              NL_List_Horiz_First(obj, data, MUIV_NList_Horiz_First_Right, NULL);
            else if(WheelFastQual)
            {
              int i;
              for(i=0; i < data->NList_WheelFast; i++)
                NL_List_First(obj, data, MUIV_NList_First_Down, NULL);
            }
            else
            {
              int i;
              for(i=0; i < data->NList_WheelStep; i++)
                NL_List_First(obj, data, MUIV_NList_First_Down, NULL);
            }

            retval = MUI_EventHandlerRC_Eat;
          }

          if(iwd->WheelX < 0)  // WHEEL_LEFT
          {
            NL_List_Horiz_First(obj, data, WheelFastQual ? MUIV_NList_Horiz_First_Left4 : MUIV_NList_Horiz_First_Left, NULL);

            retval = MUI_EventHandlerRC_Eat;
          }
          else if(iwd->WheelX > 0)  // WHEEL_RIGHT
          {
            NL_List_Horiz_First(obj, data, WheelFastQual ? MUIV_NList_Horiz_First_Right4 : MUIV_NList_Horiz_First_Right, NULL);

            retval = MUI_EventHandlerRC_Eat;
          }
        }
      }
      break;
      #endif

      case IDCMP_MOUSEBUTTONS:
        data->ScrollBarsTime = SCROLLBARSTIME;
        {
          LONG multi = 0;
          LONG isdoubleclick = 0;
          if ((data->multiselect != MUIV_NList_MultiSelect_None) &&
              (MultQual || (data->multiselect == MUIV_NList_MultiSelect_Always)))
            multi = 1;
          data->selectskiped = FALSE;
          if (msg->imsg->Code==SELECTDOWN && _isinobject2(msg->imsg->MouseX,msg->imsg->MouseY))
          {
            WORD ly = (msg->imsg->MouseY - data->vpos);
            WORD ly2 = (msg->imsg->MouseY - data->vdtitlepos);
            WORD lyl = ly / data->vinc;
            LONG lyl2 = lyl + data->NList_First;
            WORD lx = (msg->imsg->MouseX - data->hpos);
            BOOL do_else = FALSE;
            BOOL mclick = FALSE;

            if (lx < 0)
            {
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left,NULL);
              data->selectmode = MUIV_NList_Select_None;
              data->NumIntuiTick = MAX_INTUITICKS_WAIT;
              data->moves = TRUE;
              NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
            }
            else if (lx >= data->NList_Horiz_Visible)
            {
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right,NULL);
              data->selectmode = MUIV_NList_Select_None;
              data->NumIntuiTick = MAX_INTUITICKS_WAIT;
              data->moves = TRUE;
              NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
            }
            else if (ly2 < 0)
            {
              data->selectmode = data->NList_List_Select;
              if (data->NList_Input && !data->NList_TypeSelect)
              { if (data->NList_Active > data->NList_First)
                  NL_List_Active(obj,data,MUIV_NList_Active_PageUp,NULL,data->selectmode,FALSE);
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
              }
              else
                NL_List_First(obj,data,MUIV_NList_First_Up,NULL);
              data->NumIntuiTick = MAX_INTUITICKS_WAIT;
              data->moves = TRUE;
              NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
            }
            else if ((lyl >= data->NList_Visible) || (lyl2 >= data->NList_Entries))
            {
              data->selectmode = data->NList_List_Select;
              if (data->NList_Input && !data->NList_TypeSelect)
              { if (data->NList_Active < data->NList_First - 1 + data->NList_Visible)
                  NL_List_Active(obj,data,MUIV_NList_Active_PageDown,NULL,data->selectmode,FALSE);
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
              }
              else
                NL_List_First(obj,data,MUIV_NList_First_Down,NULL);
              data->NumIntuiTick = MAX_INTUITICKS_WAIT;
              data->moves = TRUE;
              NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
            }
            else
              do_else = TRUE;

            if ((lx < 0) || (lx >= data->NList_Horiz_Visible) || (ly2 < 0) || (lyl >= data->NList_Visible) || (lyl2 >= data->NList_Entries))
              lyl2 = -2;
            else if ((ly2 >= 0) && (ly < 0))
              lyl2 = -1;
            isdoubleclick = DoubleClick(data->secs,data->micros,msg->imsg->Seconds,msg->imsg->Micros);
            if ((data->click_line == lyl2) && (lyl2 >= -1) &&
                (abs(data->click_x - msg->imsg->MouseX) < 4) &&  (abs(data->click_y - msg->imsg->MouseY) < 4) &&
                isdoubleclick)
            {
              mclick = TRUE;
            }
            else
            {
              BOOL modifyActiveObject = FALSE;
              Object *newActiveObject = (Object *)MUIV_Window_ActiveObject_None;

              data->secs = msg->imsg->Seconds;
              data->micros = msg->imsg->Micros;
              data->click_line = lyl2;
              data->click_x = msg->imsg->MouseX;
              data->click_y = msg->imsg->MouseY;
              data->mouse_x = msg->imsg->MouseX;
              data->mouse_y = msg->imsg->MouseY;
              data->multiclick = 0;
              data->multiclickalone = 1;
              if (data->click_line >= 0)
              {
                DO_NOTIFY(NTF_EntryClick);
              }

              if (data->NList_Title && (lx >= 0) && (lx < data->NList_Horiz_Visible) && (ly2 >= 0) && (ly < 0))
              {
                struct MUI_NList_TestPos_Result res;
                res.char_number = -2;
                NL_List_TestPos(obj,data,MUI_MAXMAX,MUI_MAXMAX,&res);
                if ((res.flags & MUI_NLPR_BAR) && (res.flags & MUI_NLPR_TITLE) && (res.flags & MUI_NLPR_ABOVE) &&
                    (res.column < data->numcols) && (res.column >= 0))
                {
                  WORD minx = data->cols[res.column].c->minx - hfirsthpos
                              + ((data->cols[res.column].c->delta-1) / 2) + MinColWidth;
                  WORD actx = data->cols[res.column].c->maxx - hfirsthpos
                              + ((data->cols[res.column].c->delta-1) / 2);
                  if ((actx < data->mleft) || (actx > data->mright))
                    actx = msg->imsg->MouseX;
                  data->adjustcolumn = res.column;
                  {
                    if (actx <= minx)
                    { if (minx >= data->mleft)
                      { data->adjustbar = minx + hfirsthpos;
                        data->adjustbar_last = data->cols[data->adjustcolumn].c->userwidth;
                        do_draw = TRUE;
                        NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
                      }
                    }
                    else if (actx > minx)
                    { data->adjustbar = actx + hfirsthpos;
                      data->adjustbar_last = data->cols[data->adjustcolumn].c->userwidth;
                      do_draw = TRUE;
                      NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
                    }
                    if ((data->adjustbar >= 0) && BalanceQual &&
                        ((res.column < data->numcols-2) ||
                         ((res.column == data->numcols-2) && (data->cols[res.column+1].c->userwidth > 0))))
                    { data->adjustbar_last2 = data->cols[res.column+1].c->userwidth;
                      data->adjustbar2 = data->cols[res.column+1].c->maxx;
                    }
                    else
                      data->adjustbar2 = -1;
                  }
                  data->click_line = -2;
                  data->moves = FALSE;
                  drag_ok = FALSE;
                }
                else if ((res.flags & MUI_NLPR_ABOVE) && (res.flags & MUI_NLPR_TITLE) &&
                         (res.column < data->numcols) && (res.column >= 0))
                {
                  data->adjustcolumn = res.column;
                  if ((WANTED_NOTIFY(NTF_TitleClick) || WANTED_NOTIFY(NTF_TitleClick2)) && data->cols[res.column].c->titlebutton)
                  { data->adjustbar = -2;
                    do_draw = TRUE;
                    NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
                  }
                  else
                  { if (WANTED_NOTIFY(NTF_TitleClick2) && (Title2Qual || !WANTED_NOTIFY(NTF_TitleClick)))
                      data->TitleClick2 = (LONG) data->cols[res.column].c->col;
                    else
                      data->TitleClick = (LONG) data->cols[res.column].c->col;
                    if ((data->adjustcolumn >= data->NList_MinColSortable) &&
                        ((data->numcols - data->NList_MinColSortable) > 1) &&
                        (NL_OnWindow(obj,data,msg->imsg->MouseX,msg->imsg->MouseY)))
                    {
/*
 *{
 *LONG ab = (LONG) data->adjustbar;
 *LONG co = (LONG) msg->imsg->Code;
 *LONG qu = (LONG) msg->imsg->Qualifier;
 *LONG mx = (LONG) msg->imsg->MouseX;
 *LONG my = (LONG) msg->imsg->MouseY;
 *D(bug("%lx|1ab0 %ld start col sort IntuiMessage :\n",obj,ab));
 *if (msg->imsg->Class == IDCMP_MOUSEMOVE)
 *D(bug("  Class=MOUSEMOVE Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_INTUITICKS)
 *D(bug("  Class=INTUITICKS Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_MOUSEBUTTONS)
 *D(bug("  Class=MOUSEBUTTONS Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else
 *D(bug("  Class=%lx Code=%lx Qualifier=%lx mx=%ld my=%ld\n",msg->imsg->Class,co,qu,mx,my));
 *}
*/
                      data->adjustbar = -10;

                      // set a custom mouse pointer
                      ShowCustomPointer(obj, data, PT_MOVE);
                    }
                  }
                  data->moves = FALSE;
                  drag_ok = FALSE;
                }
              }
              else if (!data->NList_Title && (lx >= 0) && (lx < data->NList_Horiz_Visible) && (ly < data->vinc/2) && (ly >= 0))
              { struct MUI_NList_TestPos_Result res;
                res.char_number = -2;
                NL_List_TestPos(obj,data,MUI_MAXMAX,MUI_MAXMAX,&res);
                if ((res.flags & MUI_NLPR_BAR) &&
                    (res.column < data->numcols) && (res.column >= 0))
                {
                  WORD minx = data->cols[res.column].c->minx - hfirsthpos
                              + ((data->cols[res.column].c->delta-1) / 2) + 10;
                  WORD actx = data->cols[res.column].c->maxx - hfirsthpos
                              + ((data->cols[res.column].c->delta-1) / 2);
                  if ((actx < data->mleft) || (actx > data->mright))
                    actx = msg->imsg->MouseX;
                  data->adjustcolumn = res.column;
                  {
                    if (actx <= minx)
                    { if (minx >= data->mleft)
                      { data->adjustbar = minx + hfirsthpos;
                        data->adjustbar_last = data->cols[data->adjustcolumn].c->userwidth;
                        do_draw = TRUE;
                        NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
                      }
                    }
                    else if (actx > minx)
                    { data->adjustbar = actx + hfirsthpos;
                      data->adjustbar_last = data->cols[data->adjustcolumn].c->userwidth;
                      do_draw = TRUE;
                      NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
                    }
                    if ((data->adjustbar >= 0) && BalanceQual &&
                        ((res.column < data->numcols-2) ||
                         ((res.column == data->numcols-2) && (data->cols[res.column+1].c->userwidth > 0))))
                    { data->adjustbar_last2 = data->cols[res.column+1].c->userwidth;
                      data->adjustbar2 = data->cols[res.column+1].c->maxx;
                    }
                    else
                      data->adjustbar2 = -1;
                  }
                  data->click_line = -2;
                  data->moves = FALSE;
                  drag_ok = FALSE;
                }
              }

              if(data->NList_DefaultObjectOnClick)
              {
                ULONG tst = xget(_win(obj), MUIA_Window_ActiveObject);

                if((tst != MUIV_Window_ActiveObject_None) && (tst != data->NList_KeepActive) && (tst != (ULONG) obj))
                {
                  if (data->NList_MakeActive)
                    newActiveObject = (Object *)data->NList_MakeActive;
                  else
                    newActiveObject = (Object *)MUIV_Window_ActiveObject_None;

                  modifyActiveObject = TRUE;
                }

                set(_win(obj), MUIA_Window_DefaultObject, (LONG) obj);
              }
              else if(data->NList_MakeActive)
              {
                newActiveObject = (Object *)data->NList_MakeActive;
                modifyActiveObject = TRUE;
              }

              // now we check if the user wants to set this object
              // as the active one or not.
              if(data->NList_ActiveObjectOnClick && data->isActiveObject == FALSE)
              {
                newActiveObject = obj;
                modifyActiveObject = TRUE;
              }

              // change the window's active object if necessary
              if(modifyActiveObject == TRUE)
                set(_win(obj), MUIA_Window_ActiveObject, newActiveObject);

              if(data->NList_TypeSelect && (data->adjustbar == -1))
              {
                if(MultQual)
                  SelectSecondPoint(obj,data,data->click_x,data->click_y);
                else
                {
                  data->NList_TypeSelect = MUIV_NList_TypeSelect_Char;
                  SelectFirstPoint(obj,data,data->click_x,data->click_y);
                }
              }
            }

            if (do_else && (data->adjustbar == -1) && (ly >= 0) && (lyl >= 0) && (lyl < data->NList_Visible))
            {
              struct MUI_NList_TestPos_Result res;
              data->affbutton = -1;
              data->affbuttonline = -1;
              data->affbuttoncol = -1;
              data->affbuttonstate = 0;
              data->storebutton = TRUE;
              res.char_number = 0;
              NL_List_TestPos(obj,data,msg->imsg->MouseX,msg->imsg->MouseY,&res);
              data->storebutton = FALSE;
              if ((data->affbutton >= 0) && (data->affbuttonline >= 0))
              { data->affbuttonstate = -2;
                NL_Changed(data,data->affbuttonline);
                do_draw = TRUE;
                data->moves = FALSE;
                drag_ok = FALSE;
                mclick = FALSE;
              }
              NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
            }

            if(mclick)
            {
              data->secs = msg->imsg->Seconds;
              data->micros = msg->imsg->Micros;
              data->multiclick++;
              data->multiclickalone++;

              if (data->multiclick == 1)
              {
                DO_NOTIFY(NTF_Doubleclick | NTF_LV_Doubleclick);
              }
              else if (data->multiclick > 1)
              {
                DO_NOTIFY(NTF_Multiclick);
              }

              if (data->NList_TypeSelect && (data->adjustbar == -1))
              {
                if (MultQual)
                { SelectSecondPoint(obj,data,data->click_x,data->click_y);
                  do_else = FALSE;
                }
                else
                {
                  if ((data->multiclick % 3) == 0)
                    data->NList_TypeSelect = MUIV_NList_TypeSelect_Char;
                  else if ((data->multiclick % 3) == 1)
                    data->NList_TypeSelect = MUIV_NList_TypeSelect_CWord;
                  else if ((data->multiclick % 3) == 2)
                    data->NList_TypeSelect = MUIV_NList_TypeSelect_CLine;
                  SelectFirstPoint(obj,data,data->click_x,data->click_y);
                  do_else = FALSE;
                }
              }
            }

            if (do_else && (data->adjustbar == -1) && (data->affbutton < 0) && (ly >= 0) && (lyl >= 0) && (lyl < data->NList_Visible))
            {
              long lactive = lyl + data->NList_First;
              if ((lactive >= 0) && (lactive < data->NList_Entries))
              {
                if (data->NList_Input && !data->NList_TypeSelect)
                {
                  if (data->multiselect == MUIV_NList_MultiSelect_None)
                  {
                    data->selectmode = MUIV_NList_Select_On;
                    NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                  }
                  else if (!multi)
                  {
                    MOREQUIET;
                    NL_UnSelectAll(obj,data,lactive);
                    data->selectmode = MUIV_NList_Select_On;
                    NL_List_Active(obj,data,lactive,NULL,data->selectmode,TRUE);
                    LESSQUIET;
                    data->selectskiped = TRUE;
                  }
                  else
                  {
                    if (data->EntriesArray[lactive]->Select == TE_Select_None)
                      data->selectmode = MUIV_NList_Select_On;
                    else
                      data->selectmode = MUIV_NList_Select_Off;
                    NL_List_Active(obj,data,lactive,NULL,data->selectmode,TRUE);
                    data->selectskiped = TRUE;
                  }
                }
              }
              data->moves = TRUE;
              NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);

              if (data->drag_type != MUIV_NList_DragType_None)
              {
                if (DragQual)
                  drag_ok = TRUE;
                else if ((data->drag_type == MUIV_NList_DragType_Immediate) || (data->drag_type == MUIV_NList_DragType_Borders))
                  data->drag_border = TRUE;
              }
            }

            // the click happened inside our realm, so signal MUI that we ate that click
            retval = MUI_EventHandlerRC_Eat;
          }
          else
          {
            if((msg->imsg->Code==SELECTDOWN) && data->NList_DefaultObjectOnClick)
            {
              /* click was not in _isinobject2() */
              ULONG tst = xget(_win(obj), MUIA_Window_DefaultObject);

              if(tst == (ULONG)obj)
                set(_win(obj), MUIA_Window_DefaultObject, NULL);

              // do not return MUI_EventHandlerRC_Eat, because this click happened outside
              // of our realm and most probably needs to be handled by another object.
            }
/*
            if (!(((msg->imsg->Code==MIDDLEDOWN) || (msg->imsg->Code==MIDDLEUP)) && (data->drag_qualifier & IEQUALIFIER_MIDBUTTON)) &&
                !(((msg->imsg->Code==MENUDOWN) || (msg->imsg->Code==MENUUP)) && (data->drag_qualifier & IEQUALIFIER_RBUTTON)))
*/
            {
              NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,FALSE);
              data->selectskiped = FALSE;
              data->moves = FALSE;
              drag_ok = FALSE;
            }
          }
          if (msg->imsg->Code==SELECTUP)
          {
            data->moves = data->selectskiped = FALSE;
/*
 *{
 *LONG ab = (LONG) data->adjustbar;
 *LONG co = (LONG) msg->imsg->Code;
 *LONG qu = (LONG) msg->imsg->Qualifier;
 *LONG mx = (LONG) msg->imsg->MouseX;
 *LONG my = (LONG) msg->imsg->MouseY;
 *D(bug("18a  Class=%lx Code=%lx Qualifier=%lx mx=%ld my=%ld %ld.%ld (%ld)\n",msg->imsg->Class,co,qu,mx,my,msg->imsg->Seconds,msg->imsg->Micros,data->moves));
 *}
*/
            if(data->NList_Input == FALSE && (data->NList_TypeSelect == MUIV_NList_TypeSelect_Char) &&
               data->NList_AutoClip == TRUE && (data->sel_pt[0].ent >= 0) && (data->sel_pt[1].ent >= 0) && !MultQual)
            {
              NL_CopyTo(obj,data,MUIV_NList_CopyToClip_Selected,NULL,0L,NULL,NULL);
              SelectFirstPoint(obj,data,data->click_x,data->click_y);
            }
          }
          if ((msg->imsg->Code==SELECTUP) && ((data->adjustbar >= 0) || (data->adjustbar_old >= 0)))
          {
            if ((data->adjustcolumn < data->numcols) && (data->adjustcolumn >= 0))
            { WORD userwidth = data->adjustbar
                               - ((data->cols[data->adjustcolumn].c->delta-1) / 2)
                               - data->cols[data->adjustcolumn].c->minx;
              if (userwidth < MinColWidth)
                userwidth = MinColWidth;
              if (data->cols[data->adjustcolumn].c->userwidth != userwidth)
              { data->cols[data->adjustcolumn].c->userwidth = userwidth;
                if (data->adjustbar2 >= 0)
                { userwidth = data->adjustbar2 - data->cols[data->adjustcolumn].c->minx
                              - userwidth - data->cols[data->adjustcolumn].c->delta;
                  if (userwidth < MinColWidth)
                    userwidth = MinColWidth;
                  data->cols[data->adjustcolumn+1].c->userwidth = userwidth;
                }
                data->do_setcols = TRUE;
                /*data->ScrollBarsPos = -2;*/
              }
            }
            data->adjustbar = -1;
            do_draw = TRUE;
            NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
            data->click_line = -2;
            data->moves = FALSE;
            drag_ok = FALSE;
          }
          else if (((msg->imsg->Code == MENUDOWN) || (msg->imsg->Code == MENUUP)) &&
                   ((data->adjustbar >= 0) || (data->adjustbar_old >= 0)))
          {
            if (data->NList_ColWidthDrag == MUIV_NList_ColWidthDrag_Visual)
            {
              data->cols[data->adjustcolumn].c->userwidth = data->adjustbar_last;
              if (data->adjustbar2 > 0)
                data->cols[data->adjustcolumn+1].c->userwidth = data->adjustbar_last2;
              data->do_setcols = TRUE;
            }
            data->adjustbar = -1;
            data->adjustbar2 = -1;
            NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,FALSE);
            do_draw = TRUE;
            data->click_line = -2;
            data->moves = FALSE;
            drag_ok = FALSE;
          }
          if ((msg->imsg->Code==SELECTUP) && (data->adjustbar >= -4) && (data->adjustbar <= -2))
          { NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
            data->moves = FALSE;
            drag_ok = FALSE;
            if ((data->adjustbar > -4) &&
                (data->adjustcolumn < data->numcols) && (data->adjustcolumn >= 0) &&
                (msg->imsg->MouseX >= data->mleft) &&
                (msg->imsg->MouseX <= data->mright) &&
                (msg->imsg->MouseY >= data->vdtitlepos) &&
                (msg->imsg->MouseY < data->mbottom))
            { data->click_line = -2;
              if (WANTED_NOTIFY(NTF_TitleClick2) && (Title2Qual || !WANTED_NOTIFY(NTF_TitleClick)))
              { data->TitleClick2 = (LONG) data->cols[data->adjustcolumn].c->col;
                DO_NOTIFY(NTF_TitleClick2);
              }
              else
              { data->TitleClick = (LONG) data->cols[data->adjustcolumn].c->col;
                DO_NOTIFY(NTF_TitleClick);
              }
            }
            data->adjustbar = -1;
            do_draw = TRUE;
          }
          else if ((msg->imsg->Code==SELECTUP) && (data->adjustbar == -10))
          {
            NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
            data->moves = FALSE;
            drag_ok = FALSE;
            data->adjustbar = -1;
            do_draw = TRUE;
            HideCustomPointer(obj, data);
          }
          if (((msg->imsg->Code==SELECTUP) || (msg->imsg->Code==MENUDOWN)) &&
              (data->affbutton >= 0))
          {
            if ((data->affbuttonstate == 2) || (data->affbuttonstate == -2))
            {
              data->NList_ButtonClick = data->affbutton;
              DO_NOTIFY(NTF_ButtonClick);
            }
            NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
            NL_Changed(data,data->affbuttonline);
            do_draw = TRUE;
            data->affbutton = -1;
            data->affbuttonline = -1;
            data->affbuttoncol = -1;
            data->affbuttonstate = 0;
            data->storebutton = FALSE;
          }
        }
        //retval = MUI_EventHandlerRC_Eat;
        break;
      case IDCMP_INTUITICKS:
        {
        struct MUI_NList_TestPos_Result res;
/*
** Store Current Values to Temp Variables
*/
        tempbutton      = data->affbutton;
        tempbuttonline  = data->affbuttonline;
        tempbuttoncol   = data->affbuttoncol;
        tempbuttonstate = data->affbuttonstate;
        tempstorebutton = data->storebutton;
        data->affimage = -1;
        data->affbutton = -1;
        data->affbuttonline = -1;
        data->affbuttoncol = -1;
        data->affbuttonstate = 0;
        data->storebutton = TRUE;
        res.char_number = 0;
        NL_List_TestPos(obj,data,msg->imsg->MouseX,msg->imsg->MouseY,&res);
        if (data->affbutton>=0) {
           if (data->affover!=data->affbutton) {
/*
** Set the ShortHelp to the Button's for Being Over Button (If ShortHelp Exists)
*/
              data->affover = data->affbutton;
              if ((data->affimage >= 0) && (data->affimage < data->LastImage) && data->NList_UseImages)
              {
                 STRPTR shorthelp;

                 if((shorthelp = (STRPTR)xget(data->NList_UseImages[data->affimage].imgobj, MUIA_ShortHelp)))
                    nnset(obj,MUIA_ShortHelp,shorthelp);
              }
            }
           }
        else {
           if (data->affover!=-1) {
/*
** Reset the ShortHelp to the Default for Not Being Over Button
*/
              data->affover = -1;
              nnset(obj,MUIA_ShortHelp,data->NList_ShortHelp);
              }
           }
/*
** Restore Values from Temp Variables
*/
        data->affbutton = tempbutton;
        data->affbuttonline = tempbuttonline;
        data->affbuttoncol = tempbuttoncol;
        data->affbuttonstate = tempbuttonstate;
        data->storebutton = tempstorebutton;
        if (data->NumIntuiTick > 0)
           { data->NumIntuiTick--;
             break;
           }
        }
      // walk through to next case...

      case IDCMP_MOUSEMOVE:

        if((msg->imsg->Class == IDCMP_MOUSEMOVE) && data->MOUSE_MOVE)
          data->ScrollBarsTime = SCROLLBARSTIME;

        if((msg->imsg->Class == IDCMP_MOUSEMOVE) &&
           ((msg->imsg->Qualifier & IEQUALIFIER_LEFTBUTTON) != IEQUALIFIER_LEFTBUTTON) &&
           ((msg->imsg->Qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_LCOMMAND)) != (IEQUALIFIER_LALT|IEQUALIFIER_LCOMMAND)))
        {
/*
 *{
 *LONG ab = (LONG) data->adjustbar;
 *LONG co = (LONG) msg->imsg->Code;
 *LONG qu = (LONG) msg->imsg->Qualifier;
 *LONG mx = (LONG) msg->imsg->MouseX;
 *LONG my = (LONG) msg->imsg->MouseY;
 *D(bug("%lx|1ab %ld stop IntuiMessage :\n",obj,ab));
 *if (msg->imsg->Class == IDCMP_MOUSEMOVE)
 *D(bug("  Class=MOUSEMOVE Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_INTUITICKS)
 *D(bug("  Class=INTUITICKS Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else
 *D(bug("  Class=%lx Code=%lx Qualifier=%lx mx=%ld my=%ld\n",msg->imsg->Class,co,qu,mx,my));
 *}
*/
          NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
          data->selectskiped = FALSE;
          data->moves = FALSE;

          if (data->adjustbar != -1)
          { data->adjustbar = -1;
            do_draw = TRUE;
            data->click_line = -2;
          }
          if (data->affbutton >= 0)
          {
            NL_Changed(data,data->affbuttonline);
            do_draw = TRUE;
            data->affbutton = -1;
            data->affbuttonline = -1;
            data->affbuttoncol = -1;
            data->affbuttonstate = 0;
            data->storebutton = FALSE;
          }
        }

        if(!data->MOUSE_MOVE && (msg->imsg->Class == IDCMP_MOUSEMOVE))
          break;

        data->mouse_x = msg->imsg->MouseX;
        data->mouse_y = msg->imsg->MouseY;

        if ((data->ContextMenu == (LONG)MUIV_NList_ContextMenu_TopOnly) /*&& (data->numcols > 1)*/) /* sba: Contextmenu problem: Disabled */
        { if ((data->NList_Title && (msg->imsg->MouseY >= data->vpos)) || (!data->NList_Title && (msg->imsg->MouseY > data->vpos + data->vinc/3)))
          { if (data->ContextMenuOn)
              notdoset(obj,MUIA_ContextMenu,NULL);
          }
          else
          { if (!data->ContextMenuOn)
              notdoset(obj,MUIA_ContextMenu,MUIV_NList_ContextMenu_TopOnly);
          }
        }
/*        else if (((data->ContextMenu & 0x9d510030) == 0x9d510030) && (data->numcols <= 1) && data->ContextMenuOn)
          notdoset(obj,MUIA_ContextMenu,NULL);
*/ /* sba: Contextmenu problem: Disabled */
        if ((data->affbutton >= 0) && (msg->imsg->Class == IDCMP_MOUSEMOVE))
        {
          if ((msg->imsg->MouseX >= data->affbuttonpos.Left) &&
              (msg->imsg->MouseX < data->affbuttonpos.Left+data->affbuttonpos.Width) &&
              (msg->imsg->MouseY >= data->affbuttonpos.Top) &&
              (msg->imsg->MouseY < data->affbuttonpos.Top+data->affbuttonpos.Height))
          { if ((data->affbuttonstate != -2) && (data->affbuttonstate != 2))
            {
              data->affbuttonstate = -2;
              NL_Changed(data,data->affbuttonline);
              do_draw = TRUE;
            }
          }
          else
          { if ((data->affbuttonstate == -2) || (data->affbuttonstate == 2))
            {
              data->affbuttonstate = -1;
              NL_Changed(data,data->affbuttonline);
              do_draw = TRUE;
            }
          }
          data->moves = FALSE;
          drag_ok = FALSE;
        }

        if ((data->adjustbar >= 0) && (msg->imsg->Class == IDCMP_MOUSEMOVE))
        { WORD ab = msg->imsg->MouseX;
          WORD minx = data->cols[data->adjustcolumn].c->minx - hfirsthpos
                      + ((data->cols[data->adjustcolumn].c->delta-1) / 2) + MinColWidth;
          if (ab <= minx)
            ab = minx;
          if ((ab + hfirsthpos) != data->adjustbar)
          { data->adjustbar = ab + hfirsthpos;
            do_draw = TRUE;
            if (data->NList_ColWidthDrag == MUIV_NList_ColWidthDrag_Visual)
            { if ((data->adjustcolumn < data->numcols) && (data->adjustcolumn >= 0))
              { WORD userwidth = data->adjustbar
                                 - ((data->cols[data->adjustcolumn].c->delta-1) / 2)
                                 - data->cols[data->adjustcolumn].c->minx;
                if (userwidth < MinColWidth)
                  userwidth = MinColWidth;
                if (data->cols[data->adjustcolumn].c->userwidth != userwidth)
                { data->cols[data->adjustcolumn].c->userwidth = userwidth;
                  if (data->adjustbar2 >= 0)
                  { userwidth = data->adjustbar2 - data->cols[data->adjustcolumn].c->minx
                                - userwidth - data->cols[data->adjustcolumn].c->delta;
                    if (userwidth < MinColWidth)
                      userwidth = MinColWidth;
                    data->cols[data->adjustcolumn+1].c->userwidth = userwidth;
                  }
                  data->do_setcols = TRUE;
                  /*data->ScrollBarsPos = -2;*/
                }
              }
            }
          }
          if (msg->imsg->MouseX < data->mleft)
          { NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left,NULL);
/*            NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,FALSE);*/
          }
          else if (msg->imsg->MouseX >= data->mright)
          { NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right,NULL);
/*            NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,FALSE);*/
          }
          else
            NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
        }
        else if ((data->adjustbar >= -4) && (data->adjustbar <= -2) && (msg->imsg->Class == IDCMP_MOUSEMOVE))
        { if ((data->adjustcolumn < data->numcols) &&
              (data->adjustcolumn >= 0))
          { WORD minx = (data->cols[data->adjustcolumn].c->minx - hfirsthpos);
            WORD maxx = (data->cols[data->adjustcolumn].c->maxx - hfirsthpos);
            WORD maxy = data->vdtitlepos + data->vdtitleheight;
            if (!data->NList_TitleSeparator)
              maxy--;
            if (data->adjustcolumn > 0)
              minx -= (data->cols[data->adjustcolumn-1].c->delta - (((data->cols[data->adjustcolumn-1].c->delta-1) / 2) + 1));
            maxx += ((data->cols[data->adjustcolumn].c->delta-1) / 2);
            if ((data->adjustcolumn >= data->NList_MinColSortable) &&
                ((data->numcols - data->NList_MinColSortable) > 1) &&
                ((msg->imsg->MouseX < data->click_x - 6) || (msg->imsg->MouseX > data->click_x + 6)) &&
                (NL_OnWindow(obj,data,msg->imsg->MouseX,msg->imsg->MouseY)))
            {
/*
 *{
 *LONG ab = (LONG) data->adjustbar;
 *LONG co = (LONG) msg->imsg->Code;
 *LONG qu = (LONG) msg->imsg->Qualifier;
 *LONG mx = (LONG) msg->imsg->MouseX;
 *LONG my = (LONG) msg->imsg->MouseY;
 *D(bug("%lx|2ab0 %ld start col sort IntuiMessage :\n",obj,ab));
 *if (msg->imsg->Class == IDCMP_MOUSEMOVE)
 *D(bug("  Class=MOUSEMOVE Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_INTUITICKS)
 *D(bug("  Class=INTUITICKS Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else
 *D(bug("  Class=%lx Code=%lx Qualifier=%lx mx=%ld my=%ld\n",msg->imsg->Class,co,qu,mx,my));
 *}
*/
              data->adjustbar = -10;

              // set a custom mouse pointer
              ShowCustomPointer(obj, data, PT_MOVE);

              data->moves = FALSE;
              drag_ok = FALSE;
              do_draw = TRUE;
            }
            else if ((msg->imsg->MouseX < data->mleft) ||
                (msg->imsg->MouseX > data->mright) ||
                (msg->imsg->MouseX < minx) ||
                (msg->imsg->MouseX > maxx) ||
                (msg->imsg->MouseY < data->vdtitlepos) ||
                (msg->imsg->MouseY > maxy))
            { data->adjustbar = -4;
              do_draw = TRUE;
              data->click_line = -2;
            }
            else if (data->adjustbar == -4)
            { data->adjustbar = -2;
              do_draw = TRUE;
              data->click_line = -2;
            }
          }
          else
          {
/*
 *{
 *LONG ab = (LONG) data->adjustbar;
 *LONG co = (LONG) msg->imsg->Code;
 *LONG qu = (LONG) msg->imsg->Qualifier;
 *LONG mx = (LONG) msg->imsg->MouseX;
 *LONG my = (LONG) msg->imsg->MouseY;
 *D(bug("%lx|3ab %ld stop IntuiMessage :\n",obj,ab));
 *if (msg->imsg->Class == IDCMP_MOUSEMOVE)
 *D(bug("  Class=MOUSEMOVE Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else if (msg->imsg->Class == IDCMP_INTUITICKS)
 *D(bug("  Class=INTUITICKS Code=%lx Qualifier=%lx mx=%ld my=%ld\n",co,qu,mx,my));
 *else
 *D(bug("  Class=%lx Code=%lx Qualifier=%lx mx=%ld my=%ld\n",msg->imsg->Class,co,qu,mx,my));
 *}
*/
            data->adjustbar = -1;
            NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,FALSE);
            do_draw = TRUE;
            data->click_line = -2;
          }
        }

        {
          struct MUI_NList_TestPos_Result res;
          res.char_number = -2;
          NL_List_TestPos(obj,data,msg->imsg->MouseX,msg->imsg->MouseY,&res);

          if(data->adjustbar == -10)
          {
            if ((res.column >= data->NList_MinColSortable) && (data->adjustcolumn >= 0) &&
                (data->adjustcolumn != res.column) && (res.xoffset > 0) &&
                (res.xoffset < data->cols[res.column].c->dx) && (res.xoffset < data->cols[data->adjustcolumn].c->dx))
            { struct colinfo *tmpc = data->cols[res.column].c;
              data->cols[res.column].c = data->cols[data->adjustcolumn].c;
              data->cols[data->adjustcolumn].c = tmpc;
              data->adjustcolumn = res.column;
              data->do_draw_all = data->do_draw_title = data->do_draw = TRUE;
              data->do_parse = data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
              do_draw = TRUE;
              /*data->ScrollBarsPos = -2;*/
              DO_NOTIFY(NTF_Columns);
            }
          }
          else
          {
            BOOL onWindow = NL_OnWindow(obj, data, msg->imsg->MouseX, msg->imsg->MouseY);

            if (((data->adjustbar >= 0) ||
                 ((!data->moves) && (data->adjustbar >= -1) &&
                  (res.flags & MUI_NLPR_BAR) && (res.flags & MUI_NLPR_ONTOP) &&
                  (res.column < data->numcols) && (res.column >= 0))) &&
                (onWindow == TRUE))
            {
              // set a custom mouse pointer
              ShowCustomPointer(obj, data, PT_SIZE);
            }
            else if(data->NList_SelectPointer == TRUE &&
                    (data->NList_TypeSelect == MUIV_NList_TypeSelect_Char) &&
                    (onWindow == TRUE))
            {
              // in case the NList object is in charwise selection mode and the mouse
              // is above the object itself we show a specialized selection pointer
              ShowCustomPointer(obj, data, PT_SELECT);
            }
            else
            {
              HideCustomPointer(obj, data);
            }
          }
        }


        if (data->moves)
        {
          WORD ly = (msg->imsg->MouseY - data->vpos);
          WORD lyl = ly / data->vinc;
          LONG lyl2 = lyl + data->NList_First;
          WORD lx = (msg->imsg->MouseX - data->hpos);
          WORD sel_x = msg->imsg->MouseX;
          WORD sel_y = msg->imsg->MouseY;
          data->NumIntuiTick = 0;
          if ((ly >= 0) && (lyl >= 0) && (lyl < data->NList_Visible) &&
              ((lx < 0) || (lx >= data->NList_Horiz_Visible)))
          { if (data->drag_border)
            { drag_ok = TRUE;
            }
            data->drag_border = FALSE;
          }
          if (drag_ok)
          { /* do nothing */
          }
          else if (lx < 0)
          { if (msg->imsg->Class == IDCMP_MOUSEMOVE)
              break;
            if (msg->imsg->MouseX < data->mleft-40)
            { MOREQUIET;
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left4,NULL);
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left4,NULL);
              LESSQUIET;
            }
            else if (msg->imsg->MouseX < data->mleft-24)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left4,NULL);
            else if (msg->imsg->MouseX < data->mleft-8)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left2,NULL);
            else
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Left,NULL);
            data->selectskiped = FALSE;
            if (data->NList_TypeSelect && (data->sel_pt[0].ent >= 0))
            { sel_x = data->hpos;
              data->last_sel_click_x = sel_x +1;
            }
          }
          else if (lx >= data->NList_Horiz_Visible)
          { if (msg->imsg->Class == IDCMP_MOUSEMOVE)
              break;
            if (msg->imsg->MouseX > data->mright+40)
            { MOREQUIET;
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right4,NULL);
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right4,NULL);
              LESSQUIET;
            }
            else if (msg->imsg->MouseX > data->mright+24)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right4,NULL);
            else if (msg->imsg->MouseX > data->mright+8)
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right2,NULL);
            else
              NL_List_Horiz_First(obj,data,MUIV_NList_Horiz_First_Right,NULL);
            data->selectskiped = FALSE;
            if (data->NList_TypeSelect && (data->sel_pt[0].ent >= 0))
            { sel_x = data->hpos + data->NList_Horiz_Visible - 1;
              data->last_sel_click_x = sel_x -1;
            }
          }
          else if (ly < 0)
          {
            long lactive = data->NList_First - 1;
            if (lactive < 0)
              lactive = 0;

            if (msg->imsg->Class == IDCMP_MOUSEMOVE)
              break;
            if (msg->imsg->MouseY < data->vdtpos-40)
            { if (data->NList_Input && !data->NList_TypeSelect)
              { MOREQUIET;
                if (data->multiselect && data->selectskiped && (lactive >= 0) && (lactive != data->lastactived))
                { NL_List_Select(obj,data,lactive,data->lastactived,data->selectmode,NULL);
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                LESSQUIET;
              }
              else
              { MOREQUIET;
                NL_List_First(obj,data,MUIV_NList_First_Up4,NULL);
                NL_List_First(obj,data,MUIV_NList_First_Up4,NULL);
                LESSQUIET;
              }
            }
            else if (msg->imsg->MouseY < data->vdtpos-24)
            { if (data->NList_Input && !data->NList_TypeSelect)
              { MOREQUIET;
                if (data->multiselect && data->selectskiped && (lactive >= 0) && (lactive != data->lastactived))
                { NL_List_Select(obj,data,lactive,data->lastactived,data->selectmode,NULL);
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                LESSQUIET;
              }
              else
                NL_List_First(obj,data,MUIV_NList_First_Up4,NULL);
            }
            else if (msg->imsg->MouseY < data->vdtpos-8)
            { if (data->NList_Input && !data->NList_TypeSelect)
              { MOREQUIET;
                if (data->multiselect && data->selectskiped && (lactive >= 0) && (lactive != data->lastactived))
                { NL_List_Select(obj,data,lactive,data->lastactived,data->selectmode,NULL);
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
                LESSQUIET;
              }
              else
                NL_List_First(obj,data,MUIV_NList_First_Up2,NULL);
            }
            else
            { if (data->NList_Input && !data->NList_TypeSelect)
                NL_List_Active(obj,data,MUIV_NList_Active_Up,NULL,data->selectmode,FALSE);
              else
                NL_List_First(obj,data,MUIV_NList_First_Up,NULL);
            }
            data->selectskiped = FALSE;
            if (data->NList_TypeSelect && (data->sel_pt[0].ent >= 0))
            { sel_y = data->vpos;
              data->last_sel_click_y = sel_y +1;
            }
          }
          else if ((lyl >= data->NList_Visible) || (lyl2 >= data->NList_Entries))
          {
            long lactive = data->NList_First + data->NList_Visible;
            if (lactive >= data->NList_Entries)
              lactive = data->NList_Entries - 1;

            if (msg->imsg->Class == IDCMP_MOUSEMOVE)
              break;

            if (msg->imsg->MouseY > data->vdbpos+40)
            { if (data->NList_Input && !data->NList_TypeSelect)
              { MOREQUIET;
                if (data->multiselect && data->selectskiped && (lactive >= 0) && (lactive != data->lastactived))
                { NL_List_Select(obj,data,lactive,data->lastactived,data->selectmode,NULL);
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                LESSQUIET;
              }
              else
              { MOREQUIET;
                NL_List_First(obj,data,MUIV_NList_First_Down4,NULL);
                NL_List_First(obj,data,MUIV_NList_First_Down4,NULL);
                LESSQUIET;
              }
            }
            else if (msg->imsg->MouseY > data->vdbpos+24)
            { if (data->NList_Input && !data->NList_TypeSelect)
              { MOREQUIET;
                if (data->multiselect && data->selectskiped && (lactive >= 0) && (lactive != data->lastactived))
                { NL_List_Select(obj,data,lactive,data->lastactived,data->selectmode,NULL);
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                LESSQUIET;
              }
              else
                NL_List_First(obj,data,MUIV_NList_First_Down4,NULL);
            }
            else if (msg->imsg->MouseY > data->vdbpos+8)
            { if (data->NList_Input && !data->NList_TypeSelect)
              { MOREQUIET;
                if (data->multiselect && data->selectskiped && (lactive >= 0) && (lactive != data->lastactived))
                { NL_List_Select(obj,data,lactive,data->lastactived,data->selectmode,NULL);
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else
                  NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
                LESSQUIET;
              }
              else
                NL_List_First(obj,data,MUIV_NList_First_Down2,NULL);
            }
            else
            { if (data->NList_Input && !data->NList_TypeSelect)
                NL_List_Active(obj,data,MUIV_NList_Active_Down,NULL,data->selectmode,FALSE);
              else
                NL_List_First(obj,data,MUIV_NList_First_Down,NULL);
            }
            data->selectskiped = FALSE;
            if (data->NList_TypeSelect && (data->sel_pt[0].ent >= 0))
            { if (data->NList_First + data->NList_Visible < data->NList_Entries)
                sel_y = data->vpos + (data->NList_Visible * data->vinc) - 1;
              else
                sel_y = data->vpos + ((data->NList_Entries-data->NList_First) * data->vinc) - 1;
              data->last_sel_click_y = sel_y -1;
            }
          }
          else if ((lyl >= 0) && (lyl < data->NList_Visible))
          {
            if (data->NList_Input && !data->NList_TypeSelect)
            {
              long lactive = lyl + data->NList_First;
              if ((lactive != data->NList_Active) && (lactive >= 0) && (lactive < data->NList_Entries))
              {
                if (data->multiselect == MUIV_NList_MultiSelect_None)
                { NL_List_Select(obj,data,data->lastactived,MUIV_NList_Active_Off,MUIV_NList_Select_Off,NULL);
                  data->selectmode = MUIV_NList_Select_On;
                }
                if (data->multiselect && data->selectskiped)
                {
                  NL_List_Select(obj,data,lactive,data->lastactived,data->selectmode,NULL);
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else if (data->multiselect)
                { data->selectskiped = TRUE;
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
                else
                {
                  NL_List_Active(obj,data,lactive,NULL,data->selectmode,FALSE);
                }
              }
            }
            if (msg->imsg->Class == IDCMP_INTUITICKS)
            {
              NL_RequestIDCMP(obj,data,IDCMP_MOUSEMOVE);
            }
            if (data->drag_type != MUIV_NList_DragType_None)
            { if (DragQual)
              { drag_ok = TRUE;
              }
              else if ((data->drag_type == MUIV_NList_DragType_Immediate) && !data->multiselect && (msg->imsg->Class == IDCMP_MOUSEMOVE))
              { drag_ok = TRUE;
              }
              else if ((data->drag_type == MUIV_NList_DragType_Immediate) || (data->drag_type == MUIV_NList_DragType_Borders))
                data->drag_border = TRUE;
            }
          }
          if (data->NList_TypeSelect && (data->sel_pt[0].ent >= 0) &&
              ((sel_x != data->last_sel_click_x) || (sel_y != data->last_sel_click_y)))
            SelectSecondPoint(obj,data,sel_x,sel_y);
        }
        /*retval = MUI_EventHandlerRC_Eat;*/
        break;
      default:
        break;
    }

    if (do_draw)
    { REDRAW;
/*      do_notifies(NTF_AllChanges);*/
    }

/*
 *   if (NEED_NOTIFY(NTF_AllClick))
 *   { //do_notifies(NTF_AllClick);
 *     //retval = MUI_EventHandlerRC_Eat;
 *   }
 */
    if (drag_ok && (data->drag_type != MUIV_NList_DragType_None) &&
        ((msg->imsg->Class == IDCMP_MOUSEBUTTONS) || (msg->imsg->Class == IDCMP_MOUSEMOVE)) &&
        (data->NList_Active >= data->NList_First) &&
        (data->NList_Active < data->NList_First + data->NList_Visible))
    {
      LONG first,last,ent,numlines,lyl;
/*D(bug("%lx| 17 drag_ok=%ld drag should start\n",obj,drag_ok));*/
      if (!data->NList_TypeSelect)
      { first = 0;
        while ((first < data->NList_Entries) && (data->EntriesArray[first]->Select == TE_Select_None))
          first++;
        last = data->NList_Entries - 1;
        while ((last > 0) && (data->EntriesArray[last]->Select == TE_Select_None))
          last--;
        { ent = first;
          numlines = 0;
          while (ent <= last)
          { if (data->EntriesArray[ent]->Select != TE_Select_None)
              numlines++;
            ent++;
          }
        }
      }
      else
      { first = data->sel_pt[data->min_sel].ent;
        last = data->sel_pt[data->max_sel].ent;
        if ((data->sel_pt[data->max_sel].column == 0) && (data->sel_pt[data->max_sel].xoffset == PMIN))
          last--;
        numlines = last - first + 1;
      }
      if ((first > 0) && (first < data->NList_Entries) && (data->EntriesArray[first]->Wrap & TE_Wrap_TmpLine))
        first -= data->EntriesArray[first]->dnum;
      if ((last > 0) && (last < data->NList_Entries) && data->EntriesArray[last]->Wrap)
      { if (data->EntriesArray[last]->Wrap & TE_Wrap_TmpLine)
          last -= data->EntriesArray[last]->dnum;
        if (last > 0)
          last += data->EntriesArray[last]->dnum - 1;
      }
      retval = MUI_EventHandlerRC_Eat;
      if ((first >= 0) && (first <= last) && (last < data->NList_Entries))
      {
        LONG dragx;
        LONG dragy;

        data->DragRPort = NULL;

        if ((first < data->NList_First) || (last >= data->NList_First + data->NList_Visible) ||
            (numlines != (last - first + 1)) || (data->NList_DragColOnly >= 0) ||
            (numlines >= data->NList_DragLines))
        {
          data->DragEntry = first;
          if ((data->NList_Active >= 0) && (data->NList_Active >= data->NList_First) && (last < data->NList_First + data->NList_Visible))
            first = data->NList_Active;
          else
            first = data->NList_First;
          last = first-1;
        }
        dragx = msg->imsg->MouseX - data->mleft;
        dragy = msg->imsg->MouseY - (data->vpos+(data->vinc * (first - data->NList_First)));

        if((data->DragRPort = CreateDragRPort(obj,data,numlines,first,last)))
        {
          if (dragx < 0)
            dragx = 0;
          else if (dragx >= data->DragWidth)
            dragx = data->DragWidth-1;
          dragy = data->DragHeight;
        }
        else
        {
          if (first >= data->NList_First + data->NList_Visible)
            first = data->NList_First + data->NList_Visible - 1;
          if (first < data->NList_First)
            first = data->NList_First;
          if (last >= data->NList_First + data->NList_Visible)
            last = data->NList_First + data->NList_Visible - 1;
          if (last < first)
            last = first;
          lyl = last - first + 1;
          _left(obj) = data->mleft;
          _top(obj) = data->vpos+(data->vinc * (first - data->NList_First));
          _width(obj) = data->mwidth;
          _height(obj) = data->vinc * lyl;
          dragx = msg->imsg->MouseX - _left(obj);
          dragy = msg->imsg->MouseY - _top(obj);
          if (dragx < 0)
            dragx = 0;
          else if (dragx >= _width(obj))
            dragx = _width(obj)-1;
          if (dragy < 0)
            dragy = 0;
          else if (dragy >= _height(obj))
            dragy = _height(obj)-1;
          data->badrport = TRUE;
        }
/*D(bug("%lx| 18 drag_ok=%ld   drag start now\n",obj,drag_ok));*/
        data->markdrawnum = MUIM_NList_Trigger;
        NL_RejectIDCMP(obj,data,IDCMP_INTUITICKS,TRUE);
        NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,TRUE);
        DoMethod(obj,MUIM_DoDrag,dragx,dragy,0L);
        NL_RequestIDCMP(obj,data,IDCMP_INTUITICKS);
        data->markdrawnum = 0;
        NL_RejectIDCMP(obj,data,IDCMP_MOUSEMOVE,FALSE);
        data->selectskiped = FALSE;
        data->moves = FALSE;
        REDRAW_FORCE;
      }
    }
  }
  /*D(bug("NL: mNL_HandleEvent() /after \n"));*/
  { ULONG DoNotify = ~NotNotify & data->DoNotify & data->Notify;
    if (data->SETUP && DoNotify && !data->pushtrigger)
    { data->pushtrigger = 1;
      DoMethod(_app(obj),MUIM_Application_PushMethod,obj,1,MUIM_NList_Trigger);
    }
  }

  return (retval);
}


ULONG mNL_CreateDragImage(struct IClass *cl,Object *obj,struct MUIP_CreateDragImage *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  ULONG retval;
  if (data->DragRPort)
  { _rp(obj) = data->DragRPort;
    _left(obj) = 0;
    _top(obj) = 0;
    _width(obj) = data->DragWidth;
    _height(obj) = data->DragHeight;
    data->badrport = TRUE;
  }
  retval = DoSuperMethodA(cl,obj,(Msg) msg);
  if (data->badrport)
  { _left(obj) = data->left;
    _top(obj) = data->top;
    _width(obj) = data->width;
    _height(obj) = data->height;
    _rp(obj) = data->rp;
    data->badrport = FALSE;
  }
  return (retval);
}


ULONG mNL_DeleteDragImage(struct IClass *cl,Object *obj,struct MUIP_DeleteDragImage *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  ULONG retval = DoSuperMethodA(cl,obj,(Msg) msg);
  if (data->DragRPort)
    DisposeDragRPort(obj,data);
  return (retval);
}


BOOL NL_Prop_First_Adjust(Object *obj,struct NLData *data)
{
  LONG lfirst,lincr;
  lfirst = data->NList_Prop_First / data->vinc;
  if ((data->NList_Prop_Wait == 0) && data->drawall_bits)
  { REDRAW;
  }
  if ((data->NList_Prop_Wait == 0) && ((lfirst * data->vinc) != data->NList_Prop_First) && (data->NList_Prop_Add != 0))
  {
    LONG Notify_VSB = WANTED_NOTIFY(NTF_VSB);
    if (data->SHOW && (data->rp->Layer->Flags & LAYERREFRESH))
      return (FALSE);
    NOWANT_NOTIFY(NTF_VSB);
    if (data->NList_Prop_Add > 0)
    {
      data->NList_Prop_First += data->NList_Prop_Add;
      lincr = data->NList_Prop_First % data->vinc;
      if ((data->NList_Prop_First / data->vinc) > lfirst)
      { lfirst++;
        lincr = 0;
        data->NList_Prop_First = lfirst * data->vinc;
        data->NList_Prop_Add = 0;
      }
    }
    else
    {
      data->NList_Prop_First += data->NList_Prop_Add;
      lincr = data->NList_Prop_First % data->vinc;
      if ((data->NList_Prop_First / data->vinc) < lfirst)
      { lincr = 0;
        data->NList_Prop_First = lfirst * data->vinc;
        data->NList_Prop_Add = 0;
      }
    }

    data->ScrollBarsTime = SCROLLBARSTIME;

    if ((data->NList_First != lfirst) || (data->NList_First_Incr != lincr))
    {
      if (data->NList_First != lfirst)
      {
        DO_NOTIFY(NTF_First);
      }

      data->NList_First = lfirst;
      data->NList_First_Incr = lincr;
      REDRAW;
/*      do_notifies(NTF_AllChanges|NTF_MinMax);*/
    }

    if (Notify_VSB)
      WANT_NOTIFY(NTF_VSB);
  }
  else if (data->NList_Prop_Wait > 0)
  {
    data->NList_Prop_Wait--;
  }
  else
    data->NList_Prop_Add = 0;
  return (FALSE);
}


ULONG mNL_Trigger(struct IClass *cl, UNUSED Object *obj, UNUSED Msg msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  /* attention, can be called with msg = NULL */
  if (data->SHOW && data->DRAW && !data->do_draw_all)
  {
    NL_Prop_First_Adjust(obj,data);
    /*do_notifies(NTF_First|NTF_Entries|NTF_MinMax);*/
    if (data->ScrollBarsTime > 0)
      data->ScrollBarsTime--;
    else
    {
      data->ScrollBarsTime = 0;
      if (!data->NList_Quiet && !data->NList_Disabled)
        NL_UpdateScrollers(obj,data,FALSE);
      data->ScrollBarsTime = SCROLLBARSTIME;
    }
  }

  if (data->SHOW && data->DRAW && !data->NList_Quiet && !data->NList_Disabled)
  { data->pushtrigger = 2;
    if (data->DRAW > 1)
      data->DRAW = 1;

    if(data->do_draw_all || data->do_draw)
    {
      // JL: here it is important that we force do_draw_all = FALSE or otherwise the following
      // REDRAW will result in a flickering if we have more than one NList object in a window.
      data->do_draw_all = FALSE;
      REDRAW;
    }
  }
  data->pushtrigger = 0;

  if (data->SETUP && (data->DoNotify & data->Notify))
    do_notifies(NTF_All|NTF_MinMax);

  return(TRUE);
}


