
#include "StdAfx.h"

//
// Scintilla:
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The Scintilla.lic file describes the conditions under which this (Scintilla) software 
// may be distributed.
//
// This file:
// This is an external C++ lexer for the Scintilla editor. It is based
// very much on the original Scintilla C++ Lexer code (19 July 2007).
// It adds 2 new styles: SCE_C_FUNCTIONCLASS and SCE_C_TYPEDEFCLASS
// which colors functions and uppercased identifiers. That makes the
// syntax coloring slightly more rich. A number of source files
// have been imported directly from the Scintilla project to easily
// create this external plugin for the editor.
//

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "ExternalLexer.h"
#include "WindowAccessor.h"


////////////////////////////////////////////////////////////////////////
//
//

void Platform::DebugPrintf(const char *, ...) 
{ 
}

void Platform::Assert(const char* /*c*/, const char* /*file*/, int /*line*/) 
{
}

bool Platform::IsDBCSLeadByte(int codePage, char ch) 
{ 
   return ::IsDBCSLeadByteEx(codePage, ch) != 0; 
}

long Platform::SendScintilla(WindowID w, unsigned int msg, unsigned long wParam, long lParam) 
{
   return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam, lParam);
}

long Platform::SendScintillaPointer(WindowID w, unsigned int msg, unsigned long wParam, void *lParam) 
{
   return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam, reinterpret_cast<LPARAM>(lParam));
}



////////////////////////////////////////////////////////////////////////
//
//

static bool IsSpaceEquiv(int state) {
   return (state <= SCE_C_COMMENTDOC) ||
      // including SCE_C_DEFAULT, SCE_C_COMMENT, SCE_C_COMMENTLINE
      (state == SCE_C_COMMENTLINEDOC) || (state == SCE_C_COMMENTDOCKEYWORD) ||
      (state == SCE_C_COMMENTDOCKEYWORDERROR);
}

inline bool IsOperator(char ch) {
   if (isascii(ch) && isalnum(ch))
      return false;
   // '.' left out as it is used to make up numbers
   if (ch == '%' || ch == '^' || ch == '&' || ch == '*' ||
           ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
           ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
           ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
           ch == '<' || ch == '>' || ch == ',' || ch == '/' ||
           ch == '?' || ch == '!' || ch == '.' || ch == '~')
      return true;
   return false;
}


