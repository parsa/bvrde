#if !defined(AFX_MANPAGES_H__20041113_D7C2_AF22_709F_0080AD509054__INCLUDED_)
#define AFX_MANPAGES_H__20041113_D7C2_AF22_709F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef MANPAGES_EXPORTS
   #define MANPAGES_API __declspec(dllexport)
#else
   #define MANPAGES_API __declspec(dllimport)
#endif

extern "C" BOOL CALLBACK ManPages_ShowHelp(IDevEnv* pDevEnv, LPCWSTR pstrKeyword, LPCWSTR pstrLanguage);


#endif // !defined(AFX_MANPAGES_H__20041113_D7C2_AF22_709F_0080AD509054__INCLUDED_)

