@echo off

mkdir build
pushd build
cl -Zi -DWIN32 -D_WIN32 -DNOGDI -DNOUSER -O3 ^
-Iinclude ^
-I..lib\raylib\winx64_msvc16\include ^
-I..lib\xiAPI\include ^
..\src\main.c ^
user32.lib winmm.lib gdi32.lib msvcrt.lib shell32.lib ^
raylib.lib xiapi64.lib ^
-link -libpath:..lib\raylib\winx64_msvc16 ^
-libpath:..lib\xiAPI ^
-NODEFAULTLIB:libcmt ^
-out:xiclops.exe
popd
