#if !defined(AFX_CUSTOMIZEDLG_H__20030525_41C8_C513_32CC_0080AD509054__INCLUDED_)
#define AFX_CUSTOMIZEDLG_H__20030525_41C8_C513_32CC_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MainFrm.h"
#include "XmlSerializer.h"

#include "DlgTabCtrl.h"

#include "SoundsDlg.h"
#include "LayoutDlg.h"
#include "KeyboardDlg.h"
#include "MacroBindDlg.h"


class CCustomizeDlg : public CPropertySheetImpl<CCustomizeDlg>
{
public:
   CMainFrame* m_pMainFrame;
   //
   CSoundsDlg m_viewSounds;
   CLayoutDlg m_viewLayout;
   CKeyboardDlg m_viewKeyboard;
   CMacroBindDlg m_viewMacros;

   void Init(CMainFrame* pMainFrame)
   {
      m_pMainFrame = pMainFrame;

      static CString sTitle(MAKEINTRESOURCE(IDS_CUSTOMIZE));
      SetTitle(sTitle);

      HMENU hMenu = m_pMainFrame->GetMenuHandle(IDE_HWND_MAIN);
      HACCEL hDefaultAccel = m_pMainFrame->m_hAccel;
      HACCEL* phUserAccel = &m_pMainFrame->m_UserAccel.m_hAccel;
      HACCEL* phMacroAccel = &m_pMainFrame->m_MacroAccel.m_hAccel;

      m_viewLayout.Init(m_hWnd, m_pMainFrame);
      m_viewLayout.Create();
      m_viewLayout.SetTitle(IDS_LAYOUT);

      m_viewKeyboard.Init(m_hWnd, hMenu, hDefaultAccel, phUserAccel);
      m_viewKeyboard.Create();
      m_viewKeyboard.SetTitle(IDS_KEYBOARD);

      m_viewMacros.Init(m_hWnd, hMenu, hDefaultAccel, phMacroAccel);
      m_viewMacros.Create();
      m_viewMacros.SetTitle(IDS_MACROBIND);

      m_viewSounds.Init(m_hWnd, m_pMainFrame);
      m_viewSounds.Create();
      m_viewSounds.SetTitle(IDS_SOUNDS);

      AddPage(m_viewLayout);
      AddPage(m_viewKeyboard);
      AddPage(m_viewMacros);
      AddPage(m_viewSounds);

      m_psh.dwFlags |= PSH_NOCONTEXTHELP;
   }
};


#endif // !defined(AFX_CUSTOMIZEDLG_H__20030525_41C8_C513_32CC_0080AD509054__INCLUDED_)

