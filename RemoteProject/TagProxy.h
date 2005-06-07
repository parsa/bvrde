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
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aList);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aList);
   bool GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance);
   bool GetItemDeclaration(LPCTSTR pstrName, CSimpleArray<CString>& aResult, LPCTSTR pstrOwner = NULL);

public:
   enum
   {
      LEXTYPE_UNKNOWN,
      LEXTYPE_CTAGS,
      LEXTYPE_LEX,
   } m_LexType;
   CTagInfo m_TagInfo;
   CLexInfo m_LexInfo;
};


#endif // !defined(AFX_TAGPROXY_H__20040701_32A8_6FB7_CB14_0080AD509054__INCLUDED_)

