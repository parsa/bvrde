#if !defined(AFX_QUERYTHREAD_H__20031214_0E53_4B72_DBAE_0080AD509054__INCLUDED_)
#define AFX_QUERYTHREAD_H__20031214_0E53_4B72_DBAE_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Thread.h"
#include "DbOperations.h"

class CQueryThread;

#define WM_USER_DATA_AVAILABLE   WM_USER + 200


/// The types of packet we can send to the view
typedef enum PACKETTYPE
{
   PACKET_START,
   PACKET_COLUMNINFO,
   PACKET_ROWINFO,
   PACKET_ERROR,
   PACKET_FINISH,
   PACKET_DONE,
};

/// The data packet we send to the view
typedef struct DATAPACKET
{
   DATAPACKET()
   {
      iCols = 0;
      iRows = 0;
      pstrData = NULL;
      plData = NULL;
   }

   DATAPACKET(PACKETTYPE _Type, int _iCols = 0, int _iRows = 0, CString* _pstrData = NULL, LONG* _plData = NULL)
   {
      Type = _Type;
      iCols = _iCols;
      iRows = _iRows;
      pstrData = _pstrData;
      plData = _plData;
   }

   PACKETTYPE Type;
   int iCols;
   int iRows;
   CString* pstrData;
   LONG* plData;
} DATAPACKET;


/**
 * @class CQueryThread
 * This thread runs the SQL statements and posts the result to the UI view.
 * The thread is also responsible for gathering the DDL info and posting it to the view.
 */
class CQueryThread : public CThreadImpl<CQueryThread>
{
public:
   CQueryThread(CDbOperations* pDbData);

   typedef enum
   {
      STATE_NOTHING = 0,
      STATE_CONNECT,
      STATE_DISCONNECT,
      STATE_ABORT,
      STATE_EXECUTE,
      STATE_INFOREQUEST,
   } STATE;

   typedef struct
   {
      CString sSQL;
      int lLineNum;
   } SQLPART;

   DWORD Run();

   void SetNotify(HWND hWnd);
   void Connect(LPCTSTR pstrConnectString);
   void Disconnect();
   void ExecuteSql(LPCTSTR pstrSql, int lLineNum);
   void Abort();
   bool IsBusy() const;
   void Cancel();
   void MarkTable(LPCTSTR pstrName);
   void InfoRequest();

   BOOL _Execute(CDbCommand* pCmd, CDbRecordset* pRec);
   BOOL _SplitSQL(const CString& sSQL, int lLineNum, CSimpleArray<SQLPART>& aParts);
   CString _GetDbErrorText(COledbDatabase* pDb, LPCTSTR pstrSQL) const;

   CDbOperations* m_pDbData;
   CString m_sConnectString;              // Database connection string
   HWND m_hWndNotify;                     // Window handle to result view
   int m_nChunkSize;                      // Number of records in each packet
   CEvent m_event;                        // Queue event
   volatile STATE m_state;                // Current queue state
   volatile bool m_bExecuting;            // Is currently executing?
   /*volatile*/ CString m_sSQL;           // SQL statement being executed
   volatile int m_lLineNum;               // Line-number in view where SQL start
   /*volatile*/ COledbCommand* m_pDbCmd;
};



#endif // !defined(AFX_QUERYTHREAD_H__20031214_0E53_4B72_DBAE_0080AD509054__INCLUDED_)

