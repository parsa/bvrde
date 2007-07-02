
#include "StdAfx.h"
#include "resource.h"

#include "MainFrm.h"
#include "WizardSheet.h"

#pragma code_seg( "WIZARDS" )

#define LISTITEM_SELECTED(pnmlv)    ( ((pnmlv->uOldState & LVIS_SELECTED)==0) && ((pnmlv->uNewState & LVIS_SELECTED)!=0) )


////////////////////////////////////////////////////////////////////////
//

class CSimpleWizardManager : public IWizardManager
{
public:
   CPropertySheetWindow m_wndSheet;
   CPropertySheet* m_pSheet;

   CSimpleWizardManager(CPropertySheetWindow wndSheet) : m_wndSheet(wndSheet)
   {
   }
   CSimpleWizardManager(CPropertySheet* pSheet) : m_pSheet(pSheet)
   {
   }
   BOOL AddWizardGroup(LPCTSTR /*pstrParent*/, LPCTSTR /*pstrName*/)
   {
      return FALSE;
   }
   BOOL AddWizardPage(UINT /*nID*/, LPCPROPSHEETPAGE hPage)
   {
      if( m_wndSheet.IsWindow() ) return m_wndSheet.AddPage(hPage);
      if( m_pSheet ) return m_pSheet->AddPage(hPage);
      ATLASSERT(false);
      return FALSE;
   }
   BOOL SetWizardGroup(LPCTSTR /*pstrName*/)
   {
      return FALSE;
   }
};


////////////////////////////////////////////////////////////////////////
//

IProject* CMainFrame::_CreateSolutionWizard()
{
   CBitmap bmpWatermark;
   CBitmap bmpBanner;
   bmpWatermark.LoadBitmap(IDB_WATERMARK);
   bmpBanner.LoadBitmap(IDB_BANNER);

   CString sCaption(MAKEINTRESOURCE(IDS_WIZARD_NEWSOLUTION));
   CString sTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_SOLUTIONTYPE));
   CString sSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_SOLUTIONTYPE));

   CSolutionTypePage wndTypePage;
   wndTypePage.Init(this);
   wndTypePage.Create();
   wndTypePage.SetTitle((LPCTSTR)sCaption);
   wndTypePage.SetHeaderTitle(sTitle);
   wndTypePage.SetHeaderSubTitle(sSubTitle);

   CPropertySheet sheet;
   sheet.SetHeader(bmpBanner);
   sheet.SetWatermark(bmpWatermark);
   sheet.AddPage(wndTypePage);
   if( sheet.DoModal() != IDOK ) return NULL;
   return (IProject*) wndTypePage.m_psp.lParam;
}

IProject* CMainFrame::_CreateProjectWizard()
{
   CBitmap bmpWatermark;
   CBitmap bmpBanner;
   bmpWatermark.LoadBitmap(IDB_WATERMARK);
   bmpBanner.LoadBitmap(IDB_BANNER);

   CString sCaption(MAKEINTRESOURCE(IDS_WIZARD_NEWPROJECT));
   CString sTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_SOLUTIONTYPE));
   CString sSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_SOLUTIONTYPE));

   CSolutionTypePage wndTypePage;
   wndTypePage.Init(this);
   wndTypePage.SetTitle((LPCTSTR)sCaption);
   wndTypePage.SetHeaderTitle(sTitle);
   wndTypePage.SetHeaderSubTitle(sSubTitle);

   CPropertySheet sheet;
   sheet.SetTitle(sCaption);
   sheet.SetHeader(bmpBanner);
   sheet.SetWatermark(bmpWatermark);
   sheet.AddPage(wndTypePage);
   if( sheet.DoModal() != IDOK ) return NULL;
   return (IProject*) wndTypePage.m_psp.lParam;
}

IProject* CMainFrame::_CreateFileWizard(IProject* pProject)
{
   CBitmap bmpWatermark;
   CBitmap bmpBanner;
   bmpWatermark.LoadBitmap(IDB_WATERMARK);
   bmpBanner.LoadBitmap(IDB_BANNER);

   CString sCaption(MAKEINTRESOURCE(IDS_WIZARD_NEWFILE));
   CString sFinishTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_FILEFINISH));
   CString sFinishSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_FILEFINISH));

   CSolutionFinishPage wndFinishPage;
   wndFinishPage.SetHeaderTitle(sFinishTitle);
   wndFinishPage.SetHeaderSubTitle(sFinishSubTitle);
   wndFinishPage.m_psp.dwFlags |= PSP_HIDEHEADER;

   CString sName;
   pProject->GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();

   CPropertySheet sheet;
   sheet.SetTitle(sCaption);
   sheet.SetHeader(bmpBanner);
   sheet.SetWatermark(bmpWatermark);
   CSimpleWizardManager manager(&sheet);
   for( int i = 0; i < m_aWizardListeners.GetSize(); i++ ) {
      m_aWizardListeners[i]->OnInitWizard(&manager, pProject, sName);
   }
   sheet.AddPage(wndFinishPage);
   if( sheet.DoModal() != IDOK ) return NULL;
   return pProject;
}


////////////////////////////////////////////////////////////////////////
//

CSolutionTypePage::CSolutionTypePage() :
   m_pProject(NULL)
{
}

void CSolutionTypePage::Init(CMainFrame* pFrame)
{
   m_pProject = NULL;
   m_pMainFrame = pFrame;
}

LRESULT CSolutionTypePage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_Images.Create(IDB_PROJECTS, 32, 64, RGB(0,128,128));

   m_ctrlName = GetDlgItem(IDC_NAME);
   m_ctrlName.SetLimitText(32);

   CString sName;
   sName.Format(IDS_PROJECTNAME, g_pSolution->GetItemCount() + 1);
   m_ctrlName.SetWindowText(sName);

   m_ctrlList = GetDlgItem(IDC_LIST);
   m_ctrlList.SetImageList(m_Images, LVSIL_NORMAL);
   for( int i = 0; i < g_aPlugins.GetSize(); i++ ) {
      CPlugin& Plugin = g_aPlugins[i];
      if( !Plugin.IsLoaded() ) continue;
      if( (Plugin.GetType() & PLUGIN_PROJECT) == 0 ) continue;
      int iIndex = m_ctrlList.InsertItem(0, Plugin.GetName(), 0);
      m_ctrlList.SetItemData(iIndex, i);
   }
   m_ctrlList.SelectItem(0);

   return 0;
}

void CSolutionTypePage::OnReset()
{
   // Destroy previously created project in case user is
   // travelling back and forward between pages...
   // NOTE: The 3 lines below took quite some time to get right, because
   //       the Tab control internally does *very* strange things and deleteing
   //       a page tends to screw up its internal page-array.
   while( GetPropertySheet().GetPageCount() > 1 ) {
      GetPropertySheet().RemovePage(1);
   }
   if( m_pProject ) {
      m_pProject->Close();
      m_pPlugin->DestroyProject(m_pProject);
      m_pProject = NULL;
   }
}

int CSolutionTypePage::OnSetActive()
{
   BOOL bDummy;
   OnItemSelected(0, NULL, bDummy);
   return 0;
}

int CSolutionTypePage::OnApply()
{
   m_psp.lParam = (LPARAM) m_pProject;
   return PSNRET_NOERROR;
}

