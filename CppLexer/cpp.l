/*************************************************************************

    CPP lexer

    This file is derived from the DOC++ source by Roland Wunderling
    and Malte Zoeckler.

    Better handling of typedefs, linenumbers, formatting, comments
    and various other improvements was made. Still a lot to be done
    to handle all modern C++ constructs. <bjarke@viksoe.dk>

 *************************************************************************/


%{

#pragma code_seg( "MISC" )

/*
 *	Includes
 */

#include "CppTypes.h"


/* -----------------------------------------------------------------
 *
 *	statics
 */
static const char*	inputString ;
static int		inputPosition ;
static int		lastContext ;
static int		lastCContext ;
static int		lastTContext;
static int		protection   = GLOB ;
static int		bracketCount = 0 ;
static int		sharpCount   = 0 ;
static Entry*		current_root = 0 ;
static Entry*		global_root  = 0 ;
static Entry*		current      = 0 ;

static int		yyLineNr = 1 ;
static char		yyFileName[264] ;
static McString*	currentString ;

static void msg( const char* str, const char* str2=NULL )
{
#ifdef _DEBUG
	if( verb )
	{
		printf( "%s(%d):\t%s", yyFileName, yyLineNr, str ) ;
		if( str2 ) printf( "%s", str2 ) ;
		printf( "\n" ) ;
	}
#endif
}

static void lineCount()
{
	for( const char* c = yytext ; *c ; ++c )
		yyLineNr += (*c == '\n') ;
}


static void addType( Entry* current )
{
   if( !current->name.empty() ) {
      if( !current->type.empty() ) current->type += ' ' ;
      current->type += current->name ;
      current->name.clear() ;
   }
   if( !current->args.empty() ) {
      if( !current->type.empty() ) current->type += ' ' ;
      current->type += current->args ;
      current->args.clear() ;
   }
}

/* -----------------------------------------------------------------
 */
#undef	YY_INPUT
#define	YY_INPUT(buf,result,max_size) result = yyread(buf,max_size);

static int yyread(char *buf,int max_size)
{
	int c = 0;
	while( c < max_size && inputString[inputPosition] )
	{
		*buf = inputString[inputPosition++] ;
		c++; buf++;
	}
	return c;
}

%}

%x	Cxx_Memo
%x	SubDoc
%x	SubDocComment
%x	SubDocCppComment
%x	Doc
%x	VerbDoc
%x	Define
%x	DefineEnd
%x	DefineEnded

%x	ClassName
%x	TypedefName
%x	Bases

%x	NextSemi
%x	FindMembers
%x	Function
%x	Operator
%x	Throws

%x	Array
%x	Round
%x	Curly
%x	SkipCurly
%x	SkipInits
%x	SkipCPP
%x	Sharp

%x	Comment
%x	SkipComment
%x	SkipCxxComment

%x	CppDoc
%x	CppMemo
%x	See
%x	Args
%x	Type
%x	Author
%x	Name
%x	Version
%x	Param
%x	Memo
%x	Return
%x	Exception

%%

<*>\x06[^\x06]*\x06			{ 
					  yyLineNr = 1 ; 
					  int i;
					  for( i = 0 ; yytext[i+1] != 6 ; i++ )
						yyFileName[i] = yytext[i+1] ;
					  yyFileName[i] = 0 ;
					}

<NextSemi>[;,]				{ BEGIN( FindMembers ) ; }
<NextSemi>[ \t]*\"[^\n"]*\"		
<NextSemi>"{"				{ BEGIN( SkipCurly ) ; }

<FindMembers>[ \t]*"public:"[ \t\r\n]*	{ 
					  current->protection = protection = PUBL ;
					  lineCount() ;
					}
<FindMembers>[ \t]*"protected:"[ \t\r\n]*	{ 
					  current->protection = protection = PROT ;
					  lineCount() ;
					}
<FindMembers>[ \t]*"private:"[ \t\r\n]*	{ 
					  current->protection = protection = PRIV ;
					  lineCount() ;
					}

<FindMembers>[ \t]*"typedef"[ \t\r\n]+"class"[ \t\r\n]+	{
					  msg( "found class    ", current->name ) ;
					  current->section = TYPEDEF_SEC ;
					  current->lineNo = yyLineNr;
					  current->type = "typedef class" ;
					  current->name.clear() ;
					  lineCount() ;
					}

