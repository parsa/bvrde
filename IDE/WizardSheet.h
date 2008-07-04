#if !defined(AFX_WIZARDSHEET_H__20030402_9A74_AF0C_98E1_0080AD509054__INCLUDED_)
#define AFX_WIZARDSHEET_H__20030402_9A74_AF0C_98E1_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

#include "FontCombo.h"
#include "ColorCombo.h"
#include "PropertyGrid.h"

class CPlugin;  // Forward declare


///////////////////////////////////////////////////////////////
//

class CSolutionFinishPage : public CPropertyPageImpl<CSolutionFinishPage>
{
public:
   enum { IDD = IDD_WIZARD_SOLUTIONDONE };

   // Overloads

   int OnSetActive();
   int OnWizardFinish();

   // Message map and handlers

   BEGIN_MSG_MAP(CSolutionFinishPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CSolutionFinishPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CSolutionTypePage : public CPropertyPageImpl<CSolutionTypePage>
{
public:
   enum { IDD = IDD_WIZARD_SOLUTIONTYPE };

   CSolutionTypePage();

   IDevEnv* m_pDevEnv;
   CMainFrame* m_pMainFrame;
   IProject* m_pProject;
   CPlugin* m_pPlugin;
   
   CSolutionFinishPage m_pageFinish;
   CImageListCtrl m_Images;
   CEdit m_ctrlName;
   CListViewCtrl m_ctrlList;

   // Operations

   void Init(IDevEnv* pDevEnv, CMainFrame* pFrame);

   // Overloads

	void OnReset();
   int OnSetActive();
	int OnWizardNext();
	int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CSolutionTypePage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemSelected)
      NOTIFY_CODE_HANDLER(NM_DBLCLK, OnItemDblClick)
      CHAIN_MSG_MAP( CPropertyPageImpl<CSolutionTypePage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnItemSelected(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnItemDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CGeneralOptionsPage : public CPropertyPageImpl<CGeneralOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_GENERAL };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;
   bool m_bChanged;

   CButton m_ctrlTabbed;
   CButton m_ctrlMdi;
   CButton m_ctrlMultiInstance;
   CComboBox m_ctrlStartup;
   CComboBox m_ctrlLanguage;
   CStatic m_ctrlGreyed;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CGeneralOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
      COMMAND_HANDLER(IDC_LANGUAGE, CBN_SELCHANGE, OnChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<CGeneralOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CDocumentsOptionsPage : public CPropertyPageImpl<CDocumentsOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_DOCUMENTS };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CDocumentsOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CDocumentsOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CAssociationsOptionsPage : public CPropertyPageImpl<CAssociationsOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_ASSOCIATIONS };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;

   CCheckListViewCtrl m_ctrlList;

   // Overloads

   int OnApply();
   int OnSetActive();

   // Message map and handlers

   BEGIN_MSG_MAP(CAssociationsOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CAssociationsOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CMappingsOptionsPage : public CPropertyPageImpl<CMappingsOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_MAPPINGS };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;

   CPropertyGridCtrl m_ctrlList;

   // Overloads

   int OnApply();
   int OnSetActive();

   // Message map and handlers

   BEGIN_MSG_MAP(CMappingsOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      NOTIFY_CODE_HANDLER(PIN_ADDITEM, OnAddItem);
      CHAIN_MSG_MAP( CPropertyPageImpl<CMappingsOptionsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnAddItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};


////////////////////////////////////////////////////////////////////////
//

class CAutoTextOptionsPage : public CPropertyPageImpl<CAutoTextOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_AUTOTEXT };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;

   CListBox m_ctrlList;
   CEdit m_ctrlName;
   CEdit m_ctrlText;
   CButton m_ctrlNew;
   CButton m_ctrlDelete;
   CIcon m_iconNew;
   CIcon m_iconDelete;

   // Overloads

   int OnApply();
   int OnSetActive();

   // Message map and handlers

   BEGIN_MSG_MAP(CAutoTextOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
      COMMAND_HANDLER(IDC_NAME, EN_CHANGE, OnNameChange)
      COMMAND_HANDLER(IDC_TEXT, EN_CHANGE, OnTextChange)
      COMMAND_HANDLER(IDC_LIST, LBN_SELCHANGE, OnItemSelect)
      COMMAND_ID_HANDLER(IDC_NEW, OnNewItem)
      COMMAND_ID_HANDLER(IDC_DELETE, OnDeleteItem)
      CHAIN_MSG_MAP( CPropertyPageImpl<CAutoTextOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCtlColorListBox(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnItemSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnNameChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTextChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnNewItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDeleteItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // Implementation

   void _UpdateButtons();
};


////////////////////////////////////////////////////////////////////////
//

class CFoldingOptionsPage : public CPropertyPageImpl<CFoldingOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_FOLDING };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CFoldingOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CFoldingOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CColorsOptionsPage : public CPropertyPageImpl<CColorsOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_COLORS };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;
   CString m_sLanguage;

   typedef struct tagSTYLEFONT 
   {
      TCHAR szTitle[64];
      TCHAR szFaceName[LF_FACESIZE];
      COLORREF clrText;
      COLORREF clrBack;
      int iHeight;
      bool bItalic;
      bool bBold;
   } STYLEFONT;

   STYLEFONT m_aFonts[15];

   CListBox m_ctrlList;
   CFontPickerComboCtrl m_ctrlFace;
   CComboBox m_ctrlSize;
   CButton m_ctrlBold;
   CButton m_ctrlItalic;
   CColorPickerComboCtrl m_ctrlForeColor;
   CColorPickerComboCtrl m_ctrlBackColor;
   CStatic m_ctrlSample;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CColorsOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
      MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
      COMMAND_HANDLER(IDC_LIST, LBN_SELCHANGE, OnItemSelect)
      COMMAND_HANDLER(IDC_FONT, CBN_SELCHANGE, OnFaceChange)
      COMMAND_ID_HANDLER(IDC_CUSTOM_FORE, OnCustomFore)
      COMMAND_ID_HANDLER(IDC_CUSTOM_BACK, OnCustomBack)
      COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
      COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnChange)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<CColorsOptionsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCtlColorListBox(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnItemSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFaceChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCustomFore(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCustomBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   void _UpdateValues();
   void _AddComboColors(CColorPickerComboCtrl& combo) const;
   void _SetCustomColor(CColorPickerComboCtrl& ctlrColor, COLORREF clr) const;
};


////////////////////////////////////////////////////////////////////////
//

class CFormattingOptionsPage : public CPropertyPageImpl<CFormattingOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_FORMATTING };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;
   CString m_sLanguage;

   CComboBox m_ctrlIndentMode;
   CEdit m_ctrlTabWidth;
   CEdit m_ctrlIndentWidth;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CFormattingOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CFormattingOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CEditorsOptionsPage : public CPropertyPageImpl<CEditorsOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_EDITORS };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;

   CComboBox m_ctrlEOL;
   CEdit m_ctrlCaretWidth;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CEditorsOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CEditorsOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CPrintingOptionsPage : public CPropertyPageImpl<CPrintingOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_PRINTING };

   IDevEnv* m_pDevEnv;
   ISerializable* m_pArc;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CPrintingOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CPrintingOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // !defined(AFX_WIZARDSHEET_H__20030402_9A74_AF0C_98E1_0080AD509054__INCLUDED_)
