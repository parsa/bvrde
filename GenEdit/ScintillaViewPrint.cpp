
#include "StdAfx.h"
#include "resource.h"

#include "ScintillaView.h"
#include "SciLexer.h"

#pragma code_seg( "EDITOR" )


LRESULT CScintillaView::OnFilePrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   bool bWordWrap = _GetProperty(_T("priting.general.wordWrap")) == _T("true");
   SetPrintWrapMode(bWordWrap ? SC_WRAP_WORD : SC_WRAP_NONE);

   bool bColorPrint = _GetProperty(_T("priting.general.colors")) == _T("true");
   SetPrintColourMode(bColorPrint ? SC_PRINT_COLOURONWHITE : SC_PRINT_BLACKONWHITE);

   CString sWindowName(MAKEINTRESOURCE(IDS_PRINT_TITLE));

   bool showDialog = true;

   HGLOBAL hDevMode = NULL;
   HGLOBAL hDevNames = NULL;
   RECT rcMargins = { 0 };
   m_pDevEnv->GetPrinterInfo(hDevMode, hDevNames, rcMargins);

   //
   // The following code is pretty much ripped 
   // from original Scintilla SciTech sample.
   //

   PRINTDLG pdlg = 
   {
      sizeof(PRINTDLG), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
   };
   pdlg.hwndOwner = m_pDevEnv->GetHwnd(IDE_HWND_MAIN);
   pdlg.hInstance = _Module.GetModuleInstance();
   pdlg.Flags = PD_USEDEVMODECOPIES | PD_ALLPAGES | PD_RETURNDC;
   pdlg.nFromPage = 1;
   pdlg.nToPage = 1;
   pdlg.nCopies = 1;
   pdlg.nMinPage = 1;
   pdlg.nMaxPage = 0xffffU; // We do not know how many pages in the document until 
                            // the printer is selected and the paper size is known.
   pdlg.hDC = NULL;
   pdlg.hDevMode = hDevMode;
   pdlg.hDevNames = hDevNames;

   // See if a range has been selected
   CharacterRange crange = GetSelection();
   int startPos = crange.cpMin;
   int endPos = crange.cpMax;

   if( startPos == endPos ) {
      pdlg.Flags |= PD_NOSELECTION;
   } 
   else {
      pdlg.Flags |= PD_SELECTION;
   }
   // Don't display dialog box, just use the default printer and options
   if( !showDialog ) pdlg.Flags |= PD_RETURNDEFAULT;

   // Display Print dialog
   if( !::PrintDlg(&pdlg) ) return 0;

   UpdateWindow();

   CWaitCursor cursor;
   HDC hdc = pdlg.hDC;

   RECT rectMargins, rectPhysMargins;
   POINT ptPage;
   POINT ptDpi;

   // Get printer resolution
   ptDpi.x = ::GetDeviceCaps(hdc, LOGPIXELSX);    // dpi in X direction
   ptDpi.y = ::GetDeviceCaps(hdc, LOGPIXELSY);    // dpi in Y direction

   // Start by getting the physical page size (in device units).
   ptPage.x = ::GetDeviceCaps(hdc, PHYSICALWIDTH);   // device units
   ptPage.y = ::GetDeviceCaps(hdc, PHYSICALHEIGHT);  // device units

   // Get the dimensions of the unprintable
   // part of the page (in device units).
   rectPhysMargins.left = ::GetDeviceCaps(hdc, PHYSICALOFFSETX);
   rectPhysMargins.top = ::GetDeviceCaps(hdc, PHYSICALOFFSETY);

   // To get the right and lower unprintable area,
   // we take the entire width and height of the paper and
   // subtract everything else.
   rectPhysMargins.right = ptPage.x                      // total paper width
                           - GetDeviceCaps(hdc, HORZRES) // printable width
                           - rectPhysMargins.left;       // left unprintable margin

   rectPhysMargins.bottom = ptPage.y                      // total paper height
                            - GetDeviceCaps(hdc, VERTRES) // printable height
                            - rectPhysMargins.top;        // right unprintable margin

   // At this point, rectPhysMargins contains the widths of the
   // unprintable regions on all four sides of the page in device units.

   // Take in account the page setup given by the user (if one value is not null)
   if( rcMargins.left != 0 || 
       rcMargins.right != 0 ||
       rcMargins.top != 0 || 
       rcMargins.bottom != 0 ) 
   {
      RECT rectSetup;

      // Convert the hundredths of millimeters (HiMetric) or
      // thousandths of inches (HiEnglish) margin values
      // from the Page Setup dialog to device units.
      // (There are 2540 hundredths of a mm in an inch.)

      char localeInfo[3];
      ::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);

      if( localeInfo[0] == '0' ) {   // Metric system. '1' is US System
         rectSetup.left = ::MulDiv(rcMargins.left, ptDpi.x, 2540);
         rectSetup.top = ::MulDiv(rcMargins.top, ptDpi.y, 2540);
         rectSetup.right   = ::MulDiv(rcMargins.right, ptDpi.x, 2540);
         rectSetup.bottom   = ::MulDiv(rcMargins.bottom, ptDpi.y, 2540);
      } 
      else {
         rectSetup.left   = ::MulDiv(rcMargins.left, ptDpi.x, 1000);
         rectSetup.top   = ::MulDiv(rcMargins.top, ptDpi.y, 1000);
         rectSetup.right   = ::MulDiv(rcMargins.right, ptDpi.x, 1000);
         rectSetup.bottom   = ::MulDiv(rcMargins.bottom, ptDpi.y, 1000);
      }

      // Dont reduce margins below the minimum printable area
      rectMargins.left   = max(rectPhysMargins.left, rectSetup.left);
      rectMargins.top   = max(rectPhysMargins.top, rectSetup.top);
      rectMargins.right   = max(rectPhysMargins.right, rectSetup.right);
      rectMargins.bottom   = max(rectPhysMargins.bottom, rectSetup.bottom);
   } 
   else {
      rectMargins.left   = rectPhysMargins.left;
      rectMargins.top   = rectPhysMargins.top;
      rectMargins.right   = rectPhysMargins.right;
      rectMargins.bottom   = rectPhysMargins.bottom;
   }

   // rectMargins now contains the values used to shrink the printable
   // area of the page.

   // Convert device coordinates into logical coordinates
   ::DPtoLP(hdc, (LPPOINT) &rectMargins, 2);
   ::DPtoLP(hdc, (LPPOINT) &rectPhysMargins, 2);

   // Convert page size to logical units and we're done!
   ::DPtoLP(hdc, (LPPOINT) &ptPage, 1);

   CString headerFormat = _T("");
   CString footerFormat = _T("");

   int headerLineHeight = ::MulDiv(9, ptDpi.y, 72);
   HFONT fontHeader = ::CreateFont(headerLineHeight,
                                   0, 0, 0,
                                   FW_BOLD,
                                   0, // italics,
                                   0, // underlined,
                                   0, 0, 0,
                                   0, 0, 0,
                                   _T("Arial"));
   ::SelectObject(hdc, fontHeader);
   TEXTMETRIC tm;
   ::GetTextMetrics(hdc, &tm);
   headerLineHeight = tm.tmHeight + tm.tmExternalLeading;

   int footerLineHeight = ::MulDiv(9, ptDpi.y, 72);
   HFONT fontFooter = ::CreateFont(footerLineHeight,
                                   0, 0, 0,
                                   FW_NORMAL,
                                   0, // italics,
                                   0, // underlined,
                                   0, 0, 0,
                                   0, 0, 0,
                                   _T("Arial"));
   ::SelectObject(hdc, fontFooter);
   ::GetTextMetrics(hdc, &tm);
   footerLineHeight = tm.tmHeight + tm.tmExternalLeading;

   DOCINFO di = { sizeof(DOCINFO), 0, 0, 0, 0 };
   di.lpszDocName = sWindowName;
   di.lpszOutput = 0;
   di.lpszDatatype = 0;
   di.fwType = 0;
   if( ::StartDoc(hdc, &di) < 0 ) {
      CString msg(MAKEINTRESOURCE(IDS_ERR_DOCUMENT));
      m_pDevEnv->ShowMessageBox(NULL, msg, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_OK | MB_ICONEXCLAMATION);
      return 0;
   }

   LONG lengthDoc = GetLength();
   LONG lengthDocMax = lengthDoc;
   LONG lengthPrinted = 0;

   // Requested to print selection
   if( pdlg.Flags & PD_SELECTION ) {
      if( startPos > endPos ) {
         lengthPrinted = endPos;
         lengthDoc = startPos;
      } 
      else {
         lengthPrinted = startPos;
         lengthDoc = endPos;
      }

      if( lengthPrinted < 0 ) lengthPrinted = 0;
      if( lengthDoc > lengthDocMax ) lengthDoc = lengthDocMax;
   }

   // We must substract the physical margins from the printable area
   FORMATRANGE frPrint;
   frPrint.hdc = hdc;
   frPrint.hdcTarget = hdc;
   frPrint.rc.left = rectMargins.left - rectPhysMargins.left;
   frPrint.rc.top = rectMargins.top - rectPhysMargins.top;
   frPrint.rc.right = ptPage.x - rectMargins.right - rectPhysMargins.left;
   frPrint.rc.bottom = ptPage.y - rectMargins.bottom - rectPhysMargins.top;
   frPrint.rcPage.left = 0;
   frPrint.rcPage.top = 0;
   frPrint.rcPage.right = ptPage.x - rectPhysMargins.left - rectPhysMargins.right - 1;
   frPrint.rcPage.bottom = ptPage.y - rectPhysMargins.top - rectPhysMargins.bottom - 1;
   if( headerFormat.GetLength() > 0 ) {
      frPrint.rc.top += headerLineHeight + headerLineHeight / 2;
   }
   if( footerFormat.GetLength() > 0 ) {
      frPrint.rc.bottom -= footerLineHeight + footerLineHeight / 2;
   }
   // Print each page
   int pageNum = 1;
   bool printPage;
   
   while( lengthPrinted < lengthDoc ) 
   {
      printPage = (!(pdlg.Flags & PD_PAGENUMS) ||
                   (pageNum >= pdlg.nFromPage) && (pageNum <= pdlg.nToPage));

      CString sStatus;
      sStatus.Format(IDS_STATUS_PRINTING, (long) pageNum);
      m_pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus);

      if( printPage ) {
         ::StartPage(hdc);

         if( headerFormat.GetLength() > 0 ) {
            CString sHeader = _T("");
            ::SetTextColor(hdc, RGB(0,0,0));
            ::SetBkColor(hdc, RGB(255,255,255));
            ::SelectObject(hdc, fontHeader);
            UINT ta = ::SetTextAlign(hdc, TA_BOTTOM);
            RECT rcw = {frPrint.rc.left, frPrint.rc.top - headerLineHeight - headerLineHeight / 2,
                        frPrint.rc.right, frPrint.rc.top - headerLineHeight / 2};
            rcw.bottom = rcw.top + headerLineHeight;
            ::ExtTextOut(hdc, frPrint.rc.left + 5, frPrint.rc.top - headerLineHeight / 2,
                         ETO_OPAQUE, &rcw, sHeader,
                         sHeader.GetLength(), NULL);
            ::SetTextAlign(hdc, ta);
            HPEN pen = ::CreatePen(0, 1, RGB(0,0,0));
            HPEN penOld = (HPEN) ::SelectObject(hdc, pen);
            ::MoveToEx(hdc, frPrint.rc.left, frPrint.rc.top - headerLineHeight / 4, NULL);
            ::LineTo(hdc, frPrint.rc.right, frPrint.rc.top - headerLineHeight / 4);
            ::SelectObject(hdc, penOld);
            ::DeleteObject(pen);
         }
      }

      frPrint.chrg.cpMin = lengthPrinted;
      frPrint.chrg.cpMax = lengthDoc;

      lengthPrinted = FormatRange(printPage, reinterpret_cast<LPARAM>(&frPrint));

      if( printPage ) {
         if( footerFormat.GetLength() > 0 ) {
            CString sFooter = _T("");
            ::SetTextColor(hdc, RGB(80,80,80));
            ::SetBkColor(hdc, RGB(255,255,255));
            ::SelectObject(hdc, fontFooter);
            UINT ta = ::SetTextAlign(hdc, TA_TOP);
            RECT rcw = {frPrint.rc.left, frPrint.rc.bottom + footerLineHeight / 2,
                        frPrint.rc.right, frPrint.rc.bottom + footerLineHeight + footerLineHeight / 2};
            ::ExtTextOut(hdc, frPrint.rc.left + 5, frPrint.rc.bottom + footerLineHeight / 2,
                         ETO_OPAQUE, &rcw, sFooter,
                         sFooter.GetLength(), NULL);
            ::SetTextAlign(hdc, ta);
            HPEN pen = ::CreatePen(0, 1, RGB(80,80,80));
            HPEN penOld = (HPEN) ::SelectObject(hdc, pen);
            ::SetBkColor(hdc, RGB(80,80,80));
            ::MoveToEx(hdc, frPrint.rc.left, frPrint.rc.bottom + footerLineHeight / 4, NULL);
            ::LineTo(hdc, frPrint.rc.right, frPrint.rc.bottom + footerLineHeight / 4);
            ::SelectObject(hdc, penOld);
            ::DeleteObject(pen);
         }

         ::EndPage(hdc);
      }
      pageNum++;

      if( (pdlg.Flags & PD_PAGENUMS) && (pageNum > pdlg.nToPage) ) break;
   }

   FormatRange(FALSE, 0);

   ::EndDoc(hdc);
   ::DeleteDC(hdc);
   if( pdlg.hDevMode != hDevMode ) ::GlobalFree(pdlg.hDevMode); 
   if( pdlg.hDevNames != hDevNames ) ::GlobalFree(pdlg.hDevNames); 
   if( fontHeader ) ::DeleteObject(fontHeader);
   if( fontFooter ) ::DeleteObject(fontFooter);
   return 0;
}
