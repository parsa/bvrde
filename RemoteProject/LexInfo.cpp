
#include "StdAfx.h"
#include "resource.h"

#include "LexInfo.h"

#include "Project.h"
#include "ClassView.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

static CComAutoCriticalSection g_csTagData;

struct CLockTagDataInit
{
   CLockTagDataInit() { g_csTagData.Lock(); };
   ~CLockTagDataInit() { g_csTagData.Unlock(); };
};


/////////////////////////////////////////////////////////////////////////////
// CLexThread

DWORD CLexThread::Run()
{
   USES_CONVERSION;

   CAutoFree<CHAR> free(m_pstrText);

   // Load the CppLexer module
   CString sModule;
   sModule.Format(_T("%sCppLexer.dll"), CModulePath());
   static HINSTANCE s_hInst = ::LoadLibrary(sModule);

   CString sLexFilename = CLexInfo::GetLexFilename(m_pProject, m_sFilename, true);
   if( sLexFilename.IsEmpty() ) return 0;

   // Lex the text and write to a lex file.
   // It's not unlikely that the parse fails since it could stop at
   // normal syntax errors and less obvious LALR failures! In
   // this case we'll just ignore the content...
   typedef BOOL (APIENTRY* LPFNPARSE)(LPCSTR,LPCSTR,LPCWSTR);
   LPFNPARSE fnParse = (LPFNPARSE) ::GetProcAddress(s_hInst, "CppLexer_Parse");
   ATLASSERT(fnParse);
   if( fnParse == NULL ) return 0;
   BOOL bRes = FALSE;
   ATLTRY( bRes = fnParse(T2CA(m_sFilename), m_pstrText, sLexFilename) );
   if( !bRes ) return 0;

   // Get the new file into structured form
   LEXFILE* pFile = CLexInfo::ParseFile(m_pProject, m_sFilename);
   if( pFile == NULL ) return 0;

   // Submit tree update to project
   m_pProject->DelayedClassTreeInfo(m_sFilename, pFile);

   return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CLexInfo

CLexInfo::CLexInfo() : m_bLoaded(false)
{
}

CLexInfo::~CLexInfo()
{
   Clear();
}

void CLexInfo::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CLexInfo::IsLoaded() const
{
   return m_aFiles.GetSize() > 0;
}

bool CLexInfo::IsAvailable() const
{
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("editors.cpp.onlineScanner"), szBuffer, 31);
   return _tcscmp(szBuffer, _T("true")) == 0;
}

bool CLexInfo::MergeFile(LPCTSTR pstrFilename, LPCSTR pstrText, DWORD dwTimeout)
{
   ATLASSERT(m_pProject);

   // FIX: Empty files do appear as NULL text
   if( pstrText == NULL ) return false;
   if( pstrText[0] == '\0' ) return false;

   m_thread.Stop();
   m_thread.m_pLexInfo = this;
   m_thread.m_pProject = m_pProject;
   m_thread.m_sFilename = pstrFilename;
   m_thread.m_pstrText = pstrText;   // NOTE: The thread owns the 'pstrText' data now!
   m_thread.Start();
   m_thread.WaitForThread(dwTimeout);

   // And here we leave the c++ file parsing. The file contents has been 
   // sent to a separate thread which will do the parsing and eventually
   // notify the GUI thread to call MergeIntoTree() to re-populate the
   // ClassView tree.

   return true;
}

bool CLexInfo::MergeIntoTree(LPCTSTR pstrFilename, LEXFILE* pFile)
{
   ATLASSERT(::GetCurrentThreadId()==_pDevEnv->GetGuiThreadId());

   // Since we're changing the parse data, and the ClassView references
   // this data, we must *lock* it down so it doesn't use the old pointers...
   m_pProject->GetClassView()->Lock();

   m_pProject->GetSymbolView()->Clear();

   // Tag lock...
   {
      CLockTagDataInit lock;
      // Replace the old entry in the file list
      bool bFound = false;
      for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
         if( m_aFiles[i]->sFilename == pstrFilename ) {
            delete m_aFiles[i];
            m_aFiles[i] = pFile;
            bFound = true;
            break;
         }
      }
      // ...but this is a new file
      if( !bFound ) m_aFiles.Add(pFile);
   }

   // Signal the tree to rebuild itself!
   // This is important since we've removed some of the TAGINFO pointers
   // from the list and need to unlock the data.
   m_pProject->GetClassView()->Rebuild();
   m_pProject->GetClassView()->Unlock();

   return true;
}

void CLexInfo::Clear()
{
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) delete m_aFiles[i];
   m_aFiles.RemoveAll();
}