int CSolutionTypePage::OnWizardNext()
{
   ATLASSERT(m_pMainFrame);

   CWaitCursor cursor;

   OnReset();

   int iIndex = m_ctrlList.GetItemData( m_ctrlList.GetSelectedIndex() );
   ATLASSERT(iIndex>=0);
   m_pPlugin = &g_aPlugins[iIndex];
   m_pProject = m_pPlugin->CreateProject();
   ATLASSERT(m_pProject);
   if( m_pProject == NULL ) return -1;

   CString sPath;
   sPath.Format(_T("%s%s"), CModulePath(), SOLUTIONDIR);
   m_pProject->Initialize(g_pDevEnv, sPath);

   CString sName = CWindowText(m_ctrlName);
   CSimpleWizardManager manager(GetPropertySheet());
   for( int i = 0; i < m_pMainFrame->m_aWizardListeners.GetSize(); i++ ) {
      m_pMainFrame->m_aWizardListeners[i]->OnInitWizard(&manager, m_pProject, sName);
   }

   static CString sFinishTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_FINISH));
   static CString sFinishSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_FINISH));
   m_pageFinish.SetHeaderTitle(sFinishTitle);
   m_pageFinish.SetHeaderSubTitle(sFinishSubTitle);
   m_pageFinish.m_psp.dwFlags |= PSP_HIDEHEADER;
   GetPropertySheet().AddPage(m_pageFinish);

   return 0;
}

LRESULT CSolutionTypePage::OnItemSelected(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   SetWizardButtons(m_ctrlList.GetSelectedCount() == 0 ? 0 : PSWIZB_NEXT);
   return 0;
}  

LRESULT CSolutionTypePage::OnItemDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   if( m_ctrlList.GetSelectedCount() == 0 ) return 0;
   GetPropertySheet().PressButton(PSBTN_NEXT);
   return 0;
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CSolutionFinishPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   return 0;
}

int CSolutionFinishPage::OnSetActive()
{
   SetWizardButtons(PSWIZB_BACK|PSWIZB_FINISH);
   return 0;
}

int CSolutionFinishPage::OnWizardFinish()
{
   if( !GetPropertySheet().Apply() ) return PSNRET_INVALID_NOCHANGEPAGE;
   return PSNRET_NOERROR;
}


////////////////////////////////////////////////////////////////////////
//

#define SET_CHECK(id, prop) \
   { TCHAR szBuf[32] = { 0 }; g_pDevEnv->GetProperty(prop, szBuf, 31); \
     if( _tcscmp(szBuf, _T("true"))==0 ) CButton(GetDlgItem(id)).Click(); }

#define GET_CHECK(id, prop) \
   g_pDevEnv->SetProperty(prop, CButton(GetDlgItem(id)).GetCheck() == BST_CHECKED ? _T("true") : _T("false"))

#define TRANSFER_PROP(name, prop) \
   { TCHAR szBuf[32] = { 0 }; g_pDevEnv->GetProperty(prop, szBuf, 31); \
     m_pArc->Write(name, szBuf); }

#define WRITE_CHECKBOX(id, name) \
     m_pArc->Write(name, CButton(GetDlgItem(id)).GetCheck() == BST_CHECKED ? _T("true") : _T("false"));

#define WRITE_TEXT(id, name) \
     m_pArc->Write(name, CWindowText(GetDlgItem(id)));


////////////////////////////////////////////////////////////////////////
//

LRESULT CGeneralOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlTabbed = GetDlgItem(IDC_USE_TABBED);
   m_ctrlMdi = GetDlgItem(IDC_USE_MDI);
   m_ctrlStartup = GetDlgItem(IDC_STARTUP);
   m_ctrlLanguage = GetDlgItem(IDC_LANGUAGE);
   m_ctrlGreyed = GetDlgItem(IDC_GREYED);

   TCHAR szBuffer[32] = { 0 };
   g_pDevEnv->GetProperty(_T("gui.main.client"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("mdi")) == 0 ) {
      m_ctrlMdi.Click();
   }
   else {
      m_ctrlTabbed.Click();
   }

   m_ctrlStartup.AddString(CString(MAKEINTRESOURCE(IDS_STARTUP_STARTPAGE)));
   m_ctrlStartup.AddString(CString(MAKEINTRESOURCE(IDS_STARTUP_LAST)));
   m_ctrlStartup.AddString(CString(MAKEINTRESOURCE(IDS_STARTUP_OPENFILE)));
   m_ctrlStartup.AddString(CString(MAKEINTRESOURCE(IDS_STARTUP_BLANK)));
   g_pDevEnv->GetProperty(_T("gui.main.start"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("lastsolution")) == 0 ) m_ctrlStartup.SetCurSel(1);
   else if( _tcscmp(szBuffer, _T("openfile")) == 0 ) m_ctrlStartup.SetCurSel(2);
   else if( _tcscmp(szBuffer, _T("blank")) == 0 ) m_ctrlStartup.SetCurSel(3);
   else m_ctrlStartup.SetCurSel(0);

   m_ctrlLanguage.AddString(CString(MAKEINTRESOURCE(IDS_LANG_DEFAULT)));
   m_ctrlLanguage.AddString(CString(MAKEINTRESOURCE(IDS_LANG_ENGLISH)));
   m_ctrlLanguage.AddString(CString(MAKEINTRESOURCE(IDS_LANG_GERMAN)));
   g_pDevEnv->GetProperty(_T("gui.main.language"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("en")) == 0 ) m_ctrlLanguage.SetCurSel(1);
   else if( _tcscmp(szBuffer, _T("de")) == 0 ) m_ctrlLanguage.SetCurSel(2);
   else m_ctrlLanguage.SetCurSel(0);

   return 0;
}

LRESULT CGeneralOptionsPage::OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   bHandled = FALSE;
   if( (HWND) lParam != m_ctrlGreyed ) return 0;
   bHandled = TRUE;
   LRESULT lRes = DefWindowProc();
   CDCHandle dc = (HDC) wParam;
   dc.SetBkMode(TRANSPARENT);
   dc.SetTextColor(::GetSysColor(COLOR_BTNSHADOW));
   return (LPARAM) ::GetStockObject(HOLLOW_BRUSH);
}

