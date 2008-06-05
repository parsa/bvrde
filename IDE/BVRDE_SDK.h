#ifndef __BVRDE_SDK_H__
#define __BVRDE_SDK_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////
// Defines

#define BVRDE_SDK_VERSION  100

#define REG_BVRDE _T("SOFTWARE\\Viksoe.dk\\BVRDE")

// Plugin types
#define PLUGIN_PROJECT     0x00000001
#define PLUGIN_FILETYPE    0x00000002
#define PLUGIN_EXTENSION   0x00000004

// Message Box extras
#define MB_SHOWONCE        0x20000000L
#define MB_MODELESS        0x40000000L
#define MB_REMOVABLE       0x80000000L

// Main menu positions
// Either positioned from the beginning (FB) of the menu or from the end (FE)
#define MENUPOS_FILE_FB                    0
#define MENUPOS_EDIT_FB                    1
#define MENUPOS_VIEW_FB                    2
#define MENUPOS_HELP_FE                   -1
#define MENUPOS_WINDOW_FE                 -2
#define MENUPOS_TOOLS_FE                  -3
// Sub-menu positions
#define SUBMENUPOS_FILE_NEW_FB             0
#define SUBMENUPOS_FILE_ADD_FB             1
#define SUBMENUPOS_FILE_RECENT_FE         -3
#define SUBMENUPOS_VIEW_VIEWS_FB           2
#define SUBMENUPOS_VIEW_PROPERTIES_FE     -1
#define SUBMENUPOS_TOOLS_OPTIONS_FE       -2
#define SUBMENUPOS_TOOL_CUSTOMIZE_FE      -1
#define SUBMENUPOS_HELP_CONTENTS_FB        0
#define SUBMENUPOS_HELP_INDEX_FB           1
#define SUBMENUPOS_HELP_SEARCH_FB          2
#define SUBMENUPOS_HELP_ABOUT_FE          -1

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
#define ID_EDIT_MARK                0xE409
#define ID_WINDOW_VIEW              0xE40A
#define ID_WINDOW_CLOSE             0xE40B
#define ID_WINDOW_CLOSE_ALL         0xE40C
#define ID_TOOLS_OPTIONS            0xE40D
#define ID_TOOLS_CUSTOMIZE          0xE40E
#define ID_TOOLS_EXTERNAL           0xE40F
#define ID_VIEW_PROPERTIES          0xE410
#define ID_VIEW_OUTPUT              0xE411
#define ID_VIEW_COMMAND             0xE412
#define ID_VIEW_OPEN                0xE413
#define ID_VIEW_EXPLORER            0xE414
#define ID_HELP_STARTPAGE           0xE415
#define ID_HELP_SEARCH              0xE416
#define ID_HELP_CONTENTS            0xE417
#define ID_MACRO_RECORD             0xE418
#define ID_MACRO_CANCEL             0xE419

#define IDC_TREE                    0xEC00
#define ID_SB_PANE1                 0xEC01
#define ID_SB_PANE2                 0xEC02
#define ID_SB_PANE3                 0xEC03

#define ANIM_BUILD                  1
#define ANIM_PRINT                  2
#define ANIM_TRANSFER               3
#define ANIM_SAVE                   4

#define TOOLFLAGS_CONSOLEOUTPUT     1
#define TOOLFLAGS_PROMPTARGS        2


/////////////////////////////////////////////////////////
// Forward declares

struct ISolution;
struct IProject;
struct IView;
struct IElement;


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

MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC1")
ISerializable
{
public:
   virtual BOOL ReadGroupBegin(LPCWSTR pstrName) = 0;
   virtual BOOL ReadGroupEnd() = 0;
   virtual BOOL ReadItem(LPCWSTR pstrName) = 0;
   virtual BOOL Read(LPCWSTR pstrName, LPWSTR szValue, UINT cchMax) = 0;
   virtual BOOL Read(LPCWSTR pstrName, SYSTEMTIME& stValue) = 0;
   virtual BOOL Read(LPCWSTR pstrName, long& lValue) = 0;
   virtual BOOL Read(LPCWSTR pstrName, BOOL& bValue) = 0;
   //
   virtual BOOL WriteGroupBegin(LPCWSTR pstrName) = 0;
   virtual BOOL WriteGroupEnd() = 0;
   virtual BOOL WriteItem(LPCWSTR pstrName) = 0;
   virtual BOOL Write(LPCWSTR pstrName, LPCWSTR pstrValue) = 0;
   virtual BOOL Write(LPCWSTR pstrName, SYSTEMTIME stValue) = 0;
   virtual BOOL Write(LPCWSTR pstrName, long lValue) = 0;
   virtual BOOL Write(LPCWSTR pstrName, BOOL bValue) = 0;
   virtual BOOL WriteExternal(LPCWSTR pstrName) = 0;
   //
   virtual BOOL Delete(LPCWSTR pstrName) = 0;
};


