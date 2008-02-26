#if !defined(AFX_MIINFO_H__20030516_FC75_CD09_72EA_0080AD509054__INCLUDED_)
#define AFX_MIINFO_H__20030516_FC75_CD09_72EA_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/**
 * @class CMiInfo
 * Parse a formatted GDB Debugger output. Will take any
 * GDB debugger output in "Machine Interface" format and parse
 * it into manageable structures.
 * The MI information will be split up into a Key/Group/Frame 
 * hierarchy. MI is slightly more flexible than this but rarely 
 * expands more than 2 nested groups.
 */
class CMiInfo
{
public:
   CMiInfo(const CMiInfo& src);
   CMiInfo(LPCTSTR pstrInput = NULL);
   ~CMiInfo();

// Operations
public:
   void Release();
   bool Parse(LPCTSTR pstrInput);
   void Copy(const CMiInfo& src);
   CString GetItem(LPCTSTR pstrKey, LPCTSTR pstrGroup = NULL, LPCTSTR pstrFrame = NULL, long lPosition = 0);
   CString FindNext(LPCTSTR pstrKey, LPCTSTR pstrGroup = NULL, LPCTSTR pstrFrame = NULL);
   CString GetSubItem(LPCTSTR pstrKey) const;

// Implementation
private:
   typedef struct MIINFO
   {
      LPCTSTR pstrKey;
      LPCTSTR pstrFrame;
      LPCTSTR pstrGroup;
      LPCTSTR pstrValue;
      short iIndex;
   };
   LPTSTR m_pstrData;                       /// Original MI string
   LONG* m_plRefCount;                      /// Reference count
   CSimpleValArray<MIINFO> m_aItems;        /// Parsed structures
   int m_iSearchIndex;                      /// Current search index

   void _ConvertToPlainText(LPTSTR pstrText);
   bool _FindBlockEnd(LPTSTR pstrText, int& iPos) const;
   bool _ParseString(LPTSTR pstrSrc, int iStart, LPCTSTR pstrFrame, LPCTSTR pstrGroup, short iIndex);
};


#endif // !defined(AFX_MIINFO_H__20030516_FC75_CD09_72EA_0080AD509054__INCLUDED_)

