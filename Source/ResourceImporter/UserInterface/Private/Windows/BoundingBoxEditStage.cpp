#include "BoundingBoxEditStage.h"
#include "DataModel.h"
#include "LibraryWindow.h"
#include "Paths.h"
#include "AUI/Core.h"
#include "AUI/ScalingHelpers.h"

namespace AM
{
namespace ResourceImporter
{
BoundingBoxEditStage::BoundingBoxEditStage(DataModel& inDataModel,
                                           LibraryWindow& inLibraryWindow)
: AUI::Window({320, 58, 1297, 1022}, "BoundingBoxEditStage")
, dataModel{inDataModel}
, libraryWindow{inLibraryWindow}
, activeBoundingBoxID{NULL_BOUNDING_BOX_ID}
, topText{{0, 0, logicalExtent.w, 34}, "TopText"}
, modifyText{{0, 58, 1297, 24}, "ModifyText"}
, checkerboardImage{{0, 0, 100, 100}, "BackgroundImage"}
, spriteImage{{0, 0, 256, 512}, "SpriteImage"}
, boundingBoxGizmo{inDataModel}
, previewSpriteButton{{581, 692, 136, 46},
                      "Preview Sprite",
                      "PreviewSpriteButton"}
, descText{{24, 806, 1240, 144}, "DescText"}
{
    // Add our children so they're included in rendering, etc.
    children.push_back(topText);
    children.push_back(modifyText);
    children.push_back(checkerboardImage);
    children.push_back(spriteImage);
    children.push_back(boundingBoxGizmo);
    children.push_back(previewSpriteButton);
    children.push_back(descText);

    /* Text */
    topText.setFont((Paths::FONT_DIR + "B612-Regular.ttf"), 26);
    topText.setColor({255, 255, 255, 255});
    topText.setHorizontalAlignment(AUI::Text::HorizontalAlignment::Center);
    topText.setText("Bounding Box");

    styleText(descText);
    descText.setText(
        "Bounding boxes are used by Sprites and Animations for collision and "
        "to make them clickable in build mode.\n\nHere, you can create a "
        "shared bounding box that may be used by all Sprites and Animations. "
        "Alternatively, the Sprite and Animation editors provide a way to "
        "create a custom bounding box, specific to that Sprite or Animation.");

    styleText(modifyText);
    modifyText.setHorizontalAlignment(AUI::Text::HorizontalAlignment::Center);
    modifyText.setText("To preview: select a Sprite or Animation in the "
                       "Library window, then press the Preview Sprite button.");

    /* Active sprite and checkerboard background. */
    checkerboardImage.setTiledImage(Paths::TEXTURE_DIR
                                    + "SpriteEditStage/Checkerboard.png");
    checkerboardImage.setIsVisible(false);
    spriteImage.setIsVisible(false);

    /* Bounding box gizmo. */
    boundingBoxGizmo.setIsVisible(false);

    /* Preview sprite button. */
    previewSpriteButton.setOnPressed([&]() { onPreviewSpriteButtonPressed(); });
    previewSpriteButton.text.setFont((Paths::FONT_DIR + "B612-Regular.ttf"), 16);

    // When the active bounding box is updated, update it in this widget.
    dataModel.activeLibraryItemChanged
        .connect<&BoundingBoxEditStage::onActiveLibraryItemChanged>(*this);
    dataModel.boundingBoxModel.boundingBoxBoundsChanged
        .connect<&BoundingBoxEditStage::onBoundingBoxBoundsChanged>(*this);
    dataModel.boundingBoxModel.boundingBoxRemoved
        .connect<&BoundingBoxEditStage::onBoundingBoxRemoved>(*this);

    // When the gizmo updates the active sprite's bounds, push it to the model.
    boundingBoxGizmo.setOnBoundingBoxUpdated(
        [&](const BoundingBox& updatedBounds) {
            onGizmoBoundingBoxUpdated(updatedBounds);
        });

    // When a library item is selected, update the preview button.
    libraryWindow.selectedItemsChanged
        .connect<&BoundingBoxEditStage::onLibrarySelectedItemsChanged>(*this);
}

void BoundingBoxEditStage::onPreviewSpriteButtonPressed()
{
    // If a graphic is selected, set it as the preview image.
    // Note: This just uses the first selected sprite. Multi-select is ignored.
    const auto& selectedListItems{libraryWindow.getSelectedListItems()};
    bool imageSelected{false};
    for (const LibraryListItem* selectedItem : selectedListItems) {
        // If this is a sprite or animation, try to get a valid sprite from it.
        const EditorSprite* sprite{nullptr};
        if (selectedItem->type == LibraryListItem::Type::Sprite) {
            sprite = &(dataModel.spriteModel.getSprite(selectedItem->ID));
        }
        else if (selectedItem->type == LibraryListItem::Type::Animation) {
            const EditorAnimation animation{
                dataModel.animationModel.getAnimation(selectedItem->ID)};
            // Note: This returns nullptr if the animation has no frames.
            sprite = animation.getSpriteAtTime(0);
        }

        // If we got a valid sprite, set is as the preview image.
        if (sprite) {
            // Load the sprite's image.
            std::string imagePath{dataModel.getWorkingTexturesDir()};
            imagePath += sprite->parentSpriteSheetPath;
            spriteImage.setSimpleImage(imagePath, sprite->textureExtent);

            // Center the sprite to the stage's X, but use a fixed Y.
            SDL_Rect centeredSpriteExtent{sprite->textureExtent};
            centeredSpriteExtent.x = logicalExtent.w / 2;
            centeredSpriteExtent.x -= (centeredSpriteExtent.w / 2);
            centeredSpriteExtent.y = 212 - logicalExtent.y;
            spriteImage.setLogicalExtent(centeredSpriteExtent);

            spriteImage.setIsVisible(true);
            imageSelected = true;

            break;
        }
    }

    // If a valid image type wasn't selected, reset the preview.
    if (!imageSelected) {
        spriteImage.setIsVisible(false);
        previewSpriteButton.text.setText("Preview Sprite");
        previewSpriteButton.disable();
    }
}

void BoundingBoxEditStage::onActiveLibraryItemChanged(
    const LibraryItemData& newActiveItem)
{
    // Check if the new active item is a bounding box and return early if not.
    const EditorBoundingBox* newActiveBoundingBox{
        get_if<EditorBoundingBox>(&newActiveItem)};
    if (!newActiveBoundingBox) {
        activeBoundingBoxID = NULL_BOUNDING_BOX_ID;
        return;
    }

    activeBoundingBoxID = newActiveBoundingBox->numericID;

    // Build our default spriteImage: a 256x512 sprite. Center it to the 
    // stage's X, but use a fixed Y.
    SDL_Rect centeredSpriteExtent{0, 0, 256, 512};
    centeredSpriteExtent.x = logicalExtent.w / 2;
    centeredSpriteExtent.x -= (centeredSpriteExtent.w / 2);
    centeredSpriteExtent.y = 212 - logicalExtent.y;
    spriteImage.setLogicalExtent(centeredSpriteExtent);

    // Set the background and gizmo to the size of the sprite.
    checkerboardImage.setLogicalExtent(spriteImage.getLogicalExtent());
    boundingBoxGizmo.setLogicalExtent(spriteImage.getLogicalExtent());

    // Set the background to be visible.
    checkerboardImage.setIsVisible(true);

    // Set up the gizmo with the default sprite's data.
    // Note: Our default spriteImage has a 374px Y-offset.
    boundingBoxGizmo.setXOffset(
        static_cast<int>(SharedConfig::TILE_FACE_SCREEN_WIDTH / 2.f));
    boundingBoxGizmo.setYOffset(374);
    boundingBoxGizmo.setBoundingBox(newActiveBoundingBox->modelBounds);

    // If the gizmo isn't visible, make it visible.
    boundingBoxGizmo.setIsVisible(true);
}

void BoundingBoxEditStage::onBoundingBoxBoundsChanged(
    BoundingBoxID boundingBoxID, const BoundingBox& newBounds)
{
    // If the box isn't active, do nothing.
    if (boundingBoxID != activeBoundingBoxID) {
        return;
    }

    // Update the gizmo to reflect the new bounds.
    const EditorBoundingBox& boundingBox{
        dataModel.boundingBoxModel.getBoundingBox(boundingBoxID)};
    boundingBoxGizmo.setBoundingBox(boundingBox.modelBounds);
}

void BoundingBoxEditStage::onBoundingBoxRemoved(BoundingBoxID boundingBoxID)
{
    if (boundingBoxID == activeBoundingBoxID) {
        activeBoundingBoxID = NULL_BOUNDING_BOX_ID;

        // Set everything back to being invisible.
        checkerboardImage.setIsVisible(false);
        spriteImage.setIsVisible(false);
        boundingBoxGizmo.setIsVisible(false);
    }
}

void BoundingBoxEditStage::onGizmoBoundingBoxUpdated(
    const BoundingBox& updatedBounds)
{
    if (activeBoundingBoxID) {
        // Update the model with the gizmo's new state.
        dataModel.boundingBoxModel.setBoundingBoxBounds(activeBoundingBoxID,
                                                        updatedBounds);
    }
}

void BoundingBoxEditStage::onLibrarySelectedItemsChanged(
    const std::vector<LibraryListItem*>& selectedItems)
{
    // If there's no active bounding box, do nothing.
    if (!activeBoundingBoxID) {
        return;
    }

    // If a sprite is selected, allow the user to preview it.
    if ((selectedItems.size() > 0)
        && (selectedItems[0]->type == LibraryListItem::Type::Sprite)) {
        previewSpriteButton.text.setText("Preview Sprite");
        previewSpriteButton.enable();
    }
    // If a sprite isn't selected but we're displaying an image, allow the 
    // user to clear it.
    else if (spriteImage.getIsVisible()) {
        previewSpriteButton.text.setText("Clear Preview");
        previewSpriteButton.enable();
    }
    else {
        // No selection and no image. Disable the button.
        previewSpriteButton.text.setText("Preview Sprite");
        previewSpriteButton.disable();
    }
}

void BoundingBoxEditStage::styleText(AUI::Text& text)
{
    text.setFont((Paths::FONT_DIR + "B612-Regular.ttf"), 18);
    text.setColor({255, 255, 255, 255});
}

} // End namespace ResourceImporter
} // End namespace AM
