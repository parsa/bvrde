#if !defined(AFX_TAGINFO_H__20030531_60AA_B46A_80C7_0080AD509054__INCLUDED_)
#define AFX_TAGINFO_H__20030531_60AA_B46A_80C7_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////
// Forward declare

class CRemoteProject;


////////////////////////////////////////////////////////
//

/**
 * @class CTagInfo
 * Parses a CTAG file.
 * With a reference to the CRemoteProject object, the class will scan the files
 * and extract TAG information from any relevant project file.
 */
class CTagInfo : public ITagHandler
{
public:
   CTagInfo();
   virtual ~CTagInfo();

// ITagInfoHandler
public:
   void Init(CRemoteProject* pProject);
   void Clear();
   bool IsLoaded() const;
   bool IsAvailable() const;
   void GetItemInfo(const TAGINFO* pTag, CTagDetails& Info);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aResult);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aResult);
   bool FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aTags);
   bool GetMemberList(LPCTSTR pstrType, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult);
   bool GetTypeList(LPCTSTR pstrPattern, volatile bool& bCancel, CSimpleValArray<TAGINFO*>& aResult);

// Operations
public:
   bool MergeFile(LPCTSTR pstrFilename);

// Attributtes
private:
   CRemoteProject* m_pProject;
   CSimpleArray<TAGINFO> m_aTags;
   CSimpleArray<LPTSTR> m_aFiles;
   bool m_bLoaded;
   bool m_bSorted;

// Implementation
private:
   bool _LoadTags();
   bool _ParseTagFile(LPTSTR pstrText);
   void _ResolveExtraField(TAGINFO& info, LPCTSTR pstrField) const;
   CString _FindTagParent(const TAGINFO* pTag) const;
};


#endif // !defined(AFX_TAGINFO_H__20030531_60AA_B46A_80C7_0080AD509054__INCLUDED_)

