
#include "StdAfx.h"
#include "resource.h"

#include "MainFrm.h"
#include "Solution.h"

#include "FileMissingDlg.h"

#include "XmlSerializer.h"
#include "RegSerializer.h"

#include "Thread.h"

#ifndef PROGID_XMLDOMDocument
   #define PROGID_XMLDOMDocument L"MSXML2.DOMDocument"
#endif


/////////////////////////////////////////////////////////////////
// Global Attributes

IDevEnv* g_pDevEnv = NULL;                      /// Reference to the SDK interface
CSolution* g_pSolution = NULL;                  /// Reference to the Solution
CSimpleArray<CPlugin> g_aPlugins;               /// Collection of plugins


/////////////////////////////////////////////////////////////////
// DLL Preloader threads

class CSysDepPreloaderThread : public CThreadImpl<CSysDepPreloaderThread>
{
public:
   DWORD Run()
   {
      ::LoadLibrary(_T("NETAPI32.DLL"));
      ::LoadLibrary(_T("RASAPI32.DLL"));
      ::LoadLibrary(_T("RASMAN.DLL"));
      ::LoadLibrary(_T("RTUTILS.DLL"));
      ::LoadLibrary(_T("SHDOC32.DLL"));
      ::LoadLibrary(_T("SHDOCVW.DLL"));
      ::LoadLibrary(_T("SHDOCLC.DLL"));
      ::LoadLibrary(_T("MSHTML.DLL"));
      ::LoadLibrary(_T("MSIMG32.DLL"));
      ::LoadLibrary(_T("IEFRAME.DLL"));
      ::LoadLibrary(_T("URLMON.DLL"));
      ::LoadLibrary(_T("WININET.DLL"));
      DeleteThis();
      return 0;
   }
};

class CPlugDepPreloaderThread : public CThreadImpl<CPlugDepPreloaderThread>
{
public:
   DWORD Run()
   {
      ::LoadLibrary(_T("GENEDIT.DLL"));
      ::LoadLibrary(_T("SCILEXER.DLL"));
      ::LoadLibrary(_T("CPPLEXER.DLL"));
      DeleteThis();
      return 0;
   }
};

void PreloadPluginDependencies()
{
   CPlugDepPreloaderThread* pPreloader = new CPlugDepPreloaderThread;
   pPreloader->Start();
}


/////////////////////////////////////////////////////////////////
// Settings Loader thread

/**
 * The Settings Loader thread.
 * This thread loads the program configuration asynchronously.
 * It fetches the window position information so the main window can display
 * as the very first thing. It even preloads various system DLLs to boost 
 * startup performance.
 */
class CSettingsLoaderThread : public CThreadImpl<CSettingsLoaderThread>
{
public:
   CMainFrame* m_pMain;
   bool m_bFailed;

   CSettingsLoaderThread() : m_pMain(NULL)
   {
   }

   void Init(CMainFrame* pMain)
   {
      m_pMain = pMain;
      m_bFailed = false;
   }

   DWORD Run()
   {
      ATLASSERT(m_pMain);

      CCoInitialize cominit;

      _Init();
      _LoadStartupSettings();
      _CheckConfigVersion();
      _LoadSettings();

      CSysDepPreloaderThread* pPreloader = new CSysDepPreloaderThread;
      pPreloader->Start();

      return 0;
   }

   bool IsSuccess() const
   {
      return !m_bFailed;
   }

   void _Init()
   {
      ATLASSERT(m_pMain!=NULL);
      ATLASSERT(m_pMain->m_aProperties.GetSize()==0);

      // We'll have a resonable bucket-size for our hash table
      m_pMain->m_aProperties.Resize(513);
   }

   void _LoadStartupSettings()
   {
      // Loading of the XML settings is available in the CMainFrame class as it is also used
      // for reloading after a manual configuration change.
      m_pMain->_LoadStartupSettings();
   }

