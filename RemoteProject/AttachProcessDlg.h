#if !defined(AFX_ATTACHPROCESS_H__20031126_E21E_000E_2DDB_0080AD509054__INCLUDED_)
#define AFX_ATTACHPROCESS_H__20031126_E21E_000E_2DDB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CAttachProcessDlg : 
   public CDialogImpl<CAttachProcessDlg>,
   public COwnerDraw<CAttachProcessDlg>,
   public ILineCallback
{
public:
   enum { IDD = IDD_ATTACHPROCESS };

   CRemoteProject* m_pProject;
   CString m_sProcessName;
   CListBox m_ctrlList;
   long m_lPid;

   void Init(CRemoteProject* pProject, LPCTSTR pstrName)
   {
      m_pProject = pProject;
      m_sProcessName = pstrName;
      m_sProcessName.TrimLeft(_T("."));   // Don't want "./foo.exe" because pids have absolute paths
   }
   long GetPid() const
   {
      return m_lPid;
   }

   BEGIN_MSG_MAP(CAttachProcessDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOk)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(LBN_SELCHANGE, OnChange);
      COMMAND_CODE_HANDLER(LBN_DBLCLK, OnDblClick);
      CHAIN_MSG_MAP( COwnerDraw<CAttachProcessDlg> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlList = GetDlgItem(IDC_LIST);
      m_ctrlList.SetFont(AtlGetStockFont(ANSI_FIXED_FONT));
      m_ctrlList.SetHorizontalExtent(1200);
      
      m_lPid = 0;

      // Run "ps -s" command on remote server to list all
      // processes available for attachment...
      CWaitCursor cursor;
      CComDispatchDriver dd = m_pProject->GetDispatch();
      CComVariant aParams[3];
      aParams[2] = L"ps -s";
      aParams[1] = static_cast<IUnknown*>(this);
      aParams[0] = 3000L;
      dd.InvokeN(OLESTR("ExecCommand"), aParams, 3);

      return TRUE;
   }
   LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {      
      int iIndex = m_ctrlList.GetCurSel();
      if( iIndex <= 0 ) return 0;
      m_lPid = m_ctrlList.GetItemData(iIndex);
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CWindow(GetDlgItem(IDOK)).EnableWindow(m_ctrlList.GetCurSel() > 0);
      return 0;
   }
   LRESULT OnDblClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0));
      return 0;
   }

	void DrawItem(LPDRAWITEMSTRUCT lpDIS)
	{
      CString sText;
      m_ctrlList.GetText(lpDIS->itemID, sText);
      COLORREF clrBack = ::GetSysColor(COLOR_WINDOW);
      COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
      if( lpDIS->itemData == 0 ) clrBack = ::GetSysColor(COLOR_BTNFACE);
      if( sText.Find(m_sProcessName) >= 0 ) clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
      if( (lpDIS->itemState & (ODS_SELECTED|ODS_FOCUS)) != 0 ) {
         clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
         clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
      }
      CDCHandle dc = lpDIS->hDC;
      dc.FillSolidRect(&lpDIS->rcItem, clrBack);
      dc.SetTextColor(clrText);
      dc.TextOut(lpDIS->rcItem.left, lpDIS->rcItem.top, sText);
	}

   // ILineCallback

   STDMETHOD(OnIncomingLine)(BSTR bstr)
   {
      if( ::SysStringLen(bstr) == 0 ) return S_OK;
      if( bstr[0] != ' ' ) return S_OK;
      int iIndex = m_ctrlList.AddString(CString(bstr));
      m_ctrlList.SetItemData(iIndex, _wtol(bstr));
      return S_OK;
   }

   // IUnknown

   STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
   {
      if( riid == __uuidof(ILineCallback) || riid == IID_IUnknown ) {
         *ppvObject = (ILineCallback*) this;
         return S_OK;
      }
      return E_NOINTERFACE;
   }
   ULONG STDMETHODCALLTYPE AddRef(void)
   {
      return 1;
   }
   ULONG STDMETHODCALLTYPE Release(void)
   {
      return 1;
   }
};


#endif
