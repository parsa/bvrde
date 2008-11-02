
#include "StdAfx.h"
#include "resource.h"

#include "DiffCvsView.h"

#pragma code_seg( "MISC" )

#pragma warning(disable : 4239)


////////////////////////////////////////////////////////
// Parse Original Diff format

BOOL CDiffCvsView::_ParseDiffOriginal(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, DIFFINFO& Info)
{
   // Parsing original DIFF format. Older versions of cvs and other source control
   // systems are still using this.
   enum { STATE_IGNORE, STATE_INSERT, STATE_DELETE, STATE_MERGE } State = STATE_IGNORE; 
   CString& sHTML = Info.sHTML;
   CString sEmpty;
   CString sTemp;
   int iLineNo = 0;                         // Current line-no on right side
   int iLeftRange = 0;                      // Diff left range count
   int iRightRange = 0;                     // Diff right range count
   int nFileLines = aFile.GetSize();        // Number of lines in file
   int nDiffLines = aLines.GetSize();       // Number of lines in diff output
   for( int i = 0; i < nDiffLines && iLineNo < nFileLines; ) {
      switch( State ) {
      case STATE_IGNORE:
         {
            CString& sLine = aLines[i++];
            if( _tcsncmp(sLine, _T("diff "), 5) == 0 ) {
               Info.sGeneralFileInfo = sLine.Mid(5);
            }
            else if( !sLine.IsEmpty() && _istdigit(sLine[0]) ) {
               iLeftRange = 1;
               iRightRange = 1;
               LPCTSTR p = sLine;
               int iLeftStart = _ttoi(p);
               while( _istdigit(*p) ) p++;
               if( *p == ',' ) iLeftRange = _ttoi(++p) - iLeftStart + 1;
               while( _istdigit(*p) ) p++;
               if( *p == 'c' ) State = STATE_MERGE;
               else if( *p == 'a' ) State = STATE_INSERT;
               else if( *p == 'd' ) State = STATE_DELETE;
               else break;
               int iRightStart = _ttoi(++p);
               while( _istdigit(*p) ) p++;
               if( *p == ',' ) iRightRange = _ttoi(++p) - iRightStart + 1;
               if( Info.bListUnchanged ) {
                  while( iLineNo < iRightStart - 1 && iLineNo < nFileLines ) {
                     _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
                     iLineNo++;
                  }
                  if( Info.iFirstChange < 0 ) Info.iFirstChange = iLineNo;
               }
               else {
                  _GenerateSectionHeader(sHTML, iLeftStart, iRightStart);
                  iLineNo = iRightStart - 1;
               }
            }
         }
         break;
      case STATE_INSERT:
         {
            ATLASSERT(iRightRange>0);
            while( iRightRange > 0 && i < nDiffLines && iLineNo < nFileLines ) {
               CString& sLine = aLines[i++];
               switch( sLine[0] ) {
               case '>':
                  {
                     _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("ins"), sEmpty, sLine.Mid(2));
                     iRightRange--; iLineNo++;
                  }
                  break;
               }
            }
            State = STATE_IGNORE;
         }
         break;
      case STATE_DELETE:
         {
            ATLASSERT(iLeftRange>0);
            while( iLeftRange > 0 && i < nDiffLines && iLineNo < nFileLines ) {
               CString& sLine = aLines[i++];
               switch( sLine[0] ) {
               case '<':
                  {
                     _GenerateRow(sHTML, sTemp, 0, _T("del"), sLine.Mid(2), sEmpty);
                     iLeftRange--;
                  }
                  break;
               }
            }
            State = STATE_IGNORE;
         }
         break;
      case STATE_MERGE:
         {
            int iRightSkip = 0;
            while( (iLeftRange > 0 || iRightRange > 0 || iRightSkip > 0) && i < nDiffLines && iLineNo < nFileLines ) {
               CString& sLine = aLines[i++];
               switch( sLine[0] ) {
               case '-': break;
               case '<':
                  {
                     ATLASSERT(iLeftRange>0);
                     int nMinusLines = 0;
                     for( int j = i; j < nDiffLines && aLines[j][0] == '<'; j++ ) nMinusLines++;
                     if( iRightRange > 0 
                         && i + nMinusLines + iRightSkip + 1 < nDiffLines 
                         && aLines[i + nMinusLines][0] == '-'
                         && aLines[i + nMinusLines + iRightSkip + 1][0] == '>' ) 
                     {
                        _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("mod"), sLine.Mid(2), aLines[i + nMinusLines + iRightSkip + 1].Mid(2));
                        iLeftRange--; iRightRange--; iRightSkip++; iLineNo++;
                     }
                     else 
                     {
                        _GenerateRow(sHTML, sTemp, 0, _T("del"), sLine.Mid(2), sEmpty);
                        iLeftRange--;
                     }
                  }
                  break;
               case '>':
                  {
                     ATLASSERT(iRightRange+iRightSkip>0);
                     if( iRightSkip > 0 ) 
                     {
                        iRightSkip--;
                     }
                     else 
                     {
                        _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("ins"), sEmpty, sLine.Mid(2));
                        iRightRange--; iLineNo++;
                     }
                  }
                  break;
               }
            }
            State = STATE_IGNORE;
         }
         break;
      }
   }
   if( Info.bListUnchanged ) {
      while( iLineNo > 0 && iLineNo < nFileLines ) {
         _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
         iLineNo++;
      }
   }
   return (iLineNo > 0);
}


