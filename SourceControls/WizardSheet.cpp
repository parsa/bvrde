
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
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SYSTEM_CUSTOM)));

   long iIndex = 0;
   if( _Commands.sType == _T("cvs") ) iIndex = 1;
   if( _Commands.sType == _T("custom") ) iIndex = 2;
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

   _Commands.sProgram = _T("");
   _Commands.sOutput = "";
   _Commands.sCmdUpdate = _T("");
   _Commands.sCmdCheckIn = _T("");
   _Commands.sCmdCheckOut = _T("");
   _Commands.sCmdAddFile = _T("");
   _Commands.sCmdRemoveFile = _T("");
   _Commands.sCmdLogIn = _T("");
   _Commands.sCmdLogOut = _T("");
   _Commands.sCmdDiff = _T("");
   _Commands.sCmdStatus = _T("");
   _Commands.sOptRecursive = _T("");
   _Commands.sOptStickyTag = _T("");
   _Commands.sOptMessage = _T("");

   switch( iIndex ) {
   case 1:
      _Commands.sProgram = _T("cvs");
      _Commands.sOutput = "cvs server: ";
      _Commands.sCmdUpdate = _T("update");
      _Commands.sCmdCheckIn = _T("commit");
      _Commands.sCmdCheckOut = _T("");
      _Commands.sCmdAddFile = _T("add");
      _Commands.sCmdRemoveFile = _T("remove");
      _Commands.sCmdLogIn = _T("login");
      _Commands.sCmdLogOut = _T("logout");
      _Commands.sCmdDiff = _T("diff");
      _Commands.sCmdStatus = _T("status");
      _Commands.sOptRecursive = _T("-R");
      _Commands.sOptStickyTag = _T("-A");
      _Commands.sOptMessage = _T("-m");
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
   if( iIndex == 1 ) _Commands.sType = _T("cvs");
   if( iIndex == 2 ) _Commands.sType = _T("custom");
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
      m_pArc->ReadGroupEnd();
   }

   _pDevEnv->SetProperty(_T("sourcecontrol.enable"), _Commands.bEnabled ? _T("true") : _T("false"));

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

   sName.LoadString(IDS_GROUP_MISC);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_MISC_OUTPUT);
   sValue = _Commands.sOutput;
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 99));

   m_ctrlList.EnableWindow(_Commands.bEnabled!=false);
}
