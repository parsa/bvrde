#if !defined(AFX_CPPLEXER_H__20040701_E30C_3544_E638_0080AD509054__INCLUDED_)
#define AFX_CPPLEXER_H__20040701_E30C_3544_E638_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef CPPLEXER_EXPORTS
   #define CPPLEXER_API __declspec(dllexport)
#else
   #define CPPLEXER_API __declspec(dllimport)
#endif

extern "C" BOOL CALLBACK CppLexer_Parse(LPCSTR pstrSourceName, LPCSTR pstrText, LPCWSTR pstrOutputFile);


#endif // !defined(AFX_CPPLEXER_H__20040701_E30C_3544_E638_0080AD509054__INCLUDED_)

