#include "SpritePanel.h"
#include "MainScreen.h"
#include "MainThumbnail.h"
#include "SpriteDataModel.h"
#include "AssetCache.h"
#include "Paths.h"
#include "Ignore.h"

namespace AM
{
namespace SpriteEditor
{
SpritePanel::SpritePanel(AssetCache& inAssetCache, MainScreen& inScreen,
                         SpriteDataModel& inSpriteDataModel)
: AUI::Component(inScreen, {-8, 732, 1936, 352}, "SpritePanel")
, assetCache{inAssetCache}
, mainScreen{inScreen}
, spriteDataModel{inSpriteDataModel}
, backgroundImage(inScreen, {0, 0, 1936, 352})
, spriteContainer(inScreen, {191, 24, 1737, 324}, "SpriteContainer")
{
    /* Background image */
    backgroundImage.addResolution(
        {1600, 900},
        assetCache.loadTexture(Paths::TEXTURE_DIR
                               + "SpritePanel/Background_1600.png"));

    /* Container */
    spriteContainer.setNumColumns(10);
    spriteContainer.setCellWidth(156);
    spriteContainer.setCellHeight(162);
}

void SpritePanel::addSprite(const SpriteSheet& sheet, SpriteStaticData& sprite)
{
    // Construct the new sprite thumbnail.
    std::unique_ptr<AUI::Component> thumbnailPtr{
        std::make_unique<MainThumbnail>(assetCache, screen, "")};
    MainThumbnail& thumbnail{static_cast<MainThumbnail&>(*thumbnailPtr)};
    thumbnail.setText(sprite.displayName);
    thumbnail.setIsSelectable(false);

    // Load the sprite's image.
    std::string imagePath{spriteDataModel.getWorkingResourcesDir()};
    imagePath += sheet.relPath;
    thumbnail.thumbnailImage.addResolution(
        {1280, 720}, assetCache.loadTexture(imagePath), sprite.textureExtent);

    // Add a callback to deactivate all other thumbnails when one is activated.
    thumbnail.setOnActivated([&](AUI::Thumbnail* selectedThumb) {
        // Deactivate all other thumbnails.
        for (auto& componentPtr : spriteContainer) {
            MainThumbnail& otherThumb
                = static_cast<MainThumbnail&>(*componentPtr);
            if (otherThumb.getIsActive() && (&otherThumb != selectedThumb)) {
                otherThumb.deactivate();
            }
        }

        // Load the data that this Thumbnail represents as the active sprite.
        mainScreen.loadActiveSprite(&sprite);
    });

    spriteContainer.push_back(std::move(thumbnailPtr));
}

void SpritePanel::refreshActiveSprite(const std::string& newDisplayName)
{
    // Look for an active sprite.
    for (unsigned int i = 0; i < spriteContainer.size(); ++i) {
        AUI::Thumbnail& thumbnail{
            dynamic_cast<AUI::Thumbnail&>(spriteContainer[i])};
        if (thumbnail.getIsActive()) {
            // Refresh the sprite's display name.
            thumbnail.setText(newDisplayName);
        }
    }
}

void SpritePanel::clearSprites()
{
    spriteContainer.clear();
}

void SpritePanel::render(const SDL_Point& parentOffset)
{
    // Keep our scaling up to date.
    refreshScaling();

    // Save the extent that we're going to render at.
    lastRenderedExtent = scaledExtent;
    lastRenderedExtent.x += parentOffset.x;
    lastRenderedExtent.y += parentOffset.y;

    // Children should render at the parent's offset + this component's offset.
    SDL_Point childOffset{parentOffset};
    childOffset.x += scaledExtent.x;
    childOffset.y += scaledExtent.y;

    // Render our children.
    backgroundImage.render(childOffset);

    spriteContainer.render(childOffset);
}

} // End namespace SpriteEditor
} // End namespace AM
