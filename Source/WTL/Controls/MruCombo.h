#if !defined(AFX_MRUCOMBO_H__20030601_9A35_F3C9_FEA9_0080AD509054__INCLUDED_)
#define AFX_MRUCOMBO_H__20030601_9A35_F3C9_FEA9_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// MRU ComboBox - Persistant combobox
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2003 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#ifndef __cplusplus
  #error WTL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLCTRLS_H__
  #error MruCombo.h requires atlctrls.h to be included first
#endif


/////////////////////////////////////////////////////////////////////////////
// 

template< class T, class TBase = CComboBox, class TWinTraits = CControlWinTraits >
class ATL_NO_VTABLE CMruComboImpl : 
   public CWindowImpl< T, TBase, TWinTraits >
{
public:
   DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

   enum { t_cchItemLen = 256 };
   enum { t_maxBuffer = 20 };
   
   struct _DocEntry
   {
      TCHAR szDocName[t_cchItemLen];
      bool operator==(const _DocEntry& de) const 
      { 
         return( ::lstrcmpi(szDocName, de.szDocName) == 0 ); 
      }
   };

   CSimpleArray<_DocEntry> m_arrDocs;
   DWORD m_nMaxEntries; // default is 5

   // Operations

   BOOL SubclassWindow(HWND hWnd)
   {
      ATLASSERT(m_hWnd==NULL);
      ATLASSERT(::IsWindow(hWnd));
      BOOL bRet = CWindowImpl< T, TBase, TWinTraits >::SubclassWindow(hWnd);
      if( bRet ) _Init();
      return bRet;
   }

   void SetMaxEntries(DWORD dwNum)
   {
      ATLASSERT(dwNum<t_maxBuffer);
      m_nMaxEntries = dwNum;
   }
   void UpdateList()
   {
      while( GetCount() > 0 ) DeleteString(0);
      for( int i = m_arrDocs.GetSize() - 1; i >= 0 ; --i ) AddString(m_arrDocs[i].szDocName);
   }
   BOOL AddToList(LPCTSTR pstrDocName = NULL)
   {
      // Grab text from combo edit control if not otherwise specified
      TCHAR szDocName[t_cchItemLen];
      if( pstrDocName == NULL ) {
         if( GetWindowTextLength() >= t_cchItemLen ) return FALSE; // Too long!
         GetWindowText(szDocName, t_cchItemLen);
         pstrDocName = szDocName;
      }

      // Add entry
      _DocEntry de;
      ::lstrcpy(de.szDocName, pstrDocName);
      if( ::lstrlen(pstrDocName) == 0 ) return FALSE; // No string
      for( int i = 0; i < m_arrDocs.GetSize(); i++ ) {
         if( ::lstrcmpi(m_arrDocs[i].szDocName, pstrDocName) == 0 ) {
            m_arrDocs.RemoveAt(i);
            break;
         }
      }
      if( m_arrDocs.GetSize() == (int) m_nMaxEntries ) m_arrDocs.RemoveAt(0);

      // Update list
      if( !m_arrDocs.Add(de) ) return FALSE;
      T* pT = static_cast<T*>(this);
      pT->UpdateList();
      return TRUE;
   }

   BOOL ReadFromRegistry(LPCTSTR pstrRegKey, LPCTSTR pstrName)
   {
      ATLASSERT(!::IsBadStringPtr(pstrRegKey,-1));
      ATLASSERT(!::IsBadStringPtr(pstrName,-1));

      CRegKey rkParent;
      if( rkParent.Open(HKEY_CURRENT_USER, pstrRegKey, KEY_READ) != ERROR_SUCCESS ) return FALSE;
      CRegKey rk;
      if( rk.Open(rkParent, pstrName, KEY_READ) != ERROR_SUCCESS ) return FALSE;

      DWORD dwRet;
#if (_ATL_VER >= 0x0700)
      LONG lRet = rk.QueryDWORDValue(_T("DocumentCount"), dwRet);
#else
      LONG lRet = rk.QueryValue(dwRet, _T("DocumentCount"));
#endif
      if( lRet != ERROR_SUCCESS ) return FALSE;
      SetMaxEntries(dwRet);

      m_arrDocs.RemoveAll();

      TCHAR szRetString[t_cchItemLen];
      for( DWORD nItem = m_nMaxEntries; nItem > 0; nItem-- ) {
         TCHAR szBuff[11];
         ::wsprintf(szBuff, _T("Document%i"), nItem);
#if (_ATL_VER >= 0x0700)
         ULONG ulCount = t_cchItemLen * sizeof(TCHAR);
         lRet = rk.QueryStringValue(szBuff, szRetString, &ulCount);
#else
         DWORD dwCount = t_cchItemLen * sizeof(TCHAR);
         lRet = rk.QueryValue(szRetString, szBuff, &dwCount);
#endif
         if( lRet == ERROR_SUCCESS ) {
            _DocEntry de;
            ::lstrcpy(de.szDocName, szRetString);
            if( ::lstrlen(szRetString) > 0 ) m_arrDocs.Add(de);
         }
      }
      rk.Close();
      rkParent.Close();

      T* pT = static_cast<T*>(this);
      pT->UpdateList();
      return TRUE;
   }
   BOOL WriteToRegistry(LPCTSTR pstrRegKey, LPCTSTR pstrName)
   {
      ATLASSERT(!::IsBadStringPtr(pstrRegKey,-1));
      ATLASSERT(!::IsBadStringPtr(pstrName,-1));

      CRegKey rkParent;
      if( rkParent.Create(HKEY_CURRENT_USER, pstrRegKey) != ERROR_SUCCESS ) return FALSE;
      CRegKey rk;
      if( rk.Create(rkParent, pstrName) != ERROR_SUCCESS ) return FALSE;

#if (_ATL_VER >= 0x0700)
      LONG lRet = rk.SetDWORDValue(_T("DocumentCount"), m_nMaxEntries);
#else
      LONG lRet = rk.SetValue(m_nMaxEntries, _T("DocumentCount"));
#endif
      ATLASSERT(lRet==ERROR_SUCCESS);

      // Set new values
      int nItem;
      for( nItem = m_arrDocs.GetSize(); nItem > 0; nItem-- ) {
         TCHAR szBuff[11];
         wsprintf(szBuff, _T("Document%i"), nItem);
         TCHAR szDocName[t_cchItemLen];
         if( GetLBText(nItem - 1, szDocName) == LB_ERR ) continue;
#if (_ATL_VER >= 0x0700)
         lRet = rk.SetStringValue(szBuff, szDocName);
#else
         lRet = rk.SetValue(szDocName, szBuff);
#endif
         ATLASSERT(lRet==ERROR_SUCCESS);
      }

      // Delete unused keys
      for( nItem = m_arrDocs.GetSize() + 1; nItem < t_maxBuffer; nItem++) {
         TCHAR szBuff[11];
         ::wsprintf(szBuff, _T("Document%i"), nItem);
         rk.DeleteValue(szBuff);
      }
      rk.Close();
      rkParent.Close();
      return TRUE;
   }

   // Implementation

   void _Init()
   {
      ATLASSERT(::IsWindow(m_hWnd));
      m_nMaxEntries = 6;
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CMruComboImpl)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();
      _Init();
      return lRes;
   }
};

class CMruComboCtrl : public CMruComboImpl<CMruComboCtrl, CComboBox, CWinTraitsOR<CBS_DROPDOWN> >
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_MruComboBox"), GetWndClassName())  
};


#endif // !defined(AFX_MRUCOMBO_H__20030601_9A35_F3C9_FEA9_0080AD509054__INCLUDED_)

