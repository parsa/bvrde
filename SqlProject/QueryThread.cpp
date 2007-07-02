
#include "StdAfx.h"
#include "resource.h"

#include "QueryThread.h"
#include "DbOperations.h"


///////////////////////////////////////////////////////7
//

CQueryThread::CQueryThread(CDbOperations* pDbData) :
  m_pDbData(pDbData),
  m_hWndNotify(NULL)
{
   m_event.Create();
   m_state = STATE_NOTHING;
   m_pDbCmd = NULL;
   m_bExecuting = false;
   m_nChunkSize = 50;     // Number of records in packet
}

DWORD CQueryThread::Run()
{
   ::SetThreadLocale(_pDevEnv->GetLCID());
   ::SetMessageQueue(100);  // HACK: For the sake of any OLE DB dependencies

   CCoInitialize cominit;

   COledbDatabase Db(&_DbSystem);
   COledbCommand* pDbCmd = NULL;
   COledbRecordset* pDbRec = NULL;

   while( true )
   {
      m_bExecuting = false;
      m_event.WaitForEvent();      
      
      STATE state = m_state;
      CString sSQL = (LPCTSTR) m_sSQL;   // BUG: Protect this statement with thread lock
      int lLineNum = m_lLineNum;         //      And this guy too!!

      m_state = STATE_NOTHING;

      if( state == STATE_CONNECT ) 
      {
         if( Db.IsOpen() ) continue;
         CComPtr<IDataInitialize> spData;
         spData.CoCreateInstance(CLSID_MSDAINITIALIZE);
         if( spData == NULL ) return FALSE;
         spData->GetDataSource(NULL, CLSCTX_INPROC_SERVER, CComBSTR(m_sConnectString), IID_IDBInitialize, (IUnknown**) &Db.m_spInit);
         Db.Connect();
      }
      else if( state == STATE_DISCONNECT )
      {
         break;
      }
      else if( state == STATE_EXECUTE )
      {
         sSQL.TrimLeft();
         if( sSQL.IsEmpty() ) continue;

         // Execute user query
         m_bExecuting = true;

         // Send start packet
         ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) new DATAPACKET(PACKET_START));

         CSimpleArray<SQLPART> aParts;
         if( !_SplitSQL(sSQL, lLineNum, aParts) ) continue;

         Db.GetErrors()->Clear();

         for( int x = 0; x < aParts.GetSize(); x++ ) 
         {            
            const SQLPART& part = aParts[x];

            // Clean up
            if( pDbCmd ) delete pDbCmd;
            if( pDbRec ) delete pDbRec;
            m_pDbCmd = NULL;

            int iAffected = 0;
            short iColumns = 0;

            // Create new command / recordset
            pDbCmd = new COledbCommand(&Db);
            pDbRec = new COledbRecordset(&Db);
            if( pDbCmd == NULL ) continue;
            if( pDbRec == NULL ) continue;
            BOOL bRes = pDbCmd->Create(part.sSQL);
            if( !bRes ) {
               DATAPACKET* pError = new DATAPACKET(PACKET_ERROR, part.lLineNum, 0, new CString[1]);
               pError->pstrData[0] = _GetDbErrorText(&Db, part.sSQL);
               ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) pError);
            }
            else 
            {
               m_pDbCmd = pDbCmd;

               // Limit Max records
               TCHAR szBuffer[32] = { 0 };
               _pDevEnv->GetProperty(_T("editors.sql.maxRecords"), szBuffer, 31);
               int iMaxRecords = _ttoi(szBuffer);
               if( iMaxRecords > 0 ) {
                  DBPROP Prop = { 0 };
                  ::VariantInit(&Prop.vValue);
                  Prop.dwOptions = DBPROPOPTIONS_REQUIRED;
                  Prop.colid = DB_NULLID;
                  Prop.dwStatus = 0;
                  Prop.dwPropertyID = DBPROP_MAXROWS;
                  Prop.vValue.vt = VT_I4;
                  Prop.vValue.iVal = iMaxRecords;
                  DBPROPSET PropSet = { 0 };
                  PropSet.guidPropertySet = DBPROPSET_ROWSET;
                  PropSet.cProperties = 1;
                  PropSet.rgProperties = &Prop;
                  CComQIPtr<ICommandProperties> spProperties = pDbCmd->m_spText;
                  spProperties->SetProperties(1, &PropSet);
               }

               // Execute
               bRes = _Execute(pDbCmd, pDbRec);
               if( !bRes ) {
                  DATAPACKET* pError = new DATAPACKET(PACKET_ERROR, part.lLineNum, 0, new CString[1]);
                  pError->pstrData[0] = _GetDbErrorText(&Db, part.sSQL);
                  ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) pError);
               }
               else 
               {
                  iColumns = pDbRec->GetColumnCount();
                  iAffected = pDbCmd->GetRowCount();
                  SHORT* aColumnType = new SHORT[ iColumns ];

                  // Post information about columns
                  DATAPACKET* pColInfo = new DATAPACKET(PACKET_COLUMNINFO, iColumns, 0, new CString[ iColumns ], new LONG[ iColumns ]);
                  for( short i = 0; i < iColumns; i++ ) {
                     TCHAR szName[100] = { 0 };
                     pDbRec->GetColumnName(i, szName, sizeof(szName)/sizeof(TCHAR)-1);
                     pColInfo->pstrData[i] = szName;
                     pColInfo->plData[i] = pDbRec->GetColumnSize(i);
                     aColumnType[i] = pDbRec->GetColumnType(i);
                  }
                  ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) pColInfo);

                  // Collect records
                  DATAPACKET* pRowInfo = new DATAPACKET(PACKET_ROWINFO, iColumns, 0, new CString[ m_nChunkSize * iColumns ]);
               
                  int iPos = 0;
                  int nTotal = 0;
                  CString sValue;
                  CComVariant vValue;
                  TCHAR szValue[64] = { 0 };
                  CString sTrue(MAKEINTRESOURCE(IDS_TRUE));
                  CString sFalse(MAKEINTRESOURCE(IDS_FALSE));
               
                  while( !pDbRec->IsEOF() ) 
                  {
                     for( int i = 0; i < iColumns; i++ ) {
                        // Extract field value
                        sValue = _T("");
                        switch( aColumnType[i] ) {
                        case DB_TYPE_UNKNOWN:
                           {
                              sValue = _T("....");
                           }
                           break;
                        case DB_TYPE_CHAR:
                           {
                              pDbRec->GetField(i, sValue);
                           }
                           break;
                        case DB_TYPE_INTEGER:
                           {
                              long lValue = 0;
                              pDbRec->GetField(i, lValue);
                              sValue.Append(lValue);
                           }
                           break;
                        case DB_TYPE_REAL:
                           {
                              float fValue = 0.0f;
                              pDbRec->GetField(i, fValue);
                              vValue = fValue;
                              vValue.ChangeType(VT_BSTR);
                              sValue = vValue.bstrVal;
                           }
                           break;
                        case DB_TYPE_DOUBLE:
                           {
                              double dblValue = 0.0;
                              pDbRec->GetField(i, dblValue);
                              vValue = dblValue;
                              vValue.ChangeType(VT_BSTR);
                              sValue = vValue.bstrVal;
                           }
                           break;
                        case DB_TYPE_DATE:
                           {
                              SYSTEMTIME stDate = { 0 };
                              pDbRec->GetField(i, stDate);
                              szValue[0] = '\0';
                              ::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stDate, NULL, szValue, (sizeof(szValue) / sizeof(TCHAR)) - 1);
                              sValue = szValue;
                           }
                           break;
                        case DB_TYPE_TIME:
                           {
                              SYSTEMTIME stDate = { 0 };
                              pDbRec->GetField(i, stDate);
                              szValue[0] = '\0';
                              ::GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stDate, NULL, szValue, (sizeof(szValue) / sizeof(TCHAR)) - 1);
                              sValue = szValue;
                           }
                           break;
                        case DB_TYPE_TIMESTAMP:
                           {
                              SYSTEMTIME stDate = { 0 };
                              pDbRec->GetField(i, stDate);
                              szValue[0] = '\0';
                              ::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stDate, NULL, szValue, (sizeof(szValue) / sizeof(TCHAR)) - 1);
                              sValue = szValue;
                              ::GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stDate, NULL, szValue, (sizeof(szValue) / sizeof(TCHAR)) - 1);
                              sValue += _T(" ");
                              sValue += szValue;
                           }
                           break;
                        case DB_TYPE_BOOLEAN:
                           {
                              bool bValue = false;
                              pDbRec->GetField(i, bValue);
                              sValue = bValue ? sTrue : sFalse;
                           }
                           break;
                        }
                        pRowInfo->pstrData[iPos++] = sValue;
                     }
                     pRowInfo->iRows++;
                     if( pRowInfo->iRows >= m_nChunkSize )
                     {
                        // Post row data
                        ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) pRowInfo);
                        // Throttle
                        ::Sleep(nTotal > 4000 ? 150L : 0L);
                        ::WaitForInputIdle(m_hWndNotify, 200L);
                        // Ready with new packet
                        pRowInfo = new DATAPACKET(PACKET_ROWINFO, iColumns, 0, new CString[ m_nChunkSize * iColumns ]);
                        nTotal += iPos;
                        iPos = 0;
                     }
                     if( m_state == STATE_DISCONNECT ) break;
                     if( m_state == STATE_ABORT ) break;
                     pDbRec->MoveNext();
                  }
                  ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) pRowInfo);
                  pDbRec->Close();
                  delete [] aColumnType;

                  // Send finish packet
                  ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) new DATAPACKET(PACKET_FINISH, iColumns, iAffected));
               }
               m_pDbCmd->Close();
            }
         }

         // Send done packet
         ::PostMessage(m_hWndNotify, WM_USER_DATA_AVAILABLE, 0, (LPARAM) new DATAPACKET(PACKET_DONE));

         // Finally play stupid sound
         ::PlaySound(_T("BVRDE_SqlComplete"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
      }
      else if( state == STATE_INFOREQUEST ) 
      {
         // Slowly populate table/column list
         TABLEINFO* pTable = m_pDbData->EstimateNextInfoRequest();
         if( pTable ) m_pDbData->LoadTableInfo(pTable, &Db);
      }
   }

   if( pDbCmd ) delete pDbCmd;
   if( pDbRec ) delete pDbRec;
   m_pDbCmd = NULL;
   Db.Close();

   return 0;
}

