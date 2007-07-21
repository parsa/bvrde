#if !defined(AFX_GLOBALS_H__20041113_38C1_016C_29F5_0080AD509054__INCLUDED_)
#define AFX_GLOBALS_H__20041113_38C1_016C_29F5_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Show Wait cursor.
// Stripped copy of WTL sources.
class CWaitCursor
{
public:
   HCURSOR m_hOldCursor;
   CWaitCursor()
   {
      m_hOldCursor = ::SetCursor( ::LoadCursor(NULL, IDC_WAIT) );
   }
   ~CWaitCursor()
   {
      ::SetCursor(m_hOldCursor);
   }
};


#endif // !defined(AFX_GLOBALS_H__20041113_38C1_016C_29F5_0080AD509054__INCLUDED_)

