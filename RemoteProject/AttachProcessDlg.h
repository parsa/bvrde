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

   typedef struct {
      CString sLine;
      long lPid;
   } PIDINFO;

   CRemoteProject* m_pProject;
   CListBox m_ctrlList;   
   long m_lPid;
   int m_iPidLinePos;
   CString m_sProcessName;
   CSimpleArray<PIDINFO> m_aList;

   void Init(CRemoteProject* pProject, LPCTSTR pstrName)
   {
      m_pProject = pProject;
      m_sProcessName = pstrName;
      m_sProcessName.TrimLeft(_T("./"));   // Don't want "./foo.exe" because pids have absolute paths
   }
   long GetSelectedPid() const
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
      m_iPidLinePos = 0;

      // Run "ps -s" command on remote server to list all
      // processes available for attachment...
      CComDispatchDriver dd = m_pProject->GetDispatch();
      CString sCommand = m_pProject->m_CompileManager.GetParam(_T("ProcessList"));
      CComVariant aParams[3];
      aParams[2] = sCommand;
      aParams[1] = static_cast<IUnknown*>(this);
      aParams[0] = 4000L;
      dd.InvokeN(OLESTR("ExecCommand"), aParams, 3);

      // Populate the list...
      for( int i = 0; i < m_aList.GetSize(); i++ ) {
         const PIDINFO& Info = m_aList[i];
         int iItem = m_ctrlList.AddString(Info.sLine);
         m_ctrlList.SetItemData(iItem, (DWORD_PTR) Info.lPid);
      }

      return TRUE;
   }
   LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {      
      int iIndex = m_ctrlList.GetCurSel();
      if( iIndex <= 0 ) return 0;
      m_lPid = (long) m_ctrlList.GetItemData(iIndex);
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
      if( lpDIS->itemID == 0 ) clrBack = ::GetSysColor(COLOR_BTNFACE);
      if( sText.Find(m_sProcessName) >= 0 ) clrText = ::GetSysColor(COLOR_HIGHLIGHT);
      if( (lpDIS->itemState & (ODS_SELECTED|ODS_FOCUS)) != 0 ) {
         clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
         clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
      }
      CDCHandle dc = lpDIS->hDC;
      dc.FillSolidRect(&lpDIS->rcItem, clrBack);
      dc.SetTextColor(clrText);
      dc.SetBkMode(TRANSPARENT);
      dc.TextOut(lpDIS->rcItem.left, lpDIS->rcItem.top, sText);
   }

   // ILineCallback

   STDMETHOD(OnIncomingLine)(BSTR bstr)
   {
      UINT cchLen = ::SysStringLen(bstr);
      if( cchLen == 0 ) return S_OK;
      // Determine the PID column position if this is the first entry...
      CString sLine = bstr;
      if( m_aList.GetSize() == 0 ) 
      {
         // A typical 'ps' command output looks like this:
         //     UID   PID  PPID  C    STIME TTY       TIME COMMAND
         //    bvik 12271 12270  0 09:36:31 pts/tb    0:00 -bash
         //    bvik 12327 12271  2 09:38:22 pts/tb    0:00 ps -f
         // We'll assume 5/6 digits for the PID entry.
         // The format varies greatly between platforms, but there seems
         // to be a consensus of using a right-aligned PID column.
         CString sUpperLine = sLine;
         sUpperLine.MakeUpper();
         m_iPidLinePos = sUpperLine.Find(_T("   PID "));
         if( m_iPidLinePos < 0 ) m_iPidLinePos = sUpperLine.Find(_T("  PID "));
         if( m_iPidLinePos < 0 ) m_iPidLinePos = sUpperLine.Find(_T(" PID "));
         if( m_iPidLinePos < 0 ) {
            m_iPidLinePos = sUpperLine.Find(_T("PID "));
            if( m_iPidLinePos != 0 ) return S_OK;  // Entry not found; abort!
         }
         // Add the column description as the first result
         PIDINFO Info;
         Info.sLine = sLine;
         Info.lPid = 0;
         m_aList.Add(Info);
         return S_OK;
      }
      // See if we can extract the PID value and then add
      // it to the result list...
      if( (int) cchLen <= m_iPidLinePos ) return S_OK;
      long lPid = _wtol(bstr + m_iPidLinePos);
      if( lPid == 0 ) return S_OK;
      PIDINFO Info;
      Info.sLine = sLine;
      Info.lPid = lPid;
      m_aList.Add(Info);
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
