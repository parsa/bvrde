#ifndef __BVRDE_SDK_H__
#define __BVRDE_SDK_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////
// Defines

#define REG_BVRDE _T("SOFTWARE\\Viksoe.dk\\BVRDE")

// Plugin types
#define PLUGIN_PROJECT     0x00000001
#define PLUGIN_FILETYPE    0x00000002
#define PLUGIN_EXTENSION   0x00000004

// Message Box extras
#define MB_SHOWONCE        0x20000000L
#define MB_MODELESS        0x40000000L
#define MB_REMOVABLE       0x80000000L

// Resources (includes standard WTL includes from atlres.h)
#define ID_CMD_FIRST                0xE400
#define ID_FILE_STARTWIZARD         0xE400
#define ID_FILE_OPENSOLUTION        0xE401
#define ID_FILE_CLOSESOLUTION       0xE402
#define ID_FILE_SAVE_ALL            0xE403
#define ID_NEW_SOLUTION             0xE404
#define ID_NEW_PROJECT              0xE405
#define ID_ADD_PROJECT              0xE406
#define ID_EDIT_GOTO                0xE407
#define ID_EDIT_DELETE              0xE408
#define ID_VIEW_EXPLORER            0xE409
#define ID_WINDOW_VIEW              0xE40A
#define ID_WINDOW_CLOSE             0xE40B
#define ID_WINDOW_CLOSE_ALL         0xE40C
#define ID_TOOLS_OPTIONS            0xE40D
#define ID_TOOLS_CUSTOMIZE          0xE40E
#define ID_TOOLS_EXTERNAL           0xE40F
#define ID_VIEW_PROPERTIES          0xE410
#define ID_VIEW_OUTPUT              0xE412
#define ID_VIEW_COMMAND             0xE413
#define ID_VIEW_OPEN                0xE414
#define ID_HELP_STARTPAGE           0xE415
#define ID_HELP_SEARCH              0xE416
#define ID_HELP_CONTENTS            0xE417
#define ID_PROCESS                  0xE418
#define ID_MACRO_RECORD             0xE419
#define ID_MACRO_CANCEL             0xE41A

#define IDC_TREE                    0xEC00
#define ID_SB_PANE1                 0xEC01
#define ID_SB_PANE2                 0xEC02
#define ID_SB_PANE3                 0xEC03

#define ANIM_BUILD                  1
#define ANIM_PRINT                  2
#define ANIM_TRANSFER               3
#define ANIM_SAVE                   4



/////////////////////////////////////////////////////////
// Forward declares

class ISolution;
class IProject;
class IView;
class IElement;


/////////////////////////////////////////////////////////
// Line Callback COM interface

MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC0")
ILineCallback : public IUnknown
{
public:
   virtual HRESULT STDMETHODCALLTYPE OnIncomingLine(BSTR bstr) = 0;
};


/////////////////////////////////////////////////////////
// ISerializable

class ISerializable
{
public:
   virtual BOOL ReadGroupBegin(LPCTSTR pstrName) = 0;
   virtual BOOL ReadGroupEnd() = 0;
   virtual BOOL ReadItem(LPCTSTR pstrName) = 0;
   virtual BOOL Read(LPCTSTR pstrName, LPTSTR szValue, UINT cchMax) = 0;
   virtual BOOL Read(LPCTSTR pstrName, SYSTEMTIME& stValue) = 0;
   virtual BOOL Read(LPCTSTR pstrName, long& lValue) = 0;
   virtual BOOL Read(LPCTSTR pstrName, BOOL& bValue) = 0;
   //
   virtual BOOL WriteGroupBegin(LPCTSTR pstrName) = 0;
   virtual BOOL WriteGroupEnd() = 0;
   virtual BOOL WriteItem(LPCTSTR pstrName) = 0;
   virtual BOOL Write(LPCTSTR pstrName, LPCTSTR pstrValue) = 0;
   virtual BOOL Write(LPCTSTR pstrName, SYSTEMTIME stValue) = 0;
   virtual BOOL Write(LPCTSTR pstrName, long lValue) = 0;
   virtual BOOL Write(LPCTSTR pstrName, BOOL bValue) = 0;
   virtual BOOL WriteExternal(LPCTSTR pstrName) = 0;
   //
   virtual BOOL Delete(LPCTSTR pstrName) = 0;
};


/////////////////////////////////////////////////////////
// Misc interfaces

class IUpdateUI
{
public:
   virtual BOOL UIEnable(INT nID, BOOL bEnable, BOOL bForceUpdate = FALSE) = 0;
   virtual BOOL UISetCheck(INT nID, INT nCheck, BOOL bForceUpdate = FALSE) = 0;
   virtual BOOL UISetRadio(INT nID, BOOL bRadio, BOOL bForceUpdate = FALSE) = 0;
   virtual BOOL UISetText(INT nID, LPCTSTR lpstrText, BOOL bForceUpdate = FALSE) = 0;
};

class IViewFrame
{
public:
   virtual HWND GetHwnd() const = 0;
   virtual HWND SetClient(HWND hWnd) = 0;
};

