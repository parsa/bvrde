#if !defined(AFX_QUICKWATCHDLG_H__20030523_9015_9229_A38A_0080AD509054__INCLUDED_)
#define AFX_QUICKWATCHDLG_H__20030523_9015_9229_A38A_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class IDevEnv;
class CRemoteProject;


class CQuickWatchDlg : 
   public CDialogImpl<CQuickWatchDlg>,
   public COwnerDraw<CQuickWatchDlg>
{
public:
   enum { IDD = IDD_QUICKWATCH };

   enum { CX_INDENT = 18 };

   typedef struct tagITEM
   {
      CString sKey;
      CString sName;
      CString sValue;
      int iParent;
      int iIndent;
      bool bHasValue;
      bool bHasChildren;
      bool bExpanded;
   } ITEM;

   CComboBox m_ctrlLine;
   CContainedWindowT<CEdit> m_ctrlEdit;
   CContainedWindowT<CListBox> m_ctrlList;
   CIcon m_PlusIcon;
   CIcon m_MinusIcon;

   CRemoteProject* m_pProject;
   IDevEnv* m_pDevEnv;

   CString m_sDefault;
   CString m_sVariableName;
   CString m_sQueryVariable;
   int m_iQueryParent;
   CSimpleArray<ITEM> m_aItems;
   int m_iColumnWidth;

   CQuickWatchDlg(IDevEnv* pDevEnv, CRemoteProject* pProject, LPCTSTR pstrDefault);

   void SetInfo(LPCTSTR pstrType, CMiInfo& info);

   BEGIN_MSG_MAP(CQuickWatchDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_HANDLER(IDC_LINE, CBN_EDITCHANGE, OnEditChange)
      COMMAND_HANDLER(IDC_LIST, LBN_SELCHANGE, OnSelChange)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      CHAIN_MSG_MAP( COwnerDraw<CQuickWatchDlg> )
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_CHAR, OnEditChar)
   ALT_MSG_MAP(2)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnListClick)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnListClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
   void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

   void _CreateVariable();
   void _RemoveVariable();
   void _CalcColumnWidth();
   void _UpdateButtons();
   int _GetItemHeight(int iIndex) const;
};


#endif // !defined(AFX_QUICKWATCHDLG_H__20030523_9015_9229_A38A_0080AD509054__INCLUDED_)