static void ColouriseCppDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler, bool caseSensitive) {

   WordList &keywords = *keywordlists[0];
   WordList &keywords2 = *keywordlists[1];
   WordList &keywords3 = *keywordlists[2];
   WordList &keywords4 = *keywordlists[3];

   bool stylingWithinPreprocessor = styler.GetPropertyInt("styling.within.preprocessor") != 0;

   CharacterSet setOKBeforeRE(CharacterSet::setNone, "(=,");

   CharacterSet setDoxygen(CharacterSet::setLower, "$@\\&<>#{}[]");

   CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
   CharacterSet setWord(CharacterSet::setAlphaNum, "._", 0x80, true);
   if (styler.GetPropertyInt("lexer.cpp.allow.dollars", 1) != 0) {
      setWordStart.Add('$');
      setWord.Add('$');
   }

   int chPrevNonWhite = ' ';
   int visibleChars = 0;
   bool lastWordWasUUID = false;
   int styleBeforeDCKeyword = SCE_C_DEFAULT;
   bool continuationLine = false;

   if (initStyle == SCE_C_PREPROCESSOR) {
      // Set continuationLine if last character of previous line is '\'
      int lineCurrent = styler.GetLine(startPos);
      if (lineCurrent > 0) {
         int chBack = styler.SafeGetCharAt(startPos-1, 0);
         int chBack2 = styler.SafeGetCharAt(startPos-2, 0);
         int lineEndChar = '!';
         if (chBack2 == '\r' && chBack == '\n') {
            lineEndChar = styler.SafeGetCharAt(startPos-3, 0);
         } else if (chBack == '\n' || chBack == '\r') {
            lineEndChar = chBack2;
         }
         continuationLine = lineEndChar == '\\';
      }
   }

   // look back to set chPrevNonWhite properly for better regex colouring
   if (startPos > 0) {
      int back = startPos;
      while (--back && IsSpaceEquiv(styler.StyleAt(back)))
         ;
      if (styler.StyleAt(back) == SCE_C_OPERATOR) {
         chPrevNonWhite = styler.SafeGetCharAt(back);
      }
   }

   StyleContext sc(startPos, length, initStyle, styler);

   for (; sc.More(); sc.Forward()) {

      if (sc.atLineStart) {
         if (sc.state == SCE_C_STRING) {
            // Prevent SCE_C_STRINGEOL from leaking back to previous line which
            // ends with a line continuation by locking in the state upto this position.
            sc.SetState(SCE_C_STRING);
         }
         // Reset states to begining of colourise so no surprises
         // if different sets of lines lexed.
         visibleChars = 0;
         lastWordWasUUID = false;
      }

      // Handle line continuation generically.
      if (sc.ch == '\\') {
         if (sc.chNext == '\n' || sc.chNext == '\r') {
            sc.Forward();
            if (sc.ch == '\r' && sc.chNext == '\n') {
               sc.Forward();
            }
            continuationLine = true;
            continue;
         }
      }

      // Determine if the current state should terminate.
      switch (sc.state) {
         case SCE_C_OPERATOR:
            sc.SetState(SCE_C_DEFAULT);
            break;
         case SCE_C_NUMBER:
            // We accept almost anything because of hex. and number suffixes
            if (!setWord.Contains(sc.ch)) {
               sc.SetState(SCE_C_DEFAULT);
            }
            break;
         case SCE_C_IDENTIFIER:
            if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
               char s1[500];
               char s2[500];
               if (caseSensitive) sc.GetCurrent(s1, sizeof(s1)); else sc.GetCurrentLowered(s1, sizeof(s1));
               if (caseSensitive) sc.GetCurrentUppered(s2, sizeof(s2)); else strcpy(s2, "|||");
               if (keywords.InList(s1)) {
                  lastWordWasUUID = strcmp(s1, "uuid") == 0;
                  sc.ChangeState(SCE_C_WORD);
               } else if (keywords2.InList(s1)) {
                  sc.ChangeState(SCE_C_WORD2);
               } else if (keywords4.InList(s1)) {
                  sc.ChangeState(SCE_C_GLOBALCLASS);
               } else if( strcmp(s1, s2) == 0 && strlen(s1) >= 4 ) {
                  sc.ChangeState(SCE_C_TYPEDEFCLASS);
               } else if( sc.ch == '(' || (IsASpace(sc.ch) && sc.chNext == '(') ) {
                  sc.ChangeState(SCE_C_FUNCTIONCLASS);
               }
               sc.SetState(SCE_C_DEFAULT);
            }
            break;
         case SCE_C_PREPROCESSOR:
            if (sc.atLineStart && !continuationLine) {
               sc.SetState(SCE_C_DEFAULT);
            } else if (stylingWithinPreprocessor) {
               if (IsASpace(sc.ch)) {
                  sc.SetState(SCE_C_DEFAULT);
               }
            } else {
               if (sc.Match('/', '*') || sc.Match('/', '/')) {
                  sc.SetState(SCE_C_DEFAULT);
               }
            }
            break;
         case SCE_C_COMMENT:
            if (sc.Match('*', '/')) {
               sc.Forward();
               sc.ForwardSetState(SCE_C_DEFAULT);
            }
            break;
         case SCE_C_COMMENTDOC:
            if (sc.Match('*', '/')) {
               sc.Forward();
               sc.ForwardSetState(SCE_C_DEFAULT);
            } else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
               // Verify that we have the conditions to mark a comment-doc-keyword
               if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
                  styleBeforeDCKeyword = SCE_C_COMMENTDOC;
                  sc.SetState(SCE_C_COMMENTDOCKEYWORD);
               }
            }
            break;
         case SCE_C_COMMENTLINE:
            if (sc.atLineStart) {
               sc.SetState(SCE_C_DEFAULT);
            }
            break;
         case SCE_C_COMMENTLINEDOC:
            if (sc.atLineStart) {
               sc.SetState(SCE_C_DEFAULT);
            } else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
               // Verify that we have the conditions to mark a comment-doc-keyword
               if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
                  styleBeforeDCKeyword = SCE_C_COMMENTLINEDOC;
                  sc.SetState(SCE_C_COMMENTDOCKEYWORD);
               }
            }
            break;
         case SCE_C_COMMENTDOCKEYWORD:
            if ((styleBeforeDCKeyword == SCE_C_COMMENTDOC) && sc.Match('*', '/')) {
               sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
               sc.Forward();
               sc.ForwardSetState(SCE_C_DEFAULT);
            } else if (!setDoxygen.Contains(sc.ch)) {
               char s[100];
               if (caseSensitive) {
                  sc.GetCurrent(s, sizeof(s));
               } else {
                  sc.GetCurrentLowered(s, sizeof(s));
               }
               if (!IsASpace(sc.ch) || !keywords3.InList(s + 1)) {
                  sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
               }
               sc.SetState(styleBeforeDCKeyword);
            }
            break;
         case SCE_C_STRING:
            if (sc.atLineEnd) {
               sc.ChangeState(SCE_C_STRINGEOL);
            } else if (sc.ch == '\\') {
               if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
                  sc.Forward();
               }
            } else if (sc.ch == '\"') {
               sc.ForwardSetState(SCE_C_DEFAULT);
            }
            break;
         case SCE_C_CHARACTER:
            if (sc.atLineEnd) {
               sc.ChangeState(SCE_C_STRINGEOL);
            } else if (sc.ch == '\\') {
               if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
                  sc.Forward();
               }
            } else if (sc.ch == '\'') {
               sc.ForwardSetState(SCE_C_DEFAULT);
            }
            break;
         case SCE_C_REGEX:
            if (sc.atLineStart) {
               sc.SetState(SCE_C_DEFAULT);
            } else if (sc.ch == '/') {
               sc.Forward();
               while ((sc.ch < 0x80) && islower(sc.ch))
                  sc.Forward();    // gobble regex flags
               sc.SetState(SCE_C_DEFAULT);
            } else if (sc.ch == '\\') {
               // Gobble up the quoted character
               if (sc.chNext == '\\' || sc.chNext == '/') {
                  sc.Forward();
               }
            }
            break;
         case SCE_C_STRINGEOL:
            if (sc.atLineStart) {
               sc.SetState(SCE_C_DEFAULT);
            }
            break;
         case SCE_C_VERBATIM:
            if (sc.ch == '\"') {
               if (sc.chNext == '\"') {
                  sc.Forward();
               } else {
                  sc.ForwardSetState(SCE_C_DEFAULT);
               }
            }
            break;
         case SCE_C_UUID:
            if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ')') {
               sc.SetState(SCE_C_DEFAULT);
            }
      }

      // Determine if a new state should be entered.
      if (sc.state == SCE_C_DEFAULT) {
         if (sc.Match('@', '\"')) {
            sc.SetState(SCE_C_VERBATIM);
            sc.Forward();
         } else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
            if (lastWordWasUUID) {
               sc.SetState(SCE_C_UUID);
               lastWordWasUUID = false;
            } else {
               sc.SetState(SCE_C_NUMBER);
            }
         } else if (setWordStart.Contains(sc.ch) || (sc.ch == '@')) {
            if (lastWordWasUUID) {
               sc.SetState(SCE_C_UUID);
               lastWordWasUUID = false;
            } else {
               sc.SetState(SCE_C_IDENTIFIER);
            }
         } else if (sc.Match('/', '*')) {
            if (sc.Match("/**") || sc.Match("/*!")) {   // Support of Qt/Doxygen doc. style
               sc.SetState(SCE_C_COMMENTDOC);
            } else {
               sc.SetState(SCE_C_COMMENT);
            }
            sc.Forward();   // Eat the * so it isn't used for the end of the comment
         } else if (sc.Match('/', '/')) {
            if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
               // Support of Qt/Doxygen doc. style
               sc.SetState(SCE_C_COMMENTLINEDOC);
            else
               sc.SetState(SCE_C_COMMENTLINE);
         } else if (sc.ch == '/' && setOKBeforeRE.Contains(chPrevNonWhite)) {
            sc.SetState(SCE_C_REGEX);   // JavaScript's RegEx
         } else if (sc.ch == '\"') {
            sc.SetState(SCE_C_STRING);
         } else if (sc.ch == '\'') {
            sc.SetState(SCE_C_CHARACTER);
         } else if (sc.ch == '#' && visibleChars == 0) {
            // Preprocessor commands are alone on their line
            sc.SetState(SCE_C_PREPROCESSOR);
            // Skip whitespace between # and preprocessor word
            do {
               sc.Forward();
            } while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
            if (sc.atLineEnd) {
               sc.SetState(SCE_C_DEFAULT);
            }
         } else if (IsOperator(static_cast<char>(sc.ch))) {
            sc.SetState(SCE_C_OPERATOR);
         }
      }

      if (!IsASpace(sc.ch) && !IsSpaceEquiv(sc.state)) {
         chPrevNonWhite = sc.ch;
         visibleChars++;
      }
      continuationLine = false;
   }
   sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
   return style == SCE_C_COMMENT ||
      style == SCE_C_COMMENTDOC ||
      style == SCE_C_COMMENTDOCKEYWORD ||
      style == SCE_C_COMMENTDOCKEYWORDERROR;
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldCppDoc(unsigned int startPos, int length, int initStyle, 
                       WordList *[], Accessor &styler) {
   bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
   bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
   bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
   bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) != 0;
   unsigned int endPos = startPos + length;
   int visibleChars = 0;
   int lineCurrent = styler.GetLine(startPos);
   int levelCurrent = SC_FOLDLEVELBASE;
   if (lineCurrent > 0)
      levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
   int levelMinCurrent = levelCurrent;
   int levelNext = levelCurrent;
   char chNext = styler[startPos];
   int styleNext = styler.StyleAt(startPos);
   int style = initStyle;
   for (unsigned int i = startPos; i < endPos; i++) {
      char ch = chNext;
      chNext = styler.SafeGetCharAt(i + 1);
      int stylePrev = style;
      style = styleNext;
      styleNext = styler.StyleAt(i + 1);
      bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
      if (foldComment && IsStreamCommentStyle(style)) {
         if (!IsStreamCommentStyle(stylePrev) && (stylePrev != SCE_C_COMMENTLINEDOC)) {
            levelNext++;
         } else if (!IsStreamCommentStyle(styleNext) && (styleNext != SCE_C_COMMENTLINEDOC) && !atEOL) {
            // Comments don't end at end of line and the next character may be unstyled.
            levelNext--;
         }
      }
      if (foldComment && (style == SCE_C_COMMENTLINE)) {
         if ((ch == '/') && (chNext == '/')) {
            char chNext2 = styler.SafeGetCharAt(i + 2);
            if (chNext2 == '{') {
               levelNext++;
            } else if (chNext2 == '}') {
               levelNext--;
            }
         }
      }
      if (foldPreprocessor && (style == SCE_C_PREPROCESSOR)) {
         if (ch == '#') {
            unsigned int j = i + 1;
            while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
               j++;
            }
            if (styler.Match(j, "region") || styler.Match(j, "if")) {
               levelNext++;
            } else if (styler.Match(j, "end")) {
               levelNext--;
            }
         }
      }
      if (style == SCE_C_OPERATOR) {
         if (ch == '{') {
            // Measure the minimum before a '{' to allow
            // folding on "} else {"
            if (levelMinCurrent > levelNext) {
               levelMinCurrent = levelNext;
            }
            levelNext++;
         } else if (ch == '}') {
            levelNext--;
         }
      }
      if (!IsASpace(ch)) 
         visibleChars++; 
      if (atEOL || (i == endPos-1)) { 
         int levelUse = levelCurrent;
         if (foldAtElse) {
            levelUse = levelMinCurrent;
         }
         int lev = levelUse | levelNext << 16;
         if (visibleChars == 0 && foldCompact)
            lev |= SC_FOLDLEVELWHITEFLAG;
         if (levelUse < levelNext)
            lev |= SC_FOLDLEVELHEADERFLAG;
         if (lev != styler.LevelAt(lineCurrent)) {
            styler.SetLevel(lineCurrent, lev);
         }
         lineCurrent++;
         levelCurrent = levelNext;
         levelMinCurrent = levelCurrent;
         visibleChars = 0;
      }
   }
}