class IWizardManager
{
public:
   virtual BOOL AddWizardGroup(LPCTSTR pstrParent, LPCTSTR pstrName) = 0;
   virtual BOOL AddWizardPage(UINT nID, LPCPROPSHEETPAGE hPage) = 0;
   virtual BOOL SetWizardGroup(LPCTSTR pstrName) = 0;
};


/////////////////////////////////////////////////////////
// Listerner interfaces

class IAppListener
{
public:
   virtual LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) = 0;
   virtual BOOL PreTranslateMessage(MSG* pMsg) = 0;
};

class IIdleListener
{
public:
   virtual void OnIdle(IUpdateUI* pUIBase) = 0;
};

class ITreeListener
{
public:
   virtual LRESULT OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled) = 0;
};

class IViewListener
{
public:
   virtual LRESULT OnViewMessage(IView* pView, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) = 0;
};

class ICommandListener
{
public:
   virtual void OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled) = 0;
};

class IWizardListener
{
public:
   virtual BOOL OnInitProperties(IWizardManager* pManager, IElement* pElement) = 0;
   virtual BOOL OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName) = 0;
   virtual BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc) = 0;
};


/////////////////////////////////////////////////////////
// IDevEnv

typedef enum IDE_UI_TYPE
{
   IDE_UI_PROJECT = 0,
   IDE_UI_VIEW = 1,
};

typedef enum IDE_DOCK_TYPE
{
   IDE_DOCK_LEFT = 0,
   IDE_DOCK_TOP = 1,
   IDE_DOCK_RIGHT = 2,
   IDE_DOCK_BOTTOM = 3,
   IDE_DOCK_FLOAT = 4,
   IDE_DOCK_HIDE = 5,
};

typedef enum IDE_HWND_TYPE
{
   IDE_HWND_MAIN = 0,
   IDE_HWND_REBAR,
   IDE_HWND_COMMANDBAR,
   IDE_HWND_TOOLBAR,
   IDE_HWND_STATUSBAR,
   IDE_HWND_AUTOHIDE,
   IDE_HWND_DOCK,
   IDE_HWND_MDICLIENT,
   IDE_HWND_TABS,
   IDE_HWND_EXPLORER_TREE,
   IDE_HWND_OUTPUTVIEW,
   IDE_HWND_COMMANDVIEW,
};

typedef enum IDE_WIZARD_TYPE
{
   IDE_WIZARD_SOLUTION = 0,
   IDE_WIZARD_PROJECT,
   IDE_WIZARD_FILE,
};


class IDevEnv
{
public:
   virtual DWORD GetVersion() const = 0;
   virtual ISolution* GetSolution() const = 0;
   virtual IDispatch* GetDispatch() = 0;
   //
   virtual BOOL AddExplorerView(HWND hWnd, LPCTSTR pstrTitle, int iImage) = 0;
   virtual BOOL RemoveExplorerView(HWND hWnd) = 0;
   virtual BOOL AddAutoHideView(HWND hWnd, IDE_DOCK_TYPE Direction, int iImage) = 0;
   virtual BOOL RemoveAutoHideView(HWND hWnd) = 0;
   virtual BOOL ActivateAutoHideView(HWND hWnd) = 0;
   virtual BOOL AddDockView(HWND hWnd, IDE_DOCK_TYPE Direction, RECT rcWin) = 0;
   virtual BOOL RemoveDockView(HWND hWnd) = 0;
   virtual BOOL GetDockState(HWND hWnd, int& iState, RECT& rcWin) = 0;
   virtual BOOL AddToolBar(HWND hWnd, LPCTSTR pstrTitle) = 0;
   virtual BOOL RemoveToolBar(HWND hWnd) = 0;
   virtual BOOL ShowToolBar(HWND hWnd, BOOL bShow = TRUE) = 0;
   virtual IViewFrame* CreateClient(LPCTSTR pstrTitle, IProject* pProject, IView* pView) = 0;
   virtual BOOL DestroyClient(HWND hWnd) = 0;
   //
   virtual IView* CreateView(LPCTSTR pstrFilename, IProject* pProject = NULL, IElement* pParent = NULL) = 0;
   virtual IDispatch* CreateStdDispatch(LPCTSTR pstrType, IElement* pElement) = 0;
   //
   virtual void EnableModeless(BOOL bEnable) = 0;
   virtual HWND GetHwnd(IDE_HWND_TYPE WinType) const = 0;
   virtual HMENU GetMenuHandle(IDE_HWND_TYPE WinType) const = 0;
   virtual BOOL GetPrinterInfo(HGLOBAL& hDevMode, HGLOBAL& hDevNames, RECT& rcMargins) const = 0;
   virtual HIMAGELIST GetImageList(IDE_HWND_TYPE  WinType) const = 0;
   virtual BOOL ReserveUIRange(UINT iStart, UINT iEnd) = 0;
   virtual BOOL AddCommandBarImage(UINT nCmd, HICON hIcon) = 0;
   virtual UINT ShowPopupMenu(IElement* pElement, HMENU hMenu, POINT pt, BOOL bSendCommand = TRUE, IIdleListener* pListener = NULL) = 0;
   virtual BOOL ShowWizard(IDE_WIZARD_TYPE Type, IProject* pProject) = 0;
   virtual BOOL ShowConfiguration(IElement* pElement) = 0;
   virtual BOOL ShowProperties(IElement* pElement, BOOL bForceVisible) = 0;
   virtual UINT ShowMessageBox(HWND hWnd, LPCTSTR pstrMessage, LPCTSTR pstrCaption, DWORD dwFlags) = 0;
   virtual BOOL ShowStatusText(UINT nID, LPCTSTR pstrText, BOOL bPermanent = FALSE) = 0;
   virtual BOOL PlayAnimation(BOOL bStart, UINT uType) = 0;
   virtual BOOL RecordMacro(LPCTSTR pstrText) = 0;
   //
   virtual BOOL SetProperty(LPCTSTR pstrKey, LPCTSTR pstrValue) = 0;
   virtual BOOL GetProperty(LPCTSTR pstrKey, LPTSTR pstrValue, UINT cchMax) = 0;
   virtual BOOL EnumProperties(int& iStart, LPCTSTR pstrPattern, LPTSTR pstrKey, LPTSTR pstrValue) = 0;
   //
   virtual BOOL AddAppListener(IAppListener* pListener) = 0;
   virtual BOOL RemoveAppListener(IAppListener* pListener) = 0;
   virtual BOOL AddIdleListener(IIdleListener* pListener) = 0;
   virtual BOOL RemoveIdleListener(IIdleListener* pListener) = 0;
   virtual BOOL AddTreeListener(ITreeListener* pListener) = 0;
   virtual BOOL RemoveTreeListener(ITreeListener* pListener) = 0;
   virtual BOOL AddViewListener(IViewListener* pListener) = 0;
   virtual BOOL RemoveViewListener(IViewListener* pListener) = 0;
   virtual BOOL AddCommandListener(ICommandListener* pListener) = 0;
   virtual BOOL RemoveCommandListener(ICommandListener* pListener) = 0;
   virtual BOOL AddWizardListener(IWizardListener* pListener) = 0;
   virtual BOOL RemoveWizardListener(IWizardListener* pListener) = 0;
};


