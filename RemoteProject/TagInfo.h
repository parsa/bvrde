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
class CTagInfo
{
public:
   CTagInfo();
   virtual ~CTagInfo();

// Attributtes
public:
   typedef enum TAGTYPE
   {
      TAGTYPE_UNKNOWN = 0,
      TAGTYPE_CLASS,
      TAGTYPE_STRUCT,
      TAGTYPE_TYPEDEF,
      TAGTYPE_DEFINE,
      TAGTYPE_FUNCTION,
      TAGTYPE_MEMBER,
      TAGTYPE_ENUM,
   };
   typedef struct
   {
      LPCTSTR pstrName;
      LPCTSTR pstrFile;
      LPCTSTR pstrToken;
      LPCTSTR pstrFields[10];
      short nFields;
      TAGTYPE Type;
   } TAGINFO;

private:
   CRemoteProject* m_pProject;
   CSimpleArray<TAGINFO> m_aTags;
   CSimpleArray<LPTSTR> m_aFiles;
   bool m_bLoaded;
   bool m_bSorted;
   bool m_bClassBrowserDone;

// Operations
public:
   void Init(CRemoteProject* pProject);
   bool IsLoaded() const;
   bool IsTagsAvailable() const;
   void Clear();
   int GetItemCount();
   int FindItem(int iStart, LPCTSTR pstrName);
   CString GetItemDeclaration(LPCTSTR pstrName, LPCTSTR pstrOwner = NULL);
   CTagInfo::TAGTYPE GetItemType(int iIndex);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aList);
   bool GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance);

// Implementation
private:
   void _LoadTags();
   bool _ParseTagFile(LPTSTR pstrText);
   TAGTYPE _GetTagType(const TAGINFO& info) const;
   CString _GetTagParent(const TAGINFO& info) const;
};


#endif // !defined(AFX_TAGINFO_H__20030531_60AA_B46A_80C7_0080AD509054__INCLUDED_)

