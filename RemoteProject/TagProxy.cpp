
#include "stdafx.h"
#include "resource.h"

#include "TagProxy.h"
#include "Project.h"


void CTagManager::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
   m_TagInfo.Init(pProject);
   m_LexInfo.Init(pProject);
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

/**
 * Return a list of possible matches.
 * This returns a list of tags that matches the name - and optionally the owner.
 * Use 'pstrOwner' to identify the type of the owner - or NULL for global search.
 * Use 'iInheritance' flag to look further into the inheritance tree for matches.
 * Use the 'dwTimeout' argument to limit the time searching for a match.
 * This function can return multiple matches. It will only return a result from
 * the Online Lexer or the CTAGS file - in that order - both never both.
 */
bool CTagManager::FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult)
{
   if( pstrName == NULL ) return false;
   if( _tcslen(pstrName) == 0 ) return false;
   if( m_LexInfo.FindItem(pstrName, pstrOwner, iInheritance, dwTimeout, aResult) ) return true;
   if( m_TagInfo.FindItem(pstrName, pstrOwner, iInheritance, dwTimeout, aResult) ) return true;
   return false;
}

/**
 * Returns a copy of the tag information.
 * The function makes a by-value copy of the TAGINFO information. This
 * is important because the TAGINFO structure contains weak references to the
 * internal tag-text memory, which may be deallocated at any time (runs in
 * the main thread). Use this function when you need to keep the tag
 * information around.
 */
void CTagManager::GetItemInfo(const TAGINFO* pTag, CTagDetails& Info)
{
   if( pTag == NULL ) return;
   if( pTag->TagSource == TAGSOURCE_LEX ) m_LexInfo.GetItemInfo(pTag, Info);
   if( pTag->TagSource == TAGSOURCE_CTAGS ) m_TagInfo.GetItemInfo(pTag, Info);
}

/**
 * Get list of global class-names and structures.
 */
bool CTagManager::GetOuterList(CSimpleValArray<TAGINFO*>& aResult)
{
   if( m_LexInfo.GetOuterList(aResult) ) return true;
   if( m_TagInfo.GetOuterList(aResult) ) return true;
   return false;
}

/**
 * Get list of global functions and enums.
 */
bool CTagManager::GetGlobalList(CSimpleValArray<TAGINFO*>& aResult)
{
   if( m_LexInfo.GetGlobalList(aResult) ) return true;
   if( m_TagInfo.GetGlobalList(aResult) ) return true;
   return false;
}

/**
 * Get members of a specific class or structure.
 */
bool CTagManager::GetMemberList(LPCTSTR pstrType, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult)
{
   if( pstrType == NULL ) return false;
   if( _tcslen(pstrType) == 0 ) return false;
   if( m_LexInfo.GetMemberList(pstrType, iInheritance, dwTimeout, aResult) ) return true;
   if( m_TagInfo.GetMemberList(pstrType, iInheritance, dwTimeout, aResult) ) return true;
   return false;
}

/**
 * Return all tags matching a pattern.
 */
bool CTagManager::GetTypeList(LPCTSTR pstrPattern, volatile bool& bCancel, CSimpleValArray<TAGINFO*>& aResult)
{
   if( pstrPattern == NULL ) return false;
   if( _tcslen(pstrPattern) == 0 ) return false;
   if( m_LexInfo.GetTypeList(pstrPattern, bCancel, aResult) ) return true;
   if( m_TagInfo.GetTypeList(pstrPattern, bCancel, aResult) ) return true;
   return false;
}

/**
 * Open a file-view from the tag information.
 */
bool CTagManager::OpenTagInView(const CTagDetails& Info)
{
   ATLASSERT(m_pProject);
   ATLASSERT(!Info.sFilename.IsEmpty());
   if( Info.sFilename.IsEmpty() ) return false;
   if( Info.iLineNum > 0 ) {
      // Line-numbers have first priority. We don't parse lineno. from
      // CTAGS files because they are too unreliable, but we will get
      // them from our own realtime C++ lexer.
      return m_pProject->OpenView(Info.sFilename, Info.iLineNum, FINDVIEW_ALL, false);
   }
   else if( m_pProject->OpenView(Info.sFilename, 1, FINDVIEW_ALL, false) ) {
      // If we don't have line-numbers, we'll just open the file
      // and search in it for the member name/declaration/reg.ex expression.
      // Hopefully one of the matches and the first appearance will be the stuff 
      // we're looking for. We have this situation because CTAGS may not contain
      // line-numbers!!
      m_pProject->DelayedLocalViewMessage(DEBUG_CMD_FINDTEXT, Info.sName, 0, SCFIND_MATCHCASE);
      m_pProject->DelayedLocalViewMessage(DEBUG_CMD_FINDTEXT, Info.sDeclaration, 0, SCFIND_MATCHCASE);
      m_pProject->DelayedLocalViewMessage(DEBUG_CMD_FINDTEXT, Info.sRegExMatch, 0, SCFIND_MATCHCASE);
      m_pProject->DelayedLocalViewMessage(DEBUG_CMD_FOLDCURSOR);
      return true;
   }
   else {
      ::MessageBeep((UINT)-1);
      return false;
   }
}

/**
 * Attempt to find implementation tag of a C++ member.
 */
bool CTagManager::FindImplementationTag(const CTagDetails& Current, CTagDetails& Info)
{
   Info.sName.Empty();
   Info.sFilename.Empty();
   if( Current.sName.IsEmpty() ) return false;
   CString sLookupName;
   sLookupName.Format(_T("%s%s%s"), Current.sBase, Current.sBase.IsEmpty() ? _T("") : _T("::"), Current.sName);
   CSimpleValArray<TAGINFO*> aList;
   FindItem(sLookupName, NULL, 0, ::GetTickCount() + 500, aList);
   for( int i = 0; i < aList.GetSize(); i++ ) {
      if( aList[i]->Type == TAGTYPE_IMPLEMENTATION ) {
         GetItemInfo(aList[i], Info);
         return true;
      }
   }
   return false;
}