////////////////////////////////////////////////////////
// Parse Context Diff format

BOOL CDiffCvsView::_ParseDiffContext(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, DIFFINFO& Info)
{
   // Parsing CONTEXT DIFF format. This was introduced under the BSD platform, and is
   // still an output option for cvs and other source control tools.
   enum { STATE_INTRO, STATE_MERGE } State = STATE_INTRO; 
   CString& sHTML = Info.sHTML;
   CString sEmpty;
   CString sTemp;
   int iLineNo = 0;                         // Current line-no on right side
   int iLeftStart = 0;                      // Diff left start 
   int iLeftRange = 0;                      // Diff left range count
   int iRightStart = 0;                     // Diff right start
   int iRightRange = 0;                     // Diff right range count
   int iDualChunkPos = 0;                   // Index in lines array of dual-chunk
   bool bSeenMod = false;                   // Seen modified line in batch?
   int nFileLines = aFile.GetSize();        // Number of lines in file
   int nDiffLines = aLines.GetSize();       // Number of lines in diff output
   for( int i = 0; i < nDiffLines && iLineNo < nFileLines; i++ ) {
      switch( State ) {
      case STATE_INTRO:
         {
            CString& sLine = aLines[i];
            if( _tcsncmp(sLine, _T("*** "), 4) == 0 ) {
               Info.sLeftFileInfo = sLine.Mid(4);
            }
            else if( _tcsncmp(sLine, _T("--- "), 4) == 0 ) {
               Info.sRightFileInfo = sLine.Mid(4);
            }
            else if( _tcsncmp(sLine, _T("*****"), 5) == 0 ) {
               State = STATE_MERGE;
            }
         }
         break;
      case STATE_MERGE:
         {
            CString& sLine1 = aLines[i];
            CString& sLine2 = iDualChunkPos > 0 && iDualChunkPos < nDiffLines ? aLines[iDualChunkPos] : _T("x");
            if( _tcsncmp(sLine1, _T("*** "), 4) == 0 ) 
            {
               LPCTSTR p = sLine1;
               p += 4;
               iLeftStart = _ttoi(p);
               iLeftRange = 1;
               while( _istdigit(*p) ) p++;
               if( *p == ',' ) iLeftRange = _ttoi(++p) - iLeftStart + 1;
               // Find the start of the other chunk-half
               bSeenMod = false;
               iRightStart = 0;
               iDualChunkPos = 0;
               if( aLines[i + 1].Left(4) != _T("--- ") ) 
               {
                  for( int j = i + 1; j < nDiffLines; j++ ) {
                     sLine2 = aLines[j]; 
                     switch( sLine2[0] ) {
                     case '-': 
                        {
                           if( sLine2.Left(4) == _T("--- ") ) {
                              iDualChunkPos = j + 1; 
                              LPCTSTR p = sLine2;
                              p += 4;
                              iRightStart = _ttoi(p);
                              iRightRange = 1;
                              while( _istdigit(*p) ) p++;
                              if( *p == ',' ) iRightRange = _ttoi(++p) - iRightStart + 1;
                              j = nDiffLines; 
                           }
                        }
                        break;
                     case '+':
                     case '!': 
                     case ' ': 
                        break;
                     default:
                        j = nDiffLines;   // exit; no 2nd half
                        break;
                     }
                  }
                  if( Info.bListUnchanged ) {
                     while( iLineNo < iRightStart - 1 && iLineNo < nFileLines ) {
                        _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
                        iLineNo++;
                     }
                     if( Info.iFirstChange < 0 ) Info.iFirstChange = iLineNo;
                  }
                  else {
                     _GenerateSectionHeader(sHTML, iLeftStart, iRightStart);
                     iLineNo = iRightStart - 1;
                  }
               }
            }
            else if( _tcsncmp(sLine1, _T("--- "), 4) == 0 )
            {
               LPCTSTR p = sLine1;
               p += 4;
               iRightStart = _ttoi(p);
               iRightRange = 1;
               while( _istdigit(*p) ) p++;
               if( *p == ',' ) iRightRange = _ttoi(++p) - iRightStart + 1;
               if( Info.bListUnchanged ) {
                  while( iLineNo < iRightStart - 1 && iLineNo < nFileLines ) {
                     _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
                     iLineNo++;
                  }
                  if( Info.iFirstChange < 0 ) Info.iFirstChange = iLineNo;
               }
               else {
                  if( iDualChunkPos == 0 ) _GenerateSectionHeader(sHTML, iLeftStart, iRightStart);
                  iLineNo = iRightStart - 1;
               }
               if( iDualChunkPos > 0 ) i = iDualChunkPos, iRightRange = 0, iDualChunkPos = 0;
               bSeenMod = false;
            }
            else if( sLine1[0] == ' ' && (sLine2[0] == ' ' || sLine2[0] == 'x') )
            {
               ATLASSERT(iLeftRange>0 && (iRightRange>0 || iDualChunkPos==0));
               _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
               iLeftRange--; iRightRange--; iLineNo++;
               if( iDualChunkPos > 0 ) iDualChunkPos++;
               bSeenMod = false;
            }
            else if( sLine1[0] == '+' )
            {
               ATLASSERT(iRightRange>0);
               _GenerateRow(sHTML, sTemp, iLineNo + 1, bSeenMod ? _T("mod2") : _T("ins"), sEmpty, sLine1.Mid(2));
               iRightRange--; iLineNo++;
            }
            else if( sLine2[0] == '+' )
            {
               ATLASSERT(iRightRange>0);
               _GenerateRow(sHTML, sTemp, iLineNo + 1, bSeenMod ? _T("mod2") : _T("ins"), sEmpty, sLine2.Mid(2));
               iRightRange--; iLineNo++; iDualChunkPos++; i--;
            }
            else if( sLine1[0] == '-' )
            {
               ATLASSERT(iLeftRange>0);
               _GenerateRow(sHTML, sTemp, 0, bSeenMod ? _T("mod2") : _T("del"), sLine1.Mid(2), sEmpty);
               iLeftRange--;
            }
            else if( sLine1[0] == '!' )
            {
               if( sLine2[0] == '!' ) {
                  _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("mod"), sLine1.Mid(2), sLine2.Mid(2));
                  iLeftRange--; iRightRange--; iLineNo++; iDualChunkPos++;
                  bSeenMod = true;
               }
               else {
                  _GenerateRow(sHTML, sTemp, 0, bSeenMod ? _T("mod2") : _T("del"), sLine1.Mid(2), sEmpty);
                  iLeftRange--;
               }
            }
            else if( sLine2[0] == '!' )
            {
               _GenerateRow(sHTML, sTemp, iLineNo + 1, bSeenMod ? _T("mod2") : _T("ins"), sEmpty, sLine2.Mid(2));
               iRightRange--; iLineNo++; iDualChunkPos++; i--;
            }
         }
      }
   }
   if( Info.bListUnchanged ) {
      while( iLineNo > 0 && iLineNo < nFileLines ) {
         _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
         iLineNo++;
      }
   }
   return (iLineNo > 0);
}


