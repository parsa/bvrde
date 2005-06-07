#if !defined(AFX_OBJECTMODEL_H__20030712_FE38_93C0_BAC5_0080AD509054__INCLUDED_)
#define AFX_OBJECTMODEL_H__20030712_FE38_93C0_BAC5_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "atldispa.h"

class CMainFrame;


template< class TBase >
class CComRefCount : public TBase
{
public:
   LONG m_dwRef;
   CComRefCount() : m_dwRef(1L) 
   { 
   }
   virtual ULONG STDMETHODCALLTYPE AddRef()
   {
      return ++m_dwRef;
   }
   virtual ULONG STDMETHODCALLTYPE Release()
   {
      LONG lRef = --m_dwRef;
      if( lRef == 0 ) delete this;
      return lRef;
   }
};


class CApplicationOM : public IDispDynImpl<CApplicationOM>
{
public:
   CApplicationOM(CMainFrame* pFrame);

   CMainFrame* m_pOwner;

   BEGIN_DISPATCH_MAP(CApplicationOM)
      DISP_PROPGET(Version, VT_BSTR)
      DISP_PROPGET(RecentProjects, VT_DISPATCH)
      DISP_PROPGET(Solution, VT_DISPATCH)
      DISP_PROPGET(ActiveWindow, VT_DISPATCH)
      DISP_PROPGET(ActiveProject, VT_DISPATCH)
      DISP_METHOD(SendRawMessage, VT_EMPTY, 3, VTS_I4 VTS_I4 VTS_I4)
      DISP_METHOD0(Quit, VT_EMPTY)
   END_DISPATCH_MAP()

   BSTR __stdcall get_Version();
   IDispatch* __stdcall get_Solution();
   IDispatch* __stdcall get_ActiveWindow();
   IDispatch* __stdcall get_ActiveProject();
   IDispatch* __stdcall get_RecentProjects();
   VOID __stdcall SendRawMessage(long uMsg, long wParam, long lParam);
   VOID __stdcall Quit();
};


class CRecentProjectsOM : public CComRefCount< IDispDynImpl<CRecentProjectsOM> >
{
public:
   CRecentProjectsOM(CMainFrame* pFrame);

   CMainFrame* m_pOwner;

   BEGIN_DISPATCH_MAP(CRecentProjectsOM)
      DISP_PROPGET(Count, VT_I4)
      DISP_METHOD1_ID(Item, DISPID_VALUE, VT_BSTR, VT_I4)
   END_DISPATCH_MAP()

   LONG __stdcall get_Count();
   BSTR __stdcall Item(LONG iIndex);
};


class CGlobalsOM : public IDispDynImpl<CGlobalsOM>
{
public:
   void Init(CApplicationOM* pApp);

   CApplicationOM* m_pOwner;

   BEGIN_DISPATCH_MAP(CGlobalsOM)
      DISP_PROPGET(App, VT_DISPATCH)
      DISP_PROPGET(Solution, VT_DISPATCH)
      DISP_PROPGET(ActiveWindow, VT_DISPATCH)
      DISP_PROPGET(ActiveProject, VT_DISPATCH)
      DISP_METHOD1(CreateObject, VT_DISPATCH, VT_BSTR)
      DISP_METHOD1(Sleep, VT_EMPTY, VT_I4)
   END_DISPATCH_MAP()

   IDispatch* __stdcall get_App();
   IDispatch* __stdcall get_Solution();
   IDispatch* __stdcall get_ActiveWindow();
   IDispatch* __stdcall get_ActiveProject();
   IDispatch* __stdcall CreateObject(BSTR bstrProgId);
   VOID __stdcall Sleep(LONG lPeriod);
};


class CSolutionOM : public IDispDynImpl<CSolutionOM>
{
public:
   CSolutionOM(ISolution* pSolution);

   ISolution* m_pOwner;

   BEGIN_DISPATCH_MAP(CSolutionOM)
      DISP_PROPGET(Name, VT_BSTR)
      DISP_PROPGET(Type, VT_BSTR)
      DISP_PROPGET(Filename, VT_BSTR)
      DISP_PROPGET(Projects, VT_DISPATCH)
      DISP_METHOD1(Open, VT_BOOL, VT_BSTR)
      DISP_METHOD1(Save, VT_BOOL, VT_BSTR)
      //DISP_METHOD0(Close, VT_EMPTY)
   END_DISPATCH_MAP()

   BSTR __stdcall get_Name();
   BSTR __stdcall get_Type();
   BSTR __stdcall get_Filename();
   IDispatch* __stdcall get_Projects();
   VARIANT_BOOL __stdcall Open(BSTR Filename);
   VOID __stdcall Close();
   VARIANT_BOOL __stdcall Save(BSTR Filename);
};


class CProjectsOM : public CComRefCount< IDispDynImpl<CProjectsOM> >
{
public:
   CProjectsOM(ISolution* pSolution);

   ISolution* m_pOwner;

   BEGIN_DISPATCH_MAP(CProjectsOM)
      DISP_PROPGET(Count, VT_I4)
      DISP_METHOD1_ID(Item, DISPID_VALUE, VT_DISPATCH, VT_I4)
   END_DISPATCH_MAP()

   LONG __stdcall get_Count();
   IDispatch* __stdcall Item(LONG iIndex);
};


class CProjectOM : public CComRefCount< IDispDynImpl<CProjectOM> >
{
public:
   CProjectOM(IProject* pProject);

   IProject* m_pOwner;

   BEGIN_DISPATCH_MAP(CProjectOM)
      DISP_PROPGET(Name, VT_BSTR)
      DISP_PROPGET(Type, VT_BSTR)
      DISP_PROPGET(Class, VT_BSTR)
      DISP_PROPGET(Files, VT_DISPATCH)
   END_DISPATCH_MAP()

   BSTR __stdcall get_Name();
   BSTR __stdcall get_Type();
   BSTR __stdcall get_Class();
   IDispatch* __stdcall get_Files();
};


class CFilesOM : public CComRefCount< IDispDynImpl<CFilesOM> >
{
public:
   CFilesOM(IProject* pProject);

   IProject* m_pOwner;

   BEGIN_DISPATCH_MAP(CFilesOM)
      DISP_PROPGET(Count, VT_I4)
      DISP_METHOD1_ID(Item, DISPID_VALUE, VT_DISPATCH, VT_I4)
   END_DISPATCH_MAP()

   LONG __stdcall get_Count();
   IDispatch* __stdcall Item(LONG iIndex);
};


class CFileOM : public CComRefCount< IDispDynImpl<CFileOM> >
{
public:
   CFileOM(IView* pView);

   IView* m_pOwner;

   BEGIN_DISPATCH_MAP(CFileOM)
      DISP_PROPGET(Name, VT_BSTR)
      DISP_PROPGET(Type, VT_BSTR)
      DISP_PROPGET(Filename, VT_BSTR)
      DISP_PROPGET(Text, VT_BSTR)
      DISP_METHOD0(Open, VT_BOOL)
      DISP_METHOD0(Close, VT_EMPTY)
      DISP_METHOD0(Save, VT_BOOL)
   END_DISPATCH_MAP()

   BSTR __stdcall get_Name();
   BSTR __stdcall get_Type();
   BSTR __stdcall get_Filename();
   BSTR __stdcall get_Text();
   VARIANT_BOOL __stdcall Open();
   VOID __stdcall Close();
   VARIANT_BOOL __stdcall Save();
};


#endif // !defined(AFX_OBJECTMODEL_H__20030712_FE38_93C0_BAC5_0080AD509054__INCLUDED_)
