#if !defined(AFX_MEMORYVIEW_H__20040710_5EB6_EA46_4F33_0080AD509054__INCLUDED_)
#define AFX_MEMORYVIEW_H__20040710_5EB6_EA46_4F33_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject; // Forward declare


class CMemoryView : 
   public CWindowImpl<CMemoryView>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_MemoryView"))

   CMemoryView();

   CRemoteProject* m_pProject;
   CContainedWindowT<CEdit> m_ctrlAddress;
   CEdit m_ctrlMemory;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);

   // Message map and handlers

   BEGIN_MSG_MAP(CMemoryView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_CHAR, OnEditChar)
      MESSAGE_HANDLER(WM_KEYDOWN, OnEditKeyDown)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // !defined(AFX_MEMORYVIEW_H__20040710_5EB6_EA46_4F33_0080AD509054__INCLUDED_)