   void _CheckConfigVersion()
   {
      // This function is part of the Windows Vista LUA compliance test, where the
      // BVRDE.xml file is stored in the user's AppData folder. However, we do want
      // to warn the user about changes in the original xml file.
      // If the local configuration is missing, we'll allow the master to be copied.
      CString sDocFilename = CMainFrame::GetSettingsFilename();
      CString sOrigFilename;
      sOrigFilename.Format(_T("%sBVRDE.xml"), CModulePath());
      if( sOrigFilename == sDocFilename ) return;  // Only on Vista...
      TCHAR szDocValue[64] = { 0 };
      m_pMain->GetProperty(_T("config.timestamp"), szDocValue, (sizeof(szDocValue) / sizeof(TCHAR)) - 1);
      FILETIME ftOrig = { 0 };
      FILETIME ftUser = { 0 };
      CFile fOrig;
      if( fOrig.Open(sOrigFilename) ) {
         fOrig.GetFileTime(NULL, NULL, &ftOrig);
         fOrig.Close();
      }
      CFile fUser;
      if( fUser.Open(sDocFilename) ) {
         fUser.GetFileTime(NULL, NULL, &ftUser);
         fUser.Close();
      }
      TCHAR szOrigValue[64] = { 0 };
      ::wsprintf(szOrigValue, _T("%08X%08X"), ftOrig.dwHighDateTime, ftOrig.dwLowDateTime);
      if( _tcscmp(szDocValue, szOrigValue) != 0 || ftUser.dwLowDateTime == 0 ) {
         // Warn user that master xml file seems to have changed - we can use it instead.
         // BUG: We don't have the xml configuration and thus the language settings, so we can only present the prompt
         //      in English.
         if( _tcslen(szDocValue) == 0 || IDYES == AtlMessageBox(NULL, _T("The Master Configuration file has changed!\r\n\r\nDo you wish to copy it and use it as the current configuration?\r\nThis is recommended if you just reinstalled the tool."), _T("BVRDE"), MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND | MB_TOPMOST) ) {
            ::CopyFile(sOrigFilename, sDocFilename, FALSE);
         }
         m_pMain->SetProperty(_T("config.timestamp"), szOrigValue);
      }
   }

