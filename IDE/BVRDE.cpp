// BVRDE.cpp : main source file for BVRDE.exe
//
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or   
//   (at your option) any later version.                                 
//
//

#include "stdafx.h"
#include "resource.h"

#include "MainFrm.h"
#include "Thread.h"
#include "SplashDlg.h"
#include "RegSerializer.h"

CAppModule _Module;

extern void LoadSettings(CMainFrame* pMain);


///////////////////////////////////////////////////
//

static bool SingleInstance()
{
   // See if another instance of BVRDE is running; send commandline
   // to that version and terminate here.
   HANDLE hMutex = ::CreateMutex(NULL, TRUE, _T("BVRDE"));
   if( hMutex == NULL || ::GetLastError() == ERROR_ALREADY_EXISTS ) {
      // We may have a configuration that allows multiple instances
      // of the tool; otherwise we'll restrict it to one instance.
      CRegSerializer reg;
      if( reg.Open(REG_BVRDE) ) {
         if( reg.ReadGroupBegin(_T("Settings")) ) {
            TCHAR szBuffer[16] = { 0 };
            reg.Read(_T("multiInstance"), szBuffer, 15);
            if( _tcscmp(szBuffer, _T("true")) == 0 ) return true;
         }
         reg.Close();
      }          
      // Find other instance and set focus
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

static LCID FindUserLanguage()
{
   LCID lcid = ::GetThreadLocale();
   LANGID lang = LANGIDFROMLCID(lcid);
   CRegSerializer reg;
   if( reg.Open(REG_BVRDE) ) {
      if( reg.ReadGroupBegin(_T("Settings")) ) {
         TCHAR szBuffer[16] = { 0 };
         reg.Read(_T("language"), szBuffer, 15);
         if( _tcscmp(szBuffer, _T("en")) == 0 ) {
            lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            lcid = MAKELCID(lang, SORT_DEFAULT);
         }
         if( _tcscmp(szBuffer, _T("de")) == 0 ) {
            lang = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
            lcid = MAKELCID(lang, SORT_DEFAULT);
         }
      }
      reg.Close();
   }
   return lcid;
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

   CMainFrame wndMain;

   // Grab the XML configuration file
   LoadSettings(&wndMain);

   // Change the language now; this is a setting in the registry because
   // we'd like all message-boxes (even errors at this stage) to appear
   // in the correct language.
   wndMain.m_Locale = FindUserLanguage();
   wndMain.m_dwGuiThreadId = ::GetCurrentThreadId();
   wndMain.SetThreadLanguage();

   // Show splash screen
   CSplashWindow splash;
   splash.ShowSplash();

   // Kick in the Windows Common Controls
   AtlInitCommonControls(ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_HOTKEY_CLASS);

   // Load the RichEdit control
   HMODULE hRichEdit = ::LoadLibrary(CRichEditCtrl::GetLibraryName());
   ATLASSERT(hRichEdit);

   // We want to embed ActiveX controls
   AtlAxWinInit();

   CMessageLoop msgloop;
   _Module.AddMessageLoop(&msgloop);

   // Create our global Solution reference
   g_pSolution = new CSolution();

   // Create main window
   if( wndMain.CreateEx() == NULL ) {
      ATLTRACE(_T("Main window creation failed!\n"));
      splash.RemoveSplash();
      return 0;
   }

   // Load all plugins
   CPluginLoader loader;
   loader.Run();

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
