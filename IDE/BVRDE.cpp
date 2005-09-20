// BVRDE.cpp : main source file for BVRDE.exe
//

#include "stdafx.h"
#include "resource.h"

#include "MainFrm.h"
#include "Thread.h"
#include "SplashDlg.h"
#include "RegSerializer.h"

CAppModule _Module;

extern void PreloadConfig();


///////////////////////////////////////////////////
//

class CPluginLoader
{
public:
   DWORD Run()
   {
      // Load all the plugins
      _LoadPlugins();
      _SendReadySignal();
      return 0;
   }
   void _SendReadySignal()
   {
      // Send signal to main window to enable UI...
      ::PostMessage(g_pDevEnv->GetHwnd(IDE_HWND_MAIN), WM_APP_INIT, 0, 0L);
   }
   BOOL _LoadPlugin(CPlugin& plugin)
   {
      __try
      {
         return plugin.LoadPackage(g_pDevEnv);
      }
      __except(1)
      {
         ATLASSERT(false);
      }
      return FALSE;
   }
   void _LoadPlugins()
   {
      // Collect all PKG files in program folder
      CString sPattern;
      sPattern.Format(_T("%s*.pkg"), CModulePath());
      CFindFile ff;
      for( BOOL bRes = ff.FindFile(sPattern); bRes; bRes = ff.FindNextFile() ) {
         if( ff.IsDots() ) continue;
         if( ff.IsDirectory() ) continue;
         CPlugin plugin;
         plugin.Init(ff.GetFilePath());
         g_aPlugins.Add(plugin);
      }
      ff.Close();
      // Load and initialize each plugin
      for( int i = 0; i< g_aPlugins.GetSize(); i++ ) {
         CPlugin& plugin = g_aPlugins[i];
         if( !_LoadPlugin(plugin) ) {
            // Failed to load, remove it again...
            g_aPlugins.RemoveAt(i--);
         }
      }
   }
};


///////////////////////////////////////////////////
//

static bool SingleInstance()
{
   // See if another instance of BVRDE is running; send commandline
   // to that version and terminate here.
   HANDLE hMutex = ::CreateMutex(NULL, TRUE, _T("BVRDE"));
   if( hMutex == NULL || ::GetLastError() == ERROR_ALREADY_EXISTS ) {
      HWND hWnd = ::FindWindow(_T("BVRDE_Main"), NULL);
      if( !::IsWindow(hWnd) ) return true;
      LPTSTR pstrCommandLine = ::GetCommandLine();
      COPYDATASTRUCT cds;
      cds.dwData = 1;
      cds.lpData = pstrCommandLine;
      cds.cbData = (_tcslen(pstrCommandLine) + 1) * sizeof(TCHAR);
      DWORD lResult = 0;
      ::SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cds, SMTO_ABORTIFHUNG, 5000, &lResult);
      ::SetForegroundWindow(hWnd);
      ::SetFocus(hWnd);
      return false;
   }
   return true;
}

static void PreloadLibraries()
{
   ::LoadLibrary(_T("NETAPI32.DLL"));
   ::LoadLibrary(_T("RASAPI32.DLL"));
   ::LoadLibrary(_T("RASMAN.DLL"));
   ::LoadLibrary(_T("RTUTILS.DLL"));
   ::LoadLibrary(_T("SHDOC32.DLL"));
   ::LoadLibrary(_T("SHDOCVW.DLL"));
   ::LoadLibrary(_T("SHDOCLC.DLL"));
   ::LoadLibrary(_T("MSHTML.DLL"));
}

static void SetLanguage()
{
   CRegSerializer reg;
   if( !reg.Open(REG_BVRDE) ) return;
   if( !reg.ReadGroupBegin(_T("Settings")) ) return;
   TCHAR szBuffer[64] = { 0 };
   reg.Read(_T("language"), szBuffer, 63);
   if( _tcscmp(szBuffer, _T("en")) == 0 ) ::SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
   if( _tcscmp(szBuffer, _T("de")) == 0 ) ::SetThreadLocale(MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT));
}


///////////////////////////////////////////////////
//

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
   // NOTE: We need single-threaded COM because of Active Scripting
   //       and OLE drag'n'drop below runs in this scope...
   HRESULT hRes = ::CoInitialize(NULL);
   ATLASSERT(SUCCEEDED(hRes));
   hRes = ::OleInitialize(NULL);
   ATLASSERT(SUCCEEDED(hRes));

   ::DefWindowProc(NULL, 0, 0, 0L);

   // Only want to see a single instance of this app
   if( !SingleInstance() ) return 0;

   // Start ATL COM/window support
   hRes = _Module.Init(NULL, hInstance);
   ATLASSERT(SUCCEEDED(hRes));

   // Show splash screen
   CSplashWindow splash;
   splash.ShowSplash();

   SetLanguage();

   // Grab the XML configuration file
   PreloadConfig();

   // Kick in the Windows Common Controls
   AtlInitCommonControls(ICC_WIN95_CLASSES | 
                         ICC_COOL_CLASSES | 
                         ICC_HOTKEY_CLASS);

   // Load the RichEdit control
   HMODULE hRichEdit = ::LoadLibrary(CRichEditCtrl::GetLibraryName());
   ATLASSERT(hRichEdit);

   // We want to embed ActiveX windows
   AtlAxWinInit();

   CMessageLoop msgloop;
   _Module.AddMessageLoop(&msgloop);

   // Create out global Solution reference
   g_pSolution = new CSolution();

   // Create main window
   CMainFrame wndMain;
   if( wndMain.CreateEx() == NULL ) {
      ATLTRACE(_T("Main window creation failed!\n"));
      splash.RemoveSplash();
      return 0;
   }

   // Load all plugins
   CPluginLoader loader;
   loader.Run();

   // Preload a couple of system DLLs for performance reasons
   PreloadLibraries();

   // Ready to remove splash screen
   splash.RemoveSplash(500);

   // Run...
   int nRet = msgloop.Run();

   _Module.RemoveMessageLoop();

   g_aPlugins.RemoveAll();

   delete g_pSolution;

   _Module.Term();

   ::FreeLibrary(hRichEdit);

   ::OleUninitialize();
   ::CoUninitialize();
   return nRet;
}