   void _LoadSettings()
   {
      CString sFilename = CMainFrame::GetSettingsFilename();

      // Create XML document
      CComPtr<IXMLDOMDocument> spConfigDoc;
      HRESULT Hr = spConfigDoc.CoCreateInstance(PROGID_XMLDOMDocument);
      ATLASSERT(SUCCEEDED(Hr));
      if( FAILED(Hr) || spConfigDoc == NULL ) {
         CFileMissingDlg dlg;
         dlg.DoModal();
         m_bFailed = true;
         return;
      }

      // Load from file
      spConfigDoc->put_async(VARIANT_FALSE);
      VARIANT_BOOL vbSuccess = VARIANT_FALSE;
      if( FAILED( spConfigDoc->load(CComVariant(sFilename), &vbSuccess) ) || vbSuccess == VARIANT_FALSE ) {
         AtlMessageBox(NULL, _T("Invalid Configuration File!"), _T("BVRDE"), MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
         m_bFailed = true;
         return;
      }
      if( spConfigDoc == NULL ) {
         AtlMessageBox(NULL, _T("Invalid Configuration File!"), _T("BVRDE"), MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
         m_bFailed = true;
         return;
      }

      // Open archive and load settings.
      // Delegate the actual loading to the CMainFrm implementation.
      CXmlSerializer arc;
      if( !arc.Open(spConfigDoc, _T("Settings"), sFilename) ) return;
      if( !m_pMain->_LoadSettings(arc) ) return;
      arc.Close();

      spConfigDoc.Release();
   }
};

CSettingsLoaderThread g_LoaderThread;           /// Thread that loads the settings


/////////////////////////////////////////////////////////////////
// Settings handler

void LoadSettings(CMainFrame* pMain)
{
   g_LoaderThread.Init(pMain);
   g_LoaderThread.Start();
}

bool CMainFrame::_IsSettingsLoaded() const
{
   g_LoaderThread.Stop();
   return g_LoaderThread.IsSuccess();
}

bool CMainFrame::_LoadSettings(CXmlSerializer& arc)
{
   TCHAR szBuffer[MAX_PATH] = { 0 };
   CString sKey;
   CString sValue;

   // Load Editor settings
   if( arc.ReadGroupBegin(_T("Editors")) ) 
   {
      if( arc.ReadItem(_T("General")) ) {
         sKey = _T("editors.general.");
         _AddProperty(&arc, _T("saveBeforeTool"), sKey + _T("saveBeforeTool"));
         _AddProperty(&arc, _T("savePrompt"), sKey + _T("savePrompt"));
         _AddProperty(&arc, _T("showEdge"), sKey + _T("showEdge"));
         _AddProperty(&arc, _T("bottomless"), sKey + _T("bottomless"));
         _AddProperty(&arc, _T("markCaretLine"), sKey + _T("markCaretLine"));
         _AddProperty(&arc, _T("eolMode"), sKey + _T("eolMode"));
         _AddProperty(&arc, _T("caretWidth"), sKey + _T("caretWidth"));
      }

      if( arc.ReadItem(_T("Folding")) ) {
         sKey = _T("editors.folding.");
         _AddProperty(&arc, _T("compact"), sKey + _T("compact"));
         _AddProperty(&arc, _T("atComment"), sKey + _T("atComment"));
         _AddProperty(&arc, _T("atElse"), sKey + _T("atElse"));
         _AddProperty(&arc, _T("atPreprocessor"), sKey + _T("atPreprocessor"));
      }

      // BUG: Design-flaw: All editor properties must be declared here
      //      even for custom projects/pages.
      // TODO: Route loading to generic algorithm or propagate to plugins.

      while( arc.ReadGroupBegin(_T("Editor")) ) {
         CString sLanguage;
         arc.Read(_T("name"), sLanguage);

         if( arc.ReadItem(_T("Keywords")) ) {
            ::wsprintf(szBuffer, _T("editors.%s.keywords"), sLanguage);
            _AddProperty(&arc, _T("words"), szBuffer);
         }

         if( arc.ReadItem(_T("CustomWords")) ) {
            ::wsprintf(szBuffer, _T("editors.%s.customwords"), sLanguage);
            _AddProperty(&arc, _T("words"), szBuffer);
         }

         if( arc.ReadItem(_T("Visuals")) ) {
            sKey.Format(_T("editors.%s."), sLanguage);
            _AddProperty(&arc, _T("showMargins"), sKey + _T("showMargins"));
            _AddProperty(&arc, _T("showFolding"), sKey + _T("showFolding"));
            _AddProperty(&arc, _T("showIndents"), sKey + _T("showIndents"));
            _AddProperty(&arc, _T("showLines"), sKey + _T("showLines"));
            _AddProperty(&arc, _T("wordWrap"), sKey + _T("wordWrap"));
            _AddProperty(&arc, _T("backUnindent"), sKey + _T("backUnindent"));
            _AddProperty(&arc, _T("tabIndent"), sKey + _T("tabIndent"));
            _AddProperty(&arc, _T("useTabs"), sKey + _T("useTabs"));
            _AddProperty(&arc, _T("indentMode"), sKey + _T("indentMode"));
            _AddProperty(&arc, _T("tabWidth"), sKey + _T("tabWidth"));
            _AddProperty(&arc, _T("indentWidth"), sKey + _T("indentWidth"));           
            _AddProperty(&arc, _T("readOnly"), sKey + _T("readOnly"));
         }

         if( arc.ReadItem(_T("Advanced")) ) {
            sKey.Format(_T("editors.%s."), sLanguage);
            _AddProperty(&arc, _T("noTips"), sKey + _T("noTips"));
            _AddProperty(&arc, _T("protectDebugged"), sKey + _T("protectDebugged"));
            _AddProperty(&arc, _T("matchBraces"), sKey + _T("matchBraces"));
            _AddProperty(&arc, _T("autoClose"), sKey + _T("autoClose"));
            _AddProperty(&arc, _T("markErrors"), sKey + _T("markErrors"));
            _AddProperty(&arc, _T("breakpointLines"), sKey + _T("breakpointLines"));
            _AddProperty(&arc, _T("autoCase"), sKey + _T("autoCase"));
            _AddProperty(&arc, _T("caseMode"), sKey + _T("caseMode"));
            _AddProperty(&arc, _T("autoComplete"), sKey + _T("autoComplete"));
            _AddProperty(&arc, _T("autoSuggest"), sKey + _T("autoSuggest"));
            _AddProperty(&arc, _T("classBrowser"), sKey + _T("classBrowser"));
            _AddProperty(&arc, _T("onlineScanner"), sKey + _T("onlineScanner"));
            _AddProperty(&arc, _T("ignoreViews"), sKey + _T("ignoreViews"));
            _AddProperty(&arc, _T("autoCommit"), sKey + _T("autoCommit"));
            _AddProperty(&arc, _T("terminator"), sKey + _T("terminator"));
            _AddProperty(&arc, _T("stripComments"), sKey + _T("stripComments"));
            _AddProperty(&arc, _T("backgroundLoad"), sKey + _T("backgroundLoad"));
            _AddProperty(&arc, _T("maxRecords"), sKey + _T("maxRecords"));
            _AddProperty(&arc, _T("maxErrors"), sKey + _T("maxErrors"));
         }

         // We'll allow 15 styles pr file-type
         for( long i = 1; i <= 15; i++ ) {
            ::wsprintf(szBuffer, _T("Style%ld"), i);
            if( arc.ReadItem(szBuffer) ) {
               sKey.Format(_T("editors.%s.style%ld."), sLanguage, i);
               _AddProperty(&arc, _T("name"), sKey + _T("name"));
               _AddProperty(&arc, _T("font"), sKey + _T("font"));
               _AddProperty(&arc, _T("height"), sKey + _T("height"));
               _AddProperty(&arc, _T("color"), sKey + _T("color"));
               _AddProperty(&arc, _T("back"), sKey + _T("back"));
               _AddProperty(&arc, _T("bold"), sKey + _T("bold"));
               _AddProperty(&arc, _T("italic"), sKey + _T("italic"));
            }
         }

         arc.ReadGroupEnd();
      }
      arc.ReadGroupEnd();
   }

   // Load SourceControl settings
   if( arc.ReadGroupBegin(_T("SourceControl")) ) 
   {
      _AddProperty(&arc, _T("type"), _T("sourcecontrol.type"));
      _AddProperty(&arc, _T("enable"), _T("sourcecontrol.enable"));
      if( arc.ReadGroupBegin(_T("Settings")) ) 
      {
         _AddProperty(&arc, _T("output"), _T("sourcecontrol.output"));
         _AddProperty(&arc, _T("program"), _T("sourcecontrol.program"));
         _AddProperty(&arc, _T("checkin"), _T("sourcecontrol.cmd.checkin"));
         _AddProperty(&arc, _T("checkout"), _T("sourcecontrol.cmd.checkout"));
         _AddProperty(&arc, _T("update"), _T("sourcecontrol.cmd.update"));
         _AddProperty(&arc, _T("addfile"), _T("sourcecontrol.cmd.addfile"));
         _AddProperty(&arc, _T("removefile"), _T("sourcecontrol.cmd.removefile"));
         _AddProperty(&arc, _T("status"), _T("sourcecontrol.cmd.status"));
         _AddProperty(&arc, _T("diff"), _T("sourcecontrol.cmd.diff"));
         _AddProperty(&arc, _T("login"), _T("sourcecontrol.cmd.login"));
         _AddProperty(&arc, _T("logout"), _T("sourcecontrol.cmd.logout"));
         _AddProperty(&arc, _T("message"), _T("sourcecontrol.opt.message"));
         _AddProperty(&arc, _T("recursive"), _T("sourcecontrol.opt.recursive"));
         _AddProperty(&arc, _T("sticky"), _T("sourcecontrol.opt.sticky"));
         _AddProperty(&arc, _T("updatedirs"), _T("sourcecontrol.opt.updatedirs"));
         _AddProperty(&arc, _T("branch"), _T("sourcecontrol.opt.branch"));
         _AddProperty(&arc, _T("general"), _T("sourcecontrol.opt.general"));
         _AddProperty(&arc, _T("browseAll"), _T("sourcecontrol.browse.all"));
         _AddProperty(&arc, _T("browseSingle"), _T("sourcecontrol.browse.single"));
         arc.ReadGroupEnd();
      }
      if( arc.ReadGroupBegin(_T("DiffView")) ) 
      {
         _AddProperty(&arc, _T("font"), _T("sourcecontrol.diffview.font"));
         _AddProperty(&arc, _T("fontsize"), _T("sourcecontrol.diffview.fontsize"));
         _AddProperty(&arc, _T("wordwrap"), _T("sourcecontrol.diffview.wordwrap"));
         _AddProperty(&arc, _T("listUnchanged"), _T("sourcecontrol.diffview.listUnchanged"));
         arc.ReadGroupEnd();
      }
      arc.ReadGroupEnd();
   }

   // Load main GUI layout & settings
   if( arc.ReadGroupBegin(_T("Gui")) ) 
   {
      _AddProperty(&arc, _T("client"), _T("gui.main.client"));
      _AddProperty(&arc, _T("start"), _T("gui.main.start"));
      arc.ReadGroupEnd();
   }

   // Load general document handling settings
   if( arc.ReadGroupBegin(_T("Document")) ) 
   {
      _AddProperty(&arc, _T("protectReadOnly"), _T("gui.document.protectReadOnly"));
      _AddProperty(&arc, _T("detectChange"), _T("gui.document.detectChange"));
      _AddProperty(&arc, _T("autoLoad"), _T("gui.document.autoLoad"));
      _AddProperty(&arc, _T("findText"), _T("gui.document.findText"));
      _AddProperty(&arc, _T("findMessages"), _T("gui.document.findMessages"));
      _AddProperty(&arc, _T("clearUndo"), _T("gui.document.clearUndo"));
      arc.ReadGroupEnd();
   }

   // Load File Extension mappings
   if( arc.ReadGroupBegin(_T("FileMappings")) ) 
   {
      CString sKey;
      CString sType;
      CString sExt;
      while( arc.ReadGroupBegin(_T("FileMapping")) ) {
         arc.Read(_T("ext"), sExt);
         arc.Read(_T("type"), sType);
         sKey.Format(_T("file.mappings.%s"), sExt);
         SetProperty(sKey, sType);
         arc.ReadGroupEnd();
      }
      arc.ReadGroupEnd();
   }

   // Load printing settings
   if( arc.ReadGroupBegin(_T("Printing")) ) 
   {
      _AddProperty(&arc, _T("wordWrap"), _T("priting.general.wordWrap"));
      _AddProperty(&arc, _T("colors"), _T("priting.general.colors"));
      arc.ReadGroupEnd();
   }

   // Load Key mappings
   if( arc.ReadGroupBegin(_T("KeyMappings")) ) 
   {
      ACCEL accel[100];
      int nCount = 0;    
      while( arc.ReadGroupBegin(_T("Key")) ) {
         long lCmd;
         long lKey;
         long lFlags;
         arc.Read(_T("cmd"), lCmd);
         arc.Read(_T("key"), lKey);
         arc.Read(_T("flags"), lFlags);
         arc.ReadGroupEnd();

         accel[nCount].cmd = (WORD) lCmd;
         accel[nCount].key = (WORD) lKey;
         accel[nCount].fVirt = (BYTE) lFlags;
         nCount++;
         if( nCount == sizeof(accel)/sizeof(ACCEL) ) break;
      }
      arc.ReadGroupEnd();

      if( !m_UserAccel.IsNull() ) m_UserAccel.DestroyObject();
      m_UserAccel = (nCount == 0 ? NULL : (::CreateAcceleratorTable(accel, nCount)));
   }

   // Load Macro mappings
   if( arc.ReadGroupBegin(_T("MacroMappings")) ) 
   {
      ACCEL accel[15];
      int nCount = 0;    
      while( arc.ReadGroupBegin(_T("Macro")) ) {
         long lCmd = 0;
         long lKey = 0;
         long lFlags = 0;
         arc.Read(_T("cmd"), lCmd);
         arc.Read(_T("key"), lKey);
         arc.Read(_T("flags"), lFlags);
         arc.ReadGroupEnd();

         accel[nCount].cmd = (WORD) lCmd;
         accel[nCount].key = (WORD) lKey;
         accel[nCount].fVirt = (BYTE) lFlags;
         nCount++;
         if( nCount == sizeof(accel)/sizeof(ACCEL) ) break;
      }
      arc.ReadGroupEnd();

      if( !m_MacroAccel.IsNull() ) m_MacroAccel.DestroyObject();
      m_MacroAccel = (nCount == 0 ? NULL : (::CreateAcceleratorTable(accel, nCount)));
   }

   // Load AutoText
   if( arc.ReadGroupBegin(_T("AutoText")) ) 
   {
      long i = 1;
      while( arc.ReadGroupBegin(_T("Text")) ) {
         sKey.Format(_T("autotext.entry%ld."), i++);
         _AddProperty(&arc, _T("name"), sKey + _T("name"));
         _AddProperty(&arc, _T("text"), sKey + _T("text"));
         arc.ReadGroupEnd();
      }
      arc.ReadGroupEnd();
   }

   // Load Toolbar settings
   if( arc.ReadGroupBegin(_T("ToolBars")) ) 
   {
      while( arc.ReadGroupBegin(_T("ToolBar")) ) {
         CString sID;
         arc.Read(_T("id"), sID);
         sKey.Format(_T("window.toolbar.%s."), sID);
         _AddProperty(&arc, _T("name"), sKey + _T("name"));
         _AddProperty(&arc, _T("show"), sKey + _T("show"));
         _AddProperty(&arc, _T("position"), sKey + _T("position"));
         _AddProperty(&arc, _T("newRow"), sKey + _T("newRow"));
         arc.ReadGroupEnd();
      }
      arc.ReadGroupEnd();
   }

   return true;
}

void CMainFrame::_LoadStartupSettings()
{
   // Default printer setup
   m_hDevMode = NULL;
   m_hDevNames = NULL;
   ::SetRectEmpty(&m_rcPageMargins);

   // Read mutable properties
   CRegSerializer reg;
   if( reg.Open(REG_BVRDE) ) {
      // Load Window positions
      if( reg.ReadGroupBegin(_T("Settings")) ) {         
         _AddProperty(&reg, _T("language"), _T("gui.main.language"));
         _AddProperty(&reg, _T("multiInstance"), _T("gui.main.multiInstance"));
         _AddProperty(&reg, _T("windowpos"), _T("window.main.position"));
         _AddProperty(&reg, _T("autohide-cx"), _T("window.autohide.cx"));
         _AddProperty(&reg, _T("autohide-cy"), _T("window.autohide.cy"));
         _AddProperty(&reg, _T("pane-left"), _T("window.pane.left"));
         _AddProperty(&reg, _T("pane-top"), _T("window.pane.top"));
         _AddProperty(&reg, _T("pane-right"), _T("window.pane.right"));
         _AddProperty(&reg, _T("pane-bottom"), _T("window.pane.bottom"));
         _AddProperty(&reg, _T("properties-cy"), _T("window.properties.cy"));
         _AddProperty(&reg, _T("properties-pos"), _T("window.properties.pos"));
         _AddProperty(&reg, _T("properties-area"), _T("window.properties.area"));
         _AddProperty(&reg, _T("explorer-cy"), _T("window.explorer.cy"));
         _AddProperty(&reg, _T("explorer-pos"), _T("window.explorer.pos"));
         _AddProperty(&reg, _T("explorer-area"), _T("window.explorer.area"));
         _AddProperty(&reg, _T("openfiles-cy"), _T("window.openfiles.cy"));
         _AddProperty(&reg, _T("openfiles-pos"), _T("window.openfiles.pos"));
         _AddProperty(&reg, _T("openfiles-area"), _T("window.openfiles.area"));
         _AddProperty(&reg, _T("classview-sort"), _T("window.classview.sort"));
         _AddProperty(&reg, _T("symbolview-show"), _T("window.symbolview.show"));
         _AddProperty(&reg, _T("symbolview-sort"), _T("window.symbolview.sort"));
         _AddProperty(&reg, _T("config-timestamp"), _T("config.timestamp"));
         reg.ReadGroupEnd();
      }
      // Load debugview settings
      if( reg.ReadGroupBegin(_T("DebugViews")) )
      {
          _AddProperty(&reg, _T("showWatch"), _T("gui.debugViews.showWatch"));
          _AddProperty(&reg, _T("showStack"), _T("gui.debugViews.showStack"));
          _AddProperty(&reg, _T("showThread"), _T("gui.debugViews.showThread"));
          _AddProperty(&reg, _T("showMemory"), _T("gui.debugViews.showMemory"));
          _AddProperty(&reg, _T("showOutput"), _T("gui.debugViews.showOutput"));
          _AddProperty(&reg, _T("showVariable"), _T("gui.debugViews.showVariable"));
          _AddProperty(&reg, _T("showRegister"), _T("gui.debugViews.showRegister"));
          _AddProperty(&reg, _T("showBreakpoint"), _T("gui.debugViews.showBreakpoint"));
          _AddProperty(&reg, _T("showDisassembly"), _T("gui.debugViews.showDisassembly"));
          reg.ReadGroupEnd();
      }
      reg.Close();
   }
}

void CMainFrame::_SaveStartupSettings()
{
   CRegSerializer reg;
   if( reg.Create(REG_BVRDE) ) {
      reg.WriteGroupBegin(_T("Settings"));
      _StoreProperty(&reg, _T("language"), _T("gui.main.language"));
      _StoreProperty(&reg, _T("multiInstance"), _T("gui.main.multiInstance"));
      _StoreProperty(&reg, _T("windowpos"), _T("window.main.position"));
      _StoreProperty(&reg, _T("autohide-cx"), _T("window.autohide.cx"));
      _StoreProperty(&reg, _T("autohide-cy"), _T("window.autohide.cy"));
      _StoreProperty(&reg, _T("pane-left"), _T("window.pane.left"));
      _StoreProperty(&reg, _T("pane-top"), _T("window.pane.top"));
      _StoreProperty(&reg, _T("pane-right"), _T("window.pane.right"));
      _StoreProperty(&reg, _T("pane-bottom"), _T("window.pane.bottom"));
      _StoreProperty(&reg, _T("properties-cy"), _T("window.properties.cy"));
      _StoreProperty(&reg, _T("properties-pos"), _T("window.properties.pos"));
      _StoreProperty(&reg, _T("properties-area"), _T("window.properties.area"));
      _StoreProperty(&reg, _T("explorer-cy"), _T("window.explorer.cy"));
      _StoreProperty(&reg, _T("explorer-pos"), _T("window.explorer.pos"));
      _StoreProperty(&reg, _T("explorer-area"), _T("window.explorer.area"));
      _StoreProperty(&reg, _T("openfiles-cy"), _T("window.openfiles.cy"));
      _StoreProperty(&reg, _T("openfiles-pos"), _T("window.openfiles.pos"));
      _StoreProperty(&reg, _T("openfiles-area"), _T("window.openfiles.area"));
      _StoreProperty(&reg, _T("classview-sort"), _T("window.classview.sort"));
      _StoreProperty(&reg, _T("symbolview-show"), _T("window.symbolview.show"));
      _StoreProperty(&reg, _T("symbolview-sort"), _T("window.symbolview.sort"));
      _StoreProperty(&reg, _T("config-timestamp"), _T("config.timestamp"));
      reg.WriteGroupEnd();
      reg.WriteGroupBegin(_T("DebugViews"));
      _StoreProperty(&reg, _T("showWatch"), _T("gui.debugViews.showWatch"));
      _StoreProperty(&reg, _T("showStack"), _T("gui.debugViews.showStack"));
      _StoreProperty(&reg, _T("showThread"), _T("gui.debugViews.showThread"));
      _StoreProperty(&reg, _T("showMemory"), _T("gui.debugViews.showMemory"));
      _StoreProperty(&reg, _T("showOutput"), _T("gui.debugViews.showOutput"));
      _StoreProperty(&reg, _T("showVariable"), _T("gui.debugViews.showVariable"));
      _StoreProperty(&reg, _T("showRegister"), _T("gui.debugViews.showRegister"));
      _StoreProperty(&reg, _T("showDisassembly"), _T("gui.debugViews.showDisassembly"));
      _StoreProperty(&reg, _T("showBreakpoint"), _T("gui.debugViews.showBreakpoint"));
      reg.WriteGroupEnd();
   }

   if( m_hDevMode != NULL ) ::GlobalFree(m_hDevMode);
   if( m_hDevNames != NULL ) ::GlobalFree(m_hDevNames);
}

CString CMainFrame::GetSettingsFilename()
{
   CString sFilename;
   sFilename.Format(_T("%sBVRDE.xml"), CModulePath());
   // Windows Vista or better. Need to be Windows Vista LUA (Least-privilege User Account)
   // compliant so we cannot write to the Program Files folder. Instead we'll suggest
   // that the BVRDE.xml file is loaded from the user's private AppData folder
   static OSVERSIONINFO ver = { sizeof(ver) };
   if( ver.dwMajorVersion == 0 ) ::GetVersionEx(&ver);
   if( ver.dwMajorVersion < 6 ) return sFilename;
   // Get user's private AppData folder
   TCHAR szPath[MAX_PATH] = { 0 };
   ::SHGetSpecialFolderPath(NULL, szPath, CSIDL_LOCAL_APPDATA, TRUE);
   CString sDocPath, sDocFilename;
   sDocPath.Format(_T("%s\\BVRDE"), szPath);
   sDocFilename.Format(_T("%s\\BVRDE.xml"), sDocPath);
   ::CreateDirectory(sDocPath, NULL);
   return sDocFilename;
}

void CMainFrame::_AddProperty(ISerializable* pArc, LPCTSTR pstrAttribute, LPCTSTR pstrKey)
{
   ATLASSERT(!::IsBadStringPtr(pstrAttribute,-1));
   ATLASSERT(!::IsBadStringPtr(pstrKey,-1));
   ATLASSERT(pArc);
   TCHAR szValue[2048];
   szValue[0] = '\0';
   szValue[2047] = '\0';
   pArc->Read(pstrAttribute, szValue, 2047);
   SetProperty(pstrKey, szValue);
}

void CMainFrame::_StoreProperty(ISerializable* pArc, LPCTSTR pstrAttribute, LPCTSTR pstrKey)
{
   ATLASSERT(!::IsBadStringPtr(pstrAttribute,-1));
   ATLASSERT(!::IsBadStringPtr(pstrKey,-1));
   ATLASSERT(pArc);
   TCHAR szValue[256] = { 0 };
   GetProperty(pstrKey, szValue, (sizeof(szValue)/sizeof(TCHAR))-1);
   pArc->Write(pstrAttribute, szValue);
}

BOOL CMainFrame::LoadWindowPos()
{
   TCHAR szBuffer[128] = { 0 };
   GetProperty(_T("window.main.position"), szBuffer, 127);
   CWindowPlacement pos;
   pos.GetPosData(szBuffer);
   if( pos.rcNormalPosition.right == 0 ) return FALSE;
   return pos.SetPosData(m_hWnd);
}

void CMainFrame::_LoadUIState()
{
   TCHAR szBuffer[64] = { 0 };
   GetProperty(_T("window.autohide.cx"), szBuffer, 63);
   if( _ttol(szBuffer) > 0 ) m_AutoHide.SetPaneSize(AUTOHIDE_LEFT, _ttoi(szBuffer));
   GetProperty(_T("window.autohide.cy"), szBuffer, 63);
   if( _ttol(szBuffer) > 0 ) m_AutoHide.SetPaneSize(AUTOHIDE_BOTTOM, _ttoi(szBuffer));
   GetProperty(_T("window.pane.left"), szBuffer, 63);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_LEFT, _ttoi(szBuffer));
   GetProperty(_T("pwindow.pane.top"), szBuffer, 63);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_TOP, _ttoi(szBuffer));
   GetProperty(_T("window.pane.right"), szBuffer, 63);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_RIGHT, _ttoi(szBuffer));
   GetProperty(_T("window.pane.bottom"), szBuffer, 63);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_BOTTOM, _ttoi(szBuffer));
}

