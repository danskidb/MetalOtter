MetalOtter
==========
Learning to use vukan here.

## Progress
https://vulkan-tutorial.com/Texture_mapping/Images

## Goals
- render texture in both windows separately, one with ImGui
- no vsync, but frame limiter
- investigate matchmaking + networking solution for turn based game
- dedicated server library

## Todo
- Finish SDL2 migration: https://gist.github.com/YukiSnowy/dc31f47448ac61dd6aedee18b5d53858 
 * Application::GetRequiredExtensions / move vulkan instances to window it can grab required extensions
 * Event handling should also be handled with this partially
 * Window Resizing
 * Surface creation
- iOS and Android compatibility
- entity hierarchy

## Some other resources
- https://snorristurluson.github.io/TextureAtlas/
- https://snorristurluson.github.io/TextRenderingWithFreetype/
- https://github.com/assimp/assimp
- https://github.com/SaschaWillems/Vulkan