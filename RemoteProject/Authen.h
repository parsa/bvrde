#if !defined(AFX_AUTHEN_H__20080605_46D3_E5EF_4038_0080AD509054__INCLUDED_)
#define AFX_AUTHEN_H__20080605_46D3_E5EF_4038_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


void SecClearPassword();
CString SecGetPassword();
CString SecDecodePassword(LPCTSTR pstrPassword);
CString SecEncodePassword(LPCTSTR pstrPassword);


#endif // !defined(AFX_AUTHEN_H__20080605_46D3_E5EF_4038_0080AD509054__INCLUDED_)