bool CLexInfo::FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult)
{
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));

   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   CLockTagDataInit lock;

   CString sParentType[2];            // Owner's parents (cache)
   LPCTSTR pstrParent[2] = { NULL };  // Owner's parents (inheritance)
   if( pstrOwner != NULL && --iInheritance >= 0 && ::GetTickCount() < dwTimeout ) 
   {
      CSimpleValArray<TAGINFO*> aOwner;
      if( FindItem(pstrOwner, NULL, 0, dwTimeout, aOwner) ) {
         for( int i = 0; pstrParent[0] == NULL && i < aOwner.GetSize(); i++  ) {
            switch( aOwner[i]->Type ) {
            case TAGTYPE_CLASS:
            case TAGTYPE_STRUCT:
            case TAGTYPE_TYPEDEF:
               sParentType[0] = _FindTagParent(aOwner[i], 0);
               if( !sParentType[0].IsEmpty() ) pstrParent[0] = sParentType[0];
               sParentType[1] = _FindTagParent(aOwner[i], 1);
               if( !sParentType[1].IsEmpty() ) pstrParent[1] = sParentType[1];
               break;
            }
         }
      }
   }

   CSimpleValArray<TAGINFO*> aResult1;
   CSimpleValArray<TAGINFO*> aResult2;
   CSimpleValArray<TAGINFO*> aResult3;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE* pFile = m_aFiles[i];
      // Binary search on sorted list
      int min = 0;
      int max = pFile->aTags.GetSize();
      int n = max / 2; 
      while( min < max ) { 
         int cmp = _tcscmp(pstrName, pFile->aTags[n].pstrName); 
         if( cmp == 0 ) break;
         if( cmp < 0 ) max = n; else min = n + 1;
         n = (min + max) / 2;
      }
      // Find instance range of the particular string
      if( min >= max ) continue;
      if( _tcscmp(pstrName, pFile->aTags[n].pstrName) != 0 ) continue;
      while( n > 0 && _tcscmp(pstrName, pFile->aTags[n - 1].pstrName) == 0 ) n--;
      // Add all instances if they have the right owner/inheritance/global scope.
      // We'd like the directly owned members at the top of the list.
      while( n < max && _tcscmp(pstrName, pFile->aTags[n].pstrName) == 0 ) {
         if( pstrOwner != NULL && _tcscmp(pFile->aTags[n].pstrOwner, pstrOwner) == 0 ) {
            TAGINFO* pTag = &pFile->aTags.GetData()[n];
            aResult1.Add(pTag);
         }
         else if( pstrParent[0] != NULL && _tcscmp(pFile->aTags[n].pstrOwner, pstrParent[0]) == 0 ) {
            TAGINFO* pTag = &pFile->aTags.GetData()[n];
            aResult2.Add(pTag);
         }
         else if( pstrParent[1] != NULL && _tcscmp(pFile->aTags[n].pstrOwner, pstrParent[1]) == 0 ) {
            TAGINFO* pTag = &pFile->aTags.GetData()[n];
            aResult2.Add(pTag);
         }
         else if( pstrOwner == NULL ) {
            TAGINFO* pTag = &pFile->aTags.GetData()[n];
            aResult3.Add(pTag);
         }
         n++;
      }
   }
   for( int x1 = 0; x1 < aResult1.GetSize(); x1++ ) aResult.Add(aResult1[x1]);
   for( int x2 = 0; x2 < aResult2.GetSize(); x2++ ) aResult.Add(aResult2[x2]);
   for( int x3 = 0; x3 < aResult3.GetSize(); x3++ ) aResult.Add(aResult3[x3]);

   return aResult.GetSize() > 0;
}

void CLexInfo::GetItemInfo(const TAGINFO* pTag, CTagDetails& Info)
{
   if( pTag == NULL ) return;
   Info.sName = pTag->pstrName;
   Info.TagType = pTag->Type;
   Info.Protection = pTag->Protection;
   Info.sBase = pTag->pstrOwner;
   Info.iLineNum = pTag->iLineNum;
   Info.sNamespace = pTag->pstrNamespace;
   Info.sRegExMatch = pTag->pstrRegExMatch;
   Info.sDeclaration = pTag->pstrDeclaration;
   Info.sFilename = pTag->pstrFile;
   Info.sComment = pTag->pstrComment;
   Info.sMemberOfScope = _T("");
}

