#if !defined(AFX_SOLUTION_H__20030307_7222_6002_DDFE_0080AD509054__INCLUDED_)
#define AFX_SOLUTION_H__20030307_7222_6002_DDFE_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Plugin.h"


class CMainFrame;    // Forward declare


class CSolution : public ISolution
{
public:
   CSolution();

   // Operations

   void Init(CMainFrame* pFrame);
   bool CreateExplorerUI();

   // ISolution

   BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   void Close();
   BOOL LoadSolution(LPCTSTR pstrFilename);
   BOOL SaveSolution(LPCTSTR pstrFilename);
   BOOL IsLoaded() const;
   BOOL IsDirty() const;
   BOOL Load(ISerializable* /*pArchive*/);
   BOOL Save(ISerializable* /*pArchive*/);
   BOOL GetFileName(LPTSTR pstrFilename, UINT cchMax) const;
   IElement* GetParent() const;
   IDispatch* GetDispatch();
   IProject* GetItem(int iIndex);
   int GetItemCount() const;
   BOOL AddProject(IProject* pProject);
   BOOL AddProject(LPCTSTR pstrFilename);
   BOOL RemoveProject(IProject* pProject);
   BOOL SetActiveProject(IProject* pProject);
   IProject* GetActiveProject() const;
   IProject* GetFocusProject() const;

protected:
   typedef struct 
   {
      IProject* pProject;
      CPlugin* pPlugin;
      CString sType;
      CString sFilename;
   } PROJECT;

   bool _LoadSolution(LPCTSTR pstrFilename, IProject*& pDefaultProject);
   bool _LoadProject(LPCTSTR pstrSolutionFilename, ISerializable* pArc);
   bool _AddProject(LPCTSTR pstrSolutionFilename, LPCTSTR pstrFilename, LPCTSTR pstrType);
   bool _SaveSolution();
   bool _SaveProject(ISerializable* pArc, const PROJECT& Project);
   CString _GetComErrorText() const;

   IDevEnv* m_pDevEnv;
   CMainFrame* m_pMainFrame;
   IProject* m_pCurProject;
   CSolutionOM m_Dispatch;
   CString m_sFilename;
   CString m_sName;
   BOOL m_bIsLoaded;
   BOOL m_bIsDirty;
   CSimpleArray<PROJECT> m_aProjects;
};


#endif // !defined(AFX_SOLUTION_H__20030307_7222_6002_DDFE_0080AD509054__INCLUDED_)

