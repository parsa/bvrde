/*************************************************************************

    CPP lexer

    The file is derived from the DOC++ source by Roland Wunderling
    and Malte Zoeckler.

 *************************************************************************/


%{

#include "StdAfx.h"

/*
 *	includes
 */
#include <stdio.h>
#include <iostream.h>
#include <assert.h>
#include <ctype.h>

#include "CppTypes.h"


/* -----------------------------------------------------------------
 *
 *	statics
 */
static const char*	inputString ;
static int			inputPosition ;
static int			lastContext ;
static int			lastCContext ;
static int			protection   = GLOB ;
static int			bracketCount = 0 ;
static int			sharpCount   = 0 ;
static Entry*		current_root = 0 ;
static Entry*		global_root  = 0 ;
static Entry*		current      = 0 ;

static int		yyLineNr = 1 ;
static char		yyFileName[264] ;
static McString*	currentString ;

static void setSection( int sec )
{
}

static void msg( const char* str, const char* str2=NULL )
{
    if( verb )
    {
	printf( "%s(%d):\t%s", yyFileName, yyLineNr, str ) ;
	if( str2 ) printf( "%s", str2 ) ;
	printf( "\n" ) ;
    }
}

static void lineCount()
{
  for( const char* c = yytext ; *c ; ++c )
    yyLineNr += (*c == '\n') ;
}


static void addType( Entry* current )
{
    if( current->type.length() > 0 ) current->type += ' ' ;
    current->type += current->name ;
    current->name.clear() ;
    if( current->type.length() > 0 ) current->type += ' ' ;
    current->type += current->args ;
    current->args.clear() ;
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
                                          yyLineNr= 1 ; int i;
					  for( i = 0 ; yytext[i+1] != 6 ; i++ )
					    yyFileName[i] = yytext[i+1] ;
					  yyFileName[i] = 0 ;
					}

<NextSemi>[;,]				{ BEGIN( FindMembers ) ; }

<FindMembers>[ \t]*"public:"[ \t\n]*	{ current->protection = protection = PUBL ;
					  lineCount() ;
					}
<FindMembers>[ \t]*"protected:"[ \t\n]*	{ current->protection = protection = PROT ;
					  lineCount() ;
					}
<FindMembers>[ \t]*"private:"[ \t\n]*	{ current->protection = protection = PRIV ;
					  lineCount() ;
					}

<FindMembers>[ \t]*"typedef"[ \t\n]+"class"[ \t\n]+	{
					  current->section = TYPEDEF_SEC ;
				      current->lineNo = yyLineNr;
					  current->type = "typedef class" ;
					  current->name.clear() ;
					  lineCount() ;
					}
<FindMembers>[ \t]*"typedef"[ \t\n]+"struct"[ \t\n]+	{
					  current->section = TYPEDEF_SEC ;
				      current->lineNo = yyLineNr;
					  current->type = "typedef struct" ;
					  current->name.clear() ;
					  lineCount() ;
					}
<FindMembers>[ \t]*"typedef"[ \t\n]+"enum"[ \t\n]+	{
					  current->section = TYPEDEF_SEC ;
				      current->lineNo = yyLineNr;
					  current->type = "typedef enum" ;
					  current->name.clear() ;
					  lineCount() ;
					}
<FindMembers>[ \t]*"typedef"[ \t\n]+"union"[ \t\n]+	{
					  current->section = TYPEDEF_SEC ;
				      current->lineNo = yyLineNr;
					  current->type = "typedef union" ;
					  current->name.clear() ;
					  lineCount() ;
					}

<FindMembers>[ \t]*"class"[ \t\n]+	{ current->section = CLASS_SEC ;
					  addType( current ) ;
				      current->lineNo = yyLineNr;
					  current->type += " class" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"struct"[ \t\n]+	{ current->section = UNION_SEC ;
					  addType( current ) ;
				      current->lineNo = yyLineNr;
					  current->type += " struct" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"enum"[ \t\n]+	{ current->section = UNION_SEC ;
					  addType( current ) ;
				      current->lineNo = yyLineNr;
					  current->type += " enum" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}
<FindMembers>[ \t]*"union"[ \t\n]+	{ current->section = UNION_SEC ;
					  addType( current ) ;
				      current->lineNo = yyLineNr;
					  current->type += " union" ;
					  lineCount() ;
					  BEGIN( ClassName ) ;
					}

<FindMembers>"operator"/[^a-z_A-Z0-9]	{ addType( current ) ;
				      current->lineNo = yyLineNr;
					  current->name  = yytext ;
					  BEGIN( Operator ) ;
					}
<Operator>[^(]*				{ current->name += yytext ;
				      current->lineNo = yyLineNr;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>[a-z_A-Z~:.0-9]+		{ addType( current ) ;
				      current->lineNo = yyLineNr;
					  current->name  = yytext ;
					}

<FindMembers>^[ \t]*"#"			{ BEGIN( SkipCPP ) ; }
<SkipCPP>.
<SkipCPP>\\\n				{ yyLineNr++ ; }
<SkipCPP>\n				{ yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>^[ \t]*"#"[ \t]*define[ \t]+	{ current->type = "#define" ;
					  BEGIN( Define ) ;
					}
<Define>[a-z_A-Z:.0-9]+			{ current->name = yytext ;
				      current->lineNo = yyLineNr;
					  BEGIN( DefineEnd ) ;
					}
<DefineEnd>[ \t]			{ BEGIN( DefineEnd ) ; }
<DefineEnd>"("[^)]*")"			{ current->args = yytext ;
					  BEGIN( DefineEnded ) ;
					}
<DefineEnd,DefineEnded>"\\\n"		{ yyLineNr++ ; }
<DefineEnd,DefineEnded>"\n"		{ if( current->done == onlyDocs )
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

<FindMembers>[*&]+		{ current->name += yytext ; 
				      current->lineNo = yyLineNr;
					}

<FindMembers>[;=,]		{ BEGIN( FindMembers ) ;
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

<FindMembers>"["			{ current->args += yytext ;
					  sharpCount=1;
					  BEGIN( Array ) ;
					}
<Array>"]"				{ current->args += *yytext ;
					  if (--sharpCount<=0)
	                                     BEGIN( FindMembers ) ;
					}
<Array>"["				{ current->args += *yytext ;
					  sharpCount++;	
					}
<Array>.				{ current->args += *yytext ; }

<FindMembers>"<"			{ addType( current ) ;
					  current->type += yytext ;
					  sharpCount=1;
					  BEGIN( Sharp ) ;
					}
<Sharp>">"				{ current->type += *yytext ;
					  if (--sharpCount<=0)
	                                     BEGIN( FindMembers ) ;
					}
<Sharp>"<"				{ current->type += *yytext ;
					  sharpCount++;	
					}
<Sharp>.				{ current->type += *yytext ; }

<Curly>[^\n{}"/]*			{ current->program += yytext ; }
<Curly>"//".*				{ current->program += yytext ; }
<Curly>\"[^\n"]*]\"			{ current->program += yytext ; }
<Curly>"/*"\**[ \t]*			{ current->program += yytext ;
					  lastContext = Curly ;
					  BEGIN( Comment ) ;
					}
<Curly>"/*"\**[ \t]*\n			{ current->program += yytext ;
					  ++yyLineNr ;
					  lastContext = Curly ;
					  BEGIN( Comment ) ;
					}
<Curly>"{"				{ current->program += yytext ;
					  ++bracketCount ;
					}
<Curly>"}"				{ if( bracketCount )
					  {
					    current->program += yytext ;
					    --bracketCount ;
					  }
					  else
					  {
					    if( current->done == onlyDocs )
					    {
					      int section = current->section;
					      current_root->addSubEntry( current ) ;
					      current = new Entry ;
					      current->protection = protection ;
					      if( section == CLASS_SEC ) current->protection = GLOB ;
					    }
					    else
					    {
					      current->name.clear() ;
					      current->type.clear() ;
					      current->args.clear() ;
					      current->section = EMPTY_SEC ;
					    }
					    BEGIN( NextSemi ) ;
					  }
					}
<Curly>\n				{ current->program += yytext ;
					  yyLineNr++ ;
					}
<Curly>.				{ current->program += yytext ; }

<FindMembers>"("			{ current->args = yytext ;
					  BEGIN( Round ) ;
					}
<Round>"("				{ current->args += *yytext ;
					  ++bracketCount ;
					}
<Round>")"				{ current->args += *yytext ;
					  if( bracketCount )
					    --bracketCount ;
					  else
					    BEGIN( Function ) ;
					}
<Round>[ \t\n]*","[ \t\n]*	{ lineCount() ; current->args += ", " ; }
<Round>[ \t\n]+			{ lineCount() ; current->args += ' ' ; }
<Round>.				{ current->args += *yytext ; }

<Function>[ \t]*"const"[ \t\n]*	{ current->args += " const " ;
					 	 lineCount() ;
						}

<Function>[ \t]*"throw"[ \t\n]*"("	{ current->args += " throw(" ;
						  lineCount() ;
					  	BEGIN( Round ) ;
						}
<Function>"("			{ current->type += current->name ;
					  current->name  = current->args ;
					  current->args  = yytext ;
			          current->lineNo = yyLineNr;
					  BEGIN( Round ) ;
					}
<Function>[:;{]			{ msg( "found method   ", current->name ) ;
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
<SkipCurly>"{"				{ ++bracketCount ; }
<SkipCurly>"}"				{ if( bracketCount )
					    --bracketCount ;
					  else
					    BEGIN( FindMembers ) ;
					}

<Bases,ClassName>";"		{ if( current->done == onlyDocs )
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
<ClassName>[a-z_A-Z0-9\[\]*&]+	{ current->type += ' ' ;
						  current->type += current->name ;
					  	  current->name = yytext ;
						  current->lineNo = yyLineNr;
						}
<ClassName>[ \t]*":"[ \t]*	{ current->args = ":" ;
					  BEGIN( Bases ) ;
					}
<Bases,ClassName>[ \t]*"{"[ \t]*	{ current->file = yyFileName ;
					  	  if( current->lineNo == 0 ) current->lineNo = yyLineNr;
						  current->startLine = yyLineNr ;
					  	  msg( "found class    ", current->name ) ;
					  	  BEGIN( Curly ) ;
						}
<Bases>[a-z_A-Z*.<>0-9]+	{ current->extends.append( new McString(yytext) ) ;
					  current->args += ' ' ;
					  current->args += yytext ;
					}
<Bases>","				{ current->args += ',' ; }

<Comment>\n				{ current->program += yytext ;
					  yyLineNr++ ;
					}
<Comment>.				{ current->program += yytext ; }
<Comment>.*"*/"			{ current->program += yytext ;
					  BEGIN( Curly ) ;
					}

<FindMembers>[ \t\n]*"///"/[^/][ \t]*	{ lineCount() ;
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
<Cxx_Memo>.*			{ current->memo += yytext ; }
<Cxx_Memo>\n			{ yyLineNr++ ;
					  BEGIN( FindMembers ) ;
					}

<FindMembers>[ \t\n]*"/*""*"+"/"
<FindMembers>[ \t\n]*"/***"	{ lastCContext = YY_START ;
					  BEGIN( SkipComment ) ;
					}
<FindMembers>[ \t\n]*"/**"[ \t]*	{ lineCount() ;
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
<VerbDoc,Doc>\n[ \t]*"*"+"/"	{ yyLineNr++; BEGIN( FindMembers ) ; }
<Doc>"\\begin{verbatim}"	{ current->doc += yytext ;
					  BEGIN( VerbDoc ) ;
					}
<VerbDoc>\n				{ current->doc += '\n' ;
					  yyLineNr++ ;
					}
<VerbDoc>"\\end{verbatim}"	{ current->doc += yytext ;
					  BEGIN( Doc ) ;
					}
<Doc>\n[ \t]*"*"*[ \t]*		{ current->doc += '\n' ;
					  yyLineNr++ ;
					}
<VerbDoc,Doc>.			{ current->doc += *yytext ; }
<VerbDoc,Doc>"//"			{ current->doc += yytext ; }
<VerbDoc,Doc>"/*"			{ current->doc += yytext ; }
<VerbDoc,Doc>"*/"			{ BEGIN( FindMembers ) ; }

<FindMembers>("//@{".*\n)|("/*@{"[^*]*\*+"/")	{ lineCount() ;
								  current->file = yyFileName ;
							  	  if( current->lineNo == 0 ) current->lineNo = yyLineNr;
								  current->startLine = yyLineNr ;
								  BEGIN( SubDoc ) ;
								}
<SubDoc>"/*"			{ current->program += yytext ;
					  BEGIN( SubDocComment ) ;
					}
<SubDoc>"//"			{ current->program += yytext ;
					  BEGIN( SubDocCppComment ) ;
					}
<SubDoc>.				{ current->program += *yytext ; }
<SubDoc>\n				{ current->program += *yytext ;
					  yyLineNr++ ;
					}
<SubDoc>("//@{".*\n)|("/*@{"[^*]*\*+"/")	{ lineCount() ;
							  current->program += yytext ;
							  ++bracketCount ;
							}
<SubDoc>("//@}".*\n)|("/*@}"[^*]*\*+"/") { lineCount() ;
							  if( bracketCount )
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
<SubDocComment>\n			{ current->program += yytext ;
					  ++yyLineNr ;
					}
<SubDocComment>"*/"			{ current->program += yytext ;
					  BEGIN( SubDoc ) ;
					}

<SubDocCppComment>.		{ current->program += yytext ; }
<SubDocCppComment>\n		{ current->program += yytext ;
					  ++yyLineNr ;
					  BEGIN( SubDoc ) ;
					}

<CppMemo>[^.]			{ current->memo += yytext ;
					  current->doc  += yytext ; }
<CppMemo>"."[ \t\n]*		{ BEGIN( CppDoc ) ;
                      yyLineNr++;
					  current->doc  += yytext ; }
<CppDoc>(\n|.)			{ current->doc  += *yytext ; 
	                  yyLineNr++;
						}
<CppMemo>"/"("*"|"/")		{ current->memo += yytext ; }
<CppDoc>"/"("*"|"/")		{ current->doc  += yytext ; }

<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@see"[ \t\n]*	{
					  current->see.append( new McString ) ;
					  BEGIN( See ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@author"[ \t\n]*	{
					  BEGIN( Author ) ; }
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@version"[ \t\n]*	{
					  BEGIN( Version ) ; }
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@param"[ \t\n]*	{
					  current->param.append( new McString  ) ;
					  BEGIN( Param ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@exception"[ \t\n]*	{
					  current->exception.append( new McString ) ;
					  BEGIN( Exception ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@return"[ \t\n]*	{
					  BEGIN( Return ) ; }
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@name"[ \t]*	{
					  current->name.clear() ;
					  current->section = MANUAL_SEC ;
					  BEGIN( Name ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@memo"[ \t\n]*	{
					  current->memo.clear() ;
					  BEGIN( Memo ) ;
					}

<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@type"[ \t\n]*	{
					  current->type.clear() ;
					  BEGIN( Type ) ;
					}
<Author,Version,See,Type,Args,Param,Exception,Memo,Return,CppMemo,CppDoc>^"@args"[ \t\n]*	{
					  current->args.clear() ;
					  BEGIN( Args ) ;
					}

<Name>.*				{ current->name = yytext ; }
<Name>\n				{ BEGIN( CppDoc ) ; }

<Type>.*				{ current->type = yytext ; }
<Type>\n				{ BEGIN( CppDoc ) ; }

<Args>.*				{ current->args = yytext ; }
<Args>\n				{ BEGIN( CppDoc ) ; }

<Author>.				{ current->author  += *yytext ; }
<Version>.				{ current->version += *yytext ; }
<See>.				{ *(current->see.last())       += *yytext ; }
<Param>.				{ *(current->param.last())     += *yytext ; }
<Exception>.			{ *(current->exception.last()) += *yytext ; }
<Memo>.				{ current->memo    += *yytext ; }
<Return>.				{ current->retrn   += *yytext ; }

<Author>\n[ \t\n]*		{ current->author  += '\n' ; }
<Version>\n[ \t\n]*		{ current->version += '\n' ; }
<See>\n[ \t\n]*			{ *(current->see.last())       += '\n' ; }
<Param>\n[ \t\n]*			{ *(current->param.last())     += '\n' ; }
<Exception>\n[ \t\n]*		{ *(current->exception.last()) += '\n' ; }
<Memo>\n[ \t\n]*			{ current->memo    += *yytext ; }
<Return>\n[ \t\n]*		{ current->retrn   += *yytext ; }

<*>.
<*>\n					{ yyLineNr++ ; }
<*>"/*"					{ lastCContext = YY_START ;
					  BEGIN( SkipComment ) ;
					}
<SkipComment>"//"
<SkipComment>[ \t]*"*/"		{ BEGIN( lastCContext ) ; }
<*>"//"				{ lastCContext = YY_START ;
					  BEGIN( SkipCxxComment ) ;
					}
<SkipCxxComment>.*\n		{ yyLineNr++ ;
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
	if( rt->memo.length() )
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
    if( rt == 0 )
	return ;
    for( Entry *cr = rt->sub ; cr ; cr = cr->next )
    {
	if( cr->program.length() > 0 )
	{
	    if (verb)
		printf("Scanning substuff of (%s) %s\n",
		       (const char *)cr->type,
		       (const char *)cr->name);
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
    assert(rt) ;

    current_root  = rt ;
    global_root   = rt ;
    protection    = PUBL ;
    current       = new Entry ;
    yyLineNr      = 1 ;
    yyFileName[0] = '\0' ;

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
