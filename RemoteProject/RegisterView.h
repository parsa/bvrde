#if !defined(AFX_REGISTERVIEW_H__20030517_26C5_F089_E118_0080AD509054__INCLUDED_)
#define AFX_REGISTERVIEW_H__20030517_26C5_F089_E118_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CRegisterView : 
   public CWindowImpl<CRegisterView, CListBox, CWinTraitsOR<LBS_MULTICOLUMN|LBS_NOINTEGRALHEIGHT> >
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_RegisterView"), CListBox::GetWndClassName())

   CRegisterView();

   CRemoteProject* m_pProject;
   CSimpleArray<CString> m_aNames;
   CSimpleArray<CString> m_aValues;
   TEXTMETRIC m_tm;
   bool m_bInitialResize;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);
   void EvaluateView(CSimpleArray<CString>& aDbgCmd);

   // Message map and handlers

   BEGIN_MSG_MAP(CRegisterView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};


#endif // !defined(AFX_REGISTERVIEW_H__20030517_26C5_F089_E118_0080AD509054__INCLUDED_)
