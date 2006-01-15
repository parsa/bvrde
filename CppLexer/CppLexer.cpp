// CppLexer.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"

#include "CppLexer.h"
#include "CppTypes.h"

#include <SHLWAPI.H>
#pragma comment(lib, "shlwapi.lib")

int verb = 0;


/////////////////////////////////////////////////////////////////////////////
// DLL Main

BOOL APIENTRY DllMain( HANDLE hInstance, DWORD /*dwReason*/, LPVOID /*lpReserved*/)
{
   ::DisableThreadLibraryCalls((HINSTANCE)hInstance);
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Parse method

LPSTR _W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
   lpa[0] = '\0';
   ::WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
   return lpa;
}

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

      // We don't want class-member implementations in .cpp files to
      // be included. We will get them from the header instead.
      if( cr->name.find("::") != std::string::npos ) continue;

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

      char prot = 'g';
      if( cr->protection == PUBL ) prot = 'p';
      if( cr->protection == PROT ) prot = 'r';
      if( cr->protection == PRIV ) prot = 'i';

      std::replace(cr->memo.begin(), cr->memo.end(), '\r', ' ');
      std::replace(cr->memo.begin(), cr->memo.end(), '\n', ' ');
      std::replace(cr->memo.begin(), cr->memo.end(), '\t', ' ');
      std::replace(cr->memo.begin(), cr->memo.end(), '|', '¦');
      std::replace(cr->type.begin(), cr->type.end(), '|', '¦');
      std::replace(cr->name.begin(), cr->name.end(), '|', '¦');
      std::replace(cr->args.begin(), cr->args.end(), '|', '¦');

      static char szBuffer[1025] = { 0 };
      ::wsprintfA(szBuffer, "%s|%c%c|%s%s%s%s|%s|%ld|%hs\n", 
         cr->name.c_str(),  
         type,
         prot,
         cr->type.c_str(),
         cr->type.empty() ? "" : " ",
         cr->name.c_str(),
         cr->args.c_str(),
         parent->name == cr->name ? "" : parent->name.c_str(),
         (long) cr->lineNo,
         cr->memo.c_str());

      static std::string sRes = szBuffer;
      sRes = szBuffer;
      aList.push_back(sRes);
   }
}


BOOL APIENTRY CppLexer_Parse(LPCWSTR pstrFilename, LPCSTR pstrText)
{   
   extern void parseCpp(Entry*);

   Entry* root = new Entry();
   if( root == NULL ) return FALSE;
   root->program = pstrText;
   try
   {
      parseCpp( root );
   }
   catch(...)
   {
      delete root;
      return FALSE;
   }

   char szFilename[MAX_PATH] = { 0 };
   _W2AHelper(szFilename, pstrFilename, sizeof(szFilename) - 1);

   STRINGLIST aList;
   _parseStructs(root, aList);

   std::sort(aList.begin(), aList.end(), _UDgreater);

   delete root;

   char szTargetFilename[MAX_PATH] = { 0 };
   ::GetModuleFileNameA(NULL, szTargetFilename, MAX_PATH);
   char* p = strrchr(szTargetFilename, '\\');
   if( p ) strcpy(p, "\\Lex\\");
   strcat(szTargetFilename, ::PathFindFileNameA(szFilename));
   p = strrchr(szTargetFilename, '.');
   if( p ) *p = '_';
   strcat(szTargetFilename, ".lex");

   char szFirstLine[MAX_PATH + 2];
   ::wsprintfA(szFirstLine, "#%s\n", szFilename);

   HANDLE hFile = ::CreateFileA(szTargetFilename, 
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

