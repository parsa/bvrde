#if !defined(AFX_GENEDIT_H__20031212_B2D2_5DF2_A42F_0080AD509054__INCLUDED_)
#define AFX_GENEDIT_H__20031212_B2D2_5DF2_A42F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef GENEDIT_EXPORTS
   #define GENEDIT_API __declspec(dllexport)
#else
   #define GENEDIT_API __declspec(dllimport)
#endif

#include "atlscintilla.h"


enum
{
   MARKER_BOOKMARK = 0,
   MARKER_BREAKPOINT,
   MARKER_CURLINE,
   MARKER_RUNNING,
};


HWND GENEDIT_API Bvrde_CreateScintillaView(HWND hWndParent, IDevEnv* pDevEnv, LPCTSTR pstrFilename, LPCTSTR pstrLanguage);


#endif // !defined(AFX_GENEDIT_H__20031212_B2D2_5DF2_A42F_0080AD509054__INCLUDED_)

