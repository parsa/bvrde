
#include "StdAfx.h"
#include "resource.h"

#include "WizardSheet.h"

#include "Commands.h"


///////////////////////////////////////////////////////////////
//

LRESULT COptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlType = GetDlgItem(IDC_TYPE);
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SYSTEM_NONE)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SYSTEM_CVS)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SYSTEM_SUBVERSION)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SYSTEM_CUSTOM)));

   long iIndex = 0;
   if( _Commands.sType == _T("cvs") ) iIndex = SC_SYSTEM_CVS;
   if( _Commands.sType == _T("subversion") ) iIndex = SC_SYSTEM_SUBVERSION;
   if( _Commands.sType == _T("custom") ) iIndex = SC_SYSTEM_CUSTOM;
   m_ctrlType.SetCurSel(iIndex);

   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   m_ctrlList.SetExtendedListStyle(PLS_EX_XPLOOK|PLS_EX_CATEGORIZED);
   m_ctrlList.SetColumnWidth(130);
   
   _PopulateList();
  
   m_ctrlList.EnableWindow(_Commands.bEnabled);

   return 0;
}

LRESULT COptionsPage::OnTypeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   int iIndex = m_ctrlType.GetCurSel();

   _Commands.sProgram       = _T("");
   _Commands.sOutput        = _T("");
   _Commands.sCmdUpdate     = _T("");
   _Commands.sCmdCheckIn    = _T("");
   _Commands.sCmdCheckOut   = _T("");
   _Commands.sCmdAddFile    = _T("");
   _Commands.sCmdRemoveFile = _T("");
   _Commands.sCmdLogIn      = _T("");
   _Commands.sCmdLogOut     = _T("");
   _Commands.sCmdDiff       = _T("");
   _Commands.sCmdStatus     = _T("");
   _Commands.sOptRecursive  = _T("");
   _Commands.sOptStickyTag  = _T("");
   _Commands.sOptMessage    = _T("");
   _Commands.sOptUpdateDirs = _T("");
   _Commands.sOptBranch     = _T("");
   _Commands.sOptCommon    = _T("");
   _Commands.sBrowseAll     = _T("");
   _Commands.sBrowseSingle  = _T("");

   switch( iIndex ) {
   case SC_SYSTEM_CVS:
      _Commands.sProgram = _T("cvs");
      _Commands.sOutput = "cvs server: ";
      _Commands.sCmdUpdate = _T("update");
      _Commands.sCmdCheckIn = _T("commit");
      _Commands.sCmdCheckOut = _T("");
      _Commands.sCmdAddFile = _T("add");
      _Commands.sCmdRemoveFile = _T("remove -f");
      _Commands.sCmdLogIn = _T("login");
      _Commands.sCmdLogOut = _T("logout");
      _Commands.sCmdDiff = _T("diff");
      _Commands.sCmdStatus = _T("status -v");
      _Commands.sOptRecursive = _T("-R");
      _Commands.sOptStickyTag = _T("-A");
      _Commands.sOptMessage = _T("-m");
      _Commands.sOptUpdateDirs = _T("-dP");
      _Commands.sOptBranch = _T("-j");
      _Commands.sOptCommon = _T("");
      _Commands.sBrowseAll = _T("cvs -q status");
      _Commands.sBrowseSingle = _T("cvs -l status $PATH$");
      break;
   case SC_SYSTEM_SUBVERSION:
      _Commands.sProgram = _T("svn");
      _Commands.sOutput = "svn: ";
      _Commands.sCmdUpdate = _T("update");
      _Commands.sCmdCheckIn = _T("commit");
      _Commands.sCmdCheckOut = _T("checkout");
      _Commands.sCmdAddFile = _T("add");
      _Commands.sCmdRemoveFile = _T("delete");
      _Commands.sCmdLogIn = _T("");
      _Commands.sCmdLogOut = _T("");
      _Commands.sCmdDiff = _T("diff");
      _Commands.sCmdStatus = _T("status -v");
      _Commands.sOptRecursive = _T("");
      _Commands.sOptStickyTag = _T("");
      _Commands.sOptMessage = _T("-m");
      _Commands.sOptUpdateDirs = _T("--non-recursive");
      _Commands.sOptBranch = _T("");
      _Commands.sOptCommon = _T("--non-interactive");
      _Commands.sBrowseAll = _T("svn --non-interactive --show-updates ");
      _Commands.sBrowseSingle = _T("svn -N --non-interactive --show-updates $PATH$");
      break;
   }

   _Commands.bEnabled = iIndex > 0;
   _PopulateList();
   return 0;
}

int COptionsPage::OnSetActive()
{
   return 0;
}

