
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

bool CTagManager::FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, bool bInheritance, CSimpleValArray<TAGINFO*>& aResult)
{
   if( pstrName == NULL ) return false;
   if( _tcslen(pstrName) == 0 ) return false;
   if( m_LexInfo.FindItem(pstrName, pstrOwner, bInheritance, aResult) ) {
      m_LexType = LEXTYPE_LEX;
      return true;
   }
   if( m_TagInfo.FindItem(pstrName, pstrOwner, bInheritance, aResult) ) {
      m_LexType = LEXTYPE_CTAGS;
      return true;
   }
   m_LexType = LEXTYPE_UNKNOWN;
   return false;
}

void CTagManager::GetItemInfo(const TAGINFO* pTag, CTagDetails& Info)
{
   if( m_LexType == LEXTYPE_LEX ) m_LexInfo.GetItemInfo(pTag, Info);
   if( m_LexType == LEXTYPE_CTAGS ) m_TagInfo.GetItemInfo(pTag, Info);
}

bool CTagManager::GetOuterList(CSimpleValArray<TAGINFO*>& aResult)
{
   m_LexInfo.GetOuterList(aResult);
   if( aResult.GetSize() == 0 ) m_TagInfo.GetOuterList(aResult);
   return true;
}

bool CTagManager::GetGlobalList(CSimpleValArray<TAGINFO*>& aResult)
{
   m_LexInfo.GetGlobalList(aResult);
   if( aResult.GetSize() == 0 ) m_TagInfo.GetGlobalList(aResult);
   return true;
}

bool CTagManager::GetMemberList(LPCTSTR pstrType, bool bInheritance, CSimpleValArray<TAGINFO*>& aResult)
{
   if( pstrType == NULL ) return false;
   if( _tcslen(pstrType) == 0 ) return false;
   m_LexInfo.GetMemberList(pstrType, bInheritance, aResult);
   if( aResult.GetSize() == 0 ) m_TagInfo.GetMemberList(pstrType, bInheritance, aResult);
   return true;
}

bool CTagManager::OpenTagInView(CTagDetails& Info)
{
   if( Info.iLineNum > 0 ) {
      return m_pProject->OpenView(Info.sFilename, Info.iLineNum);
   }
   else if( m_pProject->OpenView(Info.sFilename, 0) ) {
      m_pProject->DelayedViewMessage(DEBUG_CMD_FINDTEXT, Info.sName, 0, SCFIND_MATCHCASE);
      m_pProject->DelayedViewMessage(DEBUG_CMD_FINDTEXT, Info.sDeclaration, 0, SCFIND_MATCHCASE);
      m_pProject->DelayedViewMessage(DEBUG_CMD_FOLDCURSOR);
      return true;
   }
   else {
      ::MessageBeep((UINT)-1);
      return false;
   }
}

bool CTagManager::OpenTagInView(TAGINFO* pTag)
{
   ATLASSERT(m_pProject);
   ATLASSERT(pTag);
   if( pTag->iLineNum > 0 ) {
      // Line-numbers have first priority. We don't parse lineno. from
      // CTAGS files because they are too unreliable, but we will get
      // them from our own realtime C++ lexer.
      return m_pProject->OpenView(pTag->pstrFile, pTag->iLineNum);
   }
   else if( m_pProject->OpenView(pTag->pstrFile, 0) ) {
      // If we don't have line-numbers, we'll just open the file
      // and search in it for the member name. Hopefully the
      // first appearance will be the stuff we're looking for.
      // We have this situation because CTAGS may not contain
      // line-numbers!!
      // FIX: CTAGS doesn't actually produce sensible REGEX either
      //      so we need to strip tokens and prepare a standard search.      
      CString sToken = pTag->pstrDeclaration;
      sToken.Replace(_T("\\/"), _T("/"));
      sToken.TrimLeft(_T("/^"));
      sToken.TrimRight(_T("$/;\""));
      m_pProject->DelayedViewMessage(DEBUG_CMD_FINDTEXT, sToken, 0, SCFIND_MATCHCASE);
      m_pProject->DelayedViewMessage(DEBUG_CMD_FOLDCURSOR);
      return true;
   }
   else {
      ::MessageBeep((UINT)-1);
      return false;
   }
}

