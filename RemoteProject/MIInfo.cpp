
#include "StdAfx.h"

#include "MiInfo.h"

#pragma code_seg( "MISC" )


/////////////////////////////////////////////////////////////////
// CMiInfo

CMiInfo::CMiInfo(LPCTSTR pstrInput /*= NULL*/) :
   m_iSearchIndex(-1)
{
   if( pstrInput != NULL ) Parse(pstrInput);
}

// Operations

bool CMiInfo::Parse(LPCTSTR pstrInput)
{
   m_aItems.RemoveAll();
   CString sText = pstrInput;
   sText.TrimLeft(_T(","));
   if( sText.IsEmpty() ) return true;
#ifdef _DEBUG
   ::OutputDebugString(_T("GDB: "));
   ::OutputDebugString(pstrInput);
   ::OutputDebugString(_T("\n"));
#endif
   return _ParseString(sText, _T(""), _T(""), 0);
}

CString CMiInfo::GetItem(LPCTSTR pstrKey, 
                         LPCTSTR pstrGroup /*= NULL*/, 
                         LPCTSTR pstrFrame /*= NULL*/, 
                         long lPosition /*= 0*/)
{
   long lCurPos = 0;
   for( int i = 0; i < m_aItems.GetSize(); i++ ) {
      const MIINFO& info = m_aItems[i];
      if( _tcscmp(info.szKey, pstrKey) != 0 ) continue;
      if( pstrFrame && _tcscmp(info.szFrame, pstrFrame) != 0 ) continue;
      if( pstrGroup && _tcscmp(info.szGroup, pstrGroup) != 0 ) continue;
      if( lCurPos++ != lPosition ) continue;
      // Found it; remember position in list so we can continue
      // at this point using FindNext().
      m_iSearchIndex = i;
      return info.szValue;
   }
   return _T("");
}

CString CMiInfo::GetSubItem(LPCTSTR pstrKey) const
{
   ATLASSERT(m_iSearchIndex>=0 && m_iSearchIndex<m_aItems.GetSize());
   short iIndex = m_aItems[m_iSearchIndex].iIndex;
   for( int i = m_iSearchIndex; i < m_aItems.GetSize(); i++ ) {
      const MIINFO& info = m_aItems[i];
      if( info.iIndex != iIndex ) break;
      if( _tcscmp(info.szKey, pstrKey) != 0 ) continue;
      return info.szValue;
   }
   return _T("");
}

CString CMiInfo::FindNext(LPCTSTR pstrKey, 
                          LPCTSTR pstrGroup /*= NULL*/, 
                          LPCTSTR pstrFrame /*= NULL*/)
{
   ATLASSERT(m_iSearchIndex>=0);
   for( int i = m_iSearchIndex + 1; i < m_aItems.GetSize(); i++ ) {
      const MIINFO& info = m_aItems[i];
      if( _tcscmp(info.szKey, pstrKey) != 0 ) continue;
      if( pstrFrame && _tcscmp(info.szFrame, pstrFrame) != 0 ) continue;
      if( pstrGroup && _tcscmp(info.szGroup, pstrGroup) != 0 ) continue;
      m_iSearchIndex = i;
      return info.szValue;
   }
   return _T("");
}

void CMiInfo::Copy(const CMiInfo& src)
{
   m_aItems.RemoveAll();
   for( int i = 0; i < src.m_aItems.GetSize(); i++ ) {
      MIINFO data = src.m_aItems[i];
      m_aItems.Add(data);
   }
}

// Implementation

