// CppLexer.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"

#include "CppLexer.h"
#include "CppTypes.h"

#include <SHLWAPI.H>
#pragma comment(lib, "shlwapi.lib")

int verb = 0;
HANDLE hMutex = NULL;


/////////////////////////////////////////////////////////////////////////////
// DLL Main

BOOL APIENTRY DllMain( HANDLE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
   if( dwReason == DLL_PROCESS_ATTACH ) {
      ::DisableThreadLibraryCalls((HINSTANCE) hInstance);
      hMutex = ::CreateMutex(NULL, FALSE, NULL);
   }
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Parse method

bool _UDgreater(std::string& elem1, std::string& elem2)
{
   return elem1.substr(0, elem1.find('|')) < elem2.substr(0, elem2.find('|'));
}


typedef std::vector<std::string> STRINGLIST;

void _parseStructs(Entry* parent, STRINGLIST& aList)
{
   if( parent == NULL )  return;
   for( Entry* cr = parent; cr; cr = cr->sub )
   {
      _parseStructs(cr->next, aList);

      if( cr->name.empty() ) continue;
      if( cr->name.at(0) == '#' ) continue;
      if( cr->name.at(0) == '*' ) continue;
      if( cr->name.at(0) == '(' ) continue;

      if( cr->doc.empty() ) cr->doc = cr->memo;

      char type = 'm';
      if( cr->section == FUNCTION_SEC ) type = 'm';
      else if( cr->section == TYPEDEF_SEC ) type = 't';
      else if( cr->section == CLASS_SEC ) type = 'c';
      else if( cr->section == UNION_SEC ) type = 's';
      else if( cr->section == STRUCT_SEC ) type = 's';
      else if( cr->section == MANUAL_SEC ) type = 'm';
      else if( cr->section == VARIABLE_SEC ) type = 'v';
      else if( cr->section == MACRO_SEC ) type = 'd';
      else if( cr->section == ENUM_SEC ) type = 'e';
      else if( cr->section == IMPL_SEC ) type = 'x';
      else if( cr->section == NAMESPACE_SEC ) type = 'n';

      char prot = 'g';
      if( cr->protection == PUBL ) prot = 'p';
      if( cr->protection == PROT ) prot = 'r';
      if( cr->protection == PRIV ) prot = 'i';

      std::replace(cr->doc.begin(), cr->doc.end(), '\r', ' ');
      std::replace(cr->doc.begin(), cr->doc.end(), '\n', ' ');
      std::replace(cr->doc.begin(), cr->doc.end(), '\t', ' ');
      std::replace(cr->doc.begin(), cr->doc.end(), '|', '¦');
      std::replace(cr->type.begin(), cr->type.end(), '|', '¦');
      std::replace(cr->name.begin(), cr->name.end(), '|', '¦');
      std::replace(cr->args.begin(), cr->args.end(), '|', '¦');

      static char szBuffer[1025] = { 0 };
      ::wsprintfA(szBuffer, "%s|%c%c|%s%s%s%s|%s|%s|%d|%s\n", 
         cr->name.c_str(),  
         type,
         prot,
         cr->type.c_str(),
         cr->type.empty() ? "" : " ",
         cr->name.c_str(),
         cr->args.c_str(),
         parent->name == cr->name && parent->section == cr->section ? "" : parent->name.c_str(),
         cr->namespc.c_str(),
         cr->lineNo,
         cr->doc.c_str());

      static std::string sRes = szBuffer;
      sRes = szBuffer;
      aList.push_back(sRes);
   }
}


BOOL APIENTRY CppLexer_Parse(LPCSTR pstrSourceName, LPCSTR pstrText, LPCWSTR pstrOutputFile)
{   
   extern void parseCpp(Entry*);

   Entry* root = new Entry();
   if( root == NULL ) return FALSE;
   STRINGLIST aList;
   root->program = pstrText;
   ::WaitForSingleObject(hMutex, 5000 /*INFINITE*/);
   try
   {
      parseCpp(root);
      _parseStructs(root, aList);
   }
   catch( ... )
   {
      ::ReleaseMutex(hMutex);
      delete root;
      return FALSE;
   }
   ::ReleaseMutex(hMutex);

   std::sort(aList.begin(), aList.end(), _UDgreater);

   delete root;

   char szFirstLine[MAX_PATH + 3];
   ::wsprintfA(szFirstLine, "#%s\n", pstrSourceName);

   HANDLE hFile = ::CreateFileW(pstrOutputFile, 
      GENERIC_WRITE, 
      0, 
      NULL,
      CREATE_ALWAYS, 
      FILE_ATTRIBUTE_NORMAL, 
      NULL);
   if( hFile == INVALID_HANDLE_VALUE ) return FALSE;
   DWORD dwWritten = 0;
   ::WriteFile(hFile, szFirstLine, strlen(szFirstLine), &dwWritten, NULL);
   for( size_t i = 0; i < aList.size(); i++ ) {
      ::WriteFile(hFile, aList[i].c_str(), aList[i].length(), &dwWritten, NULL);
   }
   ::CloseHandle(hFile);

   return TRUE;
}

