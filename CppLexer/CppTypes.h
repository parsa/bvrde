#if !defined(AFX_CPPTYPES_H__20040706_E768_9830_6D75_0080AD509054__INCLUDED_)
#define AFX_CPPTYPES_H__20040706_E768_9830_6D75_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "StdAfx.h"


extern int verb;

#define GLOB     0x00000000
#define PUBL     0x00000001
#define PROT     0x00000002
#define PRIV     0x00000004

#define FUNCTION_SEC  0x00000001
#define TYPEDEF_SEC   0x00000002
#define CLASS_SEC     0x00000004
#define UNION_SEC     0x00000008
#define MANUAL_SEC    0x00000010
#define VARIABLE_SEC  0x00000020
#define EMPTY_SEC     0x00000040
#define MACRO_SEC     0x00000080
#define ENUM_SEC      0x00000100
#define STRUCT_SEC    0x00000200

#define onlyDocs 0

class McString : public std::string
{
public:
   McString() { };
   McString(const char* p) { append(p); };
   inline void clear() { assign(""); };
   inline operator const char*() const { return c_str(); }
   inline void operator +=(char ch) { char x[] = { ch, '\0' }; append(x); };
   inline void operator +=(const char* p) { append(p); };
};

template< class T >
class McList : public std::vector<T>
{
public:
   ~McList()
   {
      for( size_t i = 0; i < size(); i++ ) delete at(i);
   }
   inline T last() const { return at(size() - 1); };
   inline void append(T p) { push_back(p); };
};


class Entry
{
public:
   Entry() : protection(0), section(0), done(0), startLine(0), lineNo(0), sub(NULL), next(NULL)
   {
   }
   ~Entry()
   {
      if( sub ) delete sub;
      if( next ) delete next;
   }
   void addSubEntry(Entry* p) { p->sub = sub; sub = p; };
   void addNextEntry(Entry* p) { p->next = next; next = p; };

public:
   int protection;
   int section;
   int done;
   int startLine;
   int lineNo;

   McString program;
   McString file;
   
   McString type;
   McString name;
   McString args;
   McString retrn;
   
   McString memo;
   McString doc;
   McString author;
   McString version;
   McList<McString*> see;
   McList<McString*> param;
   McList<McString*> extends;
   McList<McString*> exception;

   Entry* sub;
   Entry* next;
};


#endif // !defined(AFX_CPPTYPES_H__20040706_E768_9830_6D75_0080AD509054__INCLUDED_)