bool CMiInfo::_ParseString(CString& sText, LPCTSTR pstrGroup, LPCTSTR pstrFrame, short iIndex)
{
   while( !sText.IsEmpty() ) {     

      // Prepare run
      LPCTSTR pstrText = sText;
      int iPos = 0;
      int iEnd = 0;

      // Extract key if available.
      // There's a number of examples to consider:
      //   register-names=["eax","ecx","edx"]
      //   stack=[frame={level="0",addr="0x77f82926",func="??"}]
      //   func="main",args=[{name="a",value="1"}, {name="b",value="2"}]
      CString sKey;
      if( *pstrText != '{' 
          && *pstrText != '[' 
          && *pstrText != '\"' ) 
      {
         sKey = sText.SpanExcluding(_T("="));
         ATLASSERT(sKey.GetLength()>0 && sKey.GetLength()<MAX_MI_KEY_LEN);
         if( sKey.IsEmpty() ) return false;
         iPos = sKey.GetLength() + 1;
      }
      else
      {
         sKey = pstrFrame;
      }

      // Check for array (frame)
      if( pstrText[iPos] == '[' ) {
         iPos++;
         iEnd = iPos;
         if( !_FindBlockEnd(pstrText, iEnd) ) return false;
         if( !_ParseString(sText.Mid(iPos, iEnd - iPos - 1), pstrGroup, sKey, 0) ) return false;
         goto next;
      }
   
      // Check for nested record (group)
      while( pstrText[iPos] == '{' ) {
         iPos++;
         iEnd = iPos;
         if( !_FindBlockEnd(pstrText, iEnd) ) return false;
         if( !_ParseString(sText.Mid(iPos, iEnd - iPos - 1), sKey, pstrFrame, ++iIndex) ) return false;
         goto next;
      }

      // Extract value string
      ATLASSERT(pstrText[iPos]=='\"');
      if( pstrText[iPos] != '\"' ) return false;
      iPos++;
      iEnd = iPos;
      while( pstrText[iEnd] != '\0' && pstrText[iEnd] != '\"' ) {
         if( pstrText[iEnd] == '\\' ) iEnd++;
         iEnd++;
      }
      while( pstrText[iEnd] != '\0' && pstrText[iEnd] == '\"' ) iEnd++;

      // Add item
      {
         MIINFO info = { 0 };
         ATLASSERT(_tcslen(pstrFrame)<sizeof(info.szFrame)/sizeof(TCHAR));
         ATLASSERT(_tcslen(pstrGroup)<sizeof(info.szGroup)/sizeof(TCHAR));
         ATLASSERT(sKey.GetLength()<sizeof(info.szKey)/sizeof(TCHAR));
         _tcsncpy(info.szFrame, pstrFrame, (sizeof(info.szFrame) / sizeof(TCHAR)) - 1);
         _tcsncpy(info.szGroup, pstrGroup, (sizeof(info.szGroup) / sizeof(TCHAR)) - 1);
         _tcsncpy(info.szKey, sKey, (sizeof(info.szKey) / sizeof(TCHAR)) - 1);
         _GetPlainText(info.szValue, sText, iPos, iEnd - iPos - 1, (sizeof(info.szValue) / sizeof(TCHAR)) - 1);
         info.iIndex = iIndex;
         m_aItems.Add(info);
         //ATLTRACE("Item: %ls=%ls f=%ls g=%ls i=%ld\n", info.szKey, info.szValue, info.szFrame, info.szGroup, (long) iIndex);
      }

next:
      // Check next item
      while( pstrText[iEnd] != '\0' && pstrText[iEnd] == ',' ) iEnd++;
      sText = sText.Mid(iEnd);
   }
   return true;
}

void CMiInfo::_GetPlainText(LPTSTR pstrDest, LPCTSTR pstrSrc, int iStart, int nLen, int cchMax) const
{
   ATLASSERT(!::IsBadWritePtr(pstrDest,nLen));
   ATLASSERT(!::IsBadStringPtr(pstrSrc+iStart,-1));
   // Convert from C-style (escaped) to ASCII text.
   // Notice that only nLen chars are converted - not the entire
   // string pointed to by pstrSrc.
   pstrSrc += iStart;
   while( *pstrSrc && --nLen >= 0 && --cchMax >= 0 ) {
      if( *pstrSrc == '\\' ) {
         pstrSrc++;
         nLen--;
      }
      *pstrDest++ = *pstrSrc++;
   }
   *pstrDest = '\0';
}

bool CMiInfo::_FindBlockEnd(LPCTSTR pstrText, int& iPos) const
{
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));
   int iLevel = 1;
   while( iLevel > 0 ) {
      switch( pstrText[iPos] ) {
      case '\0':
         return false;
      case '{':
      case '[':
         iLevel++;
         break;
      case '}':
      case ']':
         iLevel--;
         break;
      case '\"':
         iPos++;
         while( pstrText[iPos] != '\0' && pstrText[iPos] != '\"' ) 
         {
            if( pstrText[iPos] == '\\' ) iPos++;
            iPos++;
         }
         if( pstrText[iPos] == '\0' ) return false;
      }
      iPos++;
   }
   return true;
}
