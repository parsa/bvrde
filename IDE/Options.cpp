
#include "StdAfx.h"
#include "resource.h"

#include "MainFrm.h"
#include "WizardSheet.h"

#if _MSC_VER < 1300
   #pragma code_seg( "WIZARDS" )
#endif


BOOL CMainFrame::OnInitProperties(IWizardManager* /*pManager*/, IElement* /*pElement*/)
{
   return TRUE;
}

BOOL CMainFrame::OnInitWizard(IWizardManager* /*pManager*/, IProject* /*pProject*/, LPCTSTR /*pstrName*/)
{
   return TRUE;
}

BOOL CMainFrame::OnInitOptions(IWizardManager* pManager, ISerializable* pArc)
{
   static CGeneralOptionsPage s_pageGeneral;
   static CDocumentsOptionsPage s_pageDocuments;
   static CAssociationsOptionsPage s_pageAssociations;
   static CMappingsOptionsPage s_pageMappings;
   static CAutoTextOptionsPage s_pageAutoText;
   static CEditorsOptionsPage s_pageEditors;
   static CFoldingOptionsPage s_pageFolding;
   static CColorsOptionsPage s_pageCppColors;
   static CFormattingOptionsPage s_pageCppFormat;
   static CColorsOptionsPage s_pageJavaColors;
   static CFormattingOptionsPage s_pageJavaFormat;
   static CColorsOptionsPage s_pageBasicColors;
   static CFormattingOptionsPage s_pageBasicFormat;
   static CColorsOptionsPage s_pageSqlColors;
   static CFormattingOptionsPage s_pageSqlFormat;
   static CColorsOptionsPage s_pageMakeColors;
   static CFormattingOptionsPage s_pageMakeFormat;
   static CColorsOptionsPage s_pageBashColors;
   static CFormattingOptionsPage s_pageBashFormat;
   static CColorsOptionsPage s_pageHtmlColors;
   static CFormattingOptionsPage s_pageHtmlFormat;
   static CColorsOptionsPage s_pagePhpColors;
   static CFormattingOptionsPage s_pagePhpFormat;
   static CColorsOptionsPage s_pageAspColors;
   static CFormattingOptionsPage s_pageAspFormat;
   static CColorsOptionsPage s_pageXmlColors;
   static CFormattingOptionsPage s_pageXmlFormat;
   static CColorsOptionsPage s_pagePerlColors;
   static CFormattingOptionsPage s_pagePerlFormat;
   static CColorsOptionsPage s_pagePythonColors;
   static CFormattingOptionsPage s_pagePythonFormat;
   static CColorsOptionsPage s_pagePascalColors;
   static CFormattingOptionsPage s_pagePascalFormat;
   static CColorsOptionsPage s_pageTextColors;
   static CFormattingOptionsPage s_pageTextFormat;
   static CPrintingOptionsPage s_pagePrinting;

   static CString sGeneral(MAKEINTRESOURCE(IDS_TREE_GENERAL));
   static CString sAssociations(MAKEINTRESOURCE(IDS_TREE_ASSOCIATIONS));
   static CString sMappings(MAKEINTRESOURCE(IDS_TREE_MAPPINGS));
   static CString sAutoText(MAKEINTRESOURCE(IDS_TREE_AUTOTEXT));
   static CString sDocuments(MAKEINTRESOURCE(IDS_TREE_DOCUMENTS));
   static CString sFolding(MAKEINTRESOURCE(IDS_TREE_FOLDING));
   static CString sColors(MAKEINTRESOURCE(IDS_TREE_COLORS));
   static CString sFormat(MAKEINTRESOURCE(IDS_TREE_FORMATTING));

   s_pageGeneral.SetTitle((LPCTSTR)sGeneral);
   s_pageGeneral.m_pDevEnv = this;
   s_pageGeneral.m_pArc = pArc;
   s_pageDocuments.SetTitle((LPCTSTR)sDocuments);
   s_pageDocuments.m_pDevEnv = this;
   s_pageDocuments.m_pArc = pArc;
   s_pageAutoText.SetTitle((LPCTSTR)sAutoText);
   s_pageAutoText.m_pDevEnv = this;
   s_pageAutoText.m_pArc = pArc;
   s_pageAssociations.SetTitle((LPCTSTR)sAssociations);
   s_pageAssociations.m_pDevEnv = this;
   s_pageAssociations.m_pArc = pArc;
   s_pageMappings.SetTitle((LPCTSTR)sMappings);
   s_pageMappings.m_pDevEnv = this;
   s_pageMappings.m_pArc = pArc;
   s_pageEditors.SetTitle((LPCTSTR)sGeneral);
   s_pageEditors.m_pDevEnv = this;
   s_pageEditors.m_pArc = pArc;
   s_pageFolding.SetTitle((LPCTSTR)sFolding);
   s_pageFolding.m_pDevEnv = this;
   s_pageFolding.m_pArc = pArc;
   s_pageCppColors.SetTitle((LPCTSTR)sColors);
   s_pageCppColors.m_sLanguage = _T("cpp");
   s_pageCppColors.m_pDevEnv = this;
   s_pageCppColors.m_pArc = pArc;
   s_pageCppFormat.SetTitle((LPCTSTR)sFormat);
   s_pageCppFormat.m_sLanguage = _T("cpp");
   s_pageCppFormat.m_pDevEnv = this;
   s_pageCppFormat.m_pArc = pArc;
   s_pageJavaColors.SetTitle((LPCTSTR)sColors);
   s_pageJavaColors.m_sLanguage = _T("java");
   s_pageJavaColors.m_pDevEnv = this;
   s_pageJavaColors.m_pArc = pArc;
   s_pageJavaFormat.SetTitle((LPCTSTR)sFormat);
   s_pageJavaFormat.m_sLanguage = _T("java");
   s_pageJavaFormat.m_pDevEnv = this;
   s_pageJavaFormat.m_pArc = pArc;
   s_pageBasicColors.SetTitle((LPCTSTR)sColors);
   s_pageBasicColors.m_sLanguage = _T("basic");
   s_pageBasicColors.m_pDevEnv = this;
   s_pageBasicColors.m_pArc = pArc;
   s_pageBasicFormat.SetTitle((LPCTSTR)sFormat);
   s_pageBasicFormat.m_sLanguage = _T("basic");
   s_pageBasicFormat.m_pDevEnv = this;
   s_pageBasicFormat.m_pArc = pArc;
   s_pageSqlColors.SetTitle((LPCTSTR)sColors);
   s_pageSqlColors.m_sLanguage = _T("sql");
   s_pageSqlColors.m_pDevEnv = this;
   s_pageSqlColors.m_pArc = pArc;
   s_pageSqlFormat.SetTitle((LPCTSTR)sFormat);
   s_pageSqlFormat.m_sLanguage = _T("sql");
   s_pageSqlFormat.m_pDevEnv = this;
   s_pageSqlFormat.m_pArc = pArc;
   s_pageMakeColors.SetTitle((LPCTSTR)sColors);
   s_pageMakeColors.m_sLanguage = _T("makefile");
   s_pageMakeColors.m_pDevEnv = this;
   s_pageMakeColors.m_pArc = pArc;
   s_pageMakeFormat.SetTitle((LPCTSTR)sFormat);
   s_pageMakeFormat.m_sLanguage = _T("makefile");
   s_pageMakeFormat.m_pDevEnv = this;
   s_pageMakeFormat.m_pArc = pArc;
   s_pageBashColors.SetTitle((LPCTSTR)sColors);
   s_pageBashColors.m_sLanguage = _T("bash");
   s_pageBashColors.m_pDevEnv = this;
   s_pageBashColors.m_pArc = pArc;
   s_pageBashFormat.SetTitle((LPCTSTR)sFormat);
   s_pageBashFormat.m_sLanguage = _T("bash");
   s_pageBashFormat.m_pDevEnv = this;
   s_pageBashFormat.m_pArc = pArc;
   s_pageHtmlColors.SetTitle((LPCTSTR)sColors);
   s_pageHtmlColors.m_sLanguage = _T("html");
   s_pageHtmlColors.m_pDevEnv = this;
   s_pageHtmlColors.m_pArc = pArc;
   s_pageHtmlFormat.SetTitle((LPCTSTR)sFormat);
   s_pageHtmlFormat.m_sLanguage = _T("html");
   s_pageHtmlFormat.m_pDevEnv = this;
   s_pageHtmlFormat.m_pArc = pArc;
   s_pagePhpColors.SetTitle((LPCTSTR)sColors);
   s_pagePhpColors.m_sLanguage = _T("php");
   s_pagePhpColors.m_pDevEnv = this;
   s_pagePhpColors.m_pArc = pArc;
   s_pagePhpFormat.SetTitle((LPCTSTR)sFormat);
   s_pagePhpFormat.m_sLanguage = _T("php");
   s_pagePhpFormat.m_pDevEnv = this;
   s_pagePhpFormat.m_pArc = pArc;
   s_pageAspColors.SetTitle((LPCTSTR)sColors);
   s_pageAspColors.m_sLanguage = _T("asp");
   s_pageAspColors.m_pDevEnv = this;
   s_pageAspColors.m_pArc = pArc;
   s_pageAspFormat.SetTitle((LPCTSTR)sFormat);
   s_pageAspFormat.m_sLanguage = _T("asp");
   s_pageAspFormat.m_pDevEnv = this;
   s_pageAspFormat.m_pArc = pArc;
   s_pageXmlColors.SetTitle((LPCTSTR)sColors);
   s_pageXmlColors.m_sLanguage = _T("xml");
   s_pageXmlColors.m_pDevEnv = this;
   s_pageXmlColors.m_pArc = pArc;
   s_pageXmlFormat.SetTitle((LPCTSTR)sFormat);
   s_pageXmlFormat.m_sLanguage = _T("xml");
   s_pageXmlFormat.m_pDevEnv = this;
   s_pageXmlFormat.m_pArc = pArc;
   s_pagePerlColors.SetTitle((LPCTSTR)sColors);
   s_pagePerlColors.m_sLanguage = _T("perl");
   s_pagePerlColors.m_pDevEnv = this;
   s_pagePerlColors.m_pArc = pArc;
   s_pagePerlFormat.SetTitle((LPCTSTR)sFormat);
   s_pagePerlFormat.m_sLanguage = _T("perl");
   s_pagePerlFormat.m_pDevEnv = this;
   s_pagePerlFormat.m_pArc = pArc;
   s_pagePythonColors.SetTitle((LPCTSTR)sColors);
   s_pagePythonColors.m_sLanguage = _T("python");
   s_pagePythonColors.m_pDevEnv = this;
   s_pagePythonColors.m_pArc = pArc;
   s_pagePythonFormat.SetTitle((LPCTSTR)sFormat);
   s_pagePythonFormat.m_sLanguage = _T("python");
   s_pagePythonFormat.m_pDevEnv = this;
   s_pagePythonFormat.m_pArc = pArc;
   s_pagePascalColors.SetTitle((LPCTSTR)sColors);
   s_pagePascalColors.m_sLanguage = _T("pascal");
   s_pagePascalColors.m_pDevEnv = this;
   s_pagePascalColors.m_pArc = pArc;
   s_pagePascalFormat.SetTitle((LPCTSTR)sFormat);
   s_pagePascalFormat.m_sLanguage = _T("pascal");
   s_pagePascalFormat.m_pDevEnv = this;
   s_pagePascalFormat.m_pArc = pArc;
   s_pageTextColors.SetTitle((LPCTSTR)sColors);
   s_pageTextColors.m_sLanguage = _T("text");
   s_pageTextColors.m_pDevEnv = this;
   s_pageTextColors.m_pArc = pArc;
   s_pageTextFormat.SetTitle((LPCTSTR)sFormat);
   s_pageTextFormat.m_sLanguage = _T("text");
   s_pageTextFormat.m_pDevEnv = this;
   s_pageTextFormat.m_pArc = pArc;
   s_pagePrinting.SetTitle((LPCTSTR)sGeneral);
   s_pagePrinting.m_pDevEnv = this;
   s_pagePrinting.m_pArc = pArc;

   CString sRoot(MAKEINTRESOURCE(IDS_TREE_ENVIRONMENT));
   pManager->SetWizardGroup(sRoot);
   pManager->AddWizardPage(s_pageGeneral.IDD, s_pageGeneral);
   pManager->AddWizardPage(s_pageDocuments.IDD, s_pageDocuments);
   pManager->AddWizardPage(s_pageAutoText.IDD, s_pageAutoText);
   pManager->AddWizardPage(s_pageAssociations.IDD, s_pageAssociations);
   pManager->AddWizardPage(s_pageMappings.IDD, s_pageMappings);

   CString sEditors(MAKEINTRESOURCE(IDS_TREE_EDITORS));
   pManager->AddWizardGroup(sRoot, sEditors);
   pManager->AddWizardPage(s_pageEditors.IDD, s_pageEditors);
   pManager->AddWizardPage(s_pageFolding.IDD, s_pageFolding);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_CPP)));
   pManager->AddWizardPage(s_pageCppColors.IDD, s_pageCppColors);
   pManager->AddWizardPage(s_pageCppFormat.IDD, s_pageCppFormat);
   //
   pManager->AddWizardGroup(sEditors,CString(MAKEINTRESOURCE(IDS_TREE_JAVA)));
   pManager->AddWizardPage(s_pageJavaColors.IDD, s_pageJavaColors);
   pManager->AddWizardPage(s_pageJavaFormat.IDD, s_pageJavaFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_BASIC)));
   pManager->AddWizardPage(s_pageBasicColors.IDD, s_pageBasicColors);
   pManager->AddWizardPage(s_pageBasicFormat.IDD, s_pageBasicFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_PASCAL)));
   pManager->AddWizardPage(s_pagePascalColors.IDD, s_pagePascalColors);
   pManager->AddWizardPage(s_pagePascalFormat.IDD, s_pagePascalFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_PERL)));
   pManager->AddWizardPage(s_pagePerlColors.IDD, s_pagePerlColors);
   pManager->AddWizardPage(s_pagePerlFormat.IDD, s_pagePerlFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_PYTHON)));
   pManager->AddWizardPage(s_pagePythonColors.IDD, s_pagePythonColors);
   pManager->AddWizardPage(s_pagePythonFormat.IDD, s_pagePythonFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_MAKEFILE)));
   pManager->AddWizardPage(s_pageMakeColors.IDD, s_pageMakeColors);
   pManager->AddWizardPage(s_pageMakeFormat.IDD, s_pageMakeFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_BASH)));
   pManager->AddWizardPage(s_pageBashColors.IDD, s_pageBashColors);
   pManager->AddWizardPage(s_pageBashFormat.IDD, s_pageBashFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_SQL)));
   pManager->AddWizardPage(s_pageSqlColors.IDD, s_pageSqlColors);
   pManager->AddWizardPage(s_pageSqlFormat.IDD, s_pageSqlFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_HTML)));
   pManager->AddWizardPage(s_pageHtmlColors.IDD, s_pageHtmlColors);
   pManager->AddWizardPage(s_pageHtmlFormat.IDD, s_pageHtmlFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_PHP)));
   pManager->AddWizardPage(s_pageHtmlColors.IDD, s_pagePhpColors);
   pManager->AddWizardPage(s_pageHtmlFormat.IDD, s_pagePhpFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_ASP)));
   pManager->AddWizardPage(s_pageHtmlColors.IDD, s_pageAspColors);
   pManager->AddWizardPage(s_pageHtmlFormat.IDD, s_pageAspFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_XML)));
   pManager->AddWizardPage(s_pageXmlColors.IDD, s_pageXmlColors);
   pManager->AddWizardPage(s_pageXmlFormat.IDD, s_pageXmlFormat);
   //
   pManager->AddWizardGroup(sEditors, CString(MAKEINTRESOURCE(IDS_TREE_TEXT)));
   pManager->AddWizardPage(s_pageTextColors.IDD, s_pageTextColors);
   pManager->AddWizardPage(s_pageTextFormat.IDD, s_pageTextFormat);

   pManager->AddWizardGroup(sRoot, CString(MAKEINTRESOURCE(IDS_TREE_PRINTING)));
   pManager->AddWizardPage(s_pagePrinting.IDD, s_pagePrinting);

   return TRUE;
}

