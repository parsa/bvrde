#if !defined(AFX_GLOBALS_H__20030310_FB96_93A6_D313_0080AD509054__INCLUDED_)
#define AFX_GLOBALS_H__20030310_FB96_93A6_D313_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


///////////////////////////////////////////////////
// Constants

// Internal messages
// NOTE: There are unfortunate dependencies on these in
//       the StartPage project.
enum
{
   WM_APP_LOADSOLUTION = WM_APP + 1000,
   WM_APP_IDLETIME,
   WM_APP_APPMESSAGE,
   WM_APP_TREEMESSAGE,
   WM_APP_VIEWMESSAGE,
   WM_APP_PROJECTCHANGE,
   WM_APP_VIEWCHANGE,
   WM_APP_UPDATELAYOUT,
   WM_APP_BUILDSOLUTIONUI,
   WM_APP_COMMANDLINE,
   WM_APP_INIT,
   WM_APP_CLOSESTARTPAGE,
};

#define SOLUTIONDIR  "Solutions"


CString GetSystemErrorText(DWORD dwErr);


#endif // !defined(AFX_GLOBALS_H__20030310_FB96_93A6_D313_0080AD509054__INCLUDED_)

