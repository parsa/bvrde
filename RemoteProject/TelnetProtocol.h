#if !defined(AFX_TELNETMANAGER_H__20030316_6B33_F8CF_8798_0080AD509054__INCLUDED_)
#define AFX_TELNETMANAGER_H__20030316_6B33_F8CF_8798_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


///////////////////////////////////////////////////////
//

enum
{
   TELNET_IAC   = 255,         // Interpret as command escape sequence
                               // Prefix to all telnet commands 
   TELNET_DONT  = 254,         // You are not to use this option
   TELNET_DO    = 253,         // Please, you use this option
   TELNET_WONT  = 252,         // I won't use option 
   TELNET_WILL  = 251,         // I will use option 
   TELNET_SB    = 250,         // Subnegotiate
   TELNET_GA    = 249,         // Go ahead
   TELNET_EL    = 248,         // Erase line          
   TELNET_EC    = 247,         // Erase character
   TELNET_AYT   = 246,         // Are you there
   TELNET_AO    = 245,         // Abort output
   TELNET_IP    = 244,         // Interrupt process
   TELNET_BRK   = 243,         // Break
   TELNET_DM    = 242,         // Data mark
   TELNET_NOP   = 241,         // No operation.
   TELNET_SE    = 240,         // End of subnegotiation
   TELNET_EOR   = 239,         // End of record
   TELNET_ABORT = 238,         // About process
   TELNET_SUSP  = 237,         // Suspend process
   TELNET_XEOF  = 236,         // End of file; EOF already used
};

enum
{
   TELOPT_BIN    = 0,   // Binary transmission
   TELOPT_ECHO   = 1,   // Echo
   TELOPT_RECN   = 2,   // Reconnection
   TELOPT_SUPP   = 3,   // Suppress go ahead
   TELOPT_APRX   = 4,   // Approx message size negotiation
   TELOPT_STAT   = 5,   // Status
   TELOPT_TIM    = 6,   // Timing mark
   TELOPT_REM    = 7,   // Remote controlled trans/echo
   TELOPT_OLW    = 8,   // Output line width
   TELOPT_OPS    = 9,   // Output page size
   TELOPT_OCRD   = 10,  // Out carriage-return disposition
   TELOPT_OHT    = 11,  // Output horizontal tabstops
   TELOPT_OHTD   = 12,  // Out horizontal tab disposition
   TELOPT_OFD    = 13,  // Output formfeed disposition
   TELOPT_OVT    = 14,  // Output vertical tabstops
   TELOPT_OVTD   = 15,  // Output vertical tab disposition
   TELOPT_OLD    = 16,  // Output linefeed disposition
   TELOPT_EXT    = 17,  // Extended ascii character set
   TELOPT_LOGO   = 18,  // Logout
   TELOPT_BYTE   = 19,  // Byte macro
   TELOPT_DATA   = 20,  // Data entry terminal
   TELOPT_SUP    = 21,  // supdup protocol
   TELOPT_SUPO   = 22,  // supdup output
   TELOPT_SNDL   = 23,  // Send location
   TELOPT_TERM   = 24,  // Terminal type
   TELOPT_EOR    = 25,  // End of record
   TELOPT_TACACS = 26,  // Tacacs user identification
   TELOPT_OM     = 27,  // Output marking
   TELOPT_TLN    = 28,  // Terminal location number
   TELOPT_3270   = 29,  // Telnet 3270 regime
   TELOPT_X3     = 30,  // X.3 PAD
   TELOPT_NAWS   = 31,  // Negotiate about window size
   TELOPT_TS     = 32,  // Terminal speed
   TELOPT_RFC    = 33,  // Remote flow control
   TELOPT_LINE   = 34,  // Linemode
   TELOPT_XDL    = 35,  // X display location
   TELOPT_ENVIR  = 36,  // Telnet environment option
   TELOPT_AUTH   = 37,  // Telnet authentication option
   TELOPT_NENVIR = 39,  // Telnet environment option
   TELOPT_EXTOP  = 255, // Extended-options-list
};



////////////////////////////////////////////////////////
//

class CTelnetProtocol;


class CTelnetThread : public CThreadImpl
{
protected:
   enum
   {
      NEGOTIATED_DO = 1,
      NEGOTIATED_WILL = 2,
   };

public:
   DWORD Run();
   
protected:
   void _NegotiateOption(LPBYTE pList, CSimpleValArray<BYTE>& aSend, BYTE bAction, BYTE nCmd);
   CHAR _GetByte(CSocket& socket, const LPBYTE pBuffer, DWORD dwRead, DWORD& dwPos) const;
   CString _GetLine(LPBYTE pBuffer, DWORD dwStart, DWORD dwEnd) const;

public:
   CTelnetProtocol* m_pManager;
   IShellCallback* m_pCallback;
};


///////////////////////////////////////////////////////
//

class CTelnetProtocol : public IRemoteCommandProtocol
{
friend CTelnetThread;
public:
   CTelnetProtocol();
   virtual ~CTelnetProtocol();

// IRemoteCommandProtocol
public:
   void Init(CRemoteProject* pProject, IShellCallback* pCallback = NULL);
   void Clear();
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);
   bool Start();
   bool Stop();
   void SignalStop();
   bool IsConnected() const;
   bool IsBusy() const;

   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

   bool ReadData(CString& s, DWORD dwTimeout = 0);
   bool WriteData(LPCTSTR pstrData);
   bool WriteSignal(BYTE bCmd);
   bool WriteScreenSize(int w, int h);
   bool WaitForConnection();

// Attributes
protected:
   CRemoteProject* m_pProject;
   IShellCallback* m_pCallback;

   CString m_sHost;
   long m_lPort;
   CString m_sUsername;
   CString m_sPassword;
   CString m_sPath;
   CString m_sExtraCommands;
   //
   CTelnetThread m_thread;
   CSocket m_socket;
   CEvent m_event;
   volatile DWORD m_dwErrorCode;
   volatile bool m_bConnected;
   volatile bool m_bCanWindowSize;
};


#endif // !defined(AFX_TELNETMANAGER_H__20030316_6B33_F8CF_8798_0080AD509054__INCLUDED_)