bool CLexInfo::GetOuterList(CSimpleValArray<TAGINFO*>& aResult)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   CLockTagDataInit lock;

   // List all classes available in TAG file...
   // NOTE: This is doing a full filescan!
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE* pFile = m_aFiles[i];
      if( !pFile->bIncludeInBrowser ) continue;
      int nCount = pFile->aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         const TAGINFO& info = pFile->aTags[iIndex];
         if( info.pstrFile == NULL ) continue;
         switch( info.Type ) {
         case TAGTYPE_ENUM:
         case TAGTYPE_CLASS:
         case TAGTYPE_STRUCT:
         case TAGTYPE_TYPEDEF:
            TAGINFO* pTag = &pFile->aTags.GetData()[iIndex];
            aResult.Add(pTag);
            break;
         }
      }
   }

   return aResult.GetSize() > 0;
}

bool CLexInfo::GetGlobalList(CSimpleValArray<TAGINFO*>& aResult)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   CLockTagDataInit lock;

   // List all globals available in TAG file.
   // Actually we list all types - even if they are defined locally, since
   // our Class View tree does not allow multiple expansion levels.
   // NOTE: This is doing a full filescan!
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE* pFile = m_aFiles[i];
      if( !pFile->bIncludeInBrowser ) continue;
      int nCount = pFile->aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         const TAGINFO& info = pFile->aTags[iIndex];
         // Must not be parent of something
         if( info.pstrOwner[0] != '\0' ) continue;
         if( info.pstrFile == NULL ) continue;
         // Functions and variables are accepted
         switch( info.Type ) {
         case TAGTYPE_MEMBER:
         case TAGTYPE_FUNCTION:
            TAGINFO* pTag = &pFile->aTags.GetData()[iIndex];
            aResult.Add(pTag);
         }
      }
   }

   return aResult.GetSize() > 0;
}

bool CLexInfo::GetTypeList(LPCTSTR pstrPattern, volatile bool& bCancel, CSimpleValArray<TAGINFO*>& aResult)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   CLockTagDataInit lock;

   // NOTE: This is doing a full filescan!
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE* pFile = m_aFiles[i];
      if( !pFile->bIncludeInBrowser ) continue;
      int nCount = pFile->aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         const TAGINFO& info = pFile->aTags[iIndex];
         if( wildcmp(pstrPattern, info.pstrName) ) {
            TAGINFO* pTag = &pFile->aTags.GetData()[iIndex];
            aResult.Add(pTag);
         }
         if( bCancel ) return false;
      }
   }

   return aResult.GetSize() > 0;
}

bool CLexInfo::GetMemberList(LPCTSTR pstrType, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult)
{
   ATLASSERT(!::IsBadStringPtr(pstrType,-1));

   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   CLockTagDataInit lock;

   // List all classes available in TAG file...
   // NOTE: This is doing a full filescan!
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE* pFile = m_aFiles[i];
      int nCount = pFile->aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         const TAGINFO& info = pFile->aTags[iIndex];
         if( _tcscmp(info.pstrOwner, pstrType) == 0 ) {
            TAGINFO* pTag = &pFile->aTags.GetData()[iIndex];
            aResult.Add(pTag);
         }
      }
   }

   // Scan parent classes as well?
   if( --iInheritance >= 0 && ::GetTickCount() < dwTimeout ) 
   {
      CSimpleValArray<TAGINFO*> aOwner;
      if( FindItem(pstrType, NULL, 0, dwTimeout, aOwner) ) {
         for( int i = 0; i < aOwner.GetSize() && ::GetTickCount() < dwTimeout; i++ ) {
            switch( aOwner[i]->Type ) {
            case TAGTYPE_CLASS:
            case TAGTYPE_STRUCT:
            case TAGTYPE_TYPEDEF:
               {                  
                  for( int iPos = 0; iPos < 99 && ::GetTickCount() < dwTimeout; iPos++ ) {
                     CString sParent = _FindTagParent(aOwner[i], iPos);
                     if( sParent.IsEmpty() ) break;
                     GetMemberList(sParent, iInheritance, dwTimeout, aResult);
                  }
               }
               break;
            }
         }
      }
   }

   return aResult.GetSize() > 0;
}

// Implementation

