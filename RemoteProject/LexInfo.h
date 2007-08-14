#if !defined(AFX_LEXINFO_H__20040701_797A_F112_9586_0080AD509054__INCLUDED_)
#define AFX_LEXINFO_H__20040701_797A_F112_9586_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CLexInfo;
class CRemoteProject;


typedef struct LEXFILE
{
   ~LEXFILE()
   {
      free(pData);
   }
   LPTSTR pData;                         // Lex file contents
   CString sFilename;                    // Lex filename
   bool bIncludeInBrowser;               // Include this data in browser?
   CSimpleValArray<TAGINFO> aTags;       // Tag references
} LEXFILE;


/**
 * @class CLexThread
 * Parse thread.
 * This thread parses, saves and splits the file result into the internal structures
 * used by the lex lookup algorithm.
 */
class CLexThread : public CThreadImpl<CLexThread>
{
public:
   CRemoteProject* m_pProject;
   CLexInfo* m_pLexInfo;
   CString m_sFilename;
   LPCSTR m_pstrText;

   DWORD Run();
};


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

// ITagInfoHandler
public:
   void Init(CRemoteProject* pProject);
   void Clear();
   bool IsLoaded() const;
   bool IsAvailable() const;
   bool FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, bool bInheritance, CSimpleValArray<TAGINFO*>& aResult);
   void GetItemInfo(const TAGINFO* pTag, CTagDetails& Info);
   bool GetOuterList(CSimpleValArray<TAGINFO*>& aResult);
   bool GetGlobalList(CSimpleValArray<TAGINFO*>& aResult);
   bool GetMemberList(LPCTSTR pstrType, bool bInheritance, CSimpleValArray<TAGINFO*>& aResult);

// Operations
public:
   bool MergeFile(LPCTSTR pstrFilename, LPCSTR pstrText, DWORD dwTimeout);
   bool MergeIntoTree(LPCTSTR pstrFilename, LEXFILE* pFile);

   LEXFILE* _ParseFile(LPCTSTR pstrFilename) const;
   CString _GetLexFilename(LPCTSTR pstrFilename, bool bCreatePath) const;

private:
   void _LoadTags();
   CString _FindTagParent(const TAGINFO* pTag) const;

// Data Members
private:
   CRemoteProject* m_pProject;
   CSimpleArray<LEXFILE*> m_aFiles;
   CLexThread m_thread;
   bool m_bLoaded;
};


#endif // !defined(AFX_LEXINFO_H__20040701_797A_F112_9586_0080AD509054__INCLUDED_)

