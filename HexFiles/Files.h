#if !defined(AFX_FILES_H__20030608_313B_82BA_55D7_0080AD509054__INCLUDED_)
#define AFX_FILES_H__20030608_313B_82BA_55D7_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HexEditor.h"


///////////////////////////////////////////////////////7
//

class CView : 
   public IView,
   public IAppMessageListener,
   public IIdleListener
{
public:
   IProject* m_pProject;
   IElement* m_pParent;
   //
   CWindow m_wndFrame;
   CHexEditorCtrl m_wndClient;
   //
   CString m_sName;
   CString m_sFilename;

public:
   CView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename);

   // IIdleListener

   void OnIdle(IUpdateUI* /*pUIBase*/);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // IAppMessageListener

   LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   BOOL PreTranslateMessage(MSG* pMsg);

   // IView

   BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   IDispatch* GetDispatch();
   BOOL IsDirty() const;
   BOOL Load(ISerializable* pArc);
   BOOL Save(ISerializable* pArc);
   void ActivateUI();
   void DeactivateUI();
   void EnableModeless(BOOL bEnable);
   IElement* GetParent() const;
   BOOL SetName(LPCTSTR pstrName);
   BOOL Save();
   BOOL Reload();
   BOOL OpenView(long lLineNum);
   void CloseView();
   BOOL GetText(BSTR* pbstrText);
   BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   LRESULT PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
   LRESULT SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


#endif // !defined(AFX_FILES_H__20030608_313B_82BA_55D7_0080AD509054__INCLUDED_)

