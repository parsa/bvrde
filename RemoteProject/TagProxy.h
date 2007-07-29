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
   bool FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, bool bInheritance, CSimpleValArray<TAGINFO*>& aResult);
   void GetItemInfo(const TAGINFO* pTag, CTagDetails& Info);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aResult);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aResult);
   bool GetMemberList(LPCTSTR pstrType, bool bInheritance, CSimpleValArray<TAGINFO*>& aResult);

   bool OpenTagInView(TAGINFO* pTag);
   bool OpenTagInView(CTagDetails& Info);

public:
   enum
   {
      LEXTYPE_UNKNOWN,
      LEXTYPE_CTAGS,
      LEXTYPE_LEX,
   } m_LexType;

   CRemoteProject* m_pProject;
   CTagInfo m_TagInfo;
   CLexInfo m_LexInfo;
};


#endif // !defined(AFX_TAGPROXY_H__20040701_32A8_6FB7_CB14_0080AD509054__INCLUDED_)

