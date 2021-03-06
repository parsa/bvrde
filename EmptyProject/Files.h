#if !defined(AFX_PROJECTFILE_H__20030307_322B_CAFF_FBBE_0080AD509054__INCLUDED_)
#define AFX_PROJECTFILE_H__20030307_322B_CAFF_FBBE_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ScintillaView.h"

class CEmptyProject;


///////////////////////////////////////////////////////7
//

class CViewImpl : public IView
{
public:
   CEmptyProject* m_pLocalProject;
   IProject* m_pProject;
   IElement* m_pParent;
   CString m_sName;
   CString m_sFilename;
   CString m_sLocation;
   BOOL m_bIsDirty;

public:
   CViewImpl(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   // IView
   virtual BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   virtual BOOL IsDirty() const;
   virtual BOOL Load(ISerializable* pArc);
   virtual BOOL Save(ISerializable* pArc);
   virtual void ActivateUI();
   virtual void DeactivateUI();
   virtual void EnableModeless(BOOL bEnable);
   virtual BOOL SetName(LPCTSTR pstrName);
   virtual BOOL Save();
   virtual BOOL Reload();
   virtual BOOL GetText(BSTR* pbstrText);
   virtual BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   virtual LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   virtual LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   virtual IElement* GetParent() const;
   virtual IDispatch* GetDispatch();

protected:
   CString _GetRealFilename() const;
};


///////////////////////////////////////////////////////7
//

class CFolderFile : public CViewImpl
{
public:
   CFolderFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL OpenView(long lLineNum);
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   void CloseView();
};


///////////////////////////////////////////////////////7
//

class CTextFile : public CViewImpl
{
public:
   CWindow m_wndFrame;
   CScintillaView m_view;
   CString m_sLanguage;

public:
   CTextFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL IsOpen() const;

public:
   BOOL Load(ISerializable* pArc);
   BOOL Save(ISerializable* pArc);
   BOOL Save();
   BOOL IsDirty() const;
   BOOL OpenView(long lLineNum);
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;  
   BOOL GetText(BSTR* pbstrText);
   BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   void EnableModeless(BOOL bEnable);
   void CloseView();
   LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
};


///////////////////////////////////////////////////////7
//

class CCppFile : public CTextFile
{
public:
   CCppFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL Load(ISerializable* pArc);
   BOOL Save(ISerializable* pArc);
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CHeaderFile : public CCppFile
{
public:
   CHeaderFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CMakeFile : public CTextFile
{
public:
   CMakeFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CJavaFile : public CTextFile
{
public:
   CJavaFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CBasicFile : public CTextFile
{
public:
   CBasicFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CXmlFile : public CTextFile
{
public:
   CXmlFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CHtmlFile : public CTextFile
{
public:
   CHtmlFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CPhpFile : public CTextFile
{
public:
   CPhpFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CAspFile : public CTextFile
{
public:
   CAspFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CPerlFile : public CTextFile
{
public:
   CPerlFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CPythonFile : public CTextFile
{
public:
   CPythonFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


///////////////////////////////////////////////////////7
//

class CPascalFile : public CTextFile
{
public:
   CPascalFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);

public:
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
};


#endif // !defined(AFX_PROJECTFILE_H__20030307_322B_CAFF_FBBE_0080AD509054__INCLUDED_)
