
#include "stdafx.h"
#include "resource.h"

#include "TagProxy.h"
#include "Project.h"


void CTagManager::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
   m_TagInfo.Init(pProject);
   m_LexInfo.Init(pProject);
   m_LexType = LEXTYPE_UNKNOWN;
}

bool CTagManager::IsLoaded() const
{
   return m_TagInfo.IsLoaded() || m_LexInfo.IsLoaded();
}

bool CTagManager::IsAvailable() const
{
   return m_TagInfo.IsAvailable() || m_LexInfo.IsAvailable();
}

void CTagManager::Clear()
{
   m_TagInfo.Clear();
   m_LexInfo.Clear();
}

TAGTYPE CTagManager::GetItemType(int iIndex)
{
   if( m_LexType == LEXTYPE_LEX ) return m_LexInfo.GetItemType(iIndex);
   if( m_LexType == LEXTYPE_CTAGS ) return m_TagInfo.GetItemType(iIndex);
   return TAGTYPE_UNKNOWN;
}

int CTagManager::FindItem(int iStart, LPCTSTR pstrName)
{
   if( pstrName == NULL ) return -1;
   if( _tcslen(pstrName) == 0 ) return -1;
   // HACK: 'm_LexType' is a stupid hack to remember what type of 
   //       tag-handler we used for FindItem() to allow GetItemType() 
   //       to return the correct type...
   if( iStart == 0 || m_LexType == LEXTYPE_LEX ) {
      int iIndex = m_LexInfo.FindItem(iStart, pstrName);
      if( iIndex >= 0 ) {
         m_LexType = LEXTYPE_LEX;
         return iIndex;
      }
   }
   if( iStart == 0 || m_LexType == LEXTYPE_CTAGS ) {
      int iIndex = m_TagInfo.FindItem(iStart, pstrName);
      if( iIndex >= 0 ) {
         m_LexType = LEXTYPE_CTAGS;
         return iIndex;
      }
   }
   return -1;
}

bool CTagManager::GetItemDeclaration(LPCTSTR pstrName, CSimpleArray<CString>& aResult, LPCTSTR pstrOwner /*= NULL*/)
{
   if( pstrName == NULL ) return false;
   if( _tcslen(pstrName) == 0 ) return false;
   bool bRes = m_LexInfo.GetItemDeclaration(pstrName, aResult, pstrOwner);
   if( !bRes ) bRes = m_TagInfo.GetItemDeclaration(pstrName, aResult, pstrOwner);
   return bRes;
}

bool CTagManager::GetOuterList(CSimpleValArray<TAGINFO*>& aList)
{
   m_LexInfo.GetOuterList(aList);
   if( aList.GetSize() == 0 ) m_TagInfo.GetOuterList(aList);
   return true;
}

bool CTagManager::GetGlobalList(CSimpleValArray<TAGINFO*>& aList)
{
   m_LexInfo.GetGlobalList(aList);
   if( aList.GetSize() == 0 ) m_TagInfo.GetGlobalList(aList);
   return true;
}

bool CTagManager::GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance)
{
   if( pstrType == NULL ) return false;
   if( _tcslen(pstrType) == 0 ) return false;
   m_LexInfo.GetMemberList(pstrType, aList, bInheritance);
   if( aList.GetSize() == 0 ) m_TagInfo.GetMemberList(pstrType, aList, bInheritance);
   return true;
}

bool CTagManager::GoToDefinition(LPCTSTR pstrMember)
{
   CString sParent;
   CString sName;
   sParent = CString(pstrMember).SpanExcluding(_T(":"));
   CSimpleValArray<TAGINFO*> aList;
   GetMemberList(sParent, aList, true);
   if( aList.GetSize() == 0 ) return false;
   sName = CString(pstrMember + sParent.GetLength() + 2);
   for( int i = 0; i < aList.GetSize(); i++ ) {
      if( sName == aList[i]->pstrName ) return GoToDefinition(aList[i]);
   }
   return false;
}

bool CTagManager::GoToDefinition(TAGINFO* pTag)
{
   ATLASSERT(m_pProject);
   ATLASSERT(pTag);
   if( pTag->iLineNo >= 0 ) {
      // Line-numbers have first priority. We don't parse lineno. from
      // CTAGS files because they are too unreliable, but we will get
      // them from our own realtime C++ lexer.
      return m_pProject->OpenView(pTag->pstrFile, pTag->iLineNo);
   }
   else if( m_pProject->OpenView(pTag->pstrFile, 0) ) {
      // FIX: CTAGS doesn't actually produce sensible REGEX
      //      so we need to strip tokens and prepare a standard search.
      CString sToken = pTag->pstrToken;
      sToken.Replace(_T("\\/"), _T("/"));
      sToken.TrimLeft(_T("/^"));
      sToken.TrimRight(_T("$/;\""));
      int iFlags = SCFIND_MATCHCASE; //|SCFIND_REGEXP;
      m_pProject->DelayedViewMessage(DEBUG_CMD_FINDTEXT, sToken, 0, iFlags);
      m_pProject->DelayedViewMessage(DEBUG_CMD_FOLDCURSOR);
      return true;
   }
   else {
      ::MessageBeep((UINT)-1);
      return false;
   }
}

