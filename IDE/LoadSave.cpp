
#include "StdAfx.h"
#include "resource.h"

#include "MainFrm.h"
#include "Solution.h"

#include "FileMissingDlg.h"

#include "XmlSerializer.h"
#include "RegSerializer.h"

#ifndef PROGID_XMLDOMDocument
   #define PROGID_XMLDOMDocument L"MSXML2.DOMDocument.4.0"
#endif


/////////////////////////////////////////////////////////////////
// Global Attributes

IDevEnv* g_pDevEnv = NULL;
CSolution* g_pSolution = NULL;
CSimpleArray<CPlugin> g_aPlugins;
CComPtr<IXMLDOMDocument> g_spConfigDoc;


/////////////////////////////////////////////////////////////////
// Implementation

void PreloadConfig()
{
   ATLASSERT(g_spConfigDoc==NULL);

   CString sFilename;
   sFilename.Format(_T("%sBVRDE.xml"), CModulePath());

   // Create XML document
   HRESULT Hr = g_spConfigDoc.CoCreateInstance(PROGID_XMLDOMDocument);
   ATLASSERT(SUCCEEDED(Hr));
   if( FAILED(Hr) || g_spConfigDoc == NULL ) {
      CFileMissingDlg dlg;
      dlg.DoModal();
      ::PostQuitMessage(0);
      return;
   }

   // Load from file
   // NOTE: We quite boldly try to load the XML asynchroniously
   //       to increase start performance.
   g_spConfigDoc->put_async(VARIANT_TRUE);
   VARIANT_BOOL vbSuccess = VARIANT_FALSE;
   if( FAILED( g_spConfigDoc->load(CComVariant(sFilename), &vbSuccess) ) || vbSuccess == VARIANT_FALSE ) {
      AtlMessageBox(NULL, _T("Invalid Configuration File!"), _T("BVRDE"), MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
      ::PostQuitMessage(0L);
      return;
   }
}

void CMainFrame::_LoadSettings()
{
   ATLASSERT(g_spConfigDoc);
   ATLASSERT(m_aProperties.GetSize()==0);

   if( g_spConfigDoc == NULL ) return;

   m_aProperties.Resize(513);

   m_hDevMode = NULL;
   m_hDevNames = NULL;
   ::SetRectEmpty(&m_rcPageMargins);

   CRegSerializer reg;
   if( reg.Open(REG_BVRDE) ) {
      // Load Window positions
      if( reg.ReadGroupBegin(_T("Settings")) ) {
         _AddProperty(&reg, _T("windowpos"), _T("window.main.position"));
         _AddProperty(&reg, _T("autohide-cx"), _T("window.autohide.cx"));
         _AddProperty(&reg, _T("autohide-cy"), _T("window.autohide.cy"));
         _AddProperty(&reg, _T("pane-left"), _T("window.pane.left"));
         _AddProperty(&reg, _T("pane-top"), _T("window.pane.top"));
         _AddProperty(&reg, _T("pane-right"), _T("window.pane.right"));
         _AddProperty(&reg, _T("pane-bottom"), _T("window.pane.bottom"));
         _AddProperty(&reg, _T("properties-cy"), _T("window.properties.cy"));
         _AddProperty(&reg, _T("explorer-cy"), _T("window.explorer.cy"));
         reg.ReadGroupEnd();
      }
      reg.Close();
   }

   // Wait for document to complete loading.
   DWORD dwStartTick = ::GetTickCount();
   while( true ) {
      long lValue = 0;
      g_spConfigDoc->get_readyState(&lValue);
      if( lValue == 4 ) break;  // XML in complete state!
      ::Sleep(0L);
      if( ::GetTickCount() - dwStartTick > 5000 ) return;
   }

   // Open archive...
   CString sFilename;
   sFilename.Format(_T("%sBVRDE.xml"), CModulePath());

   CXmlSerializer arc;
   if( !arc.Open(g_spConfigDoc, _T("Settings"), sFilename) ) return;

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
         _AddProperty(&arc, _T("eolMode"), sKey + _T("eolMode"));
         _AddProperty(&arc, _T("caretWidth"), sKey + _T("caretWidth"));
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
            _AddProperty(&arc, _T("showLines"), sKey + _T("showLines"));
            _AddProperty(&arc, _T("wordWrap"), sKey + _T("wordWrap"));
            _AddProperty(&arc, _T("backUnindent"), sKey + _T("backUnindent"));
            _AddProperty(&arc, _T("tabIndent"), sKey + _T("tabIndent"));
            _AddProperty(&arc, _T("useTabs"), sKey + _T("useTabs"));
            _AddProperty(&arc, _T("indentMode"), sKey + _T("indentMode"));
            _AddProperty(&arc, _T("tabWidth"), sKey + _T("tabWidth"));
            _AddProperty(&arc, _T("indentWidth"), sKey + _T("indentWidth"));           
         }

         if( arc.ReadItem(_T("Advanced")) ) {
            sKey.Format(_T("editors.%s."), sLanguage);
            _AddProperty(&arc, _T("noTips"), sKey + _T("noTips"));
            _AddProperty(&arc, _T("protectDebugged"), sKey + _T("protectDebugged"));
            _AddProperty(&arc, _T("matchBraces"), sKey + _T("matchBraces"));
            _AddProperty(&arc, _T("autoClose"), sKey + _T("autoClose"));
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

         // We'll allow 8 styles pr file-type
         for( long i = 1; i <= 8; i++ ) {
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
      _AddProperty(&arc, _T("output"), _T("sourcecontrol.output"));
      _AddProperty(&arc, _T("enable"), _T("sourcecontrol.enable"));
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
      _AddProperty(&arc, _T("hotkey"), _T("autotext.hotkey"));
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
         CString sName;
         arc.Read(_T("name"), sName);
         sKey.Format(_T("window.toolbar.%s."), sName);
         _AddProperty(&arc, _T("show"), sKey + _T("show"));
         _AddProperty(&arc, _T("position"), sKey + _T("position"));
         _AddProperty(&arc, _T("newRow"), sKey + _T("newRow"));
         arc.ReadGroupEnd();
      }
      arc.ReadGroupEnd();
   }

   arc.Close();
}