int CGeneralOptionsPage::OnApply()
{
   TCHAR szBuffer[32] = { 0 };

   _tcscpy(szBuffer, _T("tabbed"));
   if( m_ctrlMdi.GetCheck() == BST_CHECKED ) _tcscpy(szBuffer, _T("mdi"));
   g_pDevEnv->SetProperty(_T("gui.main.client"), szBuffer);

   _tcscpy(szBuffer, _T("startpage"));
   if( m_ctrlStartup.GetCurSel() == 1 ) _tcscpy(szBuffer, _T("lastsolution"));
   if( m_ctrlStartup.GetCurSel() == 2 ) _tcscpy(szBuffer, _T("openfile"));
   if( m_ctrlStartup.GetCurSel() == 3 ) _tcscpy(szBuffer, _T("blank"));
   g_pDevEnv->SetProperty(_T("gui.main.start"), szBuffer);

   _tcscpy(szBuffer, _T("def"));
   if( m_ctrlLanguage.GetCurSel() == 1 ) _tcscpy(szBuffer, _T("en"));
   if( m_ctrlLanguage.GetCurSel() == 2 ) _tcscpy(szBuffer, _T("de"));
   g_pDevEnv->SetProperty(_T("gui.main.language"), szBuffer);

   m_pArc->ReadItem(_T("Gui"));
   TRANSFER_PROP(_T("client"), _T("gui.main.client"));
   TRANSFER_PROP(_T("start"), _T("gui.main.start"));

   return PSNRET_NOERROR;
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CDocumentsOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   SET_CHECK(IDC_READONLY, _T("gui.document.protectReadOnly"));
   SET_CHECK(IDC_DETECT_CHANGE, _T("gui.document.detectChange"));
   SET_CHECK(IDC_AUTOLOAD, _T("gui.document.autoLoad"));
   SET_CHECK(IDC_FINDTEXT, _T("gui.document.findText"));
   SET_CHECK(IDC_FINDMESSAGES, _T("gui.document.findMessages"));
   SET_CHECK(IDC_CLEARUNDO, _T("gui.document.clearUndo"));
   return 0;
}

int CDocumentsOptionsPage::OnApply()
{
   m_pArc->ReadItem(_T("Document"));
   WRITE_CHECKBOX(IDC_READONLY, _T("protectReadOnly"));
   WRITE_CHECKBOX(IDC_DETECT_CHANGE, _T("detectChange"));
   WRITE_CHECKBOX(IDC_AUTOLOAD, _T("autoLoad"));
   WRITE_CHECKBOX(IDC_FINDTEXT, _T("findText"));
   WRITE_CHECKBOX(IDC_FINDMESSAGES, _T("findMessages"));
   WRITE_CHECKBOX(IDC_CLEARUNDO, _T("clearUndo"));
   return PSNRET_NOERROR;
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CAssociationsOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   return 0;
}

int CAssociationsOptionsPage::OnSetActive()
{
   if( m_ctrlList.GetItemCount() > 0 ) return 0;

   while( m_ctrlList.GetHeader().GetItemCount() > 0 ) m_ctrlList.DeleteColumn(0);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_ASSOC_COL1)), 0);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_ASSOC_COL2)), 1);

   CString sModulePath = CModulePath();
   sModulePath.MakeUpper();

   int iStart = 0;
   TCHAR szKey[200];
   TCHAR szValue[200];
   while( g_pDevEnv->EnumProperties(iStart, _T("file.extension.*"), szKey, szValue) ) 
   {
      CString sKey = szKey;
      CString sExtension;
      int iPos = sKey.ReverseFind('.');
      ATLASSERT(iPos>0);
      if( iPos < 0 ) return 0;
      sExtension = sKey.Mid(iPos + 1);
      int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), sExtension);

      CString sFilename;
      sFilename.Format(_T("test.%s"), sExtension);
      SHFILEINFO shfi = { 0 };
      ::SHGetFileInfo(sFilename, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
      if( _tcslen(shfi.szTypeName) == 0 ) _tcscpy(shfi.szTypeName, szValue);
      m_ctrlList.SetItemText(iItem, 1, shfi.szTypeName);

      bool bFound = false;
      CString sDotExt;
      sDotExt.Format(_T(".%s"), sExtension);
      CRegKey reg;
      if( reg.Open(HKEY_CLASSES_ROOT, sDotExt, KEY_READ) == ERROR_SUCCESS ) {
         TCHAR szFileValue[128] = { 0 };
         DWORD dwCount = 127;
         reg.QueryValue(szFileValue, NULL, &dwCount);
         CRegKey regFile;
         CString sKey;
         sKey.Format(_T("%s\\Shell\\Open\\Command"), szFileValue);
         if( regFile.Open(HKEY_CLASSES_ROOT, sKey, KEY_READ) == ERROR_SUCCESS ) {
            TCHAR szValue[MAX_PATH] = { 0 };
            DWORD dwCount = MAX_PATH;
            regFile.QueryValue(szValue, NULL, &dwCount);
            CString sValue = szValue;
            sValue.MakeUpper();
            if( sValue.Find(sModulePath) >= 0 ) bFound = true;
         }
      }
      if( bFound ) m_ctrlList.SetCheckState(iItem, TRUE);
      m_ctrlList.SetItemData(iItem, (LPARAM) m_ctrlList.GetCheckState(iItem));
   }
   m_ctrlList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

   return 0;
}

int CAssociationsOptionsPage::OnApply()
{
   CString sFile;
   sFile.Format(_T("\"%s\\BVRDE.EXE\" %%1"), CModulePath());

   for( int i = 0; i < m_ctrlList.GetItemCount(); i++ ) {
      BOOL bNowState = m_ctrlList.GetCheckState(i);
      BOOL bOldState = (BOOL) m_ctrlList.GetItemData(i);
      if( bNowState != bOldState ) {
         CString sExtension;
         m_ctrlList.GetItemText(i, 0, sExtension);
         TCHAR szFileValue[128] = { 0 };
         CString sDotExt;
         sDotExt.Format(_T(".%s"), sExtension);
         CRegKey reg;
         if( reg.Create(HKEY_CLASSES_ROOT, sDotExt) == ERROR_SUCCESS ) {
            DWORD dwCount = 127;
            reg.QueryValue(szFileValue, NULL, &dwCount);
            reg.Close();
         }
         else {
            ::wsprintf(szFileValue, _T("%sfile"), sExtension);
            CRegKey reg;
            reg.Create(HKEY_CLASSES_ROOT, szFileValue);
            reg.Create(reg, _T("shell"));
            reg.Create(reg, _T("Open"));
            reg.Create(reg, _T("command"));
         }
         CRegKey regFile;
         CString sKey;
         sKey.Format(_T("%s\\shell\\Open\\command"), szFileValue);
         if( regFile.Open(HKEY_CLASSES_ROOT, sKey, KEY_ALL_ACCESS) == ERROR_SUCCESS ) {
            // Remember existing value
            TCHAR szOldValue[MAX_PATH] = { 0 };
            DWORD dwCount = MAX_PATH;
            regFile.QueryValue(szOldValue, NULL /*_T("command")*/, &dwCount);
            // Write a new "command" value
            TCHAR szCommandValue[MAX_PATH] = { 0 };
            if( bNowState == FALSE ) {
               dwCount = MAX_PATH;
               regFile.QueryValue(szCommandValue, _T("BVRDE.backup"), &dwCount);
            }
            else {
               _tcscpy(szCommandValue, sFile);
            }
            regFile.SetValue(szCommandValue);
            // Create backup value
            if( bNowState != FALSE ) regFile.SetValue(szOldValue, _T("BVRDE.backup"));
            // Delete anything DDE specific
            //regFile.RecurseDeleteKey(_T("ddeexec"));
         }
      }
   }

   // BUG: Need to remove existing associations when items are unchecked!

   return PSNRET_NOERROR;
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CMappingsOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   return 0;
}

