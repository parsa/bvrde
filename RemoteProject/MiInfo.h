#if !defined(AFX_MIINFO_H__20030516_FC75_CD09_72EA_0080AD509054__INCLUDED_)
#define AFX_MIINFO_H__20030516_FC75_CD09_72EA_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_MI_KEY_LEN 32
#define MAX_MI_VALUE_LEN 64


/**
 * @class CMiInfo
 * Parse a formatted GDB Debugger output. Will take any
 * GDB debugger output in "Machine Interface" format and parse
 * it into manegable structures.
 * The MI information will be split up into a Key/Group/Frame 
 * hierarchy. MI is slightly more flexible than this but rarely 
 * expands more than 2 nested groups.
 */
class CMiInfo
{
public:
   CMiInfo(LPCTSTR pstrInput = NULL);

// Operations
public:
   bool Parse(LPCTSTR pstrInput);
   void Copy(const CMiInfo& src);
   CString GetItem(LPCTSTR pstrKey, LPCTSTR pstrGroup = NULL, LPCTSTR pstrFrame = NULL, long lPosition = 0);
   CString FindNext(LPCTSTR pstrKey, LPCTSTR pstrGroup = NULL, LPCTSTR pstrFrame = NULL);
   CString GetSubItem(LPCTSTR pstrKey) const;

// Implementation
private:
   typedef struct MIINFO
   {
      TCHAR szKey[MAX_MI_KEY_LEN + 1];
      TCHAR szFrame[MAX_MI_KEY_LEN + 1];
      TCHAR szGroup[MAX_MI_KEY_LEN + 1];
      TCHAR szValue[MAX_MI_VALUE_LEN + 1];
      short iIndex;
   };
   CSimpleValArray<MIINFO> m_aItems;
   int m_iSearchIndex;

   bool _FindBlockEnd(LPCTSTR pstrText, int& iPos) const;
   bool _ParseString(CString& sText, LPCTSTR pstrFrame, LPCTSTR pstrGroup, short iIndex);
   void _GetPlainText(LPTSTR pstrDest, LPCTSTR pstrSrc, int iStart, int nLen, int cchMax) const;
};


#endif // !defined(AFX_MIINFO_H__20030516_FC75_CD09_72EA_0080AD509054__INCLUDED_)