CString CLexInfo::_FindTagParent(const TAGINFO* pTag, int iPos) const
{
   // Extract inheritance type.
   // We shall look for a number of tokens that are likely to be followed
   // by a class type. We need to discard real C++ keywords, but attempt
   // to look up any other type.
   // HACK: We simply scoop up the "class CFoo : public CBar" text from
   //       the decl. line. Unfortunately the lex file doesn't really carry that
   //       much information to safely determine the inheritance tree!
   static LPCTSTR pstrTokens[] = 
   {
      _T("public"),
      _T("protected"),
      _T("private"),
      _T("typedef"),
      _T("virtual"),
      _T(" :"),
      _T("::"),
      _T(","),
      NULL
   };
   static LPCTSTR pstrKeywords[] = 
   {
      _T("public"),
      _T("protected"),
      _T("private"),
      _T("typedef"),
      _T("virtual"),
      _T("template"),
      _T("class"),
      _T("typename"),
      NULL
   };
   CString sName;
   LPCTSTR pstrStart = pTag->pstrDeclaration;
   while( iPos >= 0 && pstrStart != NULL ) {
      sName = _T("");
      LPCTSTR pstrMatch = NULL;
      for( LPCTSTR* ppstrToken = pstrTokens; *ppstrToken != NULL; ppstrToken++ ) {
         LPCTSTR p = _tcsstr(pstrStart, *ppstrToken);
         if( p == NULL ) continue;
         p += _tcslen(*ppstrToken);
         while( _istspace(*p) ) p++;
         // Must not be among known C++ keywords
         for( LPCTSTR* ppstrKnown = pstrKeywords; *ppstrKnown != NULL; ppstrKnown++ ) {
            if( _tcsncmp(p, *ppstrKnown, _tcslen(*ppstrKnown)) == 0 ) { p = NULL; break; }
         }
         if( p == NULL ) continue;
         if( p < pstrMatch || pstrMatch == NULL ) pstrMatch = p;
      }
      if( pstrMatch != NULL ) {
         sName = _T("");
         while( _istalnum(*pstrMatch) || *pstrMatch == '_' ) sName += *pstrMatch++;
         pstrStart = pstrMatch;         
      }
      --iPos;
   }
   return sName;
}

void CLexInfo::_LoadTags()
{
   // NOTE: Now what is GUI stuff doing here? Well, we're delay-loading much of this
   //       stuff so we can't really predict when the tag files will load.
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_LOADTAG)));

   Clear();

   // Mark as "loaded" even if we don't actually find any
   // lex files to load. This prevent repeated attempts to
   // load the files in the future.
   m_bLoaded = true;

   // Don't load if configuration is turned off
   if( !IsAvailable() ) return;

   CLockTagDataInit lock;

   // Look for all project's lex files
   for( int i = 0; i < m_pProject->GetItemCount(); i++ ) {
      IView* pView = m_pProject->GetItem(i);
      CString sFileName;
      pView->GetFileName(sFileName.GetBufferSetLength(MAX_PATH), MAX_PATH);
      sFileName.ReleaseBuffer();
      LEXFILE* pFile = ParseFile(m_pProject, sFileName);
      if( pFile != NULL ) m_aFiles.Add(pFile);
   }

   // Look for the global lex file (contains standard/system functions).
   // We're dealing specially with the "CommonLex" filename as it will contain
   // a great deal of precompiler header information.
   if( m_aFiles.GetSize() > 0 ) {
      LEXFILE* pFile = ParseFile(m_pProject, _T("CommonLex"));
      if( pFile != NULL ) {
         pFile->bIncludeInBrowser = false;
         m_aFiles.Add(pFile); 
      }
   }
}

CString CLexInfo::GetLexFilename(CRemoteProject* pProject, LPCTSTR pstrFilename, bool bCreatePath)
{
   ATLASSERT(pProject);
   // Convert project-name and filename to valid names (strip filename characteristics).
   CString sProjectName;
   pProject->GetName(sProjectName.GetBuffer(128), 127);
   sProjectName.ReleaseBuffer();
   CString sName = ::PathFindFileName(pstrFilename);
   if( sName.IsEmpty() ) return _T("");
   // Remove illegal characters from name
   static TCHAR bad[] = { '.', '*', '?', ':', '\\', '/' };
   for( int i = 0; i < sizeof(bad) / sizeof(bad[0]); i++ ) {
      sProjectName.Replace(bad[i], '_');
      sName.Replace(bad[i], '_');
   }
   // As part of the Windows LUA compliance test we'll need to
   // write stuff in the user's private private AppData folder.
   OSVERSIONINFO ver = { sizeof(ver) };
   ::GetVersionEx(&ver);
   if( _tcscmp(pstrFilename, _T("CommonLex")) == 0 ) {
      sProjectName = _T(""); 
      ver.dwMajorVersion = 0;  // CommonLex is local; don't try to place in %AppData% folder
   }
   if( ver.dwMajorVersion < 6 ) 
   {
      CString sDocPath;
      sDocPath.Format(_T("%sLex\\%s"), CModulePath(), sProjectName);
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      CString sLexFile;
      sLexFile.Format(_T("%s\\%s.lex"), sDocPath, sName);
      return sLexFile;
   }
   else 
   {
      TCHAR szPath[MAX_PATH] = { 0 };
      ::SHGetSpecialFolderPath(NULL, szPath, CSIDL_LOCAL_APPDATA, TRUE);
      CString sDocPath;
      sDocPath.Format(_T("%s\\BVRDE"), szPath);
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      sDocPath += _T("\\Lex");
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      sDocPath += _T("\\");
      sDocPath += sProjectName;
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      CString sLexFile;
      sLexFile.Format(_T("%s\\%s.lex"), sDocPath, sName);
      return sLexFile;
   }
}

