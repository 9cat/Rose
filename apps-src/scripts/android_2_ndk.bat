set DST_arm=%NDK%\platforms\android-15\arch-arm\usr\lib\.

copy %SDL_sdl%\libs\armeabi\libSDL2.so %DST_arm%
copy %SDL_net%\libs\armeabi\libSDL2_net.so %DST_arm%
copy %SDL_image%\libs\armeabi\libSDL2_image.so %DST_arm%
copy %SDL_mixer%\libs\armeabi\libSDL2_mixer.so %DST_arm%
copy %SDL_ttf%\libs\armeabi\libSDL2_ttf.so %DST_arm%

rem ABI_LEVEL: 15 hasn't arch-arm64