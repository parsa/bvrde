#if !defined(AFX_FILEMANAGER_H__20030715_CE0E_FC3D_F94A_0080AD509054__INCLUDED_)
#define AFX_FILEMANAGER_H__20030715_CE0E_FC3D_F94A_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/**
 * @class CFileManager
 * A proxy class for File Transfer protocols.
 */
class CFileManager : 
   public IRemoteFileProtocol
{
public:
   CFileManager();
   virtual ~CFileManager();

// IFileManager
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
   bool EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles);

   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

   bool WaitForConnection();

protected:
   CString m_sType;
   IRemoteFileProtocol* m_pProtocol;
};


#endif // !defined(AFX_FILEMANAGER_H__20030715_CE0E_FC3D_F94A_0080AD509054__INCLUDED_)