/////////////////////////////////////////////////////////
// IProject

class IElement
{
public:
   virtual ~IElement() { };
   virtual BOOL Load(ISerializable* pArchive) = 0;
   virtual BOOL Save(ISerializable* pArchive) = 0;
   virtual BOOL GetName(LPTSTR pstrName, UINT cchMax) const = 0;
   virtual BOOL GetType(LPTSTR pstrType, UINT cchMax) const = 0;
   virtual IDispatch* GetDispatch() = 0;
};

class ISolution : public IElement
{
public:
   virtual void Close() = 0;
   virtual BOOL LoadSolution(LPCTSTR pstrFilename) = 0;
   virtual BOOL SaveSolution(LPCTSTR pstrFilename) = 0;
   virtual BOOL GetFileName(LPTSTR pstrFilename, UINT cchMax) const = 0;
   virtual BOOL IsLoaded() const = 0;
   virtual BOOL IsDirty() const = 0;
   virtual IProject* GetItem(INT iIndex) = 0;
   virtual INT GetItemCount() const = 0;
   virtual BOOL AddProject(LPCTSTR pstrFilename) = 0;
   virtual BOOL RemoveProject(IProject* pProject) = 0;
   virtual BOOL SetActiveProject(IProject* pProject) = 0;
   virtual IProject* GetActiveProject() const = 0;
   virtual IProject* GetFocusProject() const = 0;
};

class IProject : public IElement
{
public:
   virtual BOOL Initialize(IDevEnv* pEnv, LPCTSTR pstrPath) = 0;
   virtual BOOL GetClass(LPTSTR pstrType, UINT cchMax) const = 0;
   virtual BOOL IsDirty() const = 0;
   virtual BOOL Close() = 0;
   virtual BOOL CreateProject() = 0;
   virtual void ActivateProject() = 0;
   virtual void DeactivateProject() = 0;
   virtual void ActivateUI() = 0;
   virtual void DeactivateUI() = 0;
   virtual BOOL SetName(LPCTSTR pstrName) = 0;
   virtual IView* GetItem(INT iIndex) = 0;
   virtual INT GetItemCount() const = 0;
};

class IView : public IElement
{
public:
   virtual BOOL OpenView(long lPosition) = 0;
   virtual void CloseView() = 0;
   virtual IElement* GetParent() const = 0;
   virtual BOOL SetName(LPCTSTR pstrName) = 0;
   virtual BOOL GetText(BSTR* pbstrText) = 0;
   virtual BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const = 0;
   virtual BOOL Save() = 0;
   virtual BOOL IsDirty() const = 0;
   virtual void ActivateUI() = 0;
   virtual void DeactivateUI() = 0;
   virtual void EnableModeless(BOOL bEnable) = 0;
   virtual LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) = 0;
   virtual LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) = 0;
};


#endif // __BVRDE_SDK_H__
