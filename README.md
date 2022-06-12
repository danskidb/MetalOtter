MetalOtter
==========
Learning to use vukan here.

## Set up
- With CMake, vscode and the cmake extension is easiest, just configure, build & run.
- for ios, run the configure_ios.sh script. see https://github.com/leetal/ios-cmake for more info.

## Progress
https://vulkan-tutorial.com/Texture_mapping/Images

## Goals
- render texture in both windows separately, one with ImGui
- no vsync, but frame limiter
- investigate matchmaking + networking solution for turn based game
- dedicated server library

## Todo
- iOS and Android compatibility
- entity hierarchy
- better sdl event handling (only send keyboard events etc to currently focussed window)
- better handling for High DPI screens (SDL_WINDOW_ALLOW_HIGHDPI)
- change all window size things to Vec2D

## Some other resources
- https://snorristurluson.github.io/TextureAtlas/
- https://snorristurluson.github.io/TextRenderingWithFreetype/
- https://github.com/assimp/assimp
- https://github.com/SaschaWillems/Vulkan