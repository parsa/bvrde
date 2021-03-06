#if !defined(AFX_GLOBALS_H__20030315_A892_7CB2_47EF_0080AD509054__INCLUDED_)
#define AFX_GLOBALS_H__20030315_A892_7CB2_47EF_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MiInfo.h"


//////////////////////////////////////////////////////////////
//

struct LEXFILE;
class CTextFile;
class CRemoteProject;


//////////////////////////////////////////////////////////////
//

#define PLUGIN_NAME        "Remote C++"
#define PLUGIN_DESCRIPTION "Allows compiling and debugging of remote C++ projects."

// What's the story about the termination marker? Well, we need to be able to signal that
// the shell/prompt has completed our commands. We do this by submitting an additional
// command-line which gets processed by the shell interpreter.
// The extra command-line does produce any output, but as soon as we see it executed in
// the shell prompt, we'll stop the connection.
#define TERM_MARKER _T("###BVRDE###")
#define TERM_MARKER_LINE _T("$ ###BVRDE###")

#define EPSILON  0.001

#define FR_WRAP  0x40000000
#define FR_INSEL 0x20000000

#define FINDVIEW_FULLPATH      0x00000000
#define FINDVIEW_NAMEONLY      0x00000001
#define FINDVIEW_ALLPROJECTS   0x00000002
#define FINDVIEW_DEPENDENCIES  0x00000004
#define FINDVIEW_ALL           0x000000ff


enum
{
   DEBUG_CMD_SET_CURLINE,
   DEBUG_CMD_SET_RUNNING,
   DEBUG_CMD_DEBUG_START,
   DEBUG_CMD_DEBUG_STOP,
   DEBUG_CMD_SET_BREAKPOINTS,
   DEBUG_CMD_CLEAR_BREAKPOINTS,
   DEBUG_CMD_REQUEST_BREAKPOINTS,
   DEBUG_CMD_GET_CARET_TEXT,
   DEBUG_CMD_HOVERINFO,
   DEBUG_CMD_FINDTEXT,
   DEBUG_CMD_FOLDCURSOR,
   DEBUG_CMD_CURSORVISIBLE,
   DEBUG_CMD_COMPILE_START,
   DEBUG_CMD_COMPILE_STOP,
};

typedef enum VT100COLOR
{
   VT100_DEFAULT = 0,
   VT100_HIDDEN = 8,
   VT100_BLACK = 30,
   VT100_RED = 31,
   VT100_GREEN = 32,
   VT100_YELLOW = 33,
   VT100_BLUE = 34,
   VT100_MAGENTA = 35,
   VT100_CYAN = 36,
   VT100_WHITE = 37,
   VT100_PROMPT = 1000
};


//////////////////////////////////////////////////////////////
//

typedef enum LAZYACTION
{
   LAZY_OPEN_VIEW = 1,
   LAZY_GUI_ACTIVATEVIEW,
   LAZY_GUI_CLEARVIEW,
   LAZY_GUI_APPENDVIEW,
   LAZY_GUI_PLAY_ANIMATION,
   LAZY_GUI_STOP_ANIMATION,
   LAZY_GUI_FILE_RELOAD,
   LAZY_GUI_COMPILESTART,
   LAZY_GUI_PRINTOUTPUT,
   LAZY_SHOW_MESSAGE,
   LAZY_SET_STATUSBARTEXT,
   LAZY_COMPILER_LINE,
   LAZY_DEBUGCOMMAND,
   LAZY_SEND_GLOBAL_VIEW_MESSAGE,
   LAZY_SEND_PROJECT_VIEW_MESSAGE,
   LAZY_SEND_ACTIVE_VIEW_MESSAGE,
   LAZY_SET_DEBUG_BREAKPOINT,
   LAZY_DEBUG_START_EVENT,
   LAZY_DEBUG_INIT_EVENT,
   LAZY_DEBUG_KILL_EVENT,
   LAZY_DEBUG_BREAK_EVENT,
   LAZY_DEBUG_INFO,
   LAZY_CLASSTREE_INFO,
};