void CMainFrame::_SaveUIState()
{
   TCHAR szBuffer[MAX_PATH] = { 0 };

   CWindowPlacement pos;
   pos.GetPosData(m_hWnd);
   pos.SetPosData(szBuffer, 128);
   SetProperty(_T("window.main.position"), szBuffer);
   ::wsprintf(szBuffer, _T("%ld"), m_AutoHide.GetPaneSize(AUTOHIDE_LEFT));

   SetProperty(_T("window.autohide.cx"), szBuffer);
   ::wsprintf(szBuffer, _T("%ld"), m_AutoHide.GetPaneSize(AUTOHIDE_BOTTOM));
   SetProperty(_T("window.autohide.cy"), szBuffer);
   ::wsprintf(szBuffer, _T("%ld"), m_Dock.GetPaneSize(DOCK_LEFT));
   SetProperty(_T("window.pane.left"), szBuffer);
   ::wsprintf(szBuffer, _T("%ld"), m_Dock.GetPaneSize(DOCK_TOP));
   SetProperty(_T("window.pane.top"), szBuffer);
   ::wsprintf(szBuffer, _T("%ld"), m_Dock.GetPaneSize(DOCK_RIGHT));
   SetProperty(_T("window.pane.right"), szBuffer);
   ::wsprintf(szBuffer, _T("%ld"), m_Dock.GetPaneSize(DOCK_BOTTOM));
   SetProperty(_T("window.pane.bottom"), szBuffer);

   RECT rcPane = { 0 };
   int iDockState = 0;
   m_Dock.GetWindowState(m_viewExplorer, iDockState, rcPane);
   if( iDockState < IDE_DOCK_FLOAT || iDockState == IDE_DOCK_HIDE  ) {
      ::wsprintf(szBuffer, _T("%ld"), rcPane.bottom - rcPane.top);
      SetProperty(_T("window.explorer.cy"), szBuffer);
      ::wsprintf(szBuffer, _T("%ld"), m_Dock.GetPaneHeight((short)iDockState));
      SetProperty(_T("window.explorer.area"), szBuffer);
      ::wsprintf(szBuffer, _T("%ld"), iDockState);
      SetProperty(_T("window.explorer.pos"), szBuffer);
   }
   m_Dock.GetWindowState(m_viewProperties, iDockState, rcPane);
   if( iDockState < IDE_DOCK_FLOAT || iDockState == IDE_DOCK_HIDE ) {
      ::wsprintf(szBuffer, _T("%ld"), rcPane.bottom - rcPane.top);
      SetProperty(_T("window.properties.cy"), szBuffer);
      ::wsprintf(szBuffer, _T("%ld"), m_Dock.GetPaneHeight((short)iDockState));
      SetProperty(_T("window.properties.area"), szBuffer);
      ::wsprintf(szBuffer, _T("%ld"), iDockState);
      SetProperty(_T("window.properties.pos"), szBuffer);
   }
}

