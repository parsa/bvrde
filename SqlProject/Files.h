#if !defined(AFX_FILES_H__20030727_938D_6505_A32A_0080AD509054__INCLUDED_)
#define AFX_FILES_H__20030727_938D_6505_A32A_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Container.h"
#include "QueryThread.h"


///////////////////////////////////////////////////////7
//

class CSqlProject;


///////////////////////////////////////////////////////7
//

class CView : 
   public CDbOperations,
   public IView,
   public IAppListener,
   public IIdleListener
{
public:
   CView(CSqlProject* pProject, IElement* pParent, LPCTSTR pstrName = NULL, LPCTSTR pstrType = NULL, LPCTSTR pstrConnectionString = NULL);
   virtual ~CView();
  
   CSqlProject* m_pProject;
   IElement* m_pParent;
   CQueryThread m_thread;
   CWindow m_wndFrame;
   CContainerWindow m_view;
   CString m_sName;
   CString m_sConnectString;
   CString m_sType;
   CString m_sProvider;
   CString m_sServer;
   BOOL m_bDirty;

   int m_iHistoryPos;
   CSimpleArray<CString> m_aHistory;

   // IView

   BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   BOOL SetName(LPCTSTR pstrName);
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   IDispatch* GetDispatch();
   BOOL IsDirty() const;
   BOOL Load(ISerializable* pArc);
   BOOL Save(ISerializable* pArc);
   BOOL OpenView(long lPosition);
   void CloseView();
   void ActivateUI();
   void DeactivateUI();
   void EnableModeless(BOOL bEnable);
   IElement* GetParent() const;
   BOOL Save();
   BOOL GetText(BSTR* pbstrText);
   BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);

   // IIdleListener

   void OnIdle(IUpdateUI* /*pUIBase*/);

   // IAppListener

   LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   BOOL PreTranslateMessage(MSG* pMsg);

   // Operations

   BOOL Run(BOOL bSelectedText);
   BOOL Abort();
   BOOL IsQueryRunning() const;
   BOOL ChangeProperties(HWND hWnd);

   int GetHistoryPos() const;
   int GetHistoryCount() const;
   CString SetHistoryPos(int iPos);
   void DeleteHistory(int iPos);
   void CleanupHistory();

   // Implementation

   void SaveHistory(CString& sText);
};


#endif // !defined(AFX_FILES_H__20030727_938D_6505_A32A_0080AD509054__INCLUDED_)