typedef struct tagLAZYDATA
{
   LAZYACTION Action;                // What action is this?
   HWND hWnd;                        // Window handle             (often used to direct to RichEdit control in output)
   VT100COLOR Color;                 // Color value               (used to feed compiler output)
   CString sFilename;                // Filename                  (used to set cursor/pointer in editor)
   int iLineNum;                     // Line number               (used to set cursor/pointer in editor)
   CString sMessage;                 // Message                   (often used for message-boxes, or compiler output)
   CString sCaption;                 // Window Caption            (used for message-boxes)
   UINT iFlags;                      // Flags                     (often used for message-boxes)
   WPARAM wParam;                    // Windows Message parameter (used for sending messages)
   IDE_HWND_TYPE WindowType;         // Window-type               (used to direct to RichEdit control in output)
   LEXFILE* pLexFile;                // Lexer information         (used to feed class-view)
   CMiInfo MiInfo;                   // GDB MI information        (used to feed debugger views)
} LAZYDATA;


//////////////////////////////////////////////////////////////
//

#define TAGINFO_NAME          0x00000001
#define TAGINFO_DECLARATION   0x00000002
#define TAGINFO_TYPE          0x00000004
#define TAGINFO_COMMENT       0x00000008
#define TAGINFO_FILENAME      0x00000010
#define TAGINFO_LINENO        0x00000020
#define TAGINFO_NAMESPACE     0x00000040

typedef enum TAGTYPE
{
   TAGTYPE_UNKNOWN = 0,
   // TAG types
   TAGTYPE_CLASS,
   TAGTYPE_STRUCT,
   TAGTYPE_TYPEDEF,
   TAGTYPE_DEFINE,
   TAGTYPE_FUNCTION,
   TAGTYPE_MEMBER,
   TAGTYPE_ENUM,
   TAGTYPE_IMPLEMENTATION,
   TAGTYPE_NAMESPACE,
   // Internally used types
   TAGTYPE_INTRINSIC,
   TAGTYPE_PROTOTYPE,
};

typedef enum TAGPROTECTION
{
   TAGPROTECTION_GLOBAL = 0,
   TAGPROTECTION_PUBLIC,
   TAGPROTECTION_PROTECTED,
   TAGPROTECTION_PRIVATE,
};

typedef enum TAGSOURCE
{
   TAGSOURCE_UNKNOWN,
   TAGSOURCE_CTAGS,
   TAGSOURCE_LEX,
};

typedef struct tagTAGINFO
{
   TAGTYPE Type;                     // Tag type (class/struct/etc)
   LPCTSTR pstrName;                 // Name of tag
   LPCTSTR pstrFile;                 // Filename of tag
   LPCTSTR pstrOwner;                // Owner type
   TAGPROTECTION Protection;         // Access identifier (public/protected/etc)
   TAGSOURCE TagSource;              // What produced this tag (lex/ctags/etc)
   LPCTSTR pstrDeclaration;          // Member declaration
   LPCTSTR pstrNamespace;            // Namespace
   LPCTSTR pstrRegExMatch;           // Regular.expression for lookup in file
   LPCTSTR pstrComment;              // Comment
   int iLineNum;                     // Line number
} TAGINFO;

typedef struct CTagDetails
{
   CString sName;
   TAGTYPE TagType;
   TAGPROTECTION Protection;
   CString sBase;
   CString sMemberOfScope;
   int iLineNum;
   CString sDeclaration;
   CString sRegExMatch;
   CString sFilename;
   CString sNamespace;
   CString sComment;
} CTagDetails;


//////////////////////////////////////////////////////////////
//

class IOutputLineListener
{
public:
   virtual void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText) = 0;
};

class IShellCallback
{
public:
   virtual bool AddLineListener(IOutputLineListener* pListener) = 0;
   virtual bool RemoveLineListener(IOutputLineListener* pListener) = 0;
   virtual void BroadcastLine(VT100COLOR nColor, LPCTSTR pstrText) = 0;
   virtual void PreAuthenticatedLine(LPCTSTR pstrText) = 0;
};

class IDebuggerAdaptor
{
public:
   virtual void Init(CRemoteProject* pProject) = 0;
   virtual CString TransformInput(LPCTSTR pstrInput) = 0;
   virtual void TransformOutput(LPCTSTR pstrOutput, CSimpleArray<CString>& aOutput) = 0;
};

