call pa ../../../../opensrc/applications/test/khronos/glutes/win32
call p.bat
cl -DWIN32 -D_CRT_SECURE_NO_DEPRECATE /Zi -o cubes.exe board.c dawg.c diction.c graphics3.c leave.c scrabble.c search.c tga.c -Dinline= -I../../../../opensrc/applications/test/khronos/glutes/win32 -I../../../.. -I../../../../interface/khronos/include -Iwin32 -DPATH=\".\" -DINLINE= /link /LIBPATH:..\..\..\..\opensrc\applications\test\khronos\glutes\win32 glutes.lib libGLES_CM_NoE.lib libEGL.lib
