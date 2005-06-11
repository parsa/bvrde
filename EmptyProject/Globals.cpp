
#include "Stdafx.h"
#include "resource.h"

#include "Globals.h"
#include "Files.h"



CString ToString(long lValue)
{
   TCHAR szBuffer[32];
   ::wsprintf(szBuffer, _T("%ld"), lValue);
   return szBuffer;
}

void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask /*= 0*/, DWORD dwEffects /*= 0*/, COLORREF clrText /*= 0*/)
{
   ATLASSERT(ctrlEdit.IsWindow());
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));
   ctrlEdit.HideSelection(TRUE);
   // Remove top lines if we've filled out the buffer
   GETTEXTLENGTHEX gtlx = { GTL_DEFAULT | GTL_CLOSE, 1200 };
   while( ctrlEdit.GetTextLengthEx(&gtlx) > ctrlEdit.GetLimitText() - 3000 ) {     
      LONG iStartPos = ctrlEdit.LineIndex(0);
      LONG iEndPos = ctrlEdit.LineIndex(1);
      ctrlEdit.SetSel(iStartPos, iEndPos);
      ctrlEdit.ReplaceSel(_T(""));
   }
   // Append line
   LONG iStartPos = 0;
   LONG iEndPos = 0;
   LONG iDummy = 0;
   CHARFORMAT cf;
   cf.cbSize = sizeof(CHARFORMAT);
   cf.dwMask = dwMask;;
   cf.dwEffects = dwEffects;
   cf.crTextColor = clrText;
   ctrlEdit.SetSel(-1, -1);
   ctrlEdit.GetSel(iStartPos, iDummy);
   ctrlEdit.ReplaceSel(pstrText);
   ctrlEdit.GetSel(iDummy, iEndPos);
   ctrlEdit.SetSel(iStartPos, iEndPos);
   ctrlEdit.SetSelectionCharFormat(cf);
   ctrlEdit.HideSelection(FALSE);
   ctrlEdit.SetSel(-1, -1);
}


void GenerateError(IDevEnv* pDevEnv, UINT nErr)
{
   ATLASSERT(pDevEnv);
   ATLASSERT(nErr);
   DWORD dwErr = ::GetLastError();
   CString sMsg(MAKEINTRESOURCE(nErr));
   ATLASSERT(!sMsg.IsEmpty());
   if( dwErr != 0 ) {
      CString sTemp(MAKEINTRESOURCE(IDS_ERR_LASTERROR));
      sMsg += sTemp + GetSystemErrorText(dwErr);
   }
   HWND hWnd = ::GetActiveWindow();
   CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
   pDevEnv->ShowMessageBox(hWnd, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
}


CString GetSystemErrorText(DWORD dwErr)
{
   LPVOID lpMsgBuf = NULL;
   ::FormatMessage( 
       FORMAT_MESSAGE_ALLOCATE_BUFFER | 
       FORMAT_MESSAGE_FROM_SYSTEM | 
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       dwErr,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       (LPTSTR) &lpMsgBuf,
       0,
       NULL);
   CString s = (LPCTSTR) lpMsgBuf;
   s.Remove(_T('\r'));
   s.Replace(_T("\n"), _T(" "));
   // Free the buffer.
   if( lpMsgBuf ) ::LocalFree(lpMsgBuf);
   return s;
}


BOOL MergeMenu(HMENU hMenu, HMENU hMenuSource, UINT nPosition)
{
   ATLASSERT(::IsMenu(hMenu));
   ATLASSERT(::IsMenu(hMenuSource));
   // Get the HMENU of the popup
   if( hMenu == NULL ) return FALSE;
   if( hMenuSource == NULL ) return FALSE;
   // Make sure that we start with only one separator menu-item
   UINT iStartPos = 0;
   if( (::GetMenuState(hMenuSource, 0, MF_BYPOSITION) & (MF_SEPARATOR|MF_POPUP)) == MF_SEPARATOR ) {
      if( (nPosition == 0) || 
          (::GetMenuState(hMenu, nPosition - 1, MF_BYPOSITION) & MF_SEPARATOR) != 0 ) 
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
         state = (LOBYTE(state)) | MF_POPUP;
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


CString GetFileTypeFromFilename(LPCTSTR pstrFilename)
{  
   CString sFilename = pstrFilename;
   sFilename.MakeLower();
   TCHAR szExtension[MAX_PATH];
   _tcscpy(szExtension, sFilename);
   CString sExtension = ::PathFindExtension(szExtension);  
   CString sKey;
   sKey.Format(_T("file.mappings%s"), sExtension);
   TCHAR szValue[200] = { 0 };
   if( _pDevEnv->GetProperty(sKey, szValue, 199) ) return szValue;  
   if( sExtension == _T(".") ) return _T("makefile");
   if( sFilename.Find(_T("make")) >= 0 ) return _T("makefile");
   if( sFilename.Find(_T(".make")) >= 0 ) return _T("makefile");
   return _T("text");
}


CTextFile* CreateViewFromFilename(LPCTSTR pstrFilename, 
                                  CEmptyProject* pLocalProject, 
                                  IProject* pProject, 
                                  IElement* pParent)
{
   CString sType = GetFileTypeFromFilename(pstrFilename);
   if( sType == _T("cpp") ) return new CCppFile(pLocalProject, pProject, pParent);
   if( sType == _T("header") ) return new CHeaderFile(pLocalProject, pProject, pParent);
   if( sType == _T("makefile") ) return new CMakeFile(pLocalProject, pProject, pParent);
   if( sType == _T("java") ) return new CJavaFile(pLocalProject, pProject, pParent);
   if( sType == _T("basic") ) return new CBasicFile(pLocalProject, pProject, pParent);
   if( sType == _T("pascal") ) return new CPascalFile(pLocalProject, pProject, pParent);
   if( sType == _T("python") ) return new CPythonFile(pLocalProject, pProject, pParent);
   if( sType == _T("perl") ) return new CPerlFile(pLocalProject, pProject, pParent);
   if( sType == _T("xml") ) return new CXmlFile(pLocalProject, pProject, pParent);
   if( sType == _T("html") ) return new CHtmlFile(pLocalProject, pProject, pParent);
   if( sType == _T("php") ) return new CPhpFile(pLocalProject, pProject, pParent);
   if( sType == _T("asp") ) return new CAspFile(pLocalProject, pProject, pParent);
   return new CTextFile(pLocalProject, pProject, pParent);
}
