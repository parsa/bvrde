
#include "StdAfx.h"
#include "resource.h"

#include "DbOperations.h"


CDbOperations::CDbOperations() :
   m_Db(&_DbSystem) 
{
}

BOOL CDbOperations::ConnectDatabase(COledbDatabase* pDb, LPCTSTR pstrConnectString)
{
   // If not already connected, try now...
   if( !m_Db.IsOpen() ) 
   {
      CComPtr<IDataInitialize> spData;
      spData.CoCreateInstance(CLSID_MSDAINITIALIZE);
      if( spData == NULL ) return FALSE;
      spData->GetDataSource(NULL, CLSCTX_INPROC_SERVER, CComBSTR(pstrConnectString), IID_IDBInitialize, (IUnknown**) &m_Db.m_spInit);
      m_Db.Connect();
   }
   // Didn't succeed? Put up error message.
   if( !m_Db.IsOpen() )
   {     
      CString sError;
      if( m_Db.GetErrors()->GetCount() > 0 ) {
         m_Db.GetErrors()->GetError(0)->GetMessage(sError.GetBufferSetLength(400), 400);
         sError.ReleaseBuffer();
      }
      if( sError.GetLength() == 0 ) sError.LoadString(IDS_STDERROR);
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
      CString sMsg;
      sMsg.Format(IDS_ERR_NOCONNECTION, sError);
      _pDevEnv->ShowMessageBox(NULL, sMsg, sCaption, MB_ICONEXCLAMATION);
      m_Db.Close();
      return FALSE;
   }
   return TRUE;
}

BOOL CDbOperations::EnumTables(COledbDatabase* pDb /*=NULL*/)
{
   // Make sure database is connected
   if( pDb == NULL ) pDb = &m_Db;
   if( !pDb->IsOpen() ) return FALSE;
   // Don't collect tables if we've already done this once
   if( m_aTables.GetSize() > 0 ) return TRUE;
   // Lock data
   CLockStaticDataInit lock;
   // Get terms
   CString sTableTerm = GetPropertyStr(DBPROPSET_DATASOURCEINFO, DBPROP_TABLETERM);
   // Load table list
   CComQIPtr<IDBSchemaRowset> spSchema = pDb->m_spSession;
   if( spSchema == NULL ) return FALSE;
   CComPtr<IRowset> spRS;
   HRESULT Hr = spSchema->GetRowset(NULL, DBSCHEMA_TABLES, 0, NULL, IID_IRowset, 0, NULL, (LPUNKNOWN*) &spRS);
   if( FAILED(Hr) ) return FALSE;
   COledbRecordset rec = spRS;  
   while( !rec.IsEOF() ) {
      TABLEINFO ti;
      rec.GetField(0, ti.sSchema);
      rec.GetField(1, ti.sCatalog);
      rec.GetField(2, ti.sName);
      rec.GetField(3, ti.sType);

      ti.bFullInfo = false;
      ti.dwTouched = 0;
      ti.ItemType = DBTYPE_TABLE;
      if( ti.sType == _T("VIEW") ) ti.ItemType = DBTYPE_VIEW;
      if( ti.sType == _T("SYNONYM") ) ti.ItemType = DBTYPE_VIEW;
      if( ti.sType.Find(_T("SYSTEM")) >= 0) ti.ItemType = DBTYPE_SYSTEMTABLE;
      if( ti.sName.Find(_T("MSys")) == 0) ti.ItemType = DBTYPE_SYSTEMTABLE;
      if( ti.sName.Find(_T("SYS")) == 0) ti.ItemType = DBTYPE_SYSTEMTABLE;
      if( ti.sType == sTableTerm ) ti.ItemType = DBTYPE_TABLE;
      m_aTables.Add(ti);

      rec.MoveNext();
   }
   rec.Close();
 
   return TRUE;
}

