#if !defined(AFX_SCHEMAVIEW_H__20031213_7AC4_4E1C_FBB0_0080AD509054__INCLUDED_)
#define AFX_SCHEMAVIEW_H__20031213_7AC4_4E1C_FBB0_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DbOperations.h"


class CSchemaView : 
   public CWindowImpl<CSchemaView, CWindow>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_SchemaView"))

   CSchemaView();

   // Operations

   CSplitterWindow m_Splitter;
   CContainedWindow m_SplitterHook;
   CTreeViewCtrl m_ctrlTree;
   CListViewCtrl m_ctrlList;
   CAxWindow m_wndInfo;
   CImageList m_ImagesSmall;
   CImageList m_ImagesLarge;

   CDbOperations* m_pDb;
   CComQIPtr<IWebBrowser2> m_spBrowser;

   BOOL PreTranslateMessage(MSG* pMsg);
   void OnIdle(IUpdateUI* pUIBase);

   // Message map and handlers

   BEGIN_MSG_MAP(CSchemaView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
   ALT_MSG_MAP(1)   
      NOTIFY_HANDLER(IDC_DBLIST, LVN_ITEMACTIVATE, OnListActivate)
      NOTIFY_HANDLER(IDC_DBTREE, TVN_ITEMEXPANDING, OnTreeExpanding)
      NOTIFY_HANDLER(IDC_DBTREE, TVN_SELCHANGED, OnTreeSelection)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnListActivate(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeExpanding(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeSelection(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // Implementation

   BOOL _InitHtml();
   BOOL _SetHtml(LPARAM lParam);
   BOOL _FillTreeWithTables(LPARAM lParam, HTREEITEM hParent);
   BOOL _FillTreeWithFields(DATABASEOBJECT* pObj, HTREEITEM hParent);
   BOOL _LoadHtml(IUnknown* pUnk, LPCTSTR pstrHTML);
   HTREEITEM _FindTreeItem(HTREEITEM hItem, LPARAM lParam);
};


#endif // !defined(AFX_SCHEMAVIEW_H__20031213_7AC4_4E1C_FBB0_0080AD509054__INCLUDED_)