int COptionsPage::OnApply()
{
   int iIndex = m_ctrlType.GetCurSel();
   _Commands.sType = _T("none");
   if( iIndex == SC_SYSTEM_CVS ) _Commands.sType = _T("cvs");
   if( iIndex == SC_SYSTEM_SUBVERSION ) _Commands.sType = _T("subversion");
   if( iIndex == SC_SYSTEM_CUSTOM ) _Commands.sType = _T("custom");
   _Commands.bEnabled = iIndex > 0;

   CComVariant v;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(1), &v);
   _Commands.sProgram = v.bstrVal;

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(2), &v);
   _Commands.sCmdCheckIn = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(3), &v);
   _Commands.sCmdCheckOut = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(4), &v);
   _Commands.sCmdUpdate = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(5), &v);
   _Commands.sCmdAddFile = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(6), &v);
   _Commands.sCmdRemoveFile = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(7), &v);
   _Commands.sCmdStatus = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(8), &v);
   _Commands.sCmdDiff = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(9), &v);
   _Commands.sCmdLogIn = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(10), &v);
   _Commands.sCmdLogOut = v.bstrVal;

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(50), &v);
   _Commands.sOptMessage = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(51), &v);
   _Commands.sOptRecursive = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(52), &v);
   _Commands.sOptStickyTag = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(53), &v);
   _Commands.sOptUpdateDirs = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(54), &v);
   _Commands.sOptBranch = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(55), &v);
   _Commands.sOptCommon = v.bstrVal;

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(80), &v);
   _Commands.sBrowseAll = v.bstrVal;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(81), &v);
   _Commands.sBrowseSingle = v.bstrVal;

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(99), &v);
   _Commands.sOutput = v.bstrVal;

   if( m_pArc->ReadGroupBegin(_T("SourceControl")) ) 
   {
      m_pArc->Write(_T("type"), _Commands.sType);
      m_pArc->Write(_T("enable"), _Commands.bEnabled ? _T("true") : _T("false"));
      m_pArc->Write(_T("output"), _Commands.sOutput);
      m_pArc->Write(_T("program"), _Commands.sProgram);
      m_pArc->Write(_T("checkin"), _Commands.sCmdCheckIn);
      m_pArc->Write(_T("checkout"), _Commands.sCmdCheckOut);
      m_pArc->Write(_T("update"), _Commands.sCmdUpdate);
      m_pArc->Write(_T("addfile"), _Commands.sCmdAddFile);
      m_pArc->Write(_T("removefile"), _Commands.sCmdRemoveFile);
      m_pArc->Write(_T("status"), _Commands.sCmdStatus);
      m_pArc->Write(_T("diff"), _Commands.sCmdDiff);
      m_pArc->Write(_T("login"), _Commands.sCmdLogIn);
      m_pArc->Write(_T("logout"), _Commands.sCmdLogOut);
      m_pArc->Write(_T("message"), _Commands.sOptMessage);
      m_pArc->Write(_T("recursive"), _Commands.sOptRecursive);
      m_pArc->Write(_T("sticky"), _Commands.sOptStickyTag);
      m_pArc->Write(_T("general"), _Commands.sOptCommon);
      m_pArc->Write(_T("browseAll"), _Commands.sBrowseAll);
      m_pArc->Write(_T("browseSingle"), _Commands.sBrowseSingle);
      m_pArc->ReadGroupEnd();
   }

   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}

void COptionsPage::_PopulateList()
{
   CString sName;
   CString sValue;

   m_ctrlList.ResetContent();
   sName.LoadString(IDS_GROUP_PROGRAM);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_PROGRAM_APP);
   sValue = _Commands.sProgram;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 1));

   sName.LoadString(IDS_GROUP_COMMANDS);
   m_ctrlList.AddItem(PropCreateCategory(sName));

   sName.LoadString(IDS_COMMAND_CHECKIN);
   sValue = _Commands.sCmdCheckIn;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 2));
   sName.LoadString(IDS_COMMAND_CHECKOUT);
   sValue = _Commands.sCmdCheckOut;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 3));
   sName.LoadString(IDS_COMMAND_UPDATE);
   sValue = _Commands.sCmdUpdate;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 4));
   sName.LoadString(IDS_COMMAND_ADDFILE);
   sValue = _Commands.sCmdAddFile;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 5));
   sName.LoadString(IDS_COMMAND_REMOVEFILE);
   sValue = _Commands.sCmdRemoveFile;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 6));
   sName.LoadString(IDS_COMMAND_STATUS);
   sValue = _Commands.sCmdStatus;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 7));
   sName.LoadString(IDS_COMMAND_DIFF);
   sValue = _Commands.sCmdDiff;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 8));
   sName.LoadString(IDS_COMMAND_LOGIN);
   sValue = _Commands.sCmdLogIn;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 9));
   sName.LoadString(IDS_COMMAND_LOGOUT);
   sValue = _Commands.sCmdLogOut;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 10));

   sName.LoadString(IDS_GROUP_OPTIONS);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_OPTION_MESSAGE);
   sValue = _Commands.sOptMessage;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 50));
   sName.LoadString(IDS_OPTION_RECURSIVE);
   sValue = _Commands.sOptRecursive;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 51));
   sName.LoadString(IDS_OPTION_STICKYTAGS);
   sValue = _Commands.sOptStickyTag;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 52));
   sName.LoadString(IDS_OPTION_UPDATEDIRS);
   sValue = _Commands.sOptUpdateDirs;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 53));
   sName.LoadString(IDS_OPTION_BRANCH);
   sValue = _Commands.sOptBranch;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 54));
   sName.LoadString(IDS_OPTION_GENERAL);
   sValue = _Commands.sOptCommon;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 55));

   sName.LoadString(IDS_GROUP_BROWSER);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_BROWSER_ALL);
   sValue = _Commands.sBrowseAll;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 80));
   sName.LoadString(IDS_BROWSER_SINGLE);
   sValue = _Commands.sBrowseSingle;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 81));

   sName.LoadString(IDS_GROUP_MISC);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_MISC_OUTPUT);
   sValue = _Commands.sOutput;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 99));

   m_ctrlList.EnableWindow(_Commands.bEnabled!=false);
}