BOOL CDbOperations::LoadTableInfo(TABLEINFO* pTable, COledbDatabase* pDb /*=NULL*/)
{
   ATLASSERT(pTable);

   // Lock data
   CLockStaticDataInit lock;

   // Make sure database is connected
   if( pDb == NULL ) pDb = &m_Db;
   if( !pDb->IsOpen() ) return FALSE;
   if( pTable->aFields.GetSize() > 0 ) return TRUE;

   pTable->bFullInfo = true;

   // Does database support schemas?
   CComQIPtr<IDBSchemaRowset> spSchema = pDb->m_spSession;
   if( spSchema == NULL ) return FALSE;
   // Load columns
   CComVariant vColumnRestrictions[3];
   vColumnRestrictions[2] = pTable->sName;
   CComPtr<IRowset> spRS;
   HRESULT Hr = spSchema->GetRowset(NULL, DBSCHEMA_COLUMNS, 3, (VARIANT*) &vColumnRestrictions, IID_IRowset, 0, NULL, (LPUNKNOWN*) &spRS);
   if( FAILED(Hr) ) return FALSE;
   COledbRecordset rec = spRS;
   while( !rec.IsEOF() ) {
      FIELDINFO fi;
      fi.ItemType = DBTYPE_FIELD;
      rec.GetField(3, fi.sName);
      rec.GetField(6, fi.lPosition);
      rec.GetField(7, fi.bHasDefault);
      rec.GetField(8, fi.sDefault);
      rec.GetField(9, fi.lFlags);
      rec.GetField(10, fi.bNullable);
      rec.GetField(11, fi.lType);
      rec.GetField(13, fi.lLength);
      rec.GetField(15, fi.lPrecision);
      rec.GetField(16, fi.lDigits);
      pTable->aFields.Add(fi);
      rec.MoveNext();
   }
   rec.Close();
   spRS.Release();

   // Load Index infomation
   CComVariant vIndexRestrictions[5];
   vIndexRestrictions[4] = pTable->sName;
   Hr = spSchema->GetRowset(NULL, DBSCHEMA_INDEXES , 5, (VARIANT*) &vIndexRestrictions, IID_IRowset, 0, NULL, (LPUNKNOWN*) &spRS);
   if( SUCCEEDED(Hr) ) {
      COledbRecordset rec = spRS;
      while( !rec.IsEOF() ) {
         INDEXINFO ii;
         ii.ItemType = DBTYPE_INDEX;
         ii.nFields = 1;
         CString sField;
         long lNulls = 0;
         long lType = 0;
         rec.GetField(5, ii.sName);
         rec.GetField(6, ii.bPrimary);
         rec.GetField(7, ii.bUnique);
         rec.GetField(8, ii.bClustered);
         rec.GetField(9, lType);
         rec.GetField(12, lNulls);
         rec.GetField(16, ii.lPosition);
         rec.GetField(17, ii.sFields[0]);
         ii.sType.LoadString(IDS_STANDARD);
         if( lType == DBPROPVAL_IT_BTREE ) ii.sType = _T("B-tree");
         if( lType == DBPROPVAL_IT_HASH ) ii.sType = _T("Hash");
         if( lType == DBPROPVAL_IT_CONTENT ) ii.sType = _T("Content");
         ii.bNulls = lNulls != DBPROPVAL_IN_ALLOWNULL;
         bool bFound = false;
         for( int i = 0; i < pTable->aIndices.GetSize(); i++ ) {
            if( pTable->aIndices[i].sName == ii.sName ) {
               pTable->aIndices[i].sFields[ pTable->aIndices[i].nFields ] = ii.sFields[0];
               pTable->aIndices[i].nFields++;
               bFound = true;
               break;
            }
         }
         if( !bFound ) pTable->aIndices.Add(ii);
         rec.MoveNext();
      }
      rec.Close();
      spRS.Release();
   }
 
   return TRUE;
}

BOOL CDbOperations::GetDatabaseInfo(DATABASEINFO& info)
{
   // Make sure database is connected
   if( !m_Db.IsOpen() ) return FALSE;
   return TRUE;
}

TABLEINFO* CDbOperations::EstimateNextInfoRequest()
{
   // Make sure all tables are known
   if( !EnumTables() ) return NULL;
   // Let's find the next table to investigate
   CLockStaticDataInit lock;
   TABLEINFO* pTable = NULL;
   DWORD dwTimestamp = 0;
   for( int i = 0; i < m_aTables.GetSize(); i++ ) {
      // Find a table without table-information
      // Find a good match by looking at timestamp first
      TABLEINFO& Table = m_aTables[i];
      if( !Table.bFullInfo 
          && Table.dwTouched > dwTimestamp 
          && Table.ItemType != DBTYPE_SYSTEMTABLE ) 
      {
         pTable = &m_aTables[i];
         dwTimestamp = Table.dwTouched;
      }
   }
   return pTable;
}

void CDbOperations::MarkTable(LPCTSTR pstrName)
{
   // Mark table with timestamp. The background
   // update-thread will eventually visit this table and grab
   // column/index information.
   CLockStaticDataInit lock;
   for( int i = 0; i < m_aTables.GetSize(); i++ ) {
      if( m_aTables[i].sName == pstrName ) m_aTables[i].dwTouched = ::GetTickCount();
   }
}

BOOL CDbOperations::GetOwnerList(CSimpleArray<CString>& aList) const
{
   // Return list of schemas/owners
   CLockStaticDataInit lock;
   for( int i = 0; i < m_aTables.GetSize(); i++ ) {
      CString sOwner = m_aTables[i].sCatalog;
      if( sOwner.IsEmpty() ) sOwner = m_aTables[i].sSchema;
      if( sOwner.IsEmpty() ) continue;
      bool bFound = false;
      for( int j = 0; !bFound && j < aList.GetSize(); j++ ) bFound = aList[j] == sOwner;
      if( !bFound ) aList.Add(sOwner);
   }
   return TRUE;
}

