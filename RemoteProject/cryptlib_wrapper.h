#if !defined(AFX_CRYPTLIB_WRAPPER_H__20030717_EDCA_8D0A_76E7_0080AD509054__INCLUDED_)
#define AFX_CRYPTLIB_WRAPPER_H__20030717_EDCA_8D0A_76E7_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


////////////////////////////////////////////////////////
//

#include <time.h>

// CryptLib by Peter Gutmann
#include "cryptlib.h"

typedef int CRYPT_CONTEXT;
typedef int CRYPT_SESSION;


/**
 * @class CCryptLib
 * Wrapper around CL32.DLL (cryptlib).
 */
class CCryptLib
{
public:
   typedef int (CALLBACK* fn_cryptInit)();
   typedef int (CALLBACK* fn_cryptEnd)();
   typedef int (CALLBACK* fn_cryptAddRandom)(void*, int);
   typedef int (CALLBACK* fn_cryptCreateSession)(CRYPT_SESSION*, CRYPT_USER, CRYPT_SESSION_TYPE);
   typedef int (CALLBACK* fn_cryptDestroySession)(CRYPT_SESSION);
   typedef int (CALLBACK* fn_cryptGetAttribute)(CRYPT_HANDLE, CRYPT_ATTRIBUTE_TYPE, int*);
   typedef int (CALLBACK* fn_cryptSetAttribute)(CRYPT_HANDLE, CRYPT_ATTRIBUTE_TYPE, int);
   typedef int (CALLBACK* fn_cryptGetAttributeString)(CRYPT_HANDLE, CRYPT_ATTRIBUTE_TYPE, void*, int*);
   typedef int (CALLBACK* fn_cryptSetAttributeString)(CRYPT_HANDLE, CRYPT_ATTRIBUTE_TYPE, const void*, int);
   typedef int (CALLBACK* fn_cryptCreateContext)(CRYPT_CONTEXT CRYPT_USER, CRYPT_ALGO_TYPE);
   typedef int (CALLBACK* fn_cryptDestroyContext)(CRYPT_CONTEXT);
   typedef int (CALLBACK* fn_cryptAsyncCancel)(CRYPT_CONTEXT);
   typedef int (CALLBACK* fn_cryptPushData)(CRYPT_HANDLE, void*, int, int*);
   typedef int (CALLBACK* fn_cryptFlushData)(CRYPT_HANDLE);
   typedef int (CALLBACK* fn_cryptPopData)(CRYPT_HANDLE, void*, int, int*);
   typedef int (CALLBACK* fn_cryptKeysetOpen)(CRYPT_KEYSET*, CRYPT_USER, CRYPT_KEYSET_TYPE, const char* name, CRYPT_KEYOPT_TYPE);
   typedef int (CALLBACK* fn_cryptKeysetClose)(CRYPT_KEYSET);
   typedef int (CALLBACK* fn_cryptGetPrivateKey)(CRYPT_KEYSET, CRYPT_CONTEXT*, CRYPT_KEYID_TYPE, const char*, const char* password);
   HINSTANCE hInst;
   fn_cryptInit cryptInit;
   fn_cryptEnd cryptEnd;
   fn_cryptAddRandom cryptAddRandom;
   fn_cryptCreateSession cryptCreateSession;
   fn_cryptDestroySession cryptDestroySession;
   fn_cryptGetAttribute cryptGetAttribute;
   fn_cryptSetAttribute cryptSetAttribute;
   fn_cryptGetAttributeString cryptGetAttributeString;
   fn_cryptSetAttributeString cryptSetAttributeString;
   fn_cryptCreateContext cryptCreateContext;
   fn_cryptDestroyContext cryptDestroyContext;
   fn_cryptAsyncCancel cryptAsyncCancel;
   fn_cryptPushData cryptPushData;
   fn_cryptFlushData cryptFlushData;
   fn_cryptPopData cryptPopData;
   fn_cryptKeysetOpen cryptKeysetOpen;
   fn_cryptKeysetClose cryptKeysetClose;
   fn_cryptGetPrivateKey cryptGetPrivateKey;