BOOL CQueryThread::_Execute(CDbCommand* pCmd, CDbRecordset* pRec)
{
   // Protect application against faulty OLE DB providers!
   ATLASSERT(pCmd);
   ATLASSERT(pRec);
   __try
   {
      return pCmd->Execute(pRec);
   }
   __except(1)
   {
      // Don't be bothered with gpf's
      return FALSE;
   }
}

void CQueryThread::SetNotify(HWND hWnd)
{
   ATLASSERT(::IsWindow(hWnd));
   m_hWndNotify = hWnd;
}

void CQueryThread::Connect(LPCTSTR pstrConnectString)
{
   m_sConnectString = pstrConnectString;
   m_state = STATE_CONNECT;
   m_event.SetEvent();
}

void CQueryThread::Disconnect()
{
   m_state = STATE_DISCONNECT;
   m_event.SetEvent();
}

void CQueryThread::Abort()
{
   m_state = STATE_ABORT;
   if( m_pDbCmd ) m_pDbCmd->Cancel();
}

void CQueryThread::ExecuteSql(LPCTSTR pstrSql, int lLineNum)
{
   m_sSQL = pstrSql;
   m_lLineNum = lLineNum;
   m_state = STATE_EXECUTE;
   m_event.SetEvent();
}

void CQueryThread::MarkTable(LPCTSTR pstrName)
{
   ATLASSERT(m_pDbData);
   m_pDbData->MarkTable(pstrName);
}

