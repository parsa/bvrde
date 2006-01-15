#if !defined(AFX_LEXINFO_H__20040701_797A_F112_9586_0080AD509054__INCLUDED_)
#define AFX_LEXINFO_H__20040701_797A_F112_9586_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject;


/**
 * @class CLexInfo
 * Parses a Lex file.
 * With a reference to the CRemoteProject object, the class will scan the files
 * and extract TAG information from any relevant project file. Lex files are
 * produced by our own CppLexer module.
 */
class CLexInfo : public ITagHandler
{
public:
   CLexInfo();
   virtual ~CLexInfo();

// Types
private:
   typedef struct 
   {
      LPTSTR pData;
      CString sFilename;
      CSimpleValArray<TAGINFO> aTags;
      bool bIncludeInBrowser;
   } LEXFILE;

// ITagInfoHandler
public:
   void Init(CRemoteProject* pProject);
   void Clear();
   bool IsLoaded() const;
   bool IsAvailable() const;
   TAGTYPE GetItemType(int iIndex);
   int FindItem(int iStart, LPCTSTR pstrName);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aList);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aList);
   bool GetItemInfo(LPCTSTR pstrName, LPCTSTR pstrOwner, DWORD dwInfoType, CSimpleArray<CString>& aResult);
   bool GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance);

// Operations
public:
   bool MergeFile(LPCTSTR pstrFilename, LPCSTR pstrText);

private:
   void _LoadTags();
   CString _GetTagParent(const TAGINFO& info) const;
   bool _ParseFile(LPCTSTR pstrFilename, CString& sName, LEXFILE& file) const;

// Data Members
private:

   CRemoteProject* m_pProject;
   CSimpleArray<LEXFILE*> m_aFiles;
   bool m_bLoaded;
   int m_iFile;
};


#endif // !defined(AFX_LEXINFO_H__20040701_797A_F112_9586_0080AD509054__INCLUDED_)