LRESULT CMappingsOptionsPage::OnAddItem(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.InsertItem(-1, PropCreateSimple(_T(""), _T("")));
   m_ctrlList.SetSubItem(iItem, 1, PropCreateSimple(_T(""), _T("")));
   ((CPropertyEditItem*)m_ctrlList.GetProperty(iItem, 0))->SetEditStyle(ES_LOWERCASE);
   ((CPropertyEditItem*)m_ctrlList.GetProperty(iItem, 1))->SetEditStyle(ES_LOWERCASE);
   m_ctrlList.SelectItem(iItem);
   m_ctrlList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
   return 0;
}

int CMappingsOptionsPage::OnSetActive()
{
   if( m_ctrlList.GetItemCount() > 0 ) return 0;

   while( m_ctrlList.GetHeader().GetItemCount() > 0 ) m_ctrlList.DeleteColumn(0);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_MAPPING_COL1)), 0);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_MAPPING_COL2)), 1);

   int iStart = 0;
   TCHAR szKey[200];
   TCHAR szValue[200];
   while( g_pDevEnv->EnumProperties(iStart, _T("file.mappings.*"), szKey, szValue) ) 
   {
      int iItem = m_ctrlList.InsertItem(-1, PropCreateSimple(_T(""), _tcsrchr(szKey, '.') + 1));
      m_ctrlList.SetSubItem(iItem, 1, PropCreateSimple(_T(""), szValue));
   }
   m_ctrlList.SortItems(CompareFunc, (LPARAM) &m_ctrlList);
   m_ctrlList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
   // Finally we can add the "<< Click Here >>" item after sorting...
   m_ctrlList.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

   return 0;
}

int CMappingsOptionsPage::OnApply()
{
   USES_CONVERSION;

   // Make sure list was populated before we store data back!!!
   OnSetActive();

   // Transfer list items to properties
   int nCount = m_ctrlList.GetItemCount();
   for( int i = 0; i < nCount; i++ ) {
      CComVariant vExt;
      CComVariant vType;
      m_ctrlList.GetItemValue(m_ctrlList.GetProperty(i, 0), &vExt);
      m_ctrlList.GetItemValue(m_ctrlList.GetProperty(i, 1), &vType);
      if( vExt.vt == VT_BSTR && ::SysStringLen(vExt.bstrVal) > 0 ) {
         CString sKey;
         sKey.Format(_T("file.mappings.%s"), OLE2CT(vExt.bstrVal));
         g_pDevEnv->SetProperty(sKey, OLE2CT(vType.bstrVal));
      }
   }

   // Write out settings file
   if( m_pArc->ReadGroupBegin(_T("FileMappings")) ) 
   {  
      while( m_pArc->Delete(_T("FileMapping")) ) /* */;

      for( int i = 0; i < nCount; i++ ) {
         CComVariant vExt;
         CComVariant vType;
         m_ctrlList.GetItemValue(m_ctrlList.GetProperty(i, 0), &vExt);
         m_ctrlList.GetItemValue(m_ctrlList.GetProperty(i, 1), &vType);
         if( vExt.vt == VT_BSTR && ::SysStringLen(vExt.bstrVal) > 0 ) {
            if( m_pArc->WriteGroupBegin(_T("FileMapping")) ) {
               m_pArc->Write(_T("ext"), OLE2CT(vExt.bstrVal));
               m_pArc->Write(_T("type"), OLE2CT(vType.bstrVal));
               m_pArc->WriteGroupEnd();
            }
         }
      }
      m_pArc->ReadGroupEnd();
   }

   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}

int CALLBACK CMappingsOptionsPage::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CPropertyGridCtrl* pGrid = (CPropertyGridCtrl*) lParamSort;
   HPROPERTY hProp1 = ((IProperty**)lParam1)[0];
   HPROPERTY hProp2 = ((IProperty**)lParam2)[0];
   TCHAR szValue1[200];
   TCHAR szValue2[200];
   pGrid->GetItemText(hProp1, szValue1, 199);
   pGrid->GetItemText(hProp2, szValue2, 199);
   return _tcscmp(szValue1, szValue2);
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CAutoTextOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList = GetDlgItem(IDC_LIST);
   m_ctrlName = GetDlgItem(IDC_NAME);
   m_ctrlText = GetDlgItem(IDC_TEXT);
   m_ctrlNew = GetDlgItem(IDC_NEW);
   m_ctrlDelete = GetDlgItem(IDC_DELETE);

   m_ctrlText.SetFont( AtlGetStockFont(ANSI_FIXED_FONT) );

   if( m_iconNew.IsNull() ) {
      m_iconNew.LoadIcon(IDI_NEW, 16, 16);
      m_iconDelete.LoadIcon(IDI_DELETE, 16, 16);
   }
   m_ctrlNew.SetIcon(m_iconNew);
   m_ctrlDelete.SetIcon(m_iconDelete);

   return 0;
}

LRESULT CAutoTextOptionsPage::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   for( int i = 0; i < m_ctrlList.GetCount(); i++ ) delete [] (LPTSTR) m_ctrlList.GetItemData(i);
   return 0;
}

LRESULT CAutoTextOptionsPage::OnCtlColorListBox(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   static CBrush br;
   if( br.IsNull() ) {
      COLORREF clrWhite = ::GetSysColor(COLOR_WINDOW);
      COLORREF clrBlue = ::GetSysColor(COLOR_HIGHLIGHT);
      br.CreateSolidBrush(BlendRGB(clrWhite, clrBlue, 6));
   }
   ::SetBkMode((HDC) wParam, TRANSPARENT);
   return (LPARAM) (HBRUSH) br;
}

int CAutoTextOptionsPage::OnSetActive()
{
   if( m_ctrlList.GetCount() > 0 ) return 0;

   int i = 1;
   while( true ) {
      CString sKey;
      sKey.Format(_T("autotext.entry%ld."), i++);
      TCHAR szName[32] = { 0 };
      g_pDevEnv->GetProperty(sKey + _T("name"), szName, (sizeof(szName) / sizeof(TCHAR)) - 1);
      if( _tcslen(szName) == 0 ) break;
      // Set name
      m_ctrlName.SetWindowText(szName);
      // Create list item
      int iItem = m_ctrlList.AddString(szName);
      // Set item text
      const int cchMax = 400;
      CString sText;
      g_pDevEnv->GetProperty(sKey + _T("text"), sText.GetBufferSetLength(cchMax), cchMax);
      sText.ReleaseBuffer();
      sText.Replace(_T("\\n"), _T("\r\n"));
      sText.Replace(_T("\\t"), _T("\t"));
      LPTSTR pstrText = new TCHAR[ sText.GetLength() + 1 ];
      _tcscpy(pstrText, sText);
      m_ctrlList.SetItemData(iItem, (LPARAM) pstrText);
   }
   if( m_ctrlList.GetCount() == 0 ) SendMessage(WM_COMMAND, MAKEWPARAM(IDC_NEW, 0));
   m_ctrlList.SetCurSel(0);

   BOOL bDummy;
   OnItemSelect(0, 0, NULL, bDummy);

   return 0;
}

