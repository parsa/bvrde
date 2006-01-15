
#include "StdAfx.h"
#include "resource.h"

#include "Globals.h"


BOOL AddCommandBarImages(UINT nRes)
{
   CImageListCtrl Images;
   Images.Create(16, 16, ILC_COLOR | ILC_MASK, 40, 1);
   CBitmap bmp;
   bmp.LoadBitmap(nRes);
   ATLASSERT(!bmp.IsNull());
   if( bmp.IsNull() ) return FALSE;

   HRSRC hRsrc = ::FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(nRes), (LPTSTR) RT_TOOLBAR);
   ATLASSERT(hRsrc);
   if( hRsrc == NULL ) return FALSE;
   HGLOBAL hGlobal = ::LoadResource(_Module.GetResourceInstance(), hRsrc);
   ATLASSERT(hGlobal);
   if( hGlobal == NULL ) return FALSE;

   struct _ToolBarData   // Toolbar resource data
   {
      WORD wVersion;
      WORD wWidth;
      WORD wHeight;
      WORD wItemCount;
      WORD* GetItems() { return (WORD*)(this + 1); }
   };
   _ToolBarData* pData = (_ToolBarData*) ::LockResource(hGlobal);
   if( pData == NULL ) return FALSE;
   ATLASSERT(pData->wVersion==1);

   if( Images.Add(bmp, RGB(192,192,192)) == -1) return FALSE;

   // Fill the array with command IDs
   WORD* pItems = pData->GetItems();
   int nItems = pData->wItemCount;
   int iIcon = 0;
   for( int i = 0; i < nItems; i++ ) {
      if( pItems[i] == 0 ) continue;
      CIcon icon = Images.GetIcon(iIcon++);
      ATLASSERT(!icon.IsNull());
      _pDevEnv->AddCommandBarImage(pItems[i], icon);
   }
   return TRUE;
}


BOOL MergeMenu(HMENU hMenu, HMENU hMenuSource, UINT nPosition)
{
   ATLASSERT(::IsMenu(hMenu));
   ATLASSERT(::IsMenu(hMenuSource));
   // Validaete the menus
   if( hMenu == NULL ) return FALSE;
   if( hMenuSource == NULL ) return FALSE;
   // Make sure that we start with only one separator menu-item
   UINT iStartPos = 0;
   if( (::GetMenuState(hMenuSource, 0, MF_BYPOSITION) & (MF_SEPARATOR|MF_POPUP)) == MF_SEPARATOR ) {
      if( (nPosition == 0) 
          || (::GetMenuState(hMenu, nPosition - 1, MF_BYPOSITION) & MF_SEPARATOR) != 0 ) 
      {
         iStartPos++;
      }
   }
   // Go...
   UINT nMenuItems = ::GetMenuItemCount(hMenuSource);
   for( UINT i = iStartPos; i < nMenuItems; i++ ) {
      // Get state information
      UINT state = ::GetMenuState(hMenuSource, i, MF_BYPOSITION);
      TCHAR szItemText[256] = { 0 };
      int nLen = ::GetMenuString(hMenuSource, i, szItemText, (sizeof(szItemText) / sizeof(TCHAR)) - 1, MF_BYPOSITION);
      // Is this a separator?
      if( (state & MF_POPUP) != 0 ) {
         // Strip the HIBYTE because it contains a count of items
         state = LOBYTE(state) | MF_POPUP;
         // Then create the new submenu by using recursive call
         HMENU hSubMenu = ::CreateMenu();
         MergeMenu(hSubMenu, ::GetSubMenu(hMenuSource, i), 0);
         ATLASSERT(::GetMenuItemCount(hSubMenu)>0);
         // Non-empty popup -- add it to the shared menu bar
         ::InsertMenu(hMenu, nPosition++, state | MF_BYPOSITION, (UINT) hSubMenu, szItemText);
      }
      else if( (state & MF_SEPARATOR) != 0 ) {
         ::InsertMenu(hMenu, nPosition++, state | MF_STRING | MF_BYPOSITION, 0, _T(""));
      }
      else if( nLen > 0 ) {
         // Only non-empty items should be added
         ATLASSERT(szItemText[0]!=_T('\0'));
         // Here the state does not contain a count in the HIBYTE
         ::InsertMenu(hMenu, nPosition++, state | MF_BYPOSITION, ::GetMenuItemID(hMenuSource, i), szItemText);
      }
   }
   return TRUE;
}