void CMainFrame::_SaveToolBarState()
{
   CString sFilename = GetSettingsFilename();
   CXmlSerializer arc;
   if( !arc.Open(_T("Settings"), sFilename) ) return;
   if( arc.ReadGroupBegin(_T("ToolBars")) ) {
      while( arc.Delete(_T("ToolBar")) ) /* */;
      for( int i = 0;  i < m_aToolBars.GetSize(); i++ ) {
         const TOOLBAR& tb = m_aToolBars[i];
         for( UINT iPos = 0; iPos < m_Rebar.GetBandCount(); iPos++ ) {
            REBARBANDINFO rbbi = { 0 };
            rbbi.cbSize = sizeof(rbbi);
            rbbi.fMask = RBBIM_CHILD | RBBIM_ID | RBBIM_STYLE;
            m_Rebar.GetBandInfo(iPos, &rbbi);
            if( rbbi.hwndChild != tb.hWnd ) continue;
            arc.WriteItem(_T("ToolBar"));
            arc.Write(_T("id"), tb.szID);
            arc.Write(_T("name"), tb.szName);
            arc.Write(_T("position"), (long) iPos);
            arc.Write(_T("show"), (rbbi.fStyle & RBBS_HIDDEN) == 0 ? _T("true") : _T("false"));
            arc.Write(_T("newRow"), (rbbi.fStyle & RBBS_BREAK) != 0 ? _T("true") : _T("false"));
         }
      }
      arc.ReadGroupEnd();
   }
   arc.Save();
   arc.Close();
}


/////////////////////////////////////////////////////////////////
// Solution builders

void CMainFrame::_BuildSolutionUI()
{
   CLockWindowUpdate lock = m_hWnd;
   // Clear button state
   UIClear();
   // Rebuild explorer tree
   m_viewProperties.Clear();
   m_viewExplorer.m_viewFile.Clear();
   // Create the project UI
   g_pSolution->CreateExplorerUI();
   // Activate first project if not already active
   if( g_pSolution->GetActiveProject() == NULL ) {
      // Select root
      g_pSolution->SetActiveProject(g_pSolution->GetItem(0));
      m_viewExplorer.m_viewFile.SelectRoot();
   }
   SendMessage(WM_APP_PROJECTCHANGE, 0, (LPARAM) g_pSolution->GetActiveProject());
}