class IRemoteCommandProtocol
{
public:
   virtual void Init(CRemoteProject* pProject, IShellCallback* pCallback = NULL) = 0;
   virtual void Clear() = 0;
   virtual bool Load(ISerializable* pArc) = 0;
   virtual bool Save(ISerializable* pArc) = 0;
   virtual bool Start() = 0;
   virtual bool Stop() = 0;
   virtual void SignalStop() = 0;
   virtual bool IsConnected() const = 0;   
   virtual bool IsBusy() const = 0;
   //
   virtual bool WriteData(LPCTSTR pstrData) = 0;
   virtual bool WriteSignal(BYTE bCmd) = 0;
   virtual bool WriteScreenSize(int w, int h) = 0;
   //
   virtual CString GetParam(LPCTSTR pstrName) const = 0;
   virtual void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue) = 0;
   //
   virtual bool WaitForConnection() = 0;
};

class IRemoteFileProtocol
{
public:
   virtual void Clear() = 0;
   virtual bool Load(ISerializable* pArc) = 0;
   virtual bool Save(ISerializable* pArc) = 0;
   virtual bool Start() = 0;
   virtual bool Stop() = 0;
   virtual void SignalStop() = 0;
   virtual bool IsConnected() const = 0;   
   //
   virtual bool LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize = NULL) = 0;
   virtual bool SaveFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE ppOut, DWORD dwSize) = 0;  
   virtual bool DeleteFile(LPCTSTR pstrFilename) = 0;
   virtual bool SetCurPath(LPCTSTR pstrPath) = 0;
   virtual CString GetCurPath() = 0;
   virtual bool EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles, bool bUseCache) = 0;
   virtual CString FindFile(LPCTSTR pstrFilename) = 0;
   //
   virtual CString GetParam(LPCTSTR pstrName) const = 0;
   virtual void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue) = 0;
   //
   virtual bool WaitForConnection() = 0;
};

class ITagHandler
{
public:
   virtual void Init(CRemoteProject* pProject) = 0;
   virtual void Clear() = 0;
   virtual bool IsLoaded() const = 0;
   virtual bool IsAvailable() const = 0;
   virtual void GetItemInfo(const TAGINFO* pTag, CTagDetails& Info) = 0;
   virtual bool GetOuterList(CSimpleValArray<TAGINFO*>& aResult) = 0;
   virtual bool GetGlobalList(CSimpleValArray<TAGINFO*>& aResult) = 0;
   virtual bool FindItem(LPCTSTR pstrName, LPCTSTR pstrOwner, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult) = 0;
   virtual bool GetMemberList(LPCTSTR pstrType, int iInheritance, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult) = 0;
   virtual bool MatchSymbols(LPCTSTR pstrPattern, volatile bool& bCancel, CSimpleValArray<TAGINFO*>& aResult) = 0;
   virtual bool GetNamespaceList(LPCTSTR pstrType, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aResult) = 0;
};


//////////////////////////////////////////////////////////////
//

template <typename T >
class CAutoFree
{
public:
   LPVOID pData;
   SIZE_T cSize;
   CAutoFree(const T* p) : pData((LPVOID)p), cSize(0) { };
   CAutoFree(SIZE_T iSize) { cSize = iSize; pData = malloc(iSize); };
   ~CAutoFree() { free(pData); };
   T* GetData() const { return static_cast<T*>(pData); };
   T* Detach() const { T* p = static_cast<T*>(pData); pData = NULL; return p; };
   SIZE_T GetSize() { return cSize; };
};


//////////////////////////////////////////////////////////////
//

void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = 0);
void GenerateError(IDevEnv* pDevEnv, HWND hWnd, UINT nErr, DWORD dwErr = (DWORD)-1);
CString GetSystemErrorText(DWORD dwErr);

BOOL EnableSystemAccessPriveledge(LPCWSTR pwstrPriv);
BOOL MergeMenu(HMENU hMenu, HMENU hMenuSource, UINT nPosition);

CString ToString(long lValue);
CString ToString(double dblValue);
void ConvertToCrLf(CString& s);
CString ConvertFromCrLf(const CString& s);
bool wildcmp(LPCTSTR wild, LPCTSTR str);

CString GetFileTypeFromFilename(LPCTSTR pstrFilename);
CTextFile* CreateViewFromFilename(IDevEnv* pDevEnv, LPCTSTR pstrFilename, CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);

void PumpIdleMessages(DWORD dwTimeout);


#endif // !defined(AFX_GLOBALS_H__20030315_A892_7CB2_47EF_0080AD509054__INCLUDED_)

