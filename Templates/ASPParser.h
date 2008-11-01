#if !defined(AFX_ASPPARSER_H__20060201_D439_55BD_A8CE_0080AD509054__INCLUDED_)
#define AFX_ASPPARSER_H__20060201_D439_55BD_A8CE_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#pragma optimize("t",on)

/**
 * A class which turns ASP code (with embedded script blocks) into
 * pure script code.
 *
 * When run through the Scripting Engine be sure to "hide" the source code
 * from the user. The source code will not be recognisable. 
 * The line-numbers will match though.
 */
class CParser
{
public:
   CParser()
   {
      SetLanguage(L"VBScript");
      m_pMemoryStart = NULL;
      m_dwSize = 0;
   };

public:
   WCHAR m_szWriteEval_Start[20];
   WCHAR m_szWriteEval_End[20];
   WCHAR m_szStatement[8];
   WCHAR m_szStrConcat[8];
   //
   WCHAR m_szQuote[20];
   WCHAR m_szTab[20];
   WCHAR m_szCrLf[20];
   WCHAR m_szSlash[20];

   LPWSTR m_pMemoryStart;
   LPWSTR m_pMemoryEnd;
   DWORD m_dwSize;

private:
   enum { SIZE_ENDBOUNDRY = 512 };

   inline void Append(LPWSTR &pDst, LPCWSTR pSrc)
   {
      while( *pSrc ) *pDst++ = *pSrc++;
   };

   /**
    * Grows the memory allocated for the output.
    */
   void Grow(LPWSTR &pDst)
   {
      ATLASSERT(pDst);
      ATLASSERT(m_pMemoryStart);
      DWORD dwOffset = pDst - m_pMemoryStart;
      m_dwSize *= 2;
      LPWSTR pNew = new WCHAR[m_dwSize];
      if( pNew == NULL ) return; // AARGH!
      ::CopyMemory(pNew, m_pMemoryStart, m_dwSize / 2 );
      delete [] m_pMemoryStart;
      m_pMemoryStart = pNew;
      pDst = pNew + dwOffset;
      m_pMemoryEnd = pNew + m_dwSize - SIZE_ENDBOUNDRY;
   }

   /**
    * Parse raw script text.
    * We simply copy the stream until we reach the block end.
    */
   void ParseScript(LPWSTR &pDst, LPCWSTR &pSrc)
   {   
      ATLASSERT(pSrc);
      ATLASSERT(pDst);
      WCHAR ch;
      while( (ch = *pSrc++) != 0 ) {
         if( ch == L'%' ) {
            if( *pSrc == L'>' ) {
               pSrc++;
               break;
            }
            else *pDst++ = ch;
         }
         else *pDst++ = ch;
         if( pDst > m_pMemoryEnd ) Grow(pDst);
      }
   }

public:
   /**
    * Sets the scripting language identifiers
    */
   void SetLanguage(LPCWSTR pstrLanguage)
   {
      ATLASSERT(pstrLanguage);
      if( ::lstrcmpiW(pstrLanguage, L"VBSCRIPT") == 0 ) {
         wcscpy( m_szWriteEval_Start, L"Response.Write " );
         wcscpy( m_szWriteEval_End, L"" );
         wcscpy( m_szStatement, L":" );
         wcscpy( m_szStrConcat, L"+" );
         //
         wcscpy( m_szQuote, L"\"\"" );
         wcscpy( m_szTab, L"\" & vbTab & \"" );
         wcscpy( m_szSlash, L"\\" );
         wcscpy( m_szCrLf, L"\" & vbCrLf & \"" );
      }
      else {
         // Default to JScript
         // (also good for other C like script engines)
         wcscpy( m_szWriteEval_Start, L"Response.Write(" );
         wcscpy( m_szWriteEval_End, L")" );
         wcscpy( m_szStatement, L";" );
         wcscpy( m_szStrConcat, L"+" );
         //
         wcscpy( m_szQuote, L"\\\"" );
         wcscpy( m_szTab, L"\\t" );
         wcscpy( m_szSlash, L"\\\\" );
         wcscpy( m_szCrLf, L"\\r\\n" );
      }
   }

   /**
    * Converts the ASP text into pure scripting code.
    *
    * To make this work we take advantage of the VBScript
    * syntax is very weak (probably to make the real ASP work).
    * We are allowed to use the following syntax:
    *   : If a = 1 Then : b = 2 : Else b = 3
    * (Note the excessive use of colons). We can avoid a lot of parsing/checking
    * by relying on this syntax. JScript allows all kinds of wierd use of 
    * the ";" block scope as well.
    *
    * It is the resposibility of the caller to release
    * the returned memory.
    */
   LPWSTR Process(LPCWSTR pSrc)
   {
      ATLASSERT(pSrc);

      ATLTRACE("CASP: Script size before parse: %d\n", wcslen(pSrc));

      m_dwSize = wcslen(pSrc) * 4;
      if( m_dwSize < 4096 ) m_dwSize = 4096;
      LPWSTR pDst = new WCHAR[m_dwSize];
      if( pDst == NULL ) return NULL;
      m_pMemoryStart = pDst;
      m_pMemoryEnd = pDst + m_dwSize - SIZE_ENDBOUNDRY;

      Append(pDst, m_szWriteEval_Start);
      Append(pDst, L"\"");
      WCHAR ch;
      while( (ch = *pSrc++) != '\0' ) {
      
         if( ch == L'<' && *pSrc == L'%' ) {
            pSrc++;

            if( *pSrc == L'=' ) {
               pSrc++; // skip the '=' char
               Append(pDst, L"\"");
               Append(pDst, m_szStrConcat);

               ParseScript(pDst, pSrc);

               Append(pDst, m_szStrConcat);
               Append(pDst, L"\"");
            }
            else {
               Append(pDst, L"\"");
               Append(pDst, m_szWriteEval_End);
               Append(pDst, m_szStatement);

               ParseScript(pDst, pSrc);
               Append(pDst, m_szStatement);

               Append(pDst, m_szWriteEval_Start);
               Append(pDst, L"\"");
            }
            continue;
         }

         switch( ch ) {
         case L'\r':
            pSrc++; // assume \r\n combination and ignore \r
            // FALL THROUGH...
         case L'\n':
            Append(pDst, m_szCrLf);
            Append(pDst, L"\"");
            Append(pDst, m_szWriteEval_End);
            Append(pDst, L"\n");
            Append(pDst, m_szWriteEval_Start);
            Append(pDst, L"\"");
            break;
         case L'\"':
            Append(pDst, m_szQuote);
            break;
         case L'\\':
            Append(pDst, m_szSlash);
            break;
         case L'\t':
            Append(pDst, m_szTab);
            break;
         default:
            *pDst++ = ch;
         }
      
         if( pDst > m_pMemoryEnd ) Grow(pDst);
      }
      Append(pDst, L"\"");
      Append(pDst, m_szWriteEval_End);
      *pDst = L'\0';

#ifdef _DEBUG
      USES_CONVERSION; 
      CFile f;
      f.Create(_T("C:\\TEMP\\SCRIPT.TXT"));
      f.Write(W2CT(m_pMemoryStart),wcslen(m_pMemoryStart));
      f.Close();
#endif

      ATLTRACE("CASP: Script size after parse: %d\n", wcslen(m_pMemoryStart));
      return m_pMemoryStart;
   };

};

#pragma optimize("t",off)


#endif // !defined(AFX_ASPPARSER_H__20060201_D439_55BD_A8CE_0080AD509054__INCLUDED_)