int CAutoTextOptionsPage::OnApply()
{
   const long MAX_ITEMS = 50;

   // Make sure list was populated before we store data back!!!
   OnSetActive();

   int i;
   for( i = 0; i < m_ctrlList.GetCount(); i++ ) {
      CString sName;
      m_ctrlList.GetText(i, sName);
      CString sText = (LPTSTR) m_ctrlList.GetItemData(i);
      CString sKey;
      sKey.Format(_T("autotext.entry%ld."), i + 1);
      g_pDevEnv->SetProperty(sKey + _T("name"), sName);
      g_pDevEnv->SetProperty(sKey + _T("text"), sText);
   }
   for( ; i < MAX_ITEMS; i++ ) {
      CString sKey;
      sKey.Format(_T("autotext.entry%ld."), i + 1);
      g_pDevEnv->SetProperty(sKey + _T("name"), _T(""));
      g_pDevEnv->SetProperty(sKey + _T("text"), _T(""));
   }

   if( m_pArc->ReadGroupBegin(_T("AutoText")) ) 
   {
      while( m_pArc->Delete(_T("Text")) ) /* */;

      for( long i = 0; i < m_ctrlList.GetCount(); i++ ) {
         CString sKey;
         sKey.Format(_T("autotext.entry%ld."), i + 1);
         TCHAR szName[32] = { 0 };
         g_pDevEnv->GetProperty(sKey + _T("name"), szName, 31);
         if( szName[0] != '\0' ) {
            if( m_pArc->WriteGroupBegin(_T("Text")) ) {
               m_pArc->Write(_T("name"), szName);
               const int cchMax = 400;
               CString sText;
               g_pDevEnv->GetProperty(sKey + _T("text"), sText.GetBufferSetLength(cchMax), cchMax);
               sText.ReleaseBuffer();
               sText.Replace(_T("\r\n"), _T("\\n"));
               sText.Replace(_T("\t"), _T("\\t"));
               m_pArc->Write(_T("text"), sText);
               m_pArc->WriteGroupEnd();
            }
         }
      }
      m_pArc->ReadGroupEnd();
   }
   
   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}

LRESULT CAutoTextOptionsPage::OnItemSelect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateButtons();
   //
   int iSel = m_ctrlList.GetCurSel();
   if( iSel < 0 ) return 0;
   CString sName;
   m_ctrlList.GetText(iSel, sName);
   m_ctrlName.SetWindowText(sName);
   CString sText = (LPTSTR) m_ctrlList.GetItemData(iSel);
   m_ctrlText.SetWindowText(sText);
   return 0;
}

LRESULT CAutoTextOptionsPage::OnNameChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iSel = m_ctrlList.GetCurSel();
   if( iSel < 0 ) return 0;
   // Change item text
   CWindowText sName = m_ctrlName;
   LPTSTR pstr = (LPTSTR) m_ctrlList.GetItemData(iSel);
   if( pstr == NULL ) return 0;
   m_ctrlList.DeleteString(iSel);
   int idx = m_ctrlList.InsertString(iSel, sName);
   m_ctrlList.SetItemData(idx, (LPARAM) pstr);
   m_ctrlList.SetCurSel(iSel);
   return 0;
}

LRESULT CAutoTextOptionsPage::OnTextChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iSel = m_ctrlList.GetCurSel();
   if( iSel < 0 ) return 0;
   CWindowText sText = m_ctrlText;
   LPTSTR pstrText = (LPTSTR) m_ctrlList.GetItemData(iSel);
   delete [] pstrText;
   pstrText = new TCHAR[ sText.GetLength() + 1 ];
   _tcscpy(pstrText, sText);
   m_ctrlList.SetItemData(iSel, (LPARAM) pstrText);
   return 0;
}

LRESULT CAutoTextOptionsPage::OnNewItem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.InsertString(m_ctrlList.GetCount(), _T(""));
   LPTSTR pstrText = new TCHAR[1];
   *pstrText = '\0';
   m_ctrlList.SetItemData(iItem, (LPARAM) pstrText);
   m_ctrlList.SetCurSel(iItem);
   BOOL bDummy;
   OnItemSelect(0, 0, NULL, bDummy);
   return 0;
}

LRESULT CAutoTextOptionsPage::OnDeleteItem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iSel = m_ctrlList.GetCurSel();
   if( iSel < 0 ) return 0;
   delete [] (LPTSTR) m_ctrlList.GetItemData(iSel);
   m_ctrlList.DeleteString(iSel);
   m_ctrlList.SetCurSel(iSel);
   BOOL bDummy;
   OnItemSelect(0, 0, NULL, bDummy);
   return 0;
}

void CAutoTextOptionsPage::_UpdateButtons()
{
   CWindowText sName = m_ctrlName;
   int iSel = m_ctrlList.GetCurSel();
   m_ctrlDelete.EnableWindow(iSel >= 0 && sName.GetLength() > 0);
   m_ctrlName.EnableWindow(iSel >= 0);
   m_ctrlText.EnableWindow(iSel >= 0);
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CColorsOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(!m_sLanguage.IsEmpty());
   
   m_ctrlList = GetDlgItem(IDC_LIST);

   m_ctrlFace.SubclassWindow(GetDlgItem(IDC_FONT));
   m_ctrlSize = GetDlgItem(IDC_FONTSIZE);
   m_ctrlBold = GetDlgItem(IDC_BOLD);
   m_ctrlItalic = GetDlgItem(IDC_ITALICS);
   m_ctrlForeColor.SubclassWindow(GetDlgItem(IDC_FORE));
   m_ctrlBackColor.SubclassWindow(GetDlgItem(IDC_BACK));
   m_ctrlSample = GetDlgItem(IDC_SAMPLE);

   m_ctrlFace.SetFont(AtlGetDefaultGuiFont());
   m_ctrlFace.SetExtendedFontStyle(FPS_EX_TYPEICON | FPS_EX_FIXEDBOLD);
   m_ctrlFace.Dir();

   m_ctrlSample.ModifyStyle(0, SS_OWNERDRAW);

   _AddComboColors(m_ctrlForeColor);
   _AddComboColors(m_ctrlBackColor);

#define RGR2RGB(x) RGB((x >> 16) & 0xFF, (x >> 8) & 0xFF, x & 0xFF)

   ::ZeroMemory(m_aFonts, sizeof(m_aFonts));

   for( long i = 0; i < sizeof(m_aFonts) / sizeof(m_aFonts[0]); i++ ) {
      CString sKey;
      sKey.Format(_T("editors.%s.style%ld."), m_sLanguage, i + 1L);
      LPTSTR p = NULL;
      COLORREF clr;
      TCHAR szBuffer[64] = { 0 };
      g_pDevEnv->GetProperty(sKey + _T("name"), m_aFonts[i].szTitle, 63);
      if( _tcslen(m_aFonts[i].szTitle) == 0 ) break;
      g_pDevEnv->GetProperty(sKey + _T("font"), m_aFonts[i].szFaceName, LF_FACESIZE - 1);
      g_pDevEnv->GetProperty(sKey + _T("height"), szBuffer, 15);
      m_aFonts[i].iHeight = max(8, _ttol(szBuffer));
      ::ZeroMemory(szBuffer, sizeof(szBuffer));
      g_pDevEnv->GetProperty(sKey + _T("color"), szBuffer, 15);
      clr = _tcstol(szBuffer + 1, &p, 16);
      m_aFonts[i].clrText = RGR2RGB(clr);
      ::ZeroMemory(szBuffer, sizeof(szBuffer));
      g_pDevEnv->GetProperty(sKey + _T("back"), szBuffer, 15);
      clr = _tcstol(szBuffer + 1, &p, 16);
      m_aFonts[i].clrBack = RGR2RGB(clr);
      g_pDevEnv->GetProperty(sKey + _T("bold"), szBuffer, 15);
      m_aFonts[i].bBold = _tcscmp(szBuffer, _T("true")) == 0;
      g_pDevEnv->GetProperty(sKey + _T("italic"), szBuffer, 15);
      m_aFonts[i].bItalic = _tcscmp(szBuffer, _T("true")) == 0;

      m_ctrlList.AddString(m_aFonts[i].szTitle);
   }
   m_ctrlList.SetCurSel(0);

   BOOL bDummy;
   OnItemSelect(NULL, 0, NULL, bDummy);

   return 0;
}