/////////////////////////////////////////////////////////
// Misc interfaces

/**
 * @class IUpdateUI
 *
 * Callback interface for enabling/disabling menu- and toolbar items.
 * This interface is handed to the listener through the IIdleListener
 * interface.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC2")
IUpdateUI
{
public:
   virtual BOOL UIEnable(INT nID, BOOL bEnable, BOOL bForceUpdate = FALSE) = 0;
   virtual BOOL UISetCheck(INT nID, INT nCheck, BOOL bForceUpdate = FALSE) = 0;
   virtual BOOL UISetRadio(INT nID, BOOL bRadio, BOOL bForceUpdate = FALSE) = 0;
   virtual BOOL UISetText(INT nID, LPCWSTR lpstrText, BOOL bForceUpdate = FALSE) = 0;
};

/**
 * @class IViewFrame
 *
 * Interface implemented by the client window (MDI Client).
 * This interface is returned by the IDevEnv::CreateClient method.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC3")
IViewFrame
{
public:
   virtual HWND GetHwnd() const = 0;
   virtual HWND SetClient(HWND hWnd) = 0;
};

/**
 * @class IWizardManager
 *
 * Callback interface for controlling insertion / placement of
 * property pages in the Options and Property sheet.
 * This interface is handed to the listener through the IWizardListener
 * interface.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC4")
IWizardManager
{
public:
   virtual BOOL AddWizardGroup(LPCWSTR pstrParent, LPCWSTR pstrName) = 0;
   virtual BOOL AddWizardPage(UINT nID, LPCPROPSHEETPAGE hPage) = 0;
   virtual BOOL SetWizardGroup(LPCWSTR pstrName) = 0;
};


/////////////////////////////////////////////////////////
// Listerner interfaces

/**
 * @class IAppMessageListener
 *
 * Message routed from the main message pump. Includes
 * \em all messages from the main message pump.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC5")
IAppMessageListener
{
public:
   virtual LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) = 0;
   virtual BOOL PreTranslateMessage(MSG* pMsg) = 0;
};

/**
 * @class IIdleListener
 *
 * Idle messages from the system.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC6")
IIdleListener
{
public:
   virtual VOID OnIdle(IUpdateUI* pUIBase) = 0;
   virtual VOID OnGetMenuText(UINT wID, LPWSTR pstrText, int cchMax) = 0;
};

/**
 * @class ITreeMessageListener
 *
 * Messages from the Project Explorer tree.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC7")
ITreeMessageListener
{
public:
   virtual LRESULT OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled) = 0;
};

/**
 * @class IViewListener
 *
 * Interface for view related messages. The view forwards a number of
 * Windows messages to these listeners.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC8")
IViewMessageListener
{
public:
   virtual LRESULT OnViewMessage(IView* pView, MSG* pMsg, BOOL& bHandled) = 0;
};

/**
 * @class ICustomCommandListener
 *
 * Interface for execution of custom commands entered in the Command View window
 * or from the Tools menu.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FC9")
ICustomCommandListener
{
public:
   virtual VOID OnUserCommand(LPCWSTR pstrCommand, BOOL& bHandled) = 0;
   virtual VOID OnMenuCommand(LPCWSTR pstrType, LPCWSTR pstrCommand, LPCWSTR pstrArguments, LPCWSTR pstrPath, int iFlags, BOOL& bHandled) = 0;
};

/**
 * @class IWizardListener
 *
 * Interface for invoking the Application Wizard or Property Wizard.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FCA")
IWizardListener
{
public:
   virtual BOOL OnInitProperties(IWizardManager* pManager, IElement* pElement) = 0;
   virtual BOOL OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCWSTR pstrName) = 0;
   virtual BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc) = 0;
};


/////////////////////////////////////////////////////////
// IDevEnv

typedef enum IDE_UI_TYPE
{
   IDE_UI_PROJECT = 0,
   IDE_UI_VIEW = 1,
};

typedef enum IDE_PROJECTTREE_IMAGE
{
   IDE_TREEIMAGE_FOLDER_CLOSED = 0,
   IDE_TREEIMAGE_FOLDER_OPEN = 1,
   IDE_TREEIMAGE_JAVA = 3,
   IDE_TREEIMAGE_TEXT = 4,
   IDE_TREEIMAGE_SOLUTION = 5,
   IDE_TREEIMAGE_PROJECT = 6,
   IDE_TREEIMAGE_DATABASE = 10,
   IDE_TREEIMAGE_BASIC = 11,
   IDE_TREEIMAGE_CPP = 15,
   IDE_TREEIMAGE_XML = 17,
   IDE_TREEIMAGE_HTML = 18,
   IDE_TREEIMAGE_BASH = 39,
   IDE_TREEIMAGE_DOC = 27,
   IDE_TREEIMAGE_MAKEFILE = 28,
   IDE_TREEIMAGE_SCRIPT = 30,
   IDE_TREEIMAGE_HEADER = 41,
   IDE_TREEIMAGE_LAST
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


/**
 * @class IDevEnv
 *
 * Main programming interface for the BVRDE IDE Framework.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FCB")
IDevEnv
{
public:
   virtual DWORD GetVersion() const = 0;
   virtual ISolution* GetSolution() const = 0;
   virtual IView* GetActiveView() const = 0;
   virtual IDispatch* GetDispatch() = 0;
   virtual LCID SetThreadLanguage() = 0;
   virtual DWORD GetGuiThreadId() = 0;
   //
   virtual BOOL AddExplorerView(HWND hWnd, LPCWSTR pstrTitle, int iImage) = 0;
   virtual BOOL RemoveExplorerView(HWND hWnd) = 0;
   virtual BOOL AddAutoHideView(HWND hWnd, IDE_DOCK_TYPE Direction, int iImage) = 0;
   virtual BOOL RemoveAutoHideView(HWND hWnd) = 0;
   virtual BOOL ActivateAutoHideView(HWND hWnd) = 0;
   virtual BOOL ActivateExplorerView(HWND hWnd) = 0;
   virtual BOOL AddDockView(HWND hWnd, IDE_DOCK_TYPE Direction, RECT rcWin) = 0;
   virtual BOOL RemoveDockView(HWND hWnd) = 0;
   virtual BOOL GetDockState(HWND hWnd, int& iState, RECT& rcWin) = 0;
   virtual BOOL AddToolBar(HWND hWnd, LPCWSTR pstrID, LPCWSTR pstrTitle) = 0;
   virtual BOOL RemoveToolBar(HWND hWnd) = 0;
   virtual BOOL ShowToolBar(HWND hWnd, BOOL bShow = TRUE, BOOL bUseDefault = TRUE) = 0;
   virtual IViewFrame* CreateClient(LPCWSTR pstrTitle, IProject* pProject, IView* pView) = 0;
   virtual BOOL DestroyClient(HWND hWnd) = 0;
   //
   virtual IView* CreateView(LPCWSTR pstrFilename, IProject* pProject = NULL, IElement* pParent = NULL) = 0;
   virtual IDispatch* CreateStdDispatch(LPCWSTR pstrType, IElement* pElement) = 0;
   //
   virtual VOID EnableModeless(BOOL bEnable) = 0;
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
   virtual UINT ShowMessageBox(HWND hWnd, LPCWSTR pstrMessage, LPCWSTR pstrCaption, DWORD dwFlags) = 0;
   virtual BOOL ShowStatusText(UINT nID, LPCWSTR pstrText, BOOL bPermanent = FALSE) = 0;
   virtual BOOL PlayAnimation(BOOL bStart, UINT uType) = 0;
   virtual BOOL RecordMacro(LPCWSTR pstrText) = 0;
   //
   virtual BOOL SetProperty(LPCWSTR pstrKey, LPCWSTR pstrValue) = 0;
   virtual BOOL GetProperty(LPCWSTR pstrKey, LPWSTR pstrValue, UINT cchMax) = 0;
   virtual BOOL EnumProperties(int& iStart, LPCWSTR pstrPattern, LPWSTR pstrKey, LPWSTR pstrValue) = 0;
   //
   virtual BOOL AddAppListener(IAppMessageListener* pListener) = 0;
   virtual BOOL RemoveAppListener(IAppMessageListener* pListener) = 0;
   virtual BOOL AddIdleListener(IIdleListener* pListener) = 0;
   virtual BOOL RemoveIdleListener(IIdleListener* pListener) = 0;
   virtual BOOL AddTreeListener(ITreeMessageListener* pListener) = 0;
   virtual BOOL RemoveTreeListener(ITreeMessageListener* pListener) = 0;
   virtual BOOL AddViewListener(IViewMessageListener* pListener) = 0;
   virtual BOOL RemoveViewListener(IViewMessageListener* pListener) = 0;
   virtual BOOL AddCommandListener(ICustomCommandListener* pListener) = 0;
   virtual BOOL RemoveCommandListener(ICustomCommandListener* pListener) = 0;
   virtual BOOL AddWizardListener(IWizardListener* pListener) = 0;
   virtual BOOL RemoveWizardListener(IWizardListener* pListener) = 0;
};


/////////////////////////////////////////////////////////
// IProject

/**
 * @class IElement
 *
 * Base interface for all programmable UI objects.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FD0")
IElement
{
public:
   virtual ~IElement() { };
   virtual BOOL Load(ISerializable* pArchive) = 0;
   virtual BOOL Save(ISerializable* pArchive) = 0;
   virtual BOOL GetName(LPWSTR pstrName, UINT cchMax) const = 0;
   virtual BOOL GetType(LPWSTR pstrType, UINT cchMax) const = 0;
   virtual IElement* GetParent() const = 0;
   virtual IDispatch* GetDispatch() = 0;
};

/**
 * @class ISolution
 *
 * Interface for the Solution.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FD1")
ISolution : public IElement
{
public:
   virtual VOID Close() = 0;
   virtual BOOL LoadSolution(LPCWSTR pstrFilename) = 0;
   virtual BOOL SaveSolution(LPCWSTR pstrFilename) = 0;
   virtual BOOL IsDirty() const = 0;
   virtual BOOL GetFileName(LPWSTR pstrFilename, UINT cchMax) const = 0;
   virtual BOOL IsLoaded() const = 0;
   virtual IProject* GetItem(INT iIndex) = 0;
   virtual INT GetItemCount() const = 0;
   virtual BOOL AddProject(LPCWSTR pstrFilename) = 0;
   virtual BOOL RemoveProject(IProject* pProject) = 0;
   virtual BOOL SetActiveProject(IProject* pProject) = 0;
   virtual IProject* GetActiveProject() const = 0;
   virtual IProject* GetFocusProject() const = 0;
};

/**
 * @class IProject
 *
 * Interface for a Project.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FD2")
IProject : public IElement
{
public:
   virtual BOOL Initialize(IDevEnv* pEnv, LPCWSTR pstrPath) = 0;
   virtual BOOL GetClass(LPWSTR pstrType, UINT cchMax) const = 0;
   virtual BOOL Close() = 0;
   virtual BOOL IsDirty() const = 0;
   virtual BOOL CreateProject() = 0;
   virtual VOID ActivateProject() = 0;
   virtual VOID DeactivateProject() = 0;
   virtual VOID ActivateUI() = 0;
   virtual VOID DeactivateUI() = 0;
   virtual BOOL SetName(LPCWSTR pstrName) = 0;
   virtual IView* GetItem(INT iIndex) = 0;
   virtual INT GetItemCount() const = 0;
};

/**
 * @class IView
 *
 * Interface for a view. A view need not be visible (opened) but must
 * still implement and answer all methods in this interface; All files
 * and folders in a project implement this interface.
 */
MIDL_INTERFACE("74243F50-F8ED-4fc4-A38F-64A5FA5F4FD3")
IView : public IElement
{
public:
   virtual BOOL OpenView(LONG lLineNum) = 0;
   virtual VOID CloseView() = 0;
   virtual BOOL GetFileName(LPWSTR pstrFilename, UINT cchMax) const = 0;
   virtual BOOL SetName(LPCWSTR pstrName) = 0;
   virtual BOOL GetText(BSTR* pbstrText) = 0;
   virtual BOOL IsDirty() const = 0;
   virtual BOOL Reload() = 0;
   virtual BOOL Save() = 0;
   virtual VOID ActivateUI() = 0;
   virtual VOID DeactivateUI() = 0;
   virtual VOID EnableModeless(BOOL bEnable) = 0;
   virtual LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) = 0;
   virtual LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) = 0;
};


#endif // __BVRDE_SDK_H__
