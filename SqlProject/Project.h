#if !defined(AFX_PROJECT_H__20030727_5698_0AA0_D624_0080AD509054__INCLUDED_)
#define AFX_PROJECT_H__20030727_5698_0AA0_D624_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CView;


class CSqlProject : 
   public IProject,
   virtual public IAppListener,
   virtual public IIdleListener,
   virtual public ITreeListener,
   virtual public IWizardListener,
   virtual public ICommandListener
{
public:
   CSqlProject();
   virtual ~CSqlProject();

   // IElement

   BOOL Load(ISerializable* pArchive);
   BOOL Save(ISerializable* pArchive);
   BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   IDispatch* GetDispatch();

   // IProject

   BOOL Initialize(IDevEnv* pEnv, LPCTSTR pstrPath);
   BOOL GetClass(LPTSTR pstrType, UINT cchMax) const;
   BOOL IsDirty() const;
   BOOL Close();
   BOOL CreateProject();
   BOOL SetName(LPCTSTR pstrName);
   void ActivateProject();
   void DeactivateProject();
   void ActivateUI();
   void DeactivateUI();
   IView* GetItem(INT iIndex);
   INT GetItemCount() const;

   // IAppListener

   LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   BOOL PreTranslateMessage(MSG* pMsg);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);

   // ITreeListener

   LRESULT OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled);

   // ICommandListener

   void OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled);

   // IWizardListener

   BOOL OnInitProperties(IWizardManager* pManager, IElement* pElement);
   BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc);
   BOOL OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName);

   // Message map and handlers

   BEGIN_MSG_MAP(CSqlProject)
      COMMAND_ID_HANDLER(ID_ADD_DATABASE, OnFileAddDatabase)       
      COMMAND_ID_HANDLER(ID_FILE_DELETE, OnFileRemove)       
      COMMAND_ID_HANDLER(ID_FILE_RENAME, OnFileRename)       
      COMMAND_ID_HANDLER(ID_PROJECT_SET_DEFAULT, OnProjectSetDefault)      
      COMMAND_ID_HANDLER(ID_VIEW_OPEN, OnViewOpen)       
      COMMAND_ID_HANDLER(ID_VIEW_PROPERTIES, OnViewProperties)
      NOTIFY_HANDLER(IDC_TREE, TVN_BEGINLABELEDIT, OnTreeLabelBegin)
      NOTIFY_HANDLER(IDC_TREE, TVN_ENDLABELEDIT, OnTreeLabelEdit)
      NOTIFY_HANDLER(IDC_TREE, TVN_KEYDOWN, OnTreeKeyDown)
      NOTIFY_HANDLER(IDC_TREE, NM_DBLCLK, OnTreeDblClick)
      NOTIFY_HANDLER(IDC_TREE, NM_RCLICK, OnTreeRClick)
   END_MSG_MAP()

   LRESULT OnFileAddDatabase(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);   
   LRESULT OnFileRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileRename(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnProjectSetDefault(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);   
   LRESULT OnTreeLabelBegin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeLabelEdit(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeRClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // Operations

   static void InitializeToolBars();

// Implementation
private:
   bool _LoadSettings(ISerializable* pArchive);
   bool _SaveSettings(ISerializable* pArchive);
   bool _LoadConnections(ISerializable* pArchive);
   bool _SaveConnections(ISerializable* pArchive);
   IElement* _GetSelectedTreeElement(HTREEITEM* phItem = NULL) const;
   bool _AddCommandBarImages(UINT nRes) const;

// Attributes
public:
   CWindow m_wndMain;                        // Reference to main window
   CAccelerator m_accel;
   bool m_bLoaded;

   CString m_sName;
   CString m_sPath;
   CSimpleValArray<CView*> m_aViews;         // List of connections
   CView* m_pWizardView;                     // Used to grab Wizard/Properties
   bool m_bIsDirty;

   static CToolBarCtrl m_ctrlToolbar;
};


#endif // !defined(AFX_PROJECT_H__20030727_5698_0AA0_D624_0080AD509054__INCLUDED_)