   CCryptLib() : hInst(NULL)
   {
   }
   ~CCryptLib()
   {
      if( hInst != NULL ) cryptEnd();
      // NOTE: We don't FreeLibrary() it; we'll want it in memory for later use.
   }

   bool Init()
   {
      if( hInst != NULL ) return true;
      TCHAR szFilename[MAX_PATH];
      ::wsprintf(szFilename, _T("%sCL32.DLL"), CModulePath());
      hInst = ::LoadLibrary(szFilename);
      if( hInst == NULL ) return false;
#define GETADDRESS(x) x = (fn_##x)::GetProcAddress(hInst, #x); ATLASSERT(x)
      GETADDRESS(cryptInit);
      GETADDRESS(cryptEnd);
      GETADDRESS(cryptAddRandom);
      GETADDRESS(cryptCreateSession);
      GETADDRESS(cryptDestroySession);
      GETADDRESS(cryptCreateContext);
      GETADDRESS(cryptDestroyContext);
      GETADDRESS(cryptAsyncCancel);
      GETADDRESS(cryptGetAttribute);
      GETADDRESS(cryptSetAttribute);
      GETADDRESS(cryptGetAttributeString);
      GETADDRESS(cryptSetAttributeString);
      GETADDRESS(cryptPushData);
      GETADDRESS(cryptFlushData);
      GETADDRESS(cryptPopData);
      GETADDRESS(cryptKeysetOpen);
      GETADDRESS(cryptKeysetClose);
      GETADDRESS(cryptGetPrivateKey);
#undef GETADDRESS
      // Iniailize lib right away
      cryptInit();
      cryptAddRandom(NULL, CRYPT_RANDOM_SLOWPOLL);
      return true;
   }
};


inline int SshGetPrivateKey(CCryptLib& clib, 
                            CRYPT_CONTEXT *cryptContext, 
                            LPCSTR keysetName,
                            LPCSTR keyName, 
                            LPCSTR password)
{
   // Loads certificate. Tries to locate and load the certificate
   // if available. CryptLib should be initialized before this is called.
   CRYPT_KEYSET cryptKeyset;
   int dummy, status;
   // Read the key from the keyset
   status = clib.cryptKeysetOpen( &cryptKeyset, CRYPT_UNUSED, CRYPT_KEYSET_FILE, keysetName, CRYPT_KEYOPT_READONLY );
   if( cryptStatusError( status ) ) return status;
   status = clib.cryptGetPrivateKey( cryptKeyset, cryptContext, CRYPT_KEYID_NAME, keyName, password );
   clib.cryptKeysetClose( cryptKeyset );
   if( cryptStatusError( status ) ) return status;

   // If the key has a cert attached, make sure it's still valid before we 
   // hand it back to the self-test functions which will report the problem 
   // as being with the self-test rather than with the cert
   time_t validFrom, validTo;
   status = clib.cryptGetAttributeString( *cryptContext, CRYPT_CERTINFO_VALIDFROM, &validFrom, &dummy );
   if( cryptStatusError( status ) ) return CRYPT_OK;
   clib.cryptGetAttributeString( *cryptContext, CRYPT_CERTINFO_VALIDTO, &validTo, &dummy );
   if( ( validTo - validFrom > ( 86400L * 30L ) ) && validTo - ::time(NULL) <= ( 86400L * 30L ) )
   {
      CString s;
      if( validTo <= ::time(NULL) ) s.LoadString(IDS_ERR_KEY_EXPIRED);
      else if( validTo - time( NULL ) <= 86400 ) s.LoadString(IDS_ERR_KEY_EXPIRES_TODAY);
      else s.Format(IDS_ERR_KEY_EXPIRES_SOON, (validTo - ::time(NULL)) / 86400L);
      if( !s.IsEmpty() ) _pDevEnv->ShowMessageBox(NULL, s, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONINFORMATION|MB_MODELESS);
   }
   return CRYPT_OK;
}


#endif // !defined(AFX_CRYPTLIB_WRAPPER_H__20030717_EDCA_8D0A_76E7_0080AD509054__INCLUDED_)

