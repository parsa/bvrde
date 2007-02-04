// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__D63F65B2_035E_41E4_92EF_C3AF67F1E01E__INCLUDED_)
#define AFX_MAINFRM_H__D63F65B2_035E_41E4_92EF_C3AF67F1E01E__INCLUDED_

#pragma once

#include "atldib.h"        // For the MenuShadows
#include "MenuShadows.h"

#include "atlctrlxp.h"
#include "atlctrlxp2.h"
#include "atldock.h"
#include "atldock2.h"

#include "DummyElement.h"  // For the MDIContainer

#include "CoolTabCtrls.h"
#include "MDIContainer.h"
#include "AutoHideXp.h"

#include "ExplorerView.h"
#include "PropertiesView.h"
#include "OutputView.h"
#include "CommandView.h"
#include "UpdateUI.h"
#include "XmlSerializer.h"


class CMainFrame : 
   public CMDIFrameWindowImpl<CMainFrame>, 
   public CUpdateDynamicUI<CMainFrame>,
   public CMDICommands<CMainFrame>,
   public CMessageFilter, 
   public CIdleHandler,
   public IUpdateUI,
   public IWizardListener,
   public IDevEnv
{
public:
   DECLARE_FRAME_WND_CLASS(_T("BVRDE_Main"), IDR_MAINFRAME)

   CMainFrame();

   enum 
   { 
      ANIMATE_TIMERID = 22,
      DELAY_TIMERID = 23,
   };

   typedef struct TOOLBAR
   {
      TCHAR szID[64];       // ID of toolbar
      TCHAR szName[100];    // Name of toolbar
      int iPosition;        // Position of band in ReBar
      HWND hWnd;            // Handle to ToolBarCtrl window
      int nBand;            // Rebard insert index
      BOOL bShowDefault;    // Show as default
      BOOL bNewRow;         // Is on new line?
   };

   CMDICommandBarXPCtrl m_CmdBar;
   CReBarCtrl m_Rebar;
   CToolBarCtrl m_DefaultToolBar;
   CToolBarCtrl m_FullScreenToolBar;
   CMultiPaneStatusBarXPCtrl m_StatusBar;
   CImageListCtrl m_Images;
   CAutoHideXP m_AutoHide;
   CRecentDocumentList m_mru;
   CMDIContainer m_MDIContainer;
   CDotNetDockingWindow m_Dock;
   CAccelerator m_UserAccel;
   CAccelerator m_MacroAccel;
   CApplicationOM m_Dispatch;
   CImageList m_AnimateImages;
   //
   CExplorerView m_viewExplorer;
   CPropertiesView m_viewProperties;
   COutputView m_viewOutput;
   CCommandView m_viewCommand;
   //
   CSimpleValArray<IAppMessageListener*> m_aAppListeners;
   CSimpleValArray<IIdleListener*> m_aIdleListeners;
   CSimpleValArray<ITreeMessageListener*> m_aTreeListeners;
   CSimpleValArray<IViewMessageListener*> m_aViewListeners;
   CSimpleValArray<IWizardListener*> m_aWizardListeners;
   CSimpleValArray<ICustomCommandListener*> m_aCommandListeners;
   CHashMap<CString, CString> m_aProperties;
   CSimpleArray<TOOLBAR> m_aToolBars;      // Collection of toolbars
   CString m_sMacro;                       // Currently recorded macro content
   HGLOBAL m_hDevMode;                     // Printer setup
   HGLOBAL m_hDevNames;                    // Printer name
   RECT m_rcPageMargins;                   // Print page margins
   BOOL m_bInitialized;                    // App fully initialized (all plugins loaded)?
   BOOL m_bFullScreen;                     // Running in full-screen mode?
   BOOL m_bRecordingMacro;                 // Currently recording a macro?
   int m_iAnimatePos;                      // Frame position of statusbar animation
   LCID m_Locale;

   // UI Updates

   BEGIN_UPDATE_UI_MAP(CMainFrame)
      UPDATE_ELEMENT(0, UPDUI_STATUSBAR)
      UPDATE_ELEMENT(1, UPDUI_STATUSBAR)
      UPDATE_ELEMENT(2, UPDUI_STATUSBAR)
      UPDATE_ELEMENT(ID_APP_EXIT, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_FILE_OPENSOLUTION, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_FILE_CLOSESOLUTION, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_FILE_PRINT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_FILE_PRINT_SETUP, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_FILE_SAVE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_FILE_SAVE_ALL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_NEW_PROJECT, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_NEW_SOLUTION, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_ADD_PROJECT, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_EDIT_UNDO, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_REDO, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_COPY, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_CUT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_PASTE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_FIND, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_EDIT_REPLACE, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_EDIT_SELECT_ALL, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_EDIT_CLEAR, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_EDIT_GOTO, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_OPEN, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_EXPLORER, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_PROPERTYBAR, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_OUTPUT, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_COMMAND, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_FULLSCREEN, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_PROPERTIES, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_TOOLS_CUSTOMIZE, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_MACRO_RECORD, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_MACRO_CANCEL, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_MACRO_SAVE, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_MACRO_PLAY, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_VIEW, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_CLOSE, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_CLOSE_ALL, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_CASCADE, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_TILE_HORZ, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_TILE_VERT, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_ARRANGE, UPDUI_MENUPOPUP)      
      UPDATE_ELEMENT(ID_WINDOW_PREVIOUS, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_WINDOW_NEXT, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_HELP_CONTENTS, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_HELP_INDEX, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_HELP_SEARCH, UPDUI_MENUPOPUP)
   END_UPDATE_UI_MAP()

   // Message map and handlers

   BEGIN_MSG_MAP(CMainFrame)
      // To be able to block WM_COMMANDS we need this guy at the top
      MESSAGE_HANDLER(WM_COMMAND, OnCommand)
      CHAIN_MSG_MAP( CUpdateDynamicUI<CMainFrame> )
      // Let the listeners get a shot first
      MESSAGE_RANGE_HANDLER(0x0000, 0xFFFF, OnMessage)
      // Standard processing
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_CLOSE, OnClose)
      MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
      MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
      MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDisplayChange)
      MESSAGE_HANDLER(WM_TIMER, OnTimer)
      MESSAGE_HANDLER(WM_HELP, OnHelp)
      MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
      COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
      COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
      COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
      COMMAND_ID_HANDLER(ID_FILE_SAVE_ALL, OnFileSaveAll)
      COMMAND_ID_HANDLER(ID_FILE_PRINT_SETUP, OnFilePrintSetup)
      COMMAND_ID_HANDLER(ID_FILE_STARTWIZARD, OnFileStartWizard)
      COMMAND_ID_HANDLER(ID_FILE_OPENSOLUTION, OnFileOpenSolution)
      COMMAND_ID_HANDLER(ID_FILE_CLOSESOLUTION, OnFileCloseSolution)     
      COMMAND_ID_HANDLER(ID_NEW_SOLUTION, OnNewSolution)
      COMMAND_ID_HANDLER(ID_NEW_PROJECT, OnNewProject)
      COMMAND_ID_HANDLER(ID_ADD_PROJECT, OnAddProject)
      COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
      COMMAND_ID_HANDLER(ID_VIEW_EXPLORER, OnViewExplorer)
      COMMAND_ID_HANDLER(ID_VIEW_PROPERTYBAR, OnViewPropertyBar)
      COMMAND_ID_HANDLER(ID_VIEW_PROPERTIES, OnViewProperties)
      COMMAND_ID_HANDLER(ID_VIEW_COMMAND, OnViewCommand)
      COMMAND_ID_HANDLER(ID_VIEW_OUTPUT, OnViewOutput)
      COMMAND_ID_HANDLER(ID_VIEW_PROPERTIES, OnViewProperties)
      COMMAND_ID_HANDLER(ID_VIEW_FULLSCREEN, OnViewFullScreen)
      COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
      COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)      
      COMMAND_ID_HANDLER(ID_TOOLS_EXTERNAL, OnToolsExternal)
      COMMAND_ID_HANDLER(ID_TOOLS_OPTIONS, OnToolsOptions)
      COMMAND_ID_HANDLER(ID_TOOLS_CUSTOMIZE, OnToolsCustomize)
      COMMAND_ID_HANDLER(ID_TOOLS_ADDIN_MANAGER, OnToolsAddinManager)
      COMMAND_ID_HANDLER(ID_MACRO_RECORD, OnMacroRecord)
      COMMAND_ID_HANDLER(ID_MACRO_CANCEL, OnMacroCancel)
      COMMAND_ID_HANDLER(ID_MACRO_PLAY, OnMacroPlay)
      COMMAND_ID_HANDLER(ID_MACRO_SAVE, OnMacroSave)
      COMMAND_ID_HANDLER(ID_MACRO_MANAGER, OnMacroManager)    
      COMMAND_ID_HANDLER(ID_WINDOW_VIEW, OnWindowView)
      COMMAND_ID_HANDLER(ID_WINDOW_CLOSE, OnWindowClose)
      COMMAND_ID_HANDLER(ID_WINDOW_CLOSE_ALL, OnWindowCloseAll)
      COMMAND_ID_HANDLER(ID_WINDOW_NEXT, OnWindowNext)
      COMMAND_ID_HANDLER(ID_WINDOW_PREVIOUS, OnWindowPrevious)
      COMMAND_ID_HANDLER(ID_HELP_CONTENTS, OnHelpContents)
      COMMAND_ID_HANDLER(ID_HELP_SEARCH, OnHelpSearch)
      COMMAND_ID_HANDLER(ID_HELP_INDEX, OnHelpIndex)      
      COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
      COMMAND_RANGE_HANDLER(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE4, OnFileRecent)
      COMMAND_RANGE_HANDLER(ID_TOOLS_TOOL1, ID_TOOLS_TOOL8, OnToolsRun)
      COMMAND_RANGE_HANDLER(ID_MACROS_KEY1, ID_MACROS_KEY15, OnMacroShortcut)
      NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, OnToolTipText)
      NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnToolBarDropDown)
      NOTIFY_CODE_HANDLER(RBN_LAYOUTCHANGED, OnRebarLayoutChanged)
      NOTIFY_CODE_HANDLER(NM_RCLICK, OnRebarRClick)
      // Internal messaging
      MESSAGE_HANDLER(WM_APP_INIT, OnUserInit)
      MESSAGE_HANDLER(WM_APP_IDLETIME, OnUserIdle)
      MESSAGE_HANDLER(WM_APP_LOADSOLUTION, OnUserLoadSolution)
      MESSAGE_HANDLER(WM_APP_BUILDSOLUTIONUI, OnUserBuildSolutionUI)
      MESSAGE_HANDLER(WM_APP_TREEMESSAGE, OnUserTreeMessage)
      MESSAGE_HANDLER(WM_APP_VIEWMESSAGE, OnUserViewMessage)
      MESSAGE_HANDLER(WM_APP_VIEWCHANGE, OnUserViewChange)
      MESSAGE_HANDLER(WM_APP_UPDATELAYOUT, OnUserUpdateLayout)
      MESSAGE_HANDLER(WM_APP_PROJECTCHANGE, OnUserProjectChange)
      MESSAGE_HANDLER(WM_APP_COMMANDLINE, OnUserCommandLine)
      MESSAGE_HANDLER(WM_APP_CLOSESTARTPAGE, OnUserCloseStartPage)
      // Views automatically gets WM_COMMAND messages
      CHAIN_MDI_CHILD_COMMANDS()
      // Default handling
      CHAIN_MSG_MAP( CMDICommands<CMainFrame> )
      CHAIN_MSG_MAP( CMDIFrameWindowImpl<CMainFrame> )
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDropFiles(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileSaveAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFilePrintSetup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileRecent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileOpenSolution(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileCloseSolution(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileStartWizard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnNewSolution(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnNewProject(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnAddProject(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnToolsExternal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnToolsOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnToolsCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnToolsAddinManager(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnToolsRun(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMacroManager(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMacroRecord(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMacroCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMacroPlay(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMacroSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMacroShortcut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewExplorer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewPropertyBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewOutput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewFullScreen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewToolBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewStatusBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnWindowView(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnWindowClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnWindowCloseAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnWindowNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnWindowPrevious(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnHelpContents(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnHelpIndex(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnHelpSearch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnAppAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnToolBarDropDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnToolTipText(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnRebarLayoutChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnRebarRClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnUserInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserIdle(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserLoadSolution(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserBuildSolutionUI(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserTreeMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserViewMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserViewChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserUpdateLayout(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserProjectChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserCommandLine(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserCloseStartPage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

   // IDevEnv

   DWORD GetVersion() const;
   ISolution* GetSolution() const;
   IView* GetActiveView() const;
   IDispatch* GetDispatch();
   LCID GetLCID() const;
   //
   BOOL AddExplorerView(HWND hWnd, LPCTSTR pstrTitle, int iImage);
   BOOL RemoveExplorerView(HWND hWnd);
   BOOL AddAutoHideView(HWND hWnd, IDE_DOCK_TYPE Direction, int iImage);
   BOOL RemoveAutoHideView(HWND hWnd);
   BOOL ActivateAutoHideView(HWND hWnd);
   BOOL AddDockView(HWND hWnd, IDE_DOCK_TYPE Direction, RECT rcWin);
   BOOL RemoveDockView(HWND hWnd);
   BOOL GetDockState(HWND hWnd, int& iState, RECT& rcWin);
   BOOL AddToolBar(HWND hWnd, LPCTSTR pstrID, LPCTSTR pstrTitle);
   BOOL RemoveToolBar(HWND hWnd);
   BOOL ShowToolBar(HWND hWnd, BOOL bShow = TRUE, BOOL bUseDefault = TRUE);
   IViewFrame* CreateClient(LPCTSTR pstrTitle, IProject* pProject, IView* pView);
   BOOL DestroyClient(HWND hWnd);
   //
   IView* CreateView(LPCTSTR pstrFilename, IProject* pProject = NULL, IElement* pElement = NULL);
   IDispatch* CreateStdDispatch(LPCTSTR pstrType, IElement* pElement);
   //
   void EnableModeless(BOOL bEnable);
   HWND GetHwnd(IDE_HWND_TYPE WinType) const;
   HMENU GetMenuHandle(IDE_HWND_TYPE WinType) const;
   BOOL GetPrinterInfo(HGLOBAL& hDevMode, HGLOBAL& hDevNames, RECT& rcMargins) const;
   HIMAGELIST GetImageList(IDE_HWND_TYPE WinType) const;
   BOOL AddCommandBarImage(UINT nCmd, HICON hIcon);
   BOOL ReserveUIRange(UINT iStart, UINT iEnd);
   UINT ShowPopupMenu(IElement* pElement, HMENU hMenu, POINT pt, BOOL bSendCommand = TRUE, IIdleListener* pListener = NULL);
   BOOL ShowWizard(IDE_WIZARD_TYPE Type, IProject* pProject);
   BOOL ShowConfiguration(IElement* pElement);
   BOOL ShowProperties(IElement* pElement, BOOL bForceVisible);
   UINT ShowMessageBox(HWND hWnd, LPCTSTR pstrMessage, LPCTSTR pstrCaption, DWORD dwFlags);
   BOOL ShowStatusText(UINT nID, LPCTSTR pstrText, BOOL bPermanent = FALSE);
   BOOL PlayAnimation(BOOL bStart, UINT uType);
   BOOL RecordMacro(LPCTSTR pstrText);
   //
   BOOL GetProperty(LPCTSTR pstrKey, LPTSTR pstrValue, UINT cchMax);
   BOOL SetProperty(LPCTSTR pstrKey, LPCTSTR pstrValue);
   BOOL EnumProperties(int& iStart, LPCTSTR pstrPattern, LPTSTR pstrKey, LPTSTR pstrValue);
   //
   BOOL AddAppListener(IAppMessageListener* pListener);
   BOOL RemoveAppListener(IAppMessageListener* pListener);
   BOOL AddIdleListener(IIdleListener* pListener);
   BOOL RemoveIdleListener(IIdleListener* pListener);
   BOOL AddTreeListener(ITreeMessageListener* pListener);
   BOOL RemoveTreeListener(ITreeMessageListener* pListener);
   BOOL AddViewListener(IViewMessageListener* pListener);
   BOOL RemoveViewListener(IViewMessageListener* pListener);
   BOOL AddCommandListener(ICustomCommandListener* pListener);
   BOOL RemoveCommandListener(ICustomCommandListener* pListener);
   BOOL AddWizardListener(IWizardListener* pListener);
   BOOL RemoveWizardListener(IWizardListener* pListener);

   // IWizardListener

   BOOL OnInitProperties(IWizardManager* pManager, IElement* pElement);
   BOOL OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName);
   BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc);

   // IUpdateUI

   BOOL UIEnable(INT nID, BOOL bEnable, BOOL bForceUpdate = FALSE);
   BOOL UISetCheck(INT nID, INT nCheck, BOOL bForceUpdate = FALSE);
   BOOL UISetRadio(INT nID, BOOL bRadio, BOOL bForceUpdate = FALSE);
   BOOL UISetText(INT nID, LPCTSTR pstrText, BOOL bForceUpdate = FALSE);

   // CMessageFilter

   virtual BOOL PreTranslateMessage(MSG* pMsg);
   virtual BOOL OnIdle();

   // Operations

   BOOL LoadWindowPos();

   // Implementation

   UINT _ShowMessageBox(HWND hWnd, UINT nMessage, UINT nCaption, DWORD dwFlags);
   void _AddTextButton(CToolBarCtrl tb, UINT nID, UINT nRes);
   void _AddDropDownButton(CToolBarCtrl tb, UINT nID);
   bool _IsSettingsLoaded() const;
   void _SaveSettings();
   bool _LoadSettings(CXmlSerializer& arc);
   void _AddProperty(ISerializable* pArc, LPCTSTR pstrAttribute, LPCTSTR pstrKey);
   void _StoreProperty(ISerializable* pArc, LPCTSTR pstrAttribute, LPCTSTR pstrKey);
   void _LoadUIState();
   void _SaveUIState();
   void _SaveToolBarState();
   void UIReset();
   void UISetMenu(HMENU hMenu);
   void _ArrangeToolBars();
   void _BuildSolutionUI();
   IProject* _CreateSolutionWizard();
   IProject* _CreateProjectWizard();
   IProject* _CreateFileWizard(IProject* pProject);

   static CString GetSettingsFilename();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__D63F65B2_035E_41E4_92EF_C3AF67F1E01E__INCLUDED_)
