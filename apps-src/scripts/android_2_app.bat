@echo off
if '%1'=='' goto help
if '%1'=='help' goto help

@echo on
set DST=%1\libs\armeabi\.
copy %SDL_sdl%\libs\armeabi\libSDL2.so %DST%
copy %SDL_net%\libs\armeabi\libSDL2_net.so %DST%
copy %SDL_image%\libs\armeabi\libSDL2_image.so %DST%
copy %SDL_mixer%\libs\armeabi\libSDL2_mixer.so %DST%
copy %SDL_ttf%\libs\armeabi\libSDL2_ttf.so %DST%

set DST=%1\libs\armeabi-v7a\.
copy %SDL_sdl%\libs\armeabi-v7a\libSDL2.so %DST%
copy %SDL_net%\libs\armeabi-v7a\libSDL2_net.so %DST%
copy %SDL_image%\libs\armeabi-v7a\libSDL2_image.so %DST%
copy %SDL_mixer%\libs\armeabi-v7a\libSDL2_mixer.so %DST%
copy %SDL_ttf%\libs\armeabi-v7a\libSDL2_ttf.so %DST%
rem APP_PLATFORM: 15 hasn't arm64-v8a
goto exit

:help
echo Missing parameter, you must set app. for example: android_2_app %%studio%%

:exit