LRESULT CColorsOptionsPage::OnCtlColorListBox(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   static CBrush br;
   if( br.IsNull() ) {
      COLORREF clrWhite = ::GetSysColor(COLOR_WINDOW);
      COLORREF clrBlue = ::GetSysColor(COLOR_HIGHLIGHT);
      br.CreateSolidBrush(BlendRGB(clrWhite, clrBlue, 6));
   }
   ::SetBkMode((HDC) wParam, TRANSPARENT);
   return (LPARAM) (HBRUSH) br;
}

LRESULT CColorsOptionsPage::OnDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   if( wParam != IDC_SAMPLE ) {
      bHandled = FALSE;
      return 0;
   }

   LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) lParam;
   CDCHandle dc = lpDIS->hDC;
   RECT rc = lpDIS->rcItem;
   int iIndex = m_ctrlList.GetCurSel();

   CFont font;
   CLogFont lf;
   _tcscpy(lf.lfFaceName, m_aFonts[iIndex].szFaceName);
   lf.SetHeight(m_aFonts[iIndex].iHeight, dc);
   lf.MakeBolder(m_aFonts[iIndex].bBold ? 1 : 0);
   lf.lfItalic = m_aFonts[iIndex].bItalic == true;
   font.CreateFontIndirect(&lf);
   ATLASSERT(!font.IsNull());

   HFONT hOldFont = dc.SelectFont(font);
   dc.FillSolidRect(&rc, m_aFonts[iIndex].clrBack);
   dc.SetMapMode(MM_TEXT);
   dc.SetBkMode(TRANSPARENT);
   dc.SetTextColor(m_aFonts[iIndex].clrText);
   dc.DrawText(_T("Sample: AaBbCb..."), -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
   dc.SelectFont(hOldFont);
   return 0;
}

LRESULT CColorsOptionsPage::OnItemSelect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iIndex = m_ctrlList.GetCurSel();
   
   SetDlgItemText(IDC_FONT, m_aFonts[iIndex].szFaceName);
   if( m_ctrlFace.FindStringExact(-1, m_aFonts[iIndex].szFaceName) >= 0 ) {
      m_ctrlFace.SetCurSel(m_ctrlFace.SelectString(-1, m_aFonts[iIndex].szFaceName));
      BOOL bDummy;
      OnFaceChange(0, 0, NULL, bDummy);
   }

   SetDlgItemInt(IDC_FONTSIZE, m_aFonts[iIndex].iHeight);
   
   m_ctrlBold.SetCheck(m_aFonts[iIndex].bBold ? BST_CHECKED : BST_UNCHECKED);
   m_ctrlItalic.SetCheck(m_aFonts[iIndex].bItalic ? BST_CHECKED : BST_UNCHECKED);
   
   int iPos = m_ctrlForeColor.FindColor(m_aFonts[iIndex].clrText);
   if( iPos == -1 ) {
      _SetCustomColor(m_ctrlForeColor, m_aFonts[iIndex].clrText);
   }
   else {
      m_ctrlForeColor.SetCurSel(iPos);
   }

   iPos = m_ctrlBackColor.FindColor(m_aFonts[iIndex].clrBack);
   if( iPos == -1 ) {
      _SetCustomColor(m_ctrlBackColor, m_aFonts[iIndex].clrBack);
   }
   else {
      m_ctrlBackColor.SetCurSel(iPos);
   }

   m_ctrlSample.Invalidate();
   return 0;
}

LRESULT CColorsOptionsPage::OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateValues();
   return 0;
}

LRESULT CColorsOptionsPage::OnCustomFore(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iIndex = m_ctrlList.GetCurSel();
   CColorDialog dlg(m_ctrlForeColor.GetSelectedColor(), CC_FULLOPEN, m_ctrlForeColor);
   UINT nRes = dlg.DoModal(m_ctrlForeColor);
   if( nRes == IDOK ) {
      _SetCustomColor(m_ctrlForeColor, dlg.m_cc.rgbResult);
      _UpdateValues();
   }
   return 0;
}

LRESULT CColorsOptionsPage::OnCustomBack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iIndex = m_ctrlList.GetCurSel();
   CColorDialog dlg(m_ctrlBackColor.GetSelectedColor(), CC_FULLOPEN, m_ctrlBackColor);
   UINT nRes = dlg.DoModal(m_ctrlBackColor);
   if( nRes == IDOK ) {
      _SetCustomColor(m_ctrlBackColor, dlg.m_cc.rgbResult);
      _UpdateValues();
   }
   return 0;
}

LRESULT CColorsOptionsPage::OnFaceChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   CClientDC dc = m_hWnd;
   int iIndex = m_ctrlFace.GetCurSel();
   if( iIndex >= 0 ) {
      CSimpleArray<long> aSizes;
      LOGFONT lfOrig;
      TEXTMETRIC tmOrig;
      m_ctrlFace.GetLogFont(iIndex, lfOrig);
      m_ctrlFace.GetTextMetrics(iIndex, tmOrig);
      if( (tmOrig.tmPitchAndFamily & (TMPF_TRUETYPE|TMPF_VECTOR)) != 0 ) {
         static long truetypesize[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36 };
         for( int i = 0; i < sizeof(truetypesize)/sizeof(int); i++ ) aSizes.Add(truetypesize[i]);
      }
      else {
         for( int i = 8; i <= 36; i++ ) {
            CLogFont lf = lfOrig;
            lf.SetHeight(i, dc);
            CFont font;
            font.CreateFontIndirect(&lf);
            TEXTMETRIC tm;
            dc.GetTextMetrics(&tm);
            bool bFound = false;
            for( int j = 0; j < aSizes.GetSize(); j++ ) {
               if( aSizes[j] == tm.tmHeight ) bFound = true;
            }
            if( !bFound ) aSizes.Add(tm.tmHeight);
         }
      }
      while( m_ctrlSize.GetCount() > 0 ) m_ctrlSize.DeleteString(0);
      for( int i = 0; i < aSizes.GetSize(); i++ ) {
         CString s;
         s.Format(_T("%ld"), aSizes[i]);
         m_ctrlSize.AddString(s);
      }
   }
   bHandled = FALSE;
   return 0;
}

