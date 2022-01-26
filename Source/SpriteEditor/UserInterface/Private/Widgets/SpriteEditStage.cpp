#include "SpriteEditStage.h"
#include "MainScreen.h"
#include "Sprite.h"
#include "AssetCache.h"
#include "Paths.h"
#include "AUI/Core.h"

namespace AM
{
namespace SpriteEditor
{
SpriteEditStage::SpriteEditStage(AssetCache& inAssetCache, MainScreen& inScreen,
                                 SpriteDataModel& inSpriteDataModel)
: AUI::Widget(inScreen, {389, 60, 1142, 684}, "SpriteEditStage")
, assetCache{inAssetCache}
, mainScreen{inScreen}
, spriteDataModel{inSpriteDataModel}
, checkerboardImage(inScreen, {0, 0, 100, 100})
, spriteImage(inScreen, {0, 0, 100, 100})
, boundingBoxGizmo(inScreen)
{
    // Add our children so they're included in rendering, etc.
    children.push_back(checkerboardImage);
    children.push_back(spriteImage);
    children.push_back(boundingBoxGizmo);

    /* Active sprite and checkerboard background. */
    checkerboardImage.addResolution(
        {1920, 1080},
        assetCache.loadTexture(Paths::TEXTURE_DIR
                               + "SpriteEditStage/Checkerboard.png"));
    checkerboardImage.setIsVisible(false);
    spriteImage.setIsVisible(false);

    /* Bounding box gizmo. */
    boundingBoxGizmo.setIsVisible(false);
}

void SpriteEditStage::loadActiveSprite(Sprite* activeSprite)
{
    if (activeSprite != nullptr) {
        // Load the sprite's image.
        spriteImage.clearTextures();
        std::string imagePath{spriteDataModel.getWorkingTexturesDir()};
        imagePath += activeSprite->parentSpriteSheetPath;
        spriteImage.addResolution(AUI::Core::getLogicalScreenSize(),
                                  assetCache.loadTexture(imagePath),
                                  activeSprite->textureExtent);

        // Calc the centered sprite position.
        SDL_Rect centeredSpriteExtent{activeSprite->textureExtent};
        centeredSpriteExtent.x = logicalExtent.w / 2;
        centeredSpriteExtent.x -= (centeredSpriteExtent.w / 2);
        centeredSpriteExtent.y = logicalExtent.h / 2;
        centeredSpriteExtent.y -= (centeredSpriteExtent.h / 2);

        // Size the sprite image to the sprite extent size.
        spriteImage.setLogicalExtent(centeredSpriteExtent);

        // Set the background and gizmo to the size of the sprite.
        checkerboardImage.setLogicalExtent(spriteImage.getLogicalExtent());
        boundingBoxGizmo.setLogicalExtent(spriteImage.getLogicalExtent());

        // Set the sprite and background to be visible.
        checkerboardImage.setIsVisible(true);
        spriteImage.setIsVisible(true);

        // Load the sprite into the gizmo.
        boundingBoxGizmo.loadActiveSprite(activeSprite);
        boundingBoxGizmo.setIsVisible(true);
    }
}

void SpriteEditStage::refresh()
{
    boundingBoxGizmo.refresh();
}

void SpriteEditStage::render(const SDL_Point& parentOffset)
{
    // Keep our scaling up to date.
    refreshScaling();

    // Save the extent that we're going to render at.
    lastRenderedExtent = scaledExtent;
    lastRenderedExtent.x += parentOffset.x;
    lastRenderedExtent.y += parentOffset.y;

    // If the widget isn't visible, return without rendering.
    if (!isVisible) {
        return;
    }

    // Children should render at the parent's offset + this widget's offset.
    SDL_Point childOffset{parentOffset};
    childOffset.x += scaledExtent.x;
    childOffset.y += scaledExtent.y;

    // Render our children.
    for (Widget& child : children)
    {
        child.render(childOffset);
    }
}

} // End namespace SpriteEditor
} // End namespace AM