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
   int FindItem(int iStart, LPCTSTR pstrName);
   CTagInfo::TAGTYPE GetItemType(int iIndex);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aList);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aList);
   CString GetItemDeclaration(LPCTSTR pstrName, LPCTSTR pstrOwner = NULL);
   bool GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance);

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
   bool m_bClassBrowserDone;

// Implementation
private:
   bool _LoadTags();
   bool _ParseTagFile(LPTSTR pstrText);
   TAGTYPE _GetTagType(const TAGINFO& tag) const;
   CString _GetTagParent(const TAGINFO& tag) const;
};


#endif // !defined(AFX_TAGINFO_H__20030531_60AA_B46A_80C7_0080AD509054__INCLUDED_)

