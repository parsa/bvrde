
del lex.yy.c
del lex.cpp
flex -B -L -Pcpp cpp.l
ren lex.cpp.c Lex.cpp
pause
