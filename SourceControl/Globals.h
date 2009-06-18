#if !defined(AFX_GLOBALS_H__20031227_B212_6032_9D6F_0080AD509054__INCLUDED_)
#define AFX_GLOBALS_H__20031227_B212_6032_9D6F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


BOOL MergeMenu(HMENU hMenu, HMENU hMenuSource, UINT nPosition);
void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = 0);


// Show Wait cursor.
// Stripped copy of WTL sources.
class CWaitCursor
{
public: 
   HCURSOR m_hWaitCursor;
   HCURSOR m_hOldCursor;
   
   CWaitCursor() : m_hOldCursor(NULL)
   {
      m_hWaitCursor = ::LoadCursor(NULL, IDC_WAIT);
      ATLASSERT(m_hWaitCursor!=NULL);
      m_hOldCursor = ::SetCursor(m_hWaitCursor);
   }

   ~CWaitCursor()
   {
      ::SetCursor(m_hOldCursor);
   }
};


#endif // !defined(AFX_GLOBALS_H__20031227_B212_6032_9D6F_0080AD509054__INCLUDED_)