////////////////////////////////////////////////////////////////////////
//
//

static const char* LexerName = "BVRDE_cpp";

extern "C" void EXT_LEXER_DECL Lex(
   unsigned int /*lexer*/, 
   unsigned int startPos, 
   int length, 
   int initStyle,
   char *words[], 
   WindowID window, 
   char *props)
{
   PropSet ps;
   ps.SetMultiple(props);
   WindowAccessor wa(window, ps);

   int nWL = 0;
   for (; words[nWL]; nWL++) ;
   WordList** wl = new WordList* [nWL + 1];
   int i = 0;
   for (; i<nWL; i++) {
      wl[i] = new WordList();
      wl[i]->Set(words[i]);
   }
   wl[i] = 0;

   ColouriseCppDoc(startPos, length, initStyle, wl, wa, true);
   wa.Flush();

   for( i = nWL - 1; i >= 0; i-- ) delete wl[i];
   delete [] wl;
}

extern "C" void EXT_LEXER_DECL Fold(
   unsigned int /*lexer*/, 
   unsigned int startPos, 
   int length, 
   int initStyle,
   char *words[], 
   WindowID window, 
   char *props)
{
   PropSet ps;
   ps.SetMultiple(props);
   WindowAccessor wa(window, ps);

   int nWL = 0;
   for (; words[nWL]; nWL++) ;
   WordList** wl = new WordList* [nWL + 1];
   int i = 0;
   for (; i<nWL; i++) {
      wl[i] = new WordList();
      wl[i]->Set(words[i]);
   }
   wl[i] = 0;

   FoldCppDoc(startPos, length, initStyle, wl, wa);
   wa.Flush();

   for( i = nWL - 1; i >= 0; i-- ) delete wl[i];
   delete [] wl;
}

extern "C" int EXT_LEXER_DECL GetLexerCount()
{
   return 1;
}

extern "C" void EXT_LEXER_DECL GetLexerName(unsigned int /*Index*/, char *name, int buflength)
{
   strncpy(name, LexerName, buflength);
}

