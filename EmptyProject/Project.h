#if !defined(AFX_EMPTYPROJECT_H__20030307_E777_9147_A664_0080AD509054__INCLUDED_)
#define AFX_EMPTYPROJECT_H__20030307_E777_9147_A664_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

#include "Files.h"


class CEmptyProject : 
   public IProject,
   public IAppMessageListener,
   public IIdleListener,
   public ITreeMessageListener,
   public IWizardListener,
   public ICustomCommandListener
{
public:
   CEmptyProject();
   virtual ~CEmptyProject();

// Attributes
public:
   CWindow m_wndMain;
   bool m_bLoaded;

private:
   CString m_sName;
   CString m_sPath;
   CSimpleValArray<IView*> m_aFiles;
   bool m_bIsDirty;
   //
   static CAccelerator m_accel;

// IElement
public:
   BOOL Load(ISerializable* pArchive);
   BOOL Save(ISerializable* pArchive);
   BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   IDispatch* GetDispatch();

// IProject
public:
   BOOL Initialize(IDevEnv* pEnv, LPCTSTR pstrPath);
   BOOL IsDirty() const;
   BOOL GetClass(LPTSTR pstrType, UINT cchMax) const;
   BOOL Close();
   INT GetItemCount() const;
   IView* GetItem(INT iIndex);
   BOOL CreateProject();
   void ActivateProject();
   void DeactivateProject();
   void ActivateUI();
   void DeactivateUI();

// IAppMessageListener
public:
   LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);
   BOOL PreTranslateMessage(MSG* pMsg);

// IIdleListener
public:
   void OnIdle(IUpdateUI* pUIBase);

// ITreeMessageListener
public:
   LRESULT OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled);

// IWizardListener
public:
   BOOL OnInitProperties(IWizardManager* pManager, IElement* pElement);
   BOOL OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName);
   BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc);

// ICustomCommandListener
public:
   void OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled);
   void OnMenuCommand(LPCTSTR pstrType, LPCTSTR pstrCommand, LPCTSTR pstrArguments, LPCTSTR pstrPath, int iFlags, BOOL& bHandled);

// Operations
public:
   BOOL Reset();
   bool OpenView(LPCTSTR pstrFilename, long lLineNum);
   IView* FindView(LPCTSTR pstrFilename, bool bLocally = false) const;
   BOOL SetName(LPCTSTR pstrName);
   BOOL GetPath(LPTSTR pstrPath, UINT cchMax) const;
   static void InitializeToolBars();

// Message map and handlers
public:
   BEGIN_MSG_MAP(CEmptyProject)
      COMMAND_ID_HANDLER(ID_FILE_DELETE, OnFileRemove)
      COMMAND_ID_HANDLER(ID_FILE_RENAME, OnFileRename)      
      COMMAND_ID_HANDLER(ID_FILE_ADD_FOLDER, OnFileAddFolder)      
      COMMAND_ID_HANDLER(ID_FILE_ADD_LOCAL, OnFileAddLocal) 
      COMMAND_ID_HANDLER(ID_VIEW_OPEN, OnViewOpen)
      COMMAND_ID_HANDLER(ID_PROJECT_SET_DEFAULT, OnProjectSetDefault)      
      NOTIFY_HANDLER(IDC_TREE, TVN_BEGINLABELEDIT, OnTreeLabelBegin)
      NOTIFY_HANDLER(IDC_TREE, TVN_ENDLABELEDIT, OnTreeLabelEdit)
      NOTIFY_HANDLER(IDC_TREE, TVN_KEYDOWN, OnTreeKeyDown)
      NOTIFY_HANDLER(IDC_TREE, NM_DBLCLK, OnTreeDblClick)
      NOTIFY_HANDLER(IDC_TREE, NM_RCLICK, OnTreeRClick)
   END_MSG_MAP()

   LRESULT OnFileRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileRename(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileAddFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);   
   LRESULT OnFileAddLocal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);   
   LRESULT OnViewOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnProjectSetDefault(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);   
   LRESULT OnTreeLabelBegin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeLabelEdit(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeRClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

// Implementation
protected:
   enum
   {
      IMAGE_FOLDER = 0,
      IMAGE_TEXT = 4,
      IMAGE_JAVA = 3,
      IMAGE_BASIC = 10,
      IMAGE_HEADER = 41,
      IMAGE_CPP = 15,
      IMAGE_XML = 17,
      IMAGE_HTML = 18,
      IMAGE_MAKEFILE = 28,
   };

   void _InitializeData();
   bool _LoadSettings(ISerializable* pArc);
   bool _SaveSettings(ISerializable* pArc);
   bool _LoadFiles(ISerializable* pArc, IElement* pParent);
   bool _SaveFiles(ISerializable* pArc, IElement* pParent);
   void _PopulateTree(CTreeViewCtrl& ctrlTree, IElement* pParent, HTREEITEM hParent) const;
   IElement* _GetSelectedTreeElement(HTREEITEM* phItem = NULL) const;
   IElement* _GetDropTreeElement(HTREEITEM* phItem = NULL) const;
   int _GetElementImage(IElement* pElement) const;
   UINT _GetMenuPosFromID(HMENU hMenu, UINT ID) const;
   bool _CheckProjectFile(LPCTSTR pstrName);
   bool _AddCommandBarImages(UINT nRes) const;
   static void _AddButtonText(CToolBarCtrl tb, UINT nID, UINT nRes);
   static void _AddControlToToolbar(CToolBarCtrl tb, HWND hWnd, USHORT cx, UINT nCmdPos, bool bIsCommandId, bool bInsertBefore = true);

   static int CALLBACK _SortTreeCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};


#endif // !defined(AFX_EMPTYPROJECT_H__20030307_E777_9147_A664_0080AD509054__INCLUDED_)

