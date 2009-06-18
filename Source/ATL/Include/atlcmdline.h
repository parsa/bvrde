#if !defined(AFX_ATLCMDLINE_H__20010428_69BD_A531_8A72_0080AD509054__INCLUDED_)
#define AFX_ATLCMDLINE_H__20010428_69BD_A531_8A72_0080AD509054__INCLUDED_

#pragma once

///////////////////////////////////////////////////////////////////
// atlcmdline.h - Command Line parser class
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2000-2001 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#define MAX_CMDPARAM_LEN MAX_PATH
#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')

class CCommandLine
{
public:
   struct TCmdString
   {
      TCmdString(LPCTSTR p) { ::lstrcpy(szStr, p); };
      operator LPCTSTR() const { return szStr; };
      TCHAR szStr[MAX_CMDPARAM_LEN];
   };

   CSimpleArray<TCmdString> m_arr;

   BOOL Parse(LPCTSTR pstrCmdLine = NULL)
   {
      if( pstrCmdLine == NULL ) pstrCmdLine = ::GetCommandLine();
      while( *pstrCmdLine ) {
         LPCTSTR pstrStart = pstrCmdLine;
         TCHAR szParam[MAX_CMDPARAM_LEN];
         // Check for and handle quoted program name.
         if( *pstrCmdLine==DQUOTECHAR ) {
            pstrStart++;
            // Scan, and skip over, subsequent characters until
            // another double-quote or a null is encountered.
            do {
               pstrCmdLine = ::CharNext(pstrCmdLine);
            }
            while( (*pstrCmdLine != DQUOTECHAR) && (*pstrCmdLine != _T('\0')) );
            ::lstrcpyn(szParam, pstrStart, min(MAX_CMDPARAM_LEN, pstrCmdLine - pstrStart + 1));
            // If we stopped on a double-quote (usual case), skip over it.
            if( *pstrCmdLine == DQUOTECHAR ) pstrCmdLine = ::CharNext(pstrCmdLine);
         }
         else {
            while( *pstrCmdLine > SPACECHAR ) pstrCmdLine = ::CharNext(pstrCmdLine);
            ::lstrcpyn(szParam, pstrStart, min(MAX_CMDPARAM_LEN, pstrCmdLine - pstrStart + 1));
         }
         szParam[(sizeof(szParam) / sizeof(TCHAR)) - 1] = _T('\0');
         m_arr.Add(TCmdString(szParam));

         // Skip past any white space preceeding the second token.
         while( *pstrCmdLine && (*pstrCmdLine <= SPACECHAR) ) pstrCmdLine = ::CharNext(pstrCmdLine);
      }
      return TRUE;
   }

   LPCTSTR GetItem(int i) const
   {
      if( i < 0 || i >= m_arr.GetSize() ) return NULL;
      return m_arr[i];
   }
   
   int GetSize() const 
   { 
      return m_arr.GetSize(); 
   }
   
   BOOL IsFlag(int i) const
   {
      LPCTSTR p = GetItem(i);
      ATLASSERT(p);
      return (*p == _T('/')) || (*p == _T('-')); 
   }
   
   LPCTSTR GetFlag(int i) const
   {
      ATLASSERT(IsFlag(i));
      return GetItem(i) + 1;
   }
};


#endif // !defined(AFX_ATLCMDLINE_H__20010428_69BD_A531_8A72_0080AD509054__INCLUDED_)

