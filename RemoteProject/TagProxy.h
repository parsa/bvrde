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
   // ITagHandler

   void Init(CRemoteProject* pProject);
   bool IsLoaded() const;
   bool IsAvailable() const;
   void Clear();
   void GetItemInfo(const TAGINFO* pTag, CTagDetails& Info);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aResult);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aResult);
   bool FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult);
   bool GetMemberList(LPCTSTR pstrType, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult);
   bool GetTypeList(LPCTSTR pstrPattern, volatile bool& bCancel, CSimpleValArray<TAGINFO*>& aResult);

   // Operations

   bool OpenTagInView(const CTagDetails& Info);
   bool FindImplementationTag(const CTagDetails& Current, CTagDetails& Info);

public:
   // Data members
   CRemoteProject* m_pProject;
   CTagInfo m_TagInfo;
   CLexInfo m_LexInfo;
};


#endif // !defined(AFX_TAGPROXY_H__20040701_32A8_6FB7_CB14_0080AD509054__INCLUDED_)

