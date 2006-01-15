#if !defined(AFX_FILES_H__20030608_313B_82BA_55D7_0080AD509054__INCLUDED_)
#define AFX_FILES_H__20030608_313B_82BA_55D7_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Container.h"


///////////////////////////////////////////////////////7
// 

class CViewImpl : 
   public IView,
   public IAppMessageListener,
   public IIdleListener
{
public:
   IProject* m_pProject;
   IElement* m_pParent;
   //
   CWindow m_wndFrame;
   CContainerWindow m_wndClient;
   //
   CString m_sName;
   CString m_sFilename;
   CString m_sLanguage;
   BOOL m_bIsDirty;

public:
   CViewImpl(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename);

   // IIdleListener

   void OnIdle(IUpdateUI* /*pUIBase*/);

   // IAppMessageListener

   LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);
   BOOL PreTranslateMessage(MSG* pMsg);

   // IView

   virtual BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   virtual IDispatch* GetDispatch();
   virtual BOOL IsDirty() const;
   virtual BOOL Load(ISerializable* pArc);
   virtual BOOL Save(ISerializable* pArc);
   virtual void ActivateUI();
   virtual void DeactivateUI();
   virtual void EnableModeless(BOOL bEnable);
   virtual IElement* GetParent() const;
   virtual BOOL SetName(LPCTSTR pstrName);
   virtual BOOL Save();
   virtual BOOL Reload();
   virtual BOOL OpenView(long lLineNum);
   virtual void CloseView();
   virtual BOOL GetText(BSTR* pbstrText);
   virtual BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   virtual LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   virtual LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   virtual BOOL GetType(LPTSTR pstrType, UINT cchMax) const = 0;
};


///////////////////////////////////////////////////////7
//

class CHtmlView : public CViewImpl
{
public:
   CHtmlView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename);

   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CPhpView : public CViewImpl
{
public:
   CPhpView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename);

   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CAspView : public CViewImpl
{
public:
   CAspView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename);

   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CXmlView : public CViewImpl
{
public:
   CXmlView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename);

   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


#endif // !defined(AFX_FILES_H__20030608_313B_82BA_55D7_0080AD509054__INCLUDED_)