////////////////////////////////////////////////////////
// Parse Universal Diff format

BOOL CDiffCvsView::_ParseDiffUnidiff(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, DIFFINFO& Info)
{
   // Parsing UNIDIFF format. This is what most modern source control systems seem to
   // use these days. The GNU diff 1.15 tool features this.
   enum { STATE_IGNORE, STATE_MERGE } State = STATE_IGNORE; 
   CString& sHTML = Info.sHTML;
   CString sEmpty;
   CString sTemp;
   int iLineNo = 0;                         // Current line-no on right side
   int iLeftRange = 0;                      // Diff left range count
   int iRightRange = 0;                     // Diff right range count
   int nFileLines = aFile.GetSize();        // Number of lines in file
   int nDiffLines = aLines.GetSize();       // Number of lines in diff output
   bool bSeenMod = false;                   // Seen modified line in batch?
   for( int i = 0; i < nDiffLines && iLineNo < nFileLines; ) {
      switch( State ) {
      case STATE_IGNORE:
         {
            CString& sLine = aLines[i++];
            if( _tcsncmp(sLine, _T("--- "), 4) == 0 ) {
               Info.sLeftFileInfo = sLine.Mid(4);
            }
            else if( _tcsncmp(sLine, _T("+++ "), 4) == 0 ) {
               Info.sRightFileInfo = sLine.Mid(4);
            }
            else if( _tcsncmp(sLine, _T("@@ "), 3) == 0 ) {
               iLeftRange = 1;
               iRightRange = 1;
               LPCTSTR p = sLine;
               p += 2;
               while( _istspace(*p) ) p++;
               if( *p == '-' ) p++;
               int iLeftStart = _ttoi(p);
               while( _istdigit(*p) ) p++;
               if( *p == ',' ) iLeftRange = _ttoi(++p);
               while( _istdigit(*p) ) p++;
               while( _istspace(*p) ) p++;
               int iRightStart = 0;
               if( *p == '+' ) iRightStart = _ttoi(++p);
               while( _istdigit(*p) ) p++;
               if( *p == ',' ) iRightRange = _ttoi(++p);
               if( Info.bListUnchanged ) {
                  while( iLineNo < iRightStart - 1 && iLineNo < nFileLines ) {
                     _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
                     iLineNo++;
                  }
                  if( Info.iFirstChange < 0 ) Info.iFirstChange = iLineNo;
               }
               else {
                  _GenerateSectionHeader(sHTML, iLeftStart, iRightStart);
                  iLineNo = iRightStart - 1;
               }
               State = STATE_MERGE;
               bSeenMod = false;
            }
         }
         break;
      case STATE_MERGE:
         {
            int iRightSkip = 0;
            while( (iLeftRange > 0 || iRightRange > 0 || iRightSkip > 0) && i < nDiffLines && iLineNo < nFileLines ) {
               CString& sLine = aLines[i++];
               switch( sLine[0] ) {
               case ' ':
                  {
                     ATLASSERT(iLeftRange>0 && iRightRange>0);
                     _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), sLine.Mid(1), sLine.Mid(1));
                     iLeftRange--; iRightRange--; iLineNo++;
                     bSeenMod = false;
                  }
                  break;
               case '-':
                  {
                     ATLASSERT(iLeftRange>0);
                     int nMinusLines = 0;
                     for( int j = i; j < nDiffLines && aLines[j][0] == '-'; j++ ) nMinusLines++;
                     if( iRightRange > 0 
                         && i + nMinusLines + iRightSkip < nDiffLines 
                         && aLines[i + nMinusLines + iRightSkip][0] == '+' ) 
                     {
                        _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("mod"), sLine.Mid(1), aLines[i + nMinusLines + iRightSkip].Mid(1));
                        iLeftRange--; iRightRange--; iRightSkip++; iLineNo++;
                        bSeenMod = true;
                     }
                     else 
                     {
                        _GenerateRow(sHTML, sTemp, 0, _T("del"), sLine.Mid(1), sEmpty);
                        iLeftRange--;
                     }
                  }
                  break;
               case '+':
                  {
                     ATLASSERT(iRightRange+iRightSkip>0);
                     if( iRightSkip > 0 ) 
                     {
                        iRightSkip--;
                     }
                     else 
                     {
                        _GenerateRow(sHTML, sTemp, iLineNo + 1, bSeenMod ? _T("mod2") : _T("ins"), sEmpty, sLine.Mid(1));
                        iRightRange--; iLineNo++;
                     }
                  }
                  break;
               }
            }
            State = STATE_IGNORE;
            bSeenMod = false;
         }
         break;
      }
   }
   if( Info.bListUnchanged ) {
      while( iLineNo > 0 && iLineNo < nFileLines ) {
         _GenerateRow(sHTML, sTemp, iLineNo + 1, _T("nom"), aFile[iLineNo], aFile[iLineNo]);
         iLineNo++;
      }
   }
   return (iLineNo > 0);
}

#pragma warning(default : 4239)