BOOL CDbOperations::GetTableList(CSimpleArray<CString>& aList, bool bAllEntries /*= true*/) const
{
   // Return list of known tables
   CLockStaticDataInit lock;
   for( int i = 0; i < m_aTables.GetSize(); i++ ) {
      const TABLEINFO& Table = m_aTables[i];
      if( Table.ItemType != DBTYPE_SYSTEMTABLE
          && Table.sName.Find(' ') < 0 ) 
      {
         if( !bAllEntries && Table.ItemType != DBTYPE_TABLE ) continue;
         CString sName = Table.sName;
         aList.Add(sName);
      }
   }
   return TRUE;
}

BOOL CDbOperations::GetColumnList(LPCTSTR pstrTable, CSimpleArray<CString>& aList)
{
   // Return list of columns in a specfic table
   CLockStaticDataInit lock;
   for( int i = 0; i < m_aTables.GetSize(); i++ ) {
      if( m_aTables[i].sName.CompareNoCase(pstrTable) == 0 ) {
         TABLEINFO& Table = m_aTables[i];
         // Load information off database now if not there already
         if( !LoadTableInfo(&Table) ) return FALSE;
         // Add columns to result
         for( int j = 0; j < Table.aFields.GetSize(); j++ ) {
            aList.Add(Table.aFields[j].sName);
         }
         return TRUE;
      }
   }
   return FALSE;
}

CString CDbOperations::ChangeProperties(HWND hWnd, LPCTSTR pstrConnectString)
{
   // Default back to old connection string
   CString sRes = pstrConnectString;
   // Close any existing database
   m_Db.Close();
   // Initialize (but do not connect) the database once again
   CComPtr<IDataInitialize> spData;
   if( SUCCEEDED( spData.CoCreateInstance(CLSID_MSDAINITIALIZE) ) ) {
      CComPtr<IDBInitialize> spInit;
      spData->GetDataSource(NULL, CLSCTX_INPROC_SERVER, CComBSTR(pstrConnectString), IID_IDBInitialize, (IUnknown**) &spInit);
      // Modify properties using prompt-dialog
      CComPtr<IDBPromptInitialize> spPrompt;
      if( SUCCEEDED( spPrompt.CoCreateInstance(CLSID_DataLinks) ) ) {
         CComQIPtr<IDBProperties> spDbp = spInit;
         IUnknown* pUnk = spDbp;
         HRESULT Hr = spPrompt->PromptDataSource(NULL, 
            hWnd, 
            DBPROMPTOPTIONS_DISABLE_PROVIDER_SELECTION | DBPROMPTOPTIONS_PROPERTYSHEET,
            0, NULL,
            NULL,
            IID_IDBProperties,
            (LPUNKNOWN*) &pUnk);
         if( Hr == S_OK ) 
         {     
            // Extract data directly from provider
            CComPtr<IDataInitialize> spData;
            if( SUCCEEDED( spData.CoCreateInstance(CLSID_MSDAINITIALIZE) ) ) {
               LPOLESTR pwstr = NULL;
               spData->GetInitializationString(spInit, VARIANT_TRUE, &pwstr);
               if( pwstr ) {
                  sRes = pwstr;
                  ::CoTaskMemFree(pwstr);
               }
            }
         }
      }
   }
   // Reconnect with new properties
   ConnectDatabase(&m_Db, sRes);
   return sRes;
}

CComVariant CDbOperations::GetProperty(const GUID& guid, DBPROPID propid) const
{
   // Prepare properties
   DBPROPIDSET PropIdSet;
   PropIdSet.guidPropertySet = guid;
   PropIdSet.cPropertyIDs = 1;
   PropIdSet.rgPropertyIDs = &propid;
   // Retrieve property
   CComQIPtr<IDBProperties> spProperties = m_Db.m_spInit;
   ULONG lCount = 0;
   DBPROPSET* pPropSet = NULL;
   spProperties->GetProperties(1, &PropIdSet, &lCount, &pPropSet);
   CComVariant vRes = pPropSet->rgProperties->vValue;
   // Free memory
   if( pPropSet ) {
      ::VariantClear(&pPropSet->rgProperties[0].vValue);
      ::CoTaskMemFree(pPropSet->rgProperties);
      ::CoTaskMemFree(pPropSet);
   }
   return vRes;
}

CString CDbOperations::GetPropertyStr(const GUID& guid, DBPROPID propid) const
{
   CComVariant v = GetProperty(guid, propid);
   v.ChangeType(VT_BSTR);
   if( v.vt != VT_BSTR ) return _T("");
   if( v.bstrVal == NULL ) return _T("");
   return v.bstrVal;
}

