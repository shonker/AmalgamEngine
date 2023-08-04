#pragma once

#include "LibraryItemData.h"
#include "LibraryListItem.h"
#include "SpriteSets.h"
#include "SpriteSetSlot.h"
#include "AUI/Window.h"
#include "AUI/Text.h"
#include "AUI/Image.h"
#include "AUI/VerticalGridContainer.h"

namespace AM
{
namespace SpriteEditor
{
class SpriteDataModel;
class LibraryWindow;

/**
 * The center stage shown when the user loads a sprite set from the Library.
 * Allows the user to edit the active sprite set's sprite slots.
 */
class SpriteSetEditStage : public AUI::Window
{
public:
    //-------------------------------------------------------------------------
    // Public interface
    //-------------------------------------------------------------------------
    SpriteSetEditStage(SpriteDataModel& inSpriteDataModel,
                   const LibraryWindow& inLibraryWindow);

private:
    /**
     * If the new active item is a sprite set, loads it's data onto this stage.
     */
    void onActiveLibraryItemChanged(const LibraryItemData& newActiveItem);

    /**
     * (If type/ID matches active set) Sets this stage back to its default state.
     */
    void onSpriteSetRemoved(SpriteSet::Type type, Uint16 spriteSetID);

    /**
     * (If type/ID matches active set) Sets the given index to the given sprite.
     */
    void onSpriteSetSlotChanged(SpriteSet::Type type, Uint16 spriteSetID,
                                    std::size_t index, int newSpriteID);

    /**
     * Loads the given sprite set's data onto this stage.
     */
    template<typename T>
    void loadActiveSpriteSet(SpriteSet::Type spriteSetType,
                             const T& newActiveSpriteSet);

    /**
     * Styles the given text.
     */
    void styleText(AUI::Text& text);

    /**
     * Returns the appropriate top text for the given index.
     */
    std::string getSlotTopText(std::size_t spriteSetIndex);

    /**
     * Fills the given slot widget with the given sprite's image and name.
     */
    void fillSlotSpriteData(SpriteSetSlot& slot, int spriteID);

    /**
     * Fills the description text widgets with the appropriate strings, based 
     * on the current activeSpriteSetType.
     */
    void fillDescriptionTexts();

    /** Used to get the current working dir when displaying the sprite. */
    SpriteDataModel& spriteDataModel;

    /** Used to get the currently selected list item. */
    const LibraryWindow& libraryWindow;

    /** The active sprite set's type. */
    SpriteSet::Type activeSpriteSetType;

    /** The active sprite set's ID. */
    Uint16 activeSpriteSetID;

    //-------------------------------------------------------------------------
    // Private child widgets
    //-------------------------------------------------------------------------
    AUI::Text topText;
    AUI::Text modifyText;
    AUI::Text clearText;

    /** Holds this sprite set's sprites. */
    AUI::VerticalGridContainer spriteContainer;

    AUI::Text descText1;
    AUI::Text descText2;
    AUI::Text descText3;
    AUI::Text descText4;
};

} // End namespace SpriteEditor
} // End namespace AM