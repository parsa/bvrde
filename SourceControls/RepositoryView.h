#if !defined(AFX_RESPOSITORYVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_)
#define AFX_RESPOSITORYVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Commands.h"


class CRepositoryView : public CWindowImpl<CRepositoryView, CTreeViewCtrl>
{
public:
   CCommandThread m_thread;

   BEGIN_MSG_MAP(CRepositoryView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();
      SetFont(AtlGetDefaultGuiFont());
      return lRes;
   }

   // Operations
};


#endif // !defined(AFX_RESPOSITORYVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_)