int CColorsOptionsPage::OnApply()
{
   ATLASSERT(m_pArc);

   for( long i = 0; i < m_ctrlList.GetCount(); i++ ) {
      TCHAR szBuffer[32];
      CString sKey;
      sKey.Format(_T("editors.%s.style%ld."), m_sLanguage, i + 1);

      g_pDevEnv->SetProperty(sKey + _T("name"), m_aFonts[i].szTitle);
      //
      g_pDevEnv->SetProperty(sKey + _T("font"), m_aFonts[i].szFaceName);
      //
      ::wsprintf(szBuffer, _T("%d"), m_aFonts[i].iHeight);
      g_pDevEnv->SetProperty(sKey + _T("height"), szBuffer);
      //
      ::wsprintf(szBuffer, _T("#%02X%02X%02X"), 
         GetRValue(m_aFonts[i].clrText),
         GetGValue(m_aFonts[i].clrText),
         GetBValue(m_aFonts[i].clrText));
      g_pDevEnv->SetProperty(sKey + _T("color"), szBuffer);
      //
      ::wsprintf(szBuffer, _T("#%02X%02X%02X"), 
         GetRValue(m_aFonts[i].clrBack),
         GetGValue(m_aFonts[i].clrBack),
         GetBValue(m_aFonts[i].clrBack));
      g_pDevEnv->SetProperty(sKey + _T("back"), szBuffer);
      //
      g_pDevEnv->SetProperty(sKey + _T("bold"), m_aFonts[i].bBold ? _T("true") : _T("false"));
      //
      g_pDevEnv->SetProperty(sKey + _T("italic"), m_aFonts[i].bItalic ? _T("true") : _T("false"));
   }

   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( m_sLanguage == szLanguage ) {
            for( long i = 0; i < m_ctrlList.GetCount(); i++ ) {
               CString sTag;
               sTag.Format(_T("Style%ld"), i + 1);
               m_pArc->Delete(sTag);
               m_pArc->WriteItem(sTag);
               CString sKey;
               sKey.Format(_T("editors.%s.style%ld."), m_sLanguage, i + 1);
               TRANSFER_PROP(_T("name"), sKey + _T("name"));
               TRANSFER_PROP(_T("font"), sKey + _T("font"));
               TRANSFER_PROP(_T("height"), sKey + _T("height"));
               TRANSFER_PROP(_T("color"), sKey + _T("color"));
               TRANSFER_PROP(_T("back"), sKey + _T("back"));
               TRANSFER_PROP(_T("bold"), sKey + _T("bold"));
               TRANSFER_PROP(_T("italic"), sKey + _T("italic"));
            }
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }
   
   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}

void CColorsOptionsPage::_UpdateValues()
{
   int iIndex = m_ctrlList.GetCurSel();
   _tcscpy(m_aFonts[iIndex].szFaceName, CWindowText(m_ctrlFace));
   if( m_ctrlFace.GetCurSel() >= 0 ) {
      m_ctrlFace.GetLBText(m_ctrlFace.GetCurSel(), m_aFonts[iIndex].szFaceName);
   }
   m_aFonts[iIndex].iHeight = _ttol(CWindowText(m_ctrlSize));
   if( m_ctrlSize.GetCurSel() >= 0 ) {
      CString sSize;
      m_ctrlSize.GetLBText(m_ctrlSize.GetCurSel(), sSize);
      m_aFonts[iIndex].iHeight = _ttol(sSize);
   }
   m_aFonts[iIndex].bBold = m_ctrlBold.GetCheck() == BST_CHECKED;
   m_aFonts[iIndex].bItalic = m_ctrlItalic.GetCheck() == BST_CHECKED;
   m_aFonts[iIndex].clrText = m_ctrlForeColor.GetSelectedColor();
   m_aFonts[iIndex].clrBack = m_ctrlBackColor.GetSelectedColor();
   m_ctrlSample.Invalidate();
}

void CColorsOptionsPage::_SetCustomColor(CColorPickerComboCtrl& ctlrColor, COLORREF clr) const
{
   ctlrColor.DeleteString(ctlrColor.GetCount() - 1);
   ctlrColor.AddColor(-1, clr | 0xFF000000);
   ctlrColor.SetCurSel(ctlrColor.GetCount() - 1);
}

void CColorsOptionsPage::_AddComboColors(CColorPickerComboCtrl& combo) const
{
   combo.AddColor(-1, RGB(255,255,255));
   combo.AddColor(-1, RGB(0,0,0));
   combo.AddColor(-1, RGB(0,0,255));
   combo.AddColor(-1, RGB(0,255,0));
   combo.AddColor(-1, RGB(255,0,0));
   combo.AddColor(-1, RGB(255,128,0));
   combo.AddColor(-1, RGB(128,200,30));
   combo.AddColor(-1, RGB(0,128,255));
   combo.AddColor(-1, RGB(10,40,200));
   combo.AddColor(-1, RGB(58,50,128));
   if( ::GetSysColor(COLOR_WINDOW) != RGB(255,255,255) ) combo.AddColor(-1, ::GetSysColor(COLOR_WINDOW));
   if( ::GetSysColor(COLOR_WINDOWTEXT) != RGB(0,0,0) ) combo.AddColor(-1, ::GetSysColor(COLOR_WINDOWTEXT));
   combo.AddColor(-1, RGB(230,230,230));
   combo.AddColor(-1, RGB(180,180,180));
   combo.AddColor(-1, RGB(100,100,100));
   combo.AddColor(-1, RGB(30,30,30));
   combo.AddColor(-1, RGB(0,0,0) | 0xFF000000);
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CFormattingOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(!m_sLanguage.IsEmpty());

   m_ctrlTabWidth = GetDlgItem(IDC_TABWIDTH);
   m_ctrlIndentMode = GetDlgItem(IDC_INDENTATION);
   m_ctrlIndentWidth = GetDlgItem(IDC_INDENTWIDTH);

   m_ctrlTabWidth.SetLimitText(2);
   m_ctrlIndentWidth.SetLimitText(2);

   m_ctrlIndentMode.AddString(CString(MAKEINTRESOURCE(IDS_NONE)));
   m_ctrlIndentMode.AddString(CString(MAKEINTRESOURCE(IDS_AUTO)));
   m_ctrlIndentMode.AddString(CString(MAKEINTRESOURCE(IDS_SMART)));

   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   SET_CHECK(IDC_MARGINS, sKey + _T("showMargins"));
   SET_CHECK(IDC_FOLDING, sKey + _T("showFolding"));
   SET_CHECK(IDC_LINENUMBERS, sKey + _T("showLines"));
   SET_CHECK(IDC_WORDWRAP, sKey + _T("wordWrap"));
   SET_CHECK(IDC_BACKUNINDENT, sKey + _T("backUnindent"));
   SET_CHECK(IDC_TABINDENT, sKey + _T("tabIndent"));
   SET_CHECK(IDC_USETABS, sKey + _T("useTabs"));
   SET_CHECK(IDC_INDENTS, sKey + _T("showIndents"));

   TCHAR szBuffer[32];
   g_pDevEnv->GetProperty(sKey + _T("tabWidth"), szBuffer, 31);
   SetDlgItemInt(IDC_TABWIDTH, max(1, _ttol(szBuffer)));
   g_pDevEnv->GetProperty(sKey + _T("indentWidth"), szBuffer, 31);
   SetDlgItemInt(IDC_INDENTWIDTH, max(0, _ttol(szBuffer)));

   g_pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("auto")) == 0 ) m_ctrlIndentMode.SetCurSel(1);
   else if( _tcscmp(szBuffer, _T("smart")) == 0 ) m_ctrlIndentMode.SetCurSel(2);
   else m_ctrlIndentMode.SetCurSel(0);

   return 0;
}

