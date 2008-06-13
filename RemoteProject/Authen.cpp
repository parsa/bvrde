
#include "StdAfx.h"
#include "resource.h"

#include "Authen.h"

#include "PasswordDlg.h"

#include <wincrypt.h>


////////////////////////////////////////////////////////
//

#define CRYPT_ENVELOPE_SIZE 30

// Here we store the user's password after being prompted
// so we needn't prompt again. This implies that the same
// password should be used for both FTP and Telnet/SSH connections!
static TCHAR g_szPassword[100];

// Due to security, the password.h file is not included in the source distribution.
// It contains the single source code line of...
//   static BYTE g_password[] = { 1, 2, 3, 4, 5, 6 };
// where the numbers are the pass-phrase.
// Put this file in the .\RemoteProject source folder.
//
#include "password.h"



////////////////////////////////////////////////////////
//

void SecClearPassword()
{
   g_szPassword[0] = '\0';
}

CString SecGetPassword()
{
   // Prompts for password if not known
   static CComAutoCriticalSection s_cs;
   s_cs.Lock();
   // If we already have a password, then use it...
   CString sPassword = g_szPassword;
   if( !sPassword.IsEmpty() ) {      
      s_cs.Unlock();
      return SecDecodePassword(sPassword);
   }
   // Prompt user for password
   // TODO: Ah, we need to ensure this is called from main thread only!
   //       How to do this?
   CWaitCursor cursor;
   CPasswordDlg dlg;
   if( dlg.DoModal() != IDOK ) {
      s_cs.Unlock();
      return _T("");
   }
   sPassword = dlg.GetPassword();
   _tcscpy(g_szPassword, SecEncodePassword(sPassword));
   s_cs.Unlock();
   return sPassword;
}

CString SecEncodePassword(LPCTSTR pstrPassword)
{
   // Check if it's an empty password...
   USES_CONVERSION;
   LPCSTR pstr = T2A(pstrPassword);
   size_t cchLen = strlen(pstr);
   if( cchLen == 0 || cchLen >= CRYPT_ENVELOPE_SIZE - 2 ) return pstrPassword;
   // Prepare encryption library...
   static LPCTSTR pstrProvider = _T("Microsoft Base Cryptographic Provider v1.0");
   HCRYPTPROV hProv = NULL;
   if( !::CryptAcquireContext(&hProv, NULL, pstrProvider, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ) return pstrPassword;
   // Generate a buffer containing password and random fillings...
   BYTE bData[CRYPT_ENVELOPE_SIZE] = { 0 };
   ::CryptGenRandom(hProv, sizeof(bData), bData);
   bData[CRYPT_ENVELOPE_SIZE - 1] = cchLen;
   bData[CRYPT_ENVELOPE_SIZE - 2] = 0;
   memcpy(bData, pstr, cchLen);
   // Encrypt the data blob using Microsoft crypt library...
   HCRYPTHASH hHash = NULL;
   if( !::CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash) ) return pstrPassword;
   DWORD cbKey = sizeof(g_password);
   if( !::CryptHashData(hHash, g_password, cbKey, 0) ) return pstrPassword;
   HCRYPTKEY hKey = NULL;
   if( !::CryptDeriveKey(hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey) ) return pstrPassword;
   DWORD cbData = sizeof(bData);
   if( !::CryptEncrypt(hKey, 0, TRUE, 0, bData, &cbData, cbData) ) return pstrPassword;
   if( hKey ) ::CryptDestroyKey(hKey);
   if( hHash ) ::CryptDestroyHash(hHash);
   if( hProv ) ::CryptReleaseContext(hProv, 0);
   // Turn binary blob into a hex-encoded string...
   CHAR szPassword[(CRYPT_ENVELOPE_SIZE * 2) + 2] = { 0 };
   LPSTR pOut = szPassword;
   *pOut++ = '~';
   for( size_t i = 0; i < CRYPT_ENVELOPE_SIZE; i++ ) {
      static char s_hex[] = "0123456789abcdef";
      *pOut++ = s_hex[(bData[i] & 0xf0) >> 4];
      *pOut++ = s_hex[bData[i] & 0x0f];
   }
   return szPassword;
}

CString SecDecodePassword(LPCTSTR pstrPassword)
{
   // If the password contains the ~ (tilde) character at the first position it
   // is assumed that it was locally encrypted.
   USES_CONVERSION;
   if( pstrPassword[0] != '~' ) return pstrPassword;
   if( _tcslen(pstrPassword) != (CRYPT_ENVELOPE_SIZE * 2) + 1 ) return pstrPassword;
   // Prepare encryption library...
   static LPCTSTR pstrProvider = _T("Microsoft Base Cryptographic Provider v1.0");
   HCRYPTPROV hProv = NULL;
   if( !::CryptAcquireContext(&hProv, NULL, pstrProvider, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return _T("");
   HCRYPTHASH hHash = NULL;
   if( !::CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) return _T("");
   DWORD cbKey = sizeof(g_password);
   if( !::CryptHashData(hHash, g_password, cbKey, 0)) return _T("");
   HCRYPTKEY hKey = NULL;
   if( !::CryptDeriveKey(hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey)) return _T("");
   // Convert hex-encoded to character string...
   BYTE bData[CRYPT_ENVELOPE_SIZE * 2];
   int i = 0;
   while( i < CRYPT_ENVELOPE_SIZE ) {
      ++pstrPassword;
      CHAR iVal = (*pstrPassword > '9' ? *pstrPassword - 'a' + 10 : *pstrPassword - '0');
      ++pstrPassword;
      iVal = (iVal << 4) + (*pstrPassword > '9' ? *pstrPassword - 'a' + 10 : *pstrPassword - '0');
      bData[i++] = iVal;
   }
   // Decrypt using Microsoft crypt library...
   DWORD cbData = sizeof(bData);
   if( !::CryptDecrypt(hKey, 0, TRUE, 0, bData, &cbData)) return _T("");
   if( hKey ) ::CryptDestroyKey(hKey);
   if( hHash ) ::CryptDestroyHash(hHash);
   if( hProv ) ::CryptReleaseContext(hProv, 0);
   // Turn into a password string...
   size_t cchLen = bData[CRYPT_ENVELOPE_SIZE - 1];
   if( cchLen == 0 || cchLen >= CRYPT_ENVELOPE_SIZE - 1 ) return _T("");
   CHAR szPassword[CRYPT_ENVELOPE_SIZE] = { 0 };
   memcpy(szPassword, bData, cchLen);
   return szPassword;
}

