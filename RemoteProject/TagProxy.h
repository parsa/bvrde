#if !defined(AFX_TAGPROXY_H__20040701_32A8_6FB7_CB14_0080AD509054__INCLUDED_)
#define AFX_TAGPROXY_H__20040701_32A8_6FB7_CB14_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TagInfo.h"
#include "LexInfo.h"


class CTagManager : public ITagHandler
{
public:
   void Init(CRemoteProject* pProject);
   bool IsLoaded() const;
   bool IsAvailable() const;
   void Clear();
   TAGTYPE GetItemType(int iIndex);
   int FindItem(int iStart, LPCTSTR pstrName);
   CString GetItemDeclaration(LPCTSTR pstrName, LPCTSTR pstrOwner = NULL);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aList);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aList);
   bool GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance);

public:
   CTagInfo m_TagInfo;
   CLexInfo m_LexInfo;
   int m_iWhatType;
};


#endif // !defined(AFX_TAGPROXY_H__20040701_32A8_6FB7_CB14_0080AD509054__INCLUDED_)

