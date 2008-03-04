#if !defined(AFX_GDBADAPTOR_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_)
#define AFX_GDBADAPTOR_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


////////////////////////////////////////////////////////
//

class CGdbAdaptor : public IDebuggerAdaptor
{
public:
   CGdbAdaptor();
   virtual ~CGdbAdaptor();

// IDebuggerAdaptor
public:
   void Init(CRemoteProject* pProject);
   CString TransformInput(LPCTSTR pstrInput);
   CString TransformOutput(LPCTSTR pstrOutput);
};



#endif // !defined(AFX_GDBADAPTOR_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_)