void CQueryThread::InfoRequest()
{
   // Used for background updating of column-information
   // NOTE: We won't bother to access the database as long as it's
   //       busy doing anything else.
   if( IsBusy() ) return;
   if( !IsRunning() ) return;
   m_state = STATE_INFOREQUEST;
   m_event.SetEvent();
}

bool CQueryThread::IsBusy() const
{
   return !m_bExecuting && m_state != STATE_NOTHING;
}

void CQueryThread::Cancel()
{
   if( m_pDbCmd && m_pDbCmd->IsOpen() ) m_pDbCmd->Cancel();
}

BOOL CQueryThread::_SplitSQL(const CString& sSQL, int lLineNum, CSimpleArray<SQLPART>& aParts)
{
   // Many DBRMS do not support multiple SQL statements in one query, so we'll
   // manually split up the statements and execute them one by one.
   // TODO: This could break the logic in DBRMS that actually support mulitple
   //       SQL statements (MS SQL Server?), so this should be an option.
   TCHAR szTerminator[8] = { 0 };
   _pDevEnv->GetProperty(_T("editors.sql.terminator"), szTerminator, 7);
   if( _tcslen(szTerminator) == 0 ) return FALSE;
   TCHAR szBuffer[64] = { 0 };
   _pDevEnv->GetProperty(_T("editors.sql.stripComments"), szBuffer, 63);
   bool bStripComments = _tcscmp(szBuffer, _T("true")) == 0;

   // Single-char terminators are just that.
   // Multiple-char terminators need to be on separate line.
   CString sTerminator = szTerminator;
   if( sTerminator.GetLength() > 1 ) sTerminator.Format(_T("\n%s"), szTerminator);
   int iTermLen = sTerminator.GetLength();

   // Break the SQL statements apart.
   // Keep track of source line-no since we might want to link back to
   // the original SQL text.
   CString sPart;
   LPCTSTR p = sSQL;
   int iStartLineNo = 0;
   bool bInsideQuote = false;
   while( *p ) {
      // Strip comments?
      if( !bInsideQuote && bStripComments ) {
         if( _tcsncmp(p, _T("--"), 2) == 0 ) {
            while( *p && *p != '\n' ) p++;
         }
         if( _tcsncmp(p, _T("/*"), 2) == 0 ) {
            while( *p ) {
               if( *p == '*' && *(p + 1) == '/' ) {
                  p += 2;
                  break;
               }
               if( *p == '\n' ) lLineNum++;
               p++;
            }
         }
      }
      // Is terminator?
      if( !bInsideQuote && _tcsnicmp(p, sTerminator, iTermLen) == 0 ) {
         sPart.TrimLeft();
         sPart.TrimRight();
         if( !sPart.IsEmpty() ) {
            SQLPART part;
            part.sSQL = sPart;
            part.lLineNum = iStartLineNo;
            aParts.Add(part);
         }
         sPart = "";
         p += iTermLen;
         if( iTermLen > 1 ) lLineNum++;
         iStartLineNo = 0;
      }
      // Update state
      switch( *p ) {
      case '\0':
         break;
      case '\r':
      case '\t':
         p++;
         break;
      case '\'':
         bInsideQuote = !bInsideQuote;
         sPart += *p++;
         break;
      case '\n':
         lLineNum++;
         bInsideQuote = false;
         sPart += *p++;
         break;
      default:
         if( iStartLineNo == 0 ) iStartLineNo = lLineNum;
         sPart += *p++;
      }
   }
   sPart.TrimLeft();
   sPart.TrimRight();
   if( !sPart.IsEmpty() ) {
      SQLPART part;
      part.sSQL = sPart;
      part.lLineNum = iStartLineNo;
      aParts.Add(part);
   }

   return TRUE;
}

CString CQueryThread::_GetDbErrorText(COledbDatabase* pDb, LPCTSTR pstrSQL) const
{
   CString sSQL = pstrSQL;
   int iPos = sSQL.FindOneOf(_T("\r\n"));
   if( iPos >= 0 ) sSQL = sSQL.Left(iPos) + _T("...");
   if( sSQL.GetLength() > 40 ) sSQL = sSQL.Left(40) + _T("...");
   sSQL.TrimLeft();
   sSQL.TrimRight();
   CString sText;
   for( int i = 0; i < pDb->GetErrors()->GetCount(); i++ ) {
      CDbError* pErr = pDb->GetErrors()->GetError(i);
      long lCode = pErr->GetErrorCode();
      TCHAR szSource[128] = { 0 };
      TCHAR szMessage[512] = { 0 };
      pErr->GetSource(szSource, 127);
      pErr->GetMessage(szMessage, 511);
      CString sTemp;
      sTemp.Format(IDS_DBERROR_FORMAT, sSQL, lCode, lCode, szSource, szMessage);
      sText += sTemp;
   }
   return sText;
}
