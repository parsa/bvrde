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
static int		elseCount    = 0 ;
static Entry*		current_root = 0 ;
static Entry*		current      = 0 ;
static Entry*		previous     = 0 ;
static McString         nmspc        = "";

static int		yyLineNr = 1 ;

static void msg( const char* str, const char* str2=NULL )
{
#ifdef _DEBUG
	if( verb )
	{
		printf( "%d:\t%s", yyLineNr, str ) ;
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
%x	Doc
%x	VerbDoc
%x	Define
%x	DefineEnd
%x	DefineEnded

%x	ClassName
%x      NamespaceName
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
%x	SkipRound
%x	SkipInits
%x	SkipDefine
%x	SkipElse
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

<NextSemi>[;,]				{ BEGIN( FindMembers ) ; }
<NextSemi>\"[^\n\"]*\"
<NextSemi>"{"				{ BEGIN( SkipCurly ) ; }

<FindMembers>"public:"			{ 
					  current->protection = protection = PUBL ;
					  lineCount() ;
					}
<FindMembers>"protected:"		{ 
					  current->protection = protection = PROT ;
					  lineCount() ;
					}
<FindMembers>"private:"			{ 
					  current->protection = protection = PRIV ;
					  lineCount() ;
					}

<FindMembers>"typedef"[ \t\r\n]+"class"[ \t\r\n]+	{
					  msg( "found class      ", current->name ) ;
					  current->section = TYPEDEF_SEC ;
					  current->lineNo = yyLineNr ;
					  current->type = "typedef class" ;
					  current->name.clear() ;
					  lineCount() ;
					}

<FindMembers>"typedef"[ \t\r\n]+"struct"[ \t\r\n]+	{
					  msg( "found struct     ", current->name ) ;
					  current->section = STRUCT_SEC ;
					  current->lineNo = yyLineNr ;
					  current->type = "typedef struct" ;
					  current->name.clear() ;
					  lastTContext = 1;
					  lineCount() ;
				  	  BEGIN( ClassName ) ;
					}
<FindMembers>"typedef"[ \t\r\n]+"struct"[ \t\r\n]*"{"	{
					  msg( "found struct     ", current->name ) ;
					  current->section = STRUCT_SEC ;
					  current->lineNo = yyLineNr ;
					  current->type = "typedef struct" ;
					  current->name.clear() ;
					  lineCount() ;
					  current->startLine = yyLineNr ;
					  lastTContext = 1;
				  	  BEGIN( Curly ) ;
					}

<FindMembers>"typedef"[ \t\r\n]+"enum"[ \t\r\n]+	{
					  msg( "found enum       ", current->name ) ;
					  current->section = ENUM_SEC ;
					  current->lineNo = yyLineNr ;
					  current->type = "typedef enum" ;
					  current->name.clear() ;
					  lineCount() ;
					  current->startLine = yyLineNr ;
					  lastTContext = 1;
				  	  BEGIN( ClassName ) ;
					}

<FindMembers>"typedef"[ \t\r\n]+"enum"[ \t\r\n]*"{"	{
					  msg( "found struct     ", current->name ) ;
					  current->section = ENUM_SEC ;
					  current->lineNo = yyLineNr ;
					  current->type = "typedef enum" ;
					  current->name.clear() ;
					  lineCount() ;
					  current->startLine = yyLineNr ;
					  lastTContext = 1;
				  	  BEGIN( Curly ) ;
					}

<FindMembers>"typedef"[ \t\r\n]+"union"[ \t\r\n]+	{
					  msg( "found union      ", current->name ) ;
					  current->section = UNION_SEC ;
					  current->lineNo = yyLineNr ;
					  current->type = "typedef union" ;
					  current->name.clear() ;
					  lineCount() ;
				  	  BEGIN( ClassName ) ;
					}

<FindMembers>"interface"[ \t\r\n]+	{ 
					  current->section = CLASS_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "interface" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>"class"[ \t\r\n]+		{ 
					  msg( "found class       ", current->name ) ;
					  current->section = CLASS_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "class" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>"struct"[ \t\r\n]+		{ 
					  msg( "found struct      ", current->name ) ;
					  current->section = STRUCT_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "struct" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>"enum"[ \t\r\n]+		{ 
					  msg( "found enum      ", current->name ) ;
					  current->section = ENUM_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "enum" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>"union"[ \t\r\n]+		{ 
					  current->section = UNION_SEC ;
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  if( !current->type.empty() ) current->type += " ";
					  current->type += "union" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>("struct"|"class")[ \t\r\n]+[a-z_A-Z~:.0-9]+";"	{ 
					  msg( "found forward declare" ) ;
					  lineCount() ;
					  BEGIN( FindMembers ) ;
					}
<FindMembers>"operator"/[^a-z_A-Z0-9]	{ 
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  current->name  = yytext ;
					  BEGIN( Operator ) ;
					}

<FindMembers>"using"[ \t\r\n]+.*";"	{
					  msg( "found using namespace" ) ;
					  lineCount() ;
					  BEGIN( FindMembers ) ;
					}
<FindMembers>"namespace"[ \t\r\n]+	{
					  msg( "found namespace" ) ;
					  lineCount() ;
					  BEGIN( NamespaceName ) ;
					}
<NamespaceName>[a-z_A-Z0-9\[\]\:*&]+	{ 
				  	  msg( "  has name       ", yytext ) ;
					  if( !nmspc.empty() ) nmspc += "::" ;
					  nmspc += yytext ;
				  	  current->name = yytext ;
					  current->lineNo = yyLineNr ;
					  current->startLine = yyLineNr ;
					  current->section = NAMESPACE_SEC;
					  current_root->addSubEntry( current ) ;
					  current = new Entry ;
					  current->protection = protection ;
					  BEGIN( FindMembers ) ;
					}
<FindMembers>"}"			{
					  if( !nmspc.empty() ) {
					    if( nmspc.find("::") != McString::npos ) nmspc = nmspc.substr(0, nmspc.find_last_of(":")-2).c_str();
					    else nmspc = "";
					  }
					  BEGIN( FindMembers ) ;
					}

<FindMembers>"__"("declspec"|"attribute"|"attribute__")"("	{ 
					  BEGIN( SkipRound ) ;
					}

<Operator>[^(]*				{ 
					  current->name += yytext ;
					  current->lineNo = yyLineNr ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>"extern"[ \t]*[a-z_A-Z~:.0-9]+ { 
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  current->name  = yytext ;
					}
<FindMembers>[a-z_A-Z~:.0-9]+		{ 
					  addType( current ) ;
					  current->lineNo = yyLineNr ;
					  current->namespc = nmspc;
					  current->name  = yytext ;
					}

<FindMembers>^[ \t]*"#"			{ 
					  msg( "found compiler-directive" ) ;
					  lineCount() ; 
					  BEGIN( SkipDefine ) ; 
					}
<SkipDefine>"//"[^\n]*
<SkipDefine>\\[ \r\t]*\n		{ yyLineNr++ ; }
<SkipDefine>\n				{ 
					  yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>^[ \t]*"#"("else"|"elif")	{ 
					  msg( "found #ELSE" ) ; 
					  BEGIN( SkipElse ) ; 
					}
<SkipElse>"//"[^\n]*
<SkipElse>^[ \t]*"#"("if"|"ifdef"|"ifndef")	{ elseCount++ ; }
<SkipElse>^[ \t]*"#endif"		{ if( elseCount > 0 )
					    --elseCount ;
					  else
					    BEGIN( FindMembers ) ;
					}

<FindMembers>^[ \t]*"#"[ \t]*define[ \t]+	{ 
					  current->type = "#define" ;
					  BEGIN( Define ) ;
					}
<Define>[a-z_A-Z:.0-9]+			{ 
					  current->name = yytext ;
					  current->lineNo = yyLineNr ;
					  BEGIN( DefineEnd ) ;
					}
<DefineEnd>[ \t]			{ BEGIN( DefineEnd ) ; }
<DefineEnd>"("[^)]*")"			{ 
					  current->args = yytext ;
					  BEGIN( DefineEnded ) ;
					}
<DefineEnd,DefineEnded>\\\n		{ yyLineNr++ ; }
<DefineEnd,DefineEnded>\\\r\n		{ yyLineNr++ ; }
<DefineEnd,DefineEnded>\n		{ 
					  current->section = MACRO_SEC ;
					  current->lineNo = yyLineNr ;
					  msg( "found macro      ", current->name ) ;
					  current_root->addSubEntry( current ) ;
					  current = new Entry ;
					  current->protection = protection ;
					  yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>[*&]+			{ 
					  current->name += yytext ; 
					  current->lineNo = yyLineNr ;
					}

<FindMembers>[;=,]			{ 
					  BEGIN( FindMembers ) ;
					  if( current->lineNo == 0 ) current->lineNo = yyLineNr + 1;
					  msg( "found variable   ", current->name ) ;
					  if( current->section != TYPEDEF_SEC )
					    current->section = VARIABLE_SEC ;
					  current_root->addSubEntry( current ) ;
					  current = new Entry ;
					  current->protection = protection ;
					  if( *yytext == '=' )
					    BEGIN( NextSemi ) ;
					}

<FindMembers>"["			{ 
					  current->args += yytext ;
					  BEGIN( Array ) ;
					}
<Array>"]"				{ 
					  current->args += *yytext ;
					  if( sharpCount > 0 )
					    --sharpCount ;
					  else
					    BEGIN( FindMembers ) ;
					}
<Array>"["				{ 
					  current->args += *yytext ;
					  ++sharpCount ;	
					}
<Array>.				{ current->args += *yytext ; }

<FindMembers>"<"			{ 
					  addType( current ) ;
					  current->type += *yytext ;
					  BEGIN( Sharp ) ;
					}
<Sharp>">"				{ 
					  current->type += *yytext ;
					  if( sharpCount > 0 )
					    --sharpCount ;
					  else
					    BEGIN( FindMembers ) ;
					}
<Sharp>"<"				{ 
					  current->type += *yytext ;
					  ++sharpCount ;
					}
<Sharp>.				{ current->type += *yytext ; }

<Curly>[^\n{}\"/]*			{ current->program += yytext ; }
<Curly>"//"[^\n]*\n			{ current->program += yytext ; ++yyLineNr ; }
<Curly>\"[^\n\"]*\"			{ current->program += yytext ; }
<Curly>"/*"\**				{ 
					  current->program += yytext ;
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
					    else
					    {
					      int section = current->section;
					      current_root->addNextEntry( current ) ;
					      current = new Entry ;
					      current->protection = protection ;
					      if( section == CLASS_SEC ) current->protection = GLOB ;
					      BEGIN( NextSemi ) ;
					    }
					  }
					}
<Curly>\n				{ 
					  current->program += yytext ;
					  yyLineNr++ ;
					}
<Curly>.				{ current->program += yytext ; }

<FindMembers>[ \t\r\n]*"("[ \t\r\n]*	{ 
					  lineCount() ; 
					  current->args = "(" ;
					  BEGIN( Round ) ;
					}
<Round>"("				{ 
					  current->args += *yytext ;
					  ++bracketCount ;
					}
<Round>[ \t\r\n]*")"			{ 
					  lineCount() ; 
					  current->args += ')' ;
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
					  current->lineNo = yyLineNr ;
					  BEGIN( Round ) ;
					}
<Function>[:;{]				{ 
					  msg( "found method     ", current->name ) ;
					  current->section = FUNCTION_SEC ;
					  if( *yytext == '{' && current->protection == GLOB ) current->section = IMPL_SEC ;
					  current_root->addSubEntry(current);
					  current = new Entry ;
					  current->protection = protection ;
					  if( *yytext == '{' )
					    BEGIN( SkipCurly ) ;
					  else if( *yytext == ':' )
					    BEGIN( SkipInits ) ;
					  else
					    BEGIN( FindMembers ) ;
					}

<SkipInits>"{"				{ BEGIN( SkipCurly ) ; }
<SkipInits>";"				{ BEGIN( FindMembers ) ; }

<SkipCurly>\"[^\n\"]*\"
<SkipCurly>"{"				{ ++bracketCount ; }
<SkipCurly>"}"				{ 
					  if( bracketCount > 0 )
					    --bracketCount ;
					  else
					    BEGIN( FindMembers ) ;
					}

<SkipRound>\"[^\n\"]*\"
<SkipRound>"("				{ ++bracketCount ; }
<SkipRound>")"				{ 
					  if( bracketCount > 0 )
					    --bracketCount ;
					  else
					    BEGIN( FindMembers ) ;
					}

<Bases,ClassName>";"			{ 
					  current->section = VARIABLE_SEC ;
					  current_root->addSubEntry(current);
					  current = new Entry ;
					  current->protection = protection ;
					  BEGIN( FindMembers ) ;
					}
<ClassName>[a-z_A-Z0-9\[\]*&]+		{ 
					  if( !current->type.empty() && !current->name.empty() ) current->type += " " ;
					  current->type += current->name ;
				  	  current->name = yytext ;
					  current->namespc = nmspc;
					  current->lineNo = yyLineNr ;
					}
<ClassName>[ \t]*":"[ \t]*		{ 
					  current->args = " :" ;
					  BEGIN( Bases ) ;
					}
<Bases,ClassName>[ \t]*"{"		{ 
				  	  if( current->lineNo == 0 ) current->lineNo = yyLineNr ;
					  current->startLine = yyLineNr ;
				  	  msg( "  has name       ", current->name ) ;
				  	  BEGIN( Curly ) ;
					}
<Bases>[a-z_A-Z*.<>0-9]+		{ 
					  //current->extends += yytext ;
					  if( !current->args.empty() ) current->args += " " ;
					  current->args += yytext ;
					}
<Bases>","				{ current->args += ',' ; }

<Comment>"//"				{ current->program += yytext ; }
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
					  current->lineNo = yyLineNr ;
		
					  lastTContext = 0;
					  msg( "  has name       ", current->name ) ;

					  int section = current->section;
					  current_root->addNextEntry( current ) ;
					  current = new Entry ;
					  current->protection = protection ;
					  if( section == CLASS_SEC ) current->protection = GLOB ;

					  BEGIN( NextSemi ) ;
					}

<TypedefName>[;,]+			{
					  lastTContext = 0;
					  msg( "  has name       ", current->name ) ;

					  int section = current->section;
					  current_root->addNextEntry( current ) ;
					  current = new Entry ;
					  current->protection = protection ;
					  if( section == CLASS_SEC ) current->protection = GLOB ;

					  if( *yytext == ',' )
					    BEGIN( NextSemi ) ;
					  else
					    BEGIN( FindMembers ) ;
					}


<FindMembers>[ \t]+"///"[ \t]*		{ 
					  lineCount() ;
					  previous = current;
					  if( current_root->sub ) previous = current_root->sub;
					  if( !current->doc.empty()
					      || !current->memo.empty() )
					  {
					    msg( "found doc entry" ) ;
					    current->lineNo = yyLineNr ;
					    current_root->addSubEntry( current ) ;
					    current = new Entry ;
					  }
					  BEGIN( Cxx_Memo ) ;
					}
<FindMembers>[\r\n]+[ \t]*"///"[ \t]*	{ 
					  lineCount() ;
					  if( !current->doc.empty()
					      || !current->memo.empty() )
					  {
					    msg( "found doc entry" ) ;
					    current->lineNo = yyLineNr ;
					    current_root->addSubEntry( current ) ;
					    current = new Entry ;
					  }
					  previous = current;
					  BEGIN( Cxx_Memo ) ;
					}
<Cxx_Memo>.*				{ previous->memo += yytext ; }
<Cxx_Memo>\n				{ 
					  yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>[ \t\r\n]*"//"[^/][^\n]*\n		{ lineCount() ; }
<FindMembers>[ \t\r\n]*"///""/"+[^\n]*\n	{ lineCount() ; }
<FindMembers>[ \t\r\n]*"/*""*"+"/"		{ lineCount() ; }
<FindMembers>[ \t\r\n]*"/***"		{ 
					  lineCount() ;
					  lastCContext = YY_START ;
					  BEGIN( SkipComment ) ;
					}
<FindMembers>[ \t\r\n]*"/**"[ \t\r]*	{ 
					  lineCount() ;
					  if( !current->doc.empty() 
					      ||  !current->memo.empty() )
					  {
					    msg( "found doc entry" ) ;
					    current->lineNo = yyLineNr ;
					    current_root->addSubEntry( current ) ;
					    current = new Entry ;
					  }
					  BEGIN( Doc ) ;
					}
<VerbDoc,Doc>\n[ \t]*"*"+"/"		{ 
					  yyLineNr++; 
					  BEGIN( FindMembers ) ; 
					}
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
					  BEGIN( Param ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@exception"[ \t\r\n]*	{
					  lineCount() ;
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
<SkipComment>"*/"			{ BEGIN( lastCContext ) ; }
<SkipComment>"//"
<SkipComment>\"[^\n\"]*\"
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
	if( !rt->doc.empty() )
	{
		Entry* _current = current ;
		rt->program = rt->doc ;
		rt->doc.clear() ;
		inputString   = rt->program ;
		inputPosition = 0 ;
		current = rt ;
		cpprestart( cppin ) ;
		if( !rt->memo.empty() )
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
	if( ( !current->name.empty()  ||  !current->program.empty()  ||
		  !current->memo.empty()  ||  !current->doc.empty() ) )
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
    for( Entry *cr = rt ; cr ; cr = cr->next )
    {
		if( !cr->program.empty() )
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
			else if( cr->section == ENUM_SEC )
				current->protection = protection = PUBL ;
			else
				current->protection = protection = GLOB ;
			callcpplex() ;
			cr->program.clear() ;
		}
    }
    parseDoc( rt ) ;
}

void parseCpp(Entry* rt)
{
	current_root  = rt ;
	protection    = GLOB ;
	current       = new Entry ;
	yyLineNr      = 1 ;
	bracketCount  = 0 ;
	sharpCount    = 0 ;
	elseCount     = 0 ;
	lastTContext  = 0 ;
	lastCContext  = 0 ;
	lastContext   = 0 ;
	nmspc         = "" ;

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