<FindMembers>[ \t]*"typedef"[ \t\r\n]+"struct"[ \t\r\n]+	{
					  msg( "found struct   ", current->name ) ;
					  current->section = STRUCT_SEC ;
					  current->lineNo = yyLineNr;
					  current->type = "typedef struct" ;
					  current->name.clear() ;
					  lastTContext = 1;
					  lineCount() ;
				  	  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"typedef"[ \t\r\n]+"struct"[ \t\r\n]*"{"[ \t\r\n]*	{
					  msg( "found struct   ", current->name ) ;
					  current->section = STRUCT_SEC ;
					  current->lineNo = yyLineNr;
					  current->type = "typedef struct" ;
					  current->name.clear() ;
					  lineCount() ;
					  current->file = yyFileName ;
					  current->startLine = yyLineNr ;
					  lastTContext = 1;
				  	  BEGIN( Curly ) ;
					}

<FindMembers>[ \t]*"typedef"[ \t\r\n]+"enum"[ \t\r\n]+	{
					  msg( "found enum     ", current->name ) ;
					  current->section = ENUM_SEC ;
					  current->lineNo = yyLineNr;
					  current->type = "typedef enum" ;
					  current->name.clear() ;
					  lineCount() ;
				  	  BEGIN( ClassName ) ;
					}

<FindMembers>[ \t]*"typedef"[ \t\r\n]+"union"[ \t\r\n]+	{
					  msg( "found union    ", current->name ) ;
					  current->section = UNION_SEC ;
					  current->lineNo = yyLineNr;
					  current->type = "typedef union" ;
					  current->name.clear() ;
					  lineCount() ;
				  	  BEGIN( ClassName ) ;
					}

<FindMembers>[ \t]*"interface"[ \t\r\n]+ { 
					  current->section = CLASS_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "interface" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"class"[ \t\r\n]+	{ 
					  current->section = CLASS_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "class" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"struct"[ \t\r\n]+	{ 
					  msg( "found struct    ", current->name ) ;
					  current->section = STRUCT_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "struct" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"enum"[ \t\r\n]+	{ 
					  msg( "found enum    ", current->name ) ;
					  current->section = ENUM_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "enum" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"union"[ \t\r\n]+	{ 
					  current->section = UNION_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "union" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}

<FindMembers>"operator"/[^a-z_A-Z0-9]	{ 
					  addType( current ) ;
					  current->lineNo = yyLineNr;
					  current->name  = yytext ;
					  BEGIN( Operator ) ;
					}
<Operator>[^(]*				{ 
					  current->name += yytext ;
					  current->lineNo = yyLineNr;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>"extern"[ \t]*[a-z_A-Z~:.0-9]+ { 
					  addType( current ) ;
				          current->lineNo = yyLineNr;
					  current->name  = yytext ;
					}
<FindMembers>[a-z_A-Z~:.0-9]+		{ 
					  addType( current ) ;
				          current->lineNo = yyLineNr;
					  current->name  = yytext ;
					}

<FindMembers>^[ \t]*"#"			{ lineCount() ; BEGIN( SkipCPP ) ; }
<SkipCPP>.
<SkipCPP>"//"
<SkipCPP>\\\n				{ yyLineNr++ ; }
<SkipCPP>\n				{ 
					  yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>^[ \t]*"using"[ \t]+	{ 
					  current->type = "using " ;
					  BEGIN( Define ) ;
					}
<FindMembers>^[ \t]*"#"[ \t]*define[ \t]+	{ 
					  current->type = "#define" ;
					  BEGIN( Define ) ;
					}
<Define>[a-z_A-Z:.0-9]+			{ 
					  current->name = yytext ;
					  current->lineNo = yyLineNr;
					  BEGIN( DefineEnd ) ;
					}
<DefineEnd>[ \t]			{ BEGIN( DefineEnd ) ; }
<DefineEnd>"("[^)]*")"			{ 
					  current->args = yytext ;
					  BEGIN( DefineEnded ) ;
					}
<DefineEnd,DefineEnded>"\\\n"		{ yyLineNr++ ; }
<DefineEnd,DefineEnded>"\\\r\n"		{ yyLineNr++ ; }
<DefineEnd,DefineEnded>"\n"		{ 
					  if( current->done == onlyDocs )
					  {
					    current->section = MACRO_SEC ;
					    current->lineNo = yyLineNr;
					    msg( "found macro    ", current->name ) ;
					    current_root->addSubEntry( current ) ;
					    current = new Entry ;
					    current->protection = protection ;
					  }
					  else
					  {
					    current->name.clear() ;
					    current->type.clear() ;
					  }
					  yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>[*&]+			{ 
					  current->name += yytext ; 
					  current->lineNo = yyLineNr;
					}

<FindMembers>[;=,]			{ 
					  BEGIN( FindMembers ) ;
					  if( current->lineNo == 0 ) current->lineNo = yyLineNr + 1;
					  msg( "found variable ", current->name ) ;
					  if( current->done == onlyDocs )
					  {
					    if( current->section != TYPEDEF_SEC )
						    current->section = VARIABLE_SEC ;
					    current_root->addSubEntry( current ) ;
					    current = new Entry ;
					    current->protection = protection ;
					  }
					  else
					  {
					    current->section = EMPTY_SEC ;
					    current->name.clear() ;
					    current->type.clear() ;
					    current->args.clear() ;
					  }
					  if( *yytext == '=' )
					    BEGIN( NextSemi ) ;
					}

<FindMembers>"["			{ 
					  current->args += yytext ;
					  sharpCount=1;
					  BEGIN( Array ) ;
					}
<Array>"]"				{ 
					  current->args += *yytext ;
					  if( --sharpCount<=0 )
						BEGIN( FindMembers ) ;
					}
<Array>"["				{ 
					  current->args += *yytext ;
					  sharpCount++;	
					}
<Array>.				{ current->args += *yytext ; }

<FindMembers>"<"			{ 
					  addType( current ) ;
					  current->type += *yytext ;
					  sharpCount=1;
					  BEGIN( Sharp ) ;
					}
<Sharp>">"				{ 
					  current->type += *yytext ;
					  if (--sharpCount<=0)
	                                     BEGIN( FindMembers ) ;
					}
<Sharp>"<"				{ 
					  current->type += *yytext ;
					  sharpCount++;	
					}
<Sharp>.				{ current->type += *yytext ; }

<Curly>[^\n{}\"/]*			{ current->program += yytext ; }
<Curly>"//".*[\r]*\n			{ current->program += yytext ; ++yyLineNr ; }
<Curly>"//"[\r]*\n			{ current->program += yytext ; ++yyLineNr ; }
<Curly>\"[^\n"]*\"			{ current->program += yytext ; }
<Curly>"/*"\**[ \t]*			{ 
					  current->program += yytext ;
					  lastContext = Curly ;
					  BEGIN( Comment ) ;
					}
<Curly>"/*"\**[ \t]*\n			{ 
					  current->program += yytext ;
					  ++yyLineNr ;
					  lastContext = Curly ;
					  BEGIN( Comment ) ;
					}
<Curly>"{"				{ 
					  current->program += yytext ;
					  ++bracketCount ;
					}
<Curly>"}"				{ 
					  if( bracketCount > 0 )
					  {
						current->program += yytext ;
						--bracketCount ;
					  }
					  else
					  {
					    current->args += " {...}";

					    if( lastTContext ) 
					    {
					      BEGIN( TypedefName ) ;
					    }
					    else if( current->done == onlyDocs )
					    {
					      int section = current->section;
					      current_root->addSubEntry( current ) ;
					      current = new Entry ;
					      current->protection = protection ;
					      if( section == CLASS_SEC ) current->protection = GLOB ;
					      BEGIN( NextSemi ) ;
					    }
					    else
					    {
					      current->name.clear() ;
					      current->type.clear() ;
					      current->args.clear() ;
					      current->section = EMPTY_SEC ;
					      BEGIN( NextSemi ) ;
					    }
					  }
					}
<Curly>\n				{ 
					  current->program += yytext ;
					  yyLineNr++ ;
					}
<Curly>.				{ current->program += yytext ; }

<FindMembers>"("			{ 
					  current->args = yytext ;
					  BEGIN( Round ) ;
					}
<Round>"("				{ 
					  current->args += *yytext ;
					  ++bracketCount ;
					}
<Round>")"				{ 
					  current->args += *yytext ;
					  if( bracketCount > 0 )
					    --bracketCount ;
					  else
					    BEGIN( Function ) ;
					}
<Round>[ \t\r\n]*","[ \t\r\n]*		{ lineCount() ; current->args += ", " ; }
<Round>[ \t\r\n]+			{ lineCount() ; current->args += " " ; }
<Round>.				{ current->args += *yytext ; }

<Function>[ \t]*"const"[ \t\r\n]*	{ lineCount() ; current->args += " const" ; }

<Function>[ \t]*"throw"[ \t\r\n]*"("	{ 
					  current->args += " throw(" ;
					  lineCount() ;
				  	  BEGIN( Round ) ;
					}
<Function>"("				{ 
					  current->type += current->name ;
					  current->name  = current->args ;
					  current->args  = yytext ;
					  current->lineNo = yyLineNr;
					  BEGIN( Round ) ;
					}
<Function>[:;{]				{ 
					  msg( "found method   ", current->name ) ;
					  if( current->done == onlyDocs )
					  {
					    current->section = FUNCTION_SEC ;
					    current_root->addSubEntry(current);
					    current = new Entry ;
					    current->protection = protection ;
					  }
					  else
					  {
					    current->name.clear() ;
					    current->type.clear() ;
					    current->args.clear() ;
					  }
					  if( *yytext == '{' )
					      BEGIN( SkipCurly ) ;
					  else if( *yytext == ':' )
					      BEGIN( SkipInits ) ;
					  else
					      BEGIN( FindMembers ) ;
					}

<SkipInits>"{"				{ BEGIN( SkipCurly ) ; }

<SkipCurly>[ \t\r\n]*"/""/"+[\r]*\n	{ lineCount() ; }
<SkipCurly>\"[^\n"]*\"
<SkipCurly>"{"				{ ++bracketCount ; }
<SkipCurly>"}"				{ 
					  if( bracketCount > 0 )
					    --bracketCount ;
					  else
					    BEGIN( FindMembers ) ;
					}

<Bases,ClassName>";"			{ 
					  if( current->done == onlyDocs )
					  {
					    current->section = VARIABLE_SEC ;
					    current_root->addSubEntry(current);
					    current = new Entry ;
					    current->protection = protection ;
					  }
					  else
					  {
					    current->section = EMPTY_SEC ;
					    current->type.clear() ;
					    current->name.clear() ;
					    current->args.clear() ;
					  }
					  BEGIN( FindMembers ) ;
					}
<ClassName>[a-z_A-Z0-9\[\]*&]+		{ 
					  if( !current->type.empty() && !current->name.empty() ) current->type += " " ;
					  current->type += current->name ;
				  	  current->name = yytext ;
					  current->lineNo = yyLineNr;
					}
<ClassName>[ \t]*":"[ \t]*		{ 
					  current->args = " :" ;
					  BEGIN( Bases ) ;
					}
<Bases,ClassName>[ \t]*"{"[ \t]*	{ 
					  current->file = yyFileName ;
				  	  if( current->lineNo == 0 ) current->lineNo = yyLineNr;
					  current->startLine = yyLineNr ;
				  	  msg( "found class    ", current->name ) ;
				  	  BEGIN( Curly ) ;
					}
<Bases>[a-z_A-Z*.<>0-9]+		{ 
					  current->extends.append( new McString(yytext) ) ;
					  if( !current->args.empty() ) current->args += " " ;
					  current->args += yytext ;
					}
<Bases>","				{ current->args += ',' ; }

<Comment>\n				{ 
					  current->program += yytext ;
					  yyLineNr++ ;
					}
<Comment>.				{ current->program += yytext ; }
<Comment>.*"*/"				{ 
					  current->program += yytext ;
					  BEGIN( Curly ) ;
					}

<TypedefName>[a-z_A-Z0-9\[\]*&]+	{ 
					  if( !current->type.empty() && !current->name.empty() ) current->type += " " ;
					  current->type += current->name ;
				  	  current->name = yytext ;
					  current->lineNo = yyLineNr;
		
				          lastTContext = 0;
					  msg( "found typedef  ", current->name ) ;

				          int section = current->section;
					  current_root->addSubEntry( current ) ;
					  current = new Entry ;
					  current->protection = protection ;
					  if( section == CLASS_SEC ) current->protection = GLOB ;

					  BEGIN( NextSemi ) ;
					}

<TypedefName>[;,]+			{
				          lastTContext = 0;
					  msg( "found typedef  ", current->name ) ;

					  int section = current->section;
					  current_root->addSubEntry( current ) ;
					  current = new Entry ;
					  current->protection = protection ;
					  if( section == CLASS_SEC ) current->protection = GLOB ;

				          if( *yytext == ',' )
					    BEGIN( NextSemi ) ;
					  else
					    BEGIN( FindMembers ) ;
					}


<FindMembers>[ \t\r\n]*"///"/[^/][ \t]*	{ 
					  lineCount() ;
					  if( current->doc.length() > 0
						  ||  current->memo.length() > 0 )
					  {
						msg( "found explicit entry" ) ;
						current->lineNo = yyLineNr;
						current_root->addSubEntry( current ) ;
						current = new Entry ;
					  }
					  current->done = onlyDocs ;
					  BEGIN( Cxx_Memo ) ;
					}
<Cxx_Memo>.*				{ current->memo += yytext ; }
<Cxx_Memo>\n				{ 
					  yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>[ \t\r\n]*"/""/"+[\r]*\n	{ lineCount() ; }
<FindMembers>[ \t\r\n]*"/*""*"+"/"	{ lineCount() ; }
<FindMembers>[ \t\r\n]*"/***"		{ 
					  lineCount() ;
					  lastCContext = YY_START ;
					  BEGIN( SkipComment ) ;
					}
<FindMembers>[ \t\r\n]*"/**"[ \t\r]*	{ 
					  lineCount() ;
					  if( current->doc.length() > 0
					      ||  current->memo.length() > 0 )
					  {
					     msg( "found explicit entry" ) ;
					     current->lineNo = yyLineNr;
					     current_root->addSubEntry( current ) ;
					     current = new Entry ;
					  }
					  current->done = onlyDocs ;
					  BEGIN( Doc ) ;
					}
<VerbDoc,Doc>\n[ \t]*"*"+"/"		{ yyLineNr++; BEGIN( FindMembers ) ; }
<Doc>"\\begin{verbatim}"		{ 
					  current->doc += yytext ;
					  BEGIN( VerbDoc ) ;
					}
<VerbDoc>\n				{ 
					  current->doc += '\n' ;
					  yyLineNr++ ;
					}
<VerbDoc>"\\end{verbatim}"		{ 
					  current->doc += yytext ;
					  BEGIN( Doc ) ;
					}
<Doc>\r
<Doc>\n[ \t]*"*"*[ \t]*			{ 
					  if( !current->doc.empty() ) current->doc += '\n' ;
					  yyLineNr++ ;
					}
<VerbDoc,Doc>.				{ current->doc += *yytext ; }
<VerbDoc,Doc>"//"			{ current->doc += yytext ; }
<VerbDoc,Doc>"/*"			{ current->doc += yytext ; }
<VerbDoc,Doc>"*/"			{ BEGIN( FindMembers ) ; }

<FindMembers>("//@{".*\n)|("/*@{"[^*]*\*+"/")	{ 
					  lineCount() ;
					  current->file = yyFileName ;
					  if( current->lineNo == 0 ) current->lineNo = yyLineNr;
					  current->startLine = yyLineNr ;
					  BEGIN( SubDoc ) ;
					}
<SubDoc>"/*"				{ 
					  current->program += yytext ;
					  BEGIN( SubDocComment ) ;
					}
<SubDoc>"//"				{ 
					  current->program += yytext ;
					  BEGIN( SubDocCppComment ) ;
					}
<SubDoc>.				{ current->program += *yytext ; }
<SubDoc>\n				{ 
					  current->program += *yytext ;
					  yyLineNr++ ;
					}
<SubDoc>("//@{".*\n)|("/*@{"[^*]*\*+"/")	{ 
					  lineCount() ;
					  current->program += yytext ;
					  ++bracketCount ;
					}
<SubDoc>("//@}".*\n)|("/*@}"[^*]*\*+"/") 	{ 
					  lineCount() ;
					  if( bracketCount > 0 )
					  {
					    current->program += yytext ;
					    --bracketCount ;
					  }
					  else
					  {
					    msg( "found explicit subentry" ) ;
					              current->lineNo = yyLineNr;
					    current_root->addSubEntry( current ) ;
					    current = new Entry ;
					    current->protection = protection ;
					    BEGIN( FindMembers ) ;
					  }
					}

<SubDocComment,SubDocCppComment>"/*"	{ current->program += yytext ; }
<SubDocComment,SubDocCppComment>"//"	{ current->program += yytext ; }
<SubDocComment>.			{ current->program += yytext ; }
<SubDocComment>\n			{ 
					  current->program += yytext ;
					  ++yyLineNr ;
					}
<SubDocComment>"*/"			{ 
					  current->program += yytext ;
					  BEGIN( SubDoc ) ;
					}

<SubDocCppComment>.			{ current->program += yytext ; }
<SubDocCppComment>\n			{ 
					  current->program += yytext ;
					  ++yyLineNr ;
					  BEGIN( SubDoc ) ;
					}

<CppMemo>[^.]				{ 
					  current->memo += yytext ;
					  current->doc  += yytext ; 
					}
<CppMemo>"."[ \t\r\n]*			{ 
					  BEGIN( CppDoc ) ;
                      			  lineCount();
					  current->doc  += yytext ; 
					}
<CppDoc>(\n|.)				{ 
					  current->doc  += *yytext ; 
	                  		  lineCount();
					}
<CppMemo>"/"("*"|"/")			{ current->memo += yytext ; }
<CppDoc>"/"("*"|"/")			{ current->doc  += yytext ; }

<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@see"[ \t\r\n]*	{
					  lineCount() ;
					  current->see.append( new McString ) ;
					  BEGIN( See ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@author"[ \t\r\n]*	{
					  lineCount() ;
					  BEGIN( Author ) ; }
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@version"[ \t\r\n]*	{
					  lineCount() ;
					  BEGIN( Version ) ; }
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@param"[ \t\r\n]*	{
					  lineCount() ;
					  current->param.append( new McString  ) ;
					  BEGIN( Param ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@exception"[ \t\r\n]*	{
					  lineCount() ;
					  current->exception.append( new McString ) ;
					  BEGIN( Exception ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@return"[ \t\r\n]*	{
					  lineCount() ;
					  BEGIN( Return ) ; }
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@name"[ \t]*	{
					  current->name.clear() ;
					  current->section = MANUAL_SEC ;
					  BEGIN( Name ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@memo"[ \t\r\n]*	{
					  lineCount() ;
					  current->memo.clear() ;
					  BEGIN( Memo ) ;
					}

<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@type"[ \t\r\n]*	{
					  lineCount() ;
					  current->type.clear() ;
					  BEGIN( Type ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@args"[ \t\r\n]*	{
					  lineCount() ;
					  current->args.clear() ;
					  BEGIN( Args ) ;
					}

<Name>.*				{ current->name = yytext ; }
<Name>\n				{ ++yyLineNr ; BEGIN( CppDoc ) ; }

<Type>.*				{ current->type = yytext ; }
<Type>\n				{ ++yyLineNr ; BEGIN( CppDoc ) ; }

<Args>.*				{ current->args = yytext ; }
<Args>\n				{ ++yyLineNr ; BEGIN( CppDoc ) ; }

<Author>.				{ current->author += *yytext ; }
<Version>.				{ current->version += *yytext ; }
<See>.					{ *(current->see.last()) += *yytext ; }
<Param>.				{ *(current->param.last()) += *yytext ; }
<Exception>.				{ *(current->exception.last()) += *yytext ; }
<Memo>.					{ current->memo += *yytext ; }
<Return>.				{ current->retrn += *yytext ; }

<Author>\n[ \t\r\n]*			{ current->author  += '\n' ; lineCount() ; }
<Version>\n[ \t\r\n]*			{ current->version += '\n' ; lineCount() ; }
<See>\n[ \t\r\n]*			{ *(current->see.last()) += '\n' ; lineCount() ; }
<Param>\n[ \t\r\n]*			{ *(current->param.last()) += '\n' ; lineCount() ; }
<Exception>\n[ \t\r\n]*			{ *(current->exception.last()) += '\n' ; lineCount() ; }
<Memo>\n[ \t\r\n]*			{ current->memo += *yytext ; lineCount() ; }
<Return>\n[ \t\r\n]*			{ current->retrn += *yytext ; lineCount() ; }

<*>.
<*>\n					{ yyLineNr++ ; }
<*>"/*"					{ 
					  lastCContext = YY_START ;
					  BEGIN( SkipComment ) ;
					}
<SkipComment>"//"
<SkipComment>[ \t]*"*/"			{ BEGIN( lastCContext ) ; }
<*>"//"					{ 
					  lastCContext = YY_START ;
					  BEGIN( SkipCxxComment ) ;
					}
<SkipCxxComment>.*\n			{ 
					  yyLineNr++ ;
					  BEGIN( lastCContext ) ;
					}
%%


/*@ ----------------------------------------------------------------------------
 */
void parseDoc(Entry* rt)
{
	if( rt->doc.length() )
	{
		Entry* _current = current ;
		rt->program = rt->doc ;
		rt->doc.clear() ;
		inputString   = rt->program ;
		inputPosition = 0 ;
		current = rt ;
		cpprestart( cppin ) ;
		if( rt->memo.length() > 0 )
			BEGIN( CppDoc ) ;
		else
			BEGIN( CppMemo ) ;
		cpplex() ;
		rt->program.clear() ;
		if( rt->memo.length()+2 >= rt->doc.length() )
			rt->doc.clear() ;
		current = _current ;
	}
}

void callcpplex()
{
	cpplex() ;
	if( (current->name.length()  ||  current->program.length()  ||
		current->memo.length()  ||  current->doc.length() )
		&&  current->done == onlyDocs )
	{
		if( current->section == EMPTY_SEC )
			current->section = VARIABLE_SEC ;
		current_root->addSubEntry( current ) ;
		current = new Entry ;
		current->protection = protection ;
	}
}

void parseCppClasses(Entry* rt)
{
    if( rt == 0 ) return ;
    for( Entry *cr = rt->sub ; cr ; cr = cr->next )
    {
		if( cr->program.length() > 0 )
		{
#ifdef _DEBUG
			if (verb)
				printf("Scanning substuff of (%s) %s\n",
				   (const char *)cr->type,
				   (const char *)cr->name);
#endif
			inputString   = cr->program ;
			inputPosition = 0 ;
			cpprestart( cppin ) ;
			BEGIN( FindMembers ) ;
			current_root = cr ;
			strcpy( yyFileName, cr->file ) ;
			yyLineNr = cr->startLine ;
			current->type.clear() ;
			current->name.clear() ;
			current->args.clear() ;
			current->section = EMPTY_SEC ;
			if( cr->section == CLASS_SEC )
				current->protection = protection = PRIV ;
			else if( cr->section == STRUCT_SEC )
				current->protection = protection = PUBL ;
			else if( cr->section == UNION_SEC )
				current->protection = protection = PUBL ;
			else
				current->protection = protection = GLOB ;
			callcpplex() ;
			cr->program.clear() ;
		}
		parseCppClasses( cr ) ;
    }
    parseDoc( rt ) ;
}

void parseCpp(Entry* rt)
{
	current_root  = rt ;
	global_root   = rt ;
	protection    = GLOB ;
	current       = new Entry ;
	yyLineNr      = 1 ;
	yyFileName[0] = '\0' ;
	bracketCount  = 0 ;
	sharpCount    = 0 ;
	lastTContext  = 0 ;

	inputString   = rt->program ;
	inputPosition = 0 ;
	cpprestart( cppin ) ;
	BEGIN( FindMembers ) ;
	callcpplex() ;
	rt->program.clear() ;
	parseCppClasses( rt ) ;

	delete current;
}

extern "C" {
	int	cppwrap()	{ return 1 ; }
};