LEXFILE* CLexInfo::ParseFile(CRemoteProject* pProject, LPCTSTR pstrFilename)
{
   USES_CONVERSION;

   CString sLexFilename = GetLexFilename(pProject, pstrFilename, false);
   if( sLexFilename.IsEmpty() ) return NULL;

   LEXFILE* pFile = new LEXFILE;
   if( pFile == NULL ) return NULL;
   pFile->pData = NULL;

   LEXFILE& file = *pFile;

   CFile f;
   if( !f.Open(sLexFilename) ) {
      delete pFile;
      return NULL;
   }
   DWORD dwSize = f.GetSize();
   LPSTR pstrData = (LPSTR) malloc(dwSize + 1);
   f.Read(pstrData, dwSize);
   f.Close();
   pstrData[dwSize] = '\0';

   LPTSTR p = (LPTSTR) malloc((dwSize + 1) * sizeof(TCHAR));
   AtlA2WHelper(p, pstrData, dwSize + 1);
   free(pstrData);

   file.pData = p;
   file.sFilename = pstrFilename;
   file.bIncludeInBrowser = true;

   LPCTSTR pstrFile = NULL;
   LPCTSTR pstrNamePart = NULL;
   static LPCTSTR pstrEmpty = _T("");

   while( *p != '\0' ) {
      TAGINFO info;
      if( *p == '#' ) {
         pstrFile = p + 1;
         p = _tcschr(p, '\n');
         if( p == NULL ) break;
         *p = '\0';
         // Sanity check: filename in header must match project filename
         if( !(_tcsicmp(pstrFilename, pstrFile) == 0 || _tcscmp(pstrFilename, _T("CommonLex")) == 0) ) break;
         // Get the filename for display.
         // Common library declarations don't have a real file to back them up.
         pstrNamePart = ::PathFindFileName(pstrFile);
         if( _tcscmp(pstrFilename, _T("CommonLex")) == 0 ) pstrNamePart = pstrEmpty;
         p++;
      }
      else {
         info.TagSource = TAGSOURCE_LEX;
         info.pstrRegExMatch = pstrEmpty;
         //
         info.pstrName = p;
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         if( *p == '\0' ) break;
         switch( *p++ ) {
         case 'c': info.Type = TAGTYPE_CLASS; break;
         case 'm': info.Type = TAGTYPE_FUNCTION; break;
         case 'v': info.Type = TAGTYPE_MEMBER; break;
         case 'd': info.Type = TAGTYPE_DEFINE; break;
         case 't': info.Type = TAGTYPE_TYPEDEF; break;
         case 's': info.Type = TAGTYPE_STRUCT; break;
         case 'e': info.Type = TAGTYPE_ENUM; break;
         case 'n': info.Type = TAGTYPE_NAMESPACE; break;
         case 'x': info.Type = TAGTYPE_IMPLEMENTATION; break;
         default:  info.Type = TAGTYPE_UNKNOWN;
         }
         switch( *p++ ) {
         case 'p': info.Protection = TAGPROTECTION_PUBLIC; break;
         case 'r': info.Protection = TAGPROTECTION_PROTECTED; break;
         case 'i': info.Protection = TAGPROTECTION_PRIVATE; break;
         default:  info.Protection = TAGPROTECTION_GLOBAL;
         }
         p++;
         //
         info.pstrDeclaration = p;
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.pstrOwner = p;
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.pstrNamespace = p;
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         LPTSTR pend = _tcschr(p, '|');
         if( pend != NULL ) *pend = '\0';
         info.iLineNum = _ttoi(p);
         if( pend == NULL ) break;
         p = pend + 1;
         //
         info.pstrComment = p;
         p = _tcschr(p, '\n');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.pstrFile = pstrNamePart;
         file.aTags.Add(info);
      }
   }

   if( pstrFile == NULL || file.aTags.GetSize() == 0 ) {
      delete pFile;
      return NULL;
   }

   return pFile;
}

