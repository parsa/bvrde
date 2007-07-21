#if !defined(AFX_FILEPROTOCOL_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_)
#define AFX_FILEPROTOCOL_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


////////////////////////////////////////////////////////
//

class CFileProtocol : public IRemoteFileProtocol
{
public:
   CFileProtocol();
   virtual ~CFileProtocol();

// IRemoteFileProtocol
public:
   void Clear();
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);

   bool Start();
   bool Stop();
   void SignalStop();
   bool IsConnected() const;

   bool LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize = NULL);
   bool SaveFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE ppOut, DWORD dwSize);
   bool DeleteFile(LPCTSTR pstrFilename);
   bool SetCurPath(LPCTSTR pstrPath);
   CString GetCurPath();
   CString FindFile(LPCTSTR pstrFilename);
   bool EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles, bool bUseCache);

   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

   bool WaitForConnection();

protected:
   CString m_sPath;
   CString m_sCurDir;
   CString m_sSearchPath;
   //
   bool m_bCancel;
   bool m_bConnected;
};



#endif // !defined(AFX_FILEPROTOCOL_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_)
