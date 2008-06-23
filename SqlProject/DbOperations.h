#if !defined(AFX_DBOPERATIONS_H__20031213_A3FC_B274_2EA9_0080AD509054__INCLUDED_)
#define AFX_DBOPERATIONS_H__20031213_A3FC_B274_2EA9_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef enum
{
   DBTYPE_TABLE,
   DBTYPE_VIEW,
   DBTYPE_SYSTEMTABLE,
   DBTYPE_TEMPTABLE,
   DBTYPE_STOREDPROCEDURE,
   DBTYPE_FIELD,
   DBTYPE_INDEX,
} DATABASETYPE;

typedef struct
{
   DATABASETYPE ItemType;
   CString sName;
} DATABASEOBJECT;

typedef struct tagDATABASEINFO : DATABASEOBJECT
{
   CString sVendor;
} DATABASEINFO;

typedef struct FIELDINFO : DATABASEOBJECT
{
   FIELDINFO()
   {
      lType = 0;
      lPosition = 0;
      lFlags = 0;
      bNullable = false;
      bHasDefault = false;
      lLength = 0;
      lPrecision = 0;
      lDigits = 0;
   }
   long lType;
   long lPosition;
   long lFlags;
   bool bNullable;
   bool bHasDefault;
   CString sDefault;
   long lLength;
   long lPrecision;
   long lDigits;
} FIELDINFO;

enum { MAX_INDEX_FIELDS = 10 };

typedef struct INDEXINFO : DATABASEOBJECT
{
   INDEXINFO()
   {
      bPrimary = false;
      bUnique = false;
      bClustered = false;
      bNulls = false;
      lPosition = 0;
      lNulls = 0;
      lType = 0;
      lPropId = 0;
      nFields = 0;
   }
   bool bPrimary;
   bool bUnique;
   bool bClustered;
   bool bNulls;
   long lPosition;
   long lNulls;
   long lType;
   CString sType;
   long lPropId;
   int nFields;
   CString sFields[MAX_INDEX_FIELDS];
} INDEXINFO;

typedef struct tagTABLEINFO : DATABASEOBJECT
{
   CString sSchema;
   CString sCatalog;
   CString sType;
   CSimpleArray<FIELDINFO> aFields;
   CSimpleArray<INDEXINFO> aIndices;
   bool bFullInfo;   // All information is read
   DWORD dwTouched;  // Timestamp for when table-information was last requested
} TABLEINFO;


class CDbOperations
{
public:
   CDbOperations();

   COledbDatabase m_Db;
   DATABASEINFO m_DbInfo;
   CSimpleArray<TABLEINFO> m_aTables;

   BOOL ConnectDatabase(COledbDatabase* pDb, LPCTSTR pstrConnectString);
   BOOL EnumTables(COledbDatabase* pDb = NULL);
   BOOL LoadTableInfo(TABLEINFO* pTable, COledbDatabase* pDb = NULL);
   BOOL GetDatabaseInfo(DATABASEINFO& info);

   BOOL GetOwnerList(CSimpleArray<CString>& aList) const;
   BOOL GetTableList(CSimpleArray<CString>& aList, bool bAllEntries = true) const;
   BOOL GetColumnList(LPCTSTR pstrTable, CSimpleArray<CString>& aList);
   
   void MarkTable(LPCTSTR pstrName);
   TABLEINFO* EstimateNextInfoRequest(COledbDatabase* pDb =NULL);
   
   BOOL ChangeProperties(HWND hWnd, LPCTSTR pstrConnectString, CString& sResult);

   CString GetPropertyStr(const GUID& guid, DBPROPID propid) const;
   CComVariant GetProperty(const GUID& guid, DBPROPID propid) const;
};

#endif // !defined(AFX_DBOPERATIONS_H__20031213_A3FC_B274_2EA9_0080AD509054__INCLUDED_)

