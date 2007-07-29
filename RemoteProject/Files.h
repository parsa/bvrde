#if !defined(AFX_PROJECTFILE_H__20030307_322B_CAFF_FBBE_0080AD509054__INCLUDED_)
#define AFX_PROJECTFILE_H__20030307_322B_CAFF_FBBE_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ScintillaView.h"
#include "ObjectModel.h"

class CRemoteProject;


///////////////////////////////////////////////////////7
//

class CViewImpl : public IView
{
public:
   CRemoteProject* m_pCppProject;
   IProject* m_pProject;
   IElement* m_pParent;
   CString m_sName;
   CString m_sFilename;
   CString m_sLocation;
   CString m_sLanguage;
   CString m_sFileType;
   FILETIME m_ftCurrent;
   BOOL m_bIsDirty;

public:
   CViewImpl(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);

public:
   // IView
   virtual BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   virtual BOOL IsDirty() const;
   virtual BOOL Load(ISerializable* pArc);
   virtual BOOL Save(ISerializable* pArc);
   virtual void ActivateUI();
   virtual void DeactivateUI();
   virtual void EnableModeless(BOOL bEnable);
   virtual IElement* GetParent() const;
   virtual BOOL SetName(LPCTSTR pstrName);
   virtual BOOL Save();
   virtual BOOL GetText(BSTR* pbstrText);
   virtual BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   virtual LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   virtual LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);

protected:
   CString _GetRealFilename() const;
   bool _IsValidFile(LPBYTE pData, DWORD dwSize) const;
};


///////////////////////////////////////////////////////7
//

class CFolderFile : public CViewImpl
{
public:
   CFolderOM m_Dispatch;

public:
   CFolderFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   BOOL Reload();
   BOOL OpenView(LONG lLineNum);
   void CloseView();
   IDispatch* GetDispatch();
};


///////////////////////////////////////////////////////7
//

class CTextFile : public CViewImpl
{
public:
   CWindow m_wndFrame;
   CScintillaView m_view;
   CTextFileOM m_Dispatch;

public:
   CTextFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);

public:
   BOOL IsOpen() const;

public:
   BOOL Load(ISerializable* pArc);
   BOOL Save(ISerializable* pArc);
   BOOL Reload();
   BOOL Save();
   BOOL IsDirty() const;
   BOOL OpenView(LONG lLineNum);
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;  
   BOOL GetText(BSTR* pbstrText);
   BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   void ActivateUI();
   void EnableModeless(BOOL bEnable);
   void CloseView();
   IDispatch* GetDispatch();
   LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
};


///////////////////////////////////////////////////////7
//

class CCppFile : public CTextFile
{
public:
   CCppFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CHeaderFile : public CCppFile
{
public:
   CHeaderFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CMakeFile : public CTextFile
{
public:
   CMakeFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CBashFile : public CTextFile
{
public:
   CBashFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CJavaFile : public CTextFile
{
public:
   CJavaFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CBasicFile : public CTextFile
{
public:
   CBasicFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CXmlFile : public CTextFile
{
public:
   CXmlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CHtmlFile : public CTextFile
{
public:
   CHtmlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CPhpFile : public CTextFile
{
public:
   CPhpFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CAspFile : public CTextFile
{
public:
   CAspFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CPerlFile : public CTextFile
{
public:
   CPerlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CPythonFile : public CTextFile
{
public:
   CPythonFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};

class CPascalFile : public CTextFile
{
public:
   CPascalFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent);
};



#endif // !defined(AFX_PROJECTFILE_H__20030307_322B_CAFF_FBBE_0080AD509054__INCLUDED_)
