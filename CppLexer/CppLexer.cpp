// CppLexer.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"

#include "CppLexer.h"
#include "CppTypes.h"

int verb = 0;


BOOL APIENTRY DllMain( HANDLE hInstance, DWORD /*dwReason*/, LPVOID /*lpReserved*/)
{
   ::DisableThreadLibraryCalls((HINSTANCE)hInstance);
   return TRUE;
}

LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
   lpa[0] = '\0';
   ::WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
   return lpa;
}

bool UDgreater( std::string elem1, std::string elem2 )
{
   return elem1 < elem2;
}


BOOL CALLBACK CppLexer_Parse(LPCWSTR pstrFilename, LPCSTR pstrText)
{   
   extern void parseCpp(Entry* rt);
   extern int yyLineNr;
   extern char yyFileName[264];

   Entry* root = new Entry();
   if( root == NULL ) return FALSE;
   root->program = pstrText;
   parseCpp( root );

   char szFilename[MAX_PATH] = { 0 };
   W2AHelper(szFilename, pstrFilename, ::lstrlenW(pstrFilename));
   
   char* pstrNamePart = pstrNamePart = strrchr(szFilename, '\\');
   if( pstrNamePart == NULL ) pstrNamePart = strrchr(szFilename, '/');
   if( pstrNamePart != NULL ) pstrNamePart++; else pstrNamePart = szFilename;

   std::vector<std::string> aList;

   Entry* cl = NULL;
   char szBuffer[1000] = { 0 };
   for( Entry* cr = root->sub; cr; cr = cr->sub )
   {
      if( cr->name.length() == 0 ) continue;
      if( strstr(cr->name, "::") != NULL ) continue;

      char type = 'm';
      if( strncmp(cr->type, "class", 5) == 0 ) type = 'c';
      else if( strncmp(cr->type, "union", 5) == 0 ) type = 's';
      else if( strncmp(cr->type, "struct", 6) == 0 ) type = 's';
      else if( strncmp(cr->type, "#define", 7) == 0 ) type = 'd';
      else if( strncmp(cr->type, "typedef", 7) == 0 ) type = 't';
      if( cr->section == FUNCTION_SEC ) type = 'm';
      else if( cr->section == TYPEDEF_SEC ) type = 't';
      else if( cr->section == CLASS_SEC ) type = 'c';
      else if( cr->section == UNION_SEC ) type = 's';
      else if( cr->section == MANUAL_SEC ) type = 'm';
      else if( cr->section == VARIABLE_SEC ) type = 'v';
      else if( cr->section == MACRO_SEC ) type = 'd';

      char szMemo[100] = { 0 };
      strncpy(szMemo, cr->memo.c_str(), 99);
      for( int i = 0; szMemo[i]; i++ ) {
         switch( szMemo[i] ) {
         case '\r':
         case '\n':
         case '\t':
            szMemo[i] = ' ';
            break;
         }
      }

      ::wsprintf(szBuffer, "%s|%c|%s%s %s|%s|%ld|%s\n", 
         cr->name.c_str(),  
         type,
         cr->type.c_str(),  
         cr->name.c_str(),  
         cr->args.c_str(),  
         cl == NULL ? "" : cl->name.c_str(),
         (long) cr->lineNo, 
         szMemo);   

      if( cr->protection == GLOB ) cl = NULL;
      if( type == 'c' ) cl = cr;
      else if( type == 's' ) cl = cr;
      else if( type == 't' ) cl = cr;

      std::string sRes = szBuffer;
      aList.push_back(sRes);
   }

   std::sort(aList.begin(), aList.end(), UDgreater);

   delete root;

   char szTargetFilename[MAX_PATH] = { 0 };
   ::GetModuleFileName(NULL, szTargetFilename, MAX_PATH);
   char* p = strrchr(szTargetFilename, '\\');
   if( p ) strcpy(p, "\\Lex\\");
   strcat(szTargetFilename, szFilename);
   p = strrchr(szTargetFilename, '.');
   if( p ) *p = '_';
   strcat(szTargetFilename, ".lex");

   char szFirstLine[MAX_PATH];
   ::wsprintf(szFirstLine, "#%s\n", pstrNamePart);

   HANDLE hFile = ::CreateFile(szTargetFilename, 
      GENERIC_WRITE, 
      0, 
      NULL,
      CREATE_ALWAYS, 
      FILE_ATTRIBUTE_NORMAL, 
      NULL);
   if( hFile == INVALID_HANDLE_VALUE ) return FALSE;
   DWORD dwWritten = 0;
   ::WriteFile(hFile, szFirstLine, ::lstrlenA(szFirstLine), &dwWritten, NULL);
   for( int i = 0; i < aList.size(); i++ ) {
      ::WriteFile(hFile, aList[i].c_str(), aList[i].length(), &dwWritten, NULL);
   }
   ::CloseHandle(hFile);

   return TRUE;
}