int CFormattingOptionsPage::OnApply()
{
   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   GET_CHECK(IDC_MARGINS, sKey + _T("showMargins"));
   GET_CHECK(IDC_FOLDING, sKey + _T("showFolding"));
   GET_CHECK(IDC_INDENTS, sKey + _T("showIndents"));
   GET_CHECK(IDC_LINENUMBERS, sKey + _T("showLines"));
   GET_CHECK(IDC_WORDWRAP, sKey + _T("wordWrap"));
   GET_CHECK(IDC_BACKUNINDENT, sKey + _T("backUnindent"));
   GET_CHECK(IDC_TABINDENT, sKey + _T("tabIndent"));
   GET_CHECK(IDC_USETABS, sKey + _T("useTabs"));

   TCHAR szBuffer[32];
   _tcscpy(szBuffer, _T("none"));
   if( m_ctrlIndentMode.GetCurSel() == 1 ) _tcscpy(szBuffer, _T("auto"));
   if( m_ctrlIndentMode.GetCurSel() == 2 ) _tcscpy(szBuffer, _T("smart"));
   g_pDevEnv->SetProperty(sKey + _T("indentMode"), szBuffer);

   g_pDevEnv->SetProperty(sKey + _T("tabWidth"), CWindowText(m_ctrlTabWidth));
   g_pDevEnv->SetProperty(sKey + _T("indentWidth"), CWindowText(m_ctrlIndentWidth));

   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( m_sLanguage == szLanguage ) {
            m_pArc->Delete(_T("Visuals"));
            m_pArc->WriteItem(_T("Visuals"));
            TRANSFER_PROP(_T("showMargins"), sKey + _T("showMargins"));
            TRANSFER_PROP(_T("showFolding"), sKey + _T("showFolding"));
            TRANSFER_PROP(_T("showIndents"), sKey + _T("showIndents"));
            TRANSFER_PROP(_T("showLines"), sKey + _T("showLines"));
            TRANSFER_PROP(_T("wordWrap"), sKey + _T("wordWrap"));
            TRANSFER_PROP(_T("backUnindent"), sKey + _T("backUnindent"));
            TRANSFER_PROP(_T("tabIndent"), sKey + _T("tabIndent"));
            TRANSFER_PROP(_T("useTabs"), sKey + _T("useTabs"));
            TRANSFER_PROP(_T("indentMode"), sKey + _T("indentMode"));
            TRANSFER_PROP(_T("tabWidth"), sKey + _T("tabWidth"));
            TRANSFER_PROP(_T("indentWidth"), sKey + _T("indentWidth"));
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }
   
   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CEditorsOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlEOL = GetDlgItem(IDC_EOL);
   m_ctrlCaretWidth = GetDlgItem(IDC_CARETWIDTH);

   CString sKey = _T("editors.general.");
   SET_CHECK(IDC_SAVETOOL, sKey + _T("saveBeforeTool"));
   SET_CHECK(IDC_SAVEPROMPT, sKey + _T("savePrompt"));
   SET_CHECK(IDC_SHOWEDGE, sKey + _T("showEdge"));
   SET_CHECK(IDC_BOTTOMLESS, sKey + _T("bottomless"));
   SET_CHECK(IDC_CARETLINE, sKey + _T("markCaretLine"));

   m_ctrlCaretWidth.SetLimitText(1);

   m_ctrlEOL.AddString(_T("CR"));
   m_ctrlEOL.AddString(_T("LF"));
   m_ctrlEOL.AddString(_T("CR + LF"));
   m_ctrlEOL.AddString(CString(MAKEINTRESOURCE(IDS_AUTOMATIC)));

   TCHAR szBuffer[32];
   g_pDevEnv->GetProperty(sKey + _T("caretWidth"), szBuffer, 31);
   SetDlgItemInt(IDC_CARETWIDTH, max(1, _ttol(szBuffer)));

   g_pDevEnv->GetProperty(sKey + _T("eolMode"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("cr")) == 0 ) m_ctrlEOL.SetCurSel(0);
   else if( _tcscmp(szBuffer, _T("lf")) == 0 ) m_ctrlEOL.SetCurSel(1);
   else if( _tcscmp(szBuffer, _T("crlf")) == 0 ) m_ctrlEOL.SetCurSel(2);
   else m_ctrlEOL.SetCurSel(3);

   return 0;
}

int CEditorsOptionsPage::OnApply()
{
   CString sKey = _T("editors.general.");
   GET_CHECK(IDC_SAVETOOL, sKey + _T("saveBeforeTool"));
   GET_CHECK(IDC_SAVEPROMPT, sKey + _T("savePrompt"));
   GET_CHECK(IDC_SHOWEDGE, sKey + _T("showEdge"));
   GET_CHECK(IDC_BOTTOMLESS, sKey + _T("bottomless"));
   GET_CHECK(IDC_CARETLINE, sKey + _T("markCaretLine"));

   TCHAR szBuffer[32];
   _tcscpy(szBuffer, _T("auto"));
   if( m_ctrlEOL.GetCurSel() == 0 ) _tcscpy(szBuffer, _T("cr"));
   if( m_ctrlEOL.GetCurSel() == 1 ) _tcscpy(szBuffer, _T("lf"));
   if( m_ctrlEOL.GetCurSel() == 2 ) _tcscpy(szBuffer, _T("crlf"));
   g_pDevEnv->SetProperty(sKey + _T("eolMode"), szBuffer);

   g_pDevEnv->SetProperty(sKey + _T("caretWidth"), CWindowText(m_ctrlCaretWidth));

   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      m_pArc->Delete(_T("General"));
      m_pArc->WriteItem(_T("General"));
      TRANSFER_PROP(_T("saveBeforeTool"), sKey + _T("saveBeforeTool"));
      TRANSFER_PROP(_T("savePrompt"), sKey + _T("savePrompt"));
      TRANSFER_PROP(_T("showEdge"), sKey + _T("showEdge"));
      TRANSFER_PROP(_T("bottomless"), sKey + _T("bottomless"));
      TRANSFER_PROP(_T("markCaretLine"), sKey + _T("markCaretLine"));
      TRANSFER_PROP(_T("eolMode"), sKey + _T("eolMode"));
      TRANSFER_PROP(_T("caretWidth"), sKey + _T("caretWidth"));
      m_pArc->ReadGroupEnd();
   }

   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();
   
   return PSNRET_NOERROR;
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CPrintingOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   SET_CHECK(IDC_WORDWRAP, _T("priting.general.wordWrap"));
   SET_CHECK(IDC_COLORS, _T("priting.general.colors"));
   return 0;
}

int CPrintingOptionsPage::OnApply()
{
   m_pArc->ReadItem(_T("Printing"));
   WRITE_CHECKBOX(IDC_WORDWRAP, _T("wordWrap"));
   WRITE_CHECKBOX(IDC_COLORS, _T("colors"));
   return PSNRET_NOERROR;
}
