#if !defined(AFX_PLUGIN_H__20030304_FEC7_8E13_D7CB_0080AD509054__INCLUDED_)
#define AFX_PLUGIN_H__20030304_FEC7_8E13_D7CB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CPlugin
{
private:
   typedef VOID (CALLBACK* LPFNSETMENU)(HMENU);
   typedef VOID (CALLBACK* LPFNSETPOPUPMENU)(IElement*, HMENU);

   CLoadLibrary m_Lib;
   CString m_sFilename;
   CString m_sName;
   LONG m_lType;

   LPFNSETMENU m_pSetMenu;
   LPFNSETPOPUPMENU m_pSetPopupMenu;

public:
   void Init(LPCTSTR pstrFilename);
   BOOL LoadPackage(IDevEnv* pDevEnv);
   //
   CString GetFilename() const;
   CString GetName() const;
   CString GetDescription() const;
   LONG GetType() const;

   UINT QueryAcceptFile(LPCTSTR pstrFilename) const;
   IView* CreateView(LPCTSTR pstrFilename, IProject* pProject, IElement* pParent) const;

   IProject* CreateProject();
   BOOL DestroyProject(IProject* pProject);

   void SetMenu(HMENU hMenu);
   void SetPopupMenu(IElement* pElement, HMENU hMenu);
};


#endif // !defined(AFX_PLUGIN_H__20030304_FEC7_8E13_D7CB_0080AD509054__INCLUDED_)

