#if !defined(AFX_OBJECTMODEL_H__20030719_1CE4_A3BF_46C8_0080AD509054__INCLUDED_)
#define AFX_OBJECTMODEL_H__20030719_1CE4_A3BF_46C8_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "atldispa.h"

class CTextFile;


class CFilesOM : public IDispDynImpl<CFilesOM>
{
public:
   CFilesOM(CRemoteProject* pProject);

   CRemoteProject* m_pOwner;

   BEGIN_DISPATCH_MAP(CFilesOM)
      DISP_PROPGET(Count, VT_I4)
      DISP_METHOD1_ID(Item, DISPID_VALUE, VT_DISPATCH, VT_I4)
   END_DISPATCH_MAP()

   LONG __stdcall get_Count();
   IDispatch* __stdcall Item(LONG iIndex);
};


class CProjectOM : public IDispDynImpl<CProjectOM>
{
public:
   CProjectOM(CRemoteProject* pProject);

   CRemoteProject* m_pOwner;
   CFilesOM m_Files;

   BEGIN_DISPATCH_MAP(CProjectOM)
      DISP_PROPGET(Name, VT_BSTR)
      DISP_PROPGET(Type, VT_BSTR)
      DISP_PROPGET(Class, VT_BSTR)
      DISP_PROPGET(Files, VT_DISPATCH)
      DISP_PROPGET(CurDir, VT_BSTR)
      DISP_PROPGET(Server, VT_BSTR)
      DISP_PROPGET(Username, VT_BSTR)
      DISP_PROPGET(Password, VT_BSTR)
      DISP_PROPGET(IsConnected, VT_BOOL)
      DISP_METHOD0(Clean, VT_EMPTY)
      DISP_METHOD0(Build, VT_EMPTY)
      DISP_METHOD0(Rebuild, VT_EMPTY)
      DISP_METHOD0(StartApp, VT_EMPTY)
      DISP_METHOD0(DebugApp, VT_EMPTY)
      DISP_METHOD(ExecCommand, VT_EMPTY, 2, VTS_BSTR VTS_UNKNOWN)
   END_DISPATCH_MAP()

   BSTR __stdcall get_Name();
   BSTR __stdcall get_Type();
   BSTR __stdcall get_Class();
   BSTR __stdcall get_CurDir();
   BSTR __stdcall get_Server();
   BSTR __stdcall get_Username();
   BSTR __stdcall get_Password();
   VARIANT_BOOL get_IsConnected();
   IDispatch* __stdcall get_Files();
   VOID __stdcall Clean();
   VOID __stdcall Build();
   VOID __stdcall Rebuild();
   VOID __stdcall StartApp();
   VOID __stdcall DebugApp();
   VOID __stdcall ExecCommand(BSTR Command, IUnknown* pUnk);
};


class CFolderOM : public IDispDynImpl<CFolderOM>
{
public:
   CFolderOM(CRemoteProject* pProject, IElement* pElement);

   CRemoteProject* m_pProject;
   IElement* m_pOwner;

   BEGIN_DISPATCH_MAP(CFolderOM)
      DISP_PROPGET(Name, VT_BSTR)
      DISP_PROPGET(Type, VT_BSTR)
      DISP_PROPGET(Count, VT_I4)
      DISP_METHOD1_ID(Item, DISPID_VALUE, VT_DISPATCH, VT_I4)
   END_DISPATCH_MAP()

   BSTR __stdcall get_Name();
   BSTR __stdcall get_Type();
   LONG __stdcall get_Count();
   IDispatch* __stdcall Item(LONG iIndex);
};


class CFileOM : public IDispDynImpl<CFileOM>
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

class CTextFileOM : public IDispDynImpl<CTextFileOM>
{
public:
   CTextFileOM(CTextFile* pView);

   CTextFile* m_pOwner;

   BEGIN_DISPATCH_MAP(CTextFileOM)
      DISP_PROPGET(Name, VT_BSTR)
      DISP_PROPGET(Type, VT_BSTR)
      DISP_PROPGET(Filename, VT_BSTR)
      DISP_PROPGET(Text, VT_BSTR)
      DISP_METHOD0(Open, VT_BOOL)
      DISP_METHOD0(Close, VT_EMPTY)
      DISP_METHOD0(Save, VT_BOOL)
      //
      DISP_PROPGET(TextLength, VT_I4)
      DISP_PROPGET(Lines, VT_I4)
      DISP_PROPGET(CurPos, VT_I4)
      DISP_PROPGET(CurLine, VT_I4)
      DISP_METHOD(SetSelection, VT_EMPTY, 2, VTS_I4 VTS_I4)
      DISP_METHOD0(GetSelection, VT_BSTR)
      DISP_METHOD1(ReplaceSelection, VT_EMPTY, VT_BSTR)
      DISP_METHOD1(PosFromLine, VT_I4, VT_I4)
      DISP_METHOD1(LineLength, VT_I4, VT_I4)
      DISP_METHOD1(FindText, VT_EMPTY, VT_BSTR)
      DISP_METHOD(SendRawMessage, VT_EMPTY, 3, VTS_I4 VTS_I4 VTS_BSTR)
   END_DISPATCH_MAP()

   BSTR __stdcall get_Name();
   BSTR __stdcall get_Type();
   BSTR __stdcall get_Filename();
   BSTR __stdcall get_Text();
   VARIANT_BOOL __stdcall Open();
   VOID __stdcall Close();
   VARIANT_BOOL __stdcall Save();

   INT __stdcall get_TextLength();
   INT __stdcall get_Lines();
   INT __stdcall get_CurPos();
   INT __stdcall get_CurLine();
   VOID __stdcall SetSelection(INT iStart, INT iEnd);
   BSTR __stdcall GetSelection();
   VOID __stdcall ReplaceSelection(BSTR Text);
   INT __stdcall PosFromLine(INT iLine);
   INT __stdcall LineLength(INT iLine);
   VARIANT_BOOL __stdcall FindText(BSTR Text);
   VOID __stdcall SendRawMessage(long uMsg, long wParam, BSTR lParam);
};



#endif // !defined(AFX_OBJECTMODEL_H__20030719_1CE4_A3BF_46C8_0080AD509054__INCLUDED_)