void CMainFrame::_SaveSettings()
{
   CRegSerializer reg;
   if( reg.Create(REG_BVRDE) ) {
      reg.WriteGroupBegin(_T("Settings"));
      _StoreProperty(&reg, _T("windowpos"), _T("window.main.position"));
      _StoreProperty(&reg, _T("autohide-cx"), _T("window.autohide.cx"));
      _StoreProperty(&reg, _T("autohide-cy"), _T("window.autohide.cy"));
      _StoreProperty(&reg, _T("pane-left"), _T("window.pane.left"));
      _StoreProperty(&reg, _T("pane-top"), _T("window.pane.top"));
      _StoreProperty(&reg, _T("pane-right"), _T("window.pane.right"));
      _StoreProperty(&reg, _T("pane-bottom"), _T("window.pane.bottom"));
      _StoreProperty(&reg, _T("properties-cy"), _T("window.properties.cy"));
      _StoreProperty(&reg, _T("explorer-cy"), _T("window.explorer.cy"));
      reg.WriteGroupEnd();
   }

   if( m_hDevMode != NULL ) ::GlobalFree(m_hDevMode);
   if( m_hDevNames != NULL ) ::GlobalFree(m_hDevNames);

   g_spConfigDoc.Release();
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
   TCHAR szBuffer[128] = { 0 };
   GetProperty(_T("window.autohide.cx"), szBuffer, 127);
   if( _ttol(szBuffer) > 0 ) m_AutoHide.SetPaneSize(AUTOHIDE_LEFT, _ttol(szBuffer));
   GetProperty(_T("window.autohide.cy"), szBuffer, 127);
   if( _ttol(szBuffer) > 0 ) m_AutoHide.SetPaneSize(AUTOHIDE_BOTTOM, _ttol(szBuffer));
   GetProperty(_T("window.pane.left"), szBuffer, 127);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_LEFT, _ttol(szBuffer));
   GetProperty(_T("pwindow.pane.top"), szBuffer, 127);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_TOP, _ttol(szBuffer));
   GetProperty(_T("window.pane.right"), szBuffer, 127);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_RIGHT, _ttol(szBuffer));
   GetProperty(_T("window.pane.bottom"), szBuffer, 127);
   if( _ttol(szBuffer) > 0 ) m_Dock.SetPaneSize(DOCK_BOTTOM, _ttol(szBuffer));
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

   RECT rc = { 0 };
   int iDockState = 0;
   m_Dock.GetWindowState(m_viewExplorer, iDockState, rc);
   if( iDockState < IDE_DOCK_FLOAT ) {
      ::wsprintf(szBuffer, _T("%ld"), rc.bottom - rc.top);
      SetProperty(_T("window.explorer.cy"), szBuffer);
   }
   m_Dock.GetWindowState(m_viewProperties, iDockState, rc);
   if( iDockState < IDE_DOCK_FLOAT ) {
      ::wsprintf(szBuffer, _T("%ld"), rc.bottom - rc.top);
      SetProperty(_T("window.properties.cy"), szBuffer);
   }
}

void CMainFrame::_SaveToolBarState()
{
   CString sFilename;
   sFilename.Format(_T("%sBVRDE.XML"), CModulePath());
   CXmlSerializer arc;
   if( !arc.Open(_T("Settings"), sFilename) ) return;
   if( arc.ReadGroupBegin(_T("ToolBars")) ) {
      while( arc.Delete(_T("ToolBar")) ) /* */;
      for( int i = 0;  i < m_aToolBars.GetSize(); i++ ) {
         TOOLBAR& tb = m_aToolBars[i];
         for( UINT iPos = 0; iPos < m_Rebar.GetBandCount(); iPos++ ) {
            REBARBANDINFO rbbi = { 0 };
            rbbi.cbSize = sizeof(rbbi);
            rbbi.fMask = RBBIM_CHILD | RBBIM_ID | RBBIM_STYLE;
            m_Rebar.GetBandInfo(iPos, &rbbi);
            if( rbbi.hwndChild != tb.hWnd ) continue;
            arc.WriteItem(_T("ToolBar"));
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

void CMainFrame::_AddProperty(ISerializable* pArc, LPCTSTR pstrAttribute, LPCTSTR pstrKey)
{
   ATLASSERT(!::IsBadStringPtr(pstrAttribute,-1));
   ATLASSERT(!::IsBadStringPtr(pstrKey,-1));
   ATLASSERT(pArc);
   static TCHAR szValue[2048] = { 0 };
   szValue[0] = '\0';
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
