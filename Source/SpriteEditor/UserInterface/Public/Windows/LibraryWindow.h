#pragma once

#include "SpriteSheet.h"
#include "LibraryListItem.h"
#include "AUI/Window.h"
#include "AUI/Text.h"
#include "AUI/Image.h"
#include "AUI/VerticalListContainer.h"
#include "AUI/Button.h"
#include <unordered_map>

namespace AM
{
namespace SpriteEditor
{
class MainScreen;
class SpriteDataModel;
class SpriteSheetListItem;

/**
 * The left-side panel on the main screen. Allows the user to manage the
 * project's sprite sheets.
 */
class LibraryWindow : public AUI::Window
{
public:
    //-------------------------------------------------------------------------
    // Public interface
    //-------------------------------------------------------------------------
    LibraryWindow(MainScreen& inScreen, SpriteDataModel& inSpriteDataModel);

    //-------------------------------------------------------------------------
    // Base class overrides
    //-------------------------------------------------------------------------
    void onFocusLost(AUI::FocusLostType focusLostType) override;

    AUI::EventResult onKeyDown(SDL_Keycode keyCode) override;

private:
    /**
     * The top-level categories that we have in the library.
     * These values are used to index into libraryContainer.
     */
    struct Category {
        enum Value
        {
            SpriteSheets,
            Count
        };
    };

    /**
     * Adds the given sheet to the library.
     */
    void onSheetAdded(unsigned int sheetID, const SpriteSheet& sheet);

    /**
     * Removes the given sheet from the library.
     */
    void onSheetRemoved(unsigned int sheetID);

    /**
     * Updates the display name on the list item for the given sprite.
     */
    void onSpriteDisplayNameChanged(unsigned int spriteID,
                                    const std::string& newDisplayName);

    /**
     * If there are other currently selected list items, checks if the given 
     * list item is compatible with them. If so, adds it to the vector.
     */
    void processSelectedListItem(LibraryListItem* selectedListItem);

    /**
     * Adds the given sprite to the given sprite sheet list item.
     */
    void addSpriteToSheetListItem(
        SpriteSheetListItem& sheetListItem,
        const SpriteSheet& sheet, unsigned int spriteID);

    /**
     * @return true if the given type is removable.
     */
    bool isRemovable(LibraryListItem::Type listItemType);

    /**
     * Removes the given list item widget from the library and all secondary 
     * data structures.
     */
    void removeListItem(LibraryListItem* listItem);

    /** Used to open the confirmation dialog when removing a sheet. */
    MainScreen& mainScreen;

    /** Used to update the model when a sheet is removed. */
    SpriteDataModel& spriteDataModel;

    /** Maps sprite sheet IDs to their associated thumbnails. */
    std::unordered_map<unsigned int, LibraryListItem*> sheetListItemMap;

    /** Maps sprite IDs to their associated thumbnails. */
    std::unordered_map<unsigned int, LibraryListItem*> spriteListItemMap;

    /** Holds the currently selected list items. */
    std::vector<LibraryListItem*> selectedListItems;

    /** Holds items that are staged to be removed. */
    std::vector<LibraryListItem*> itemsToRemove;

    //-------------------------------------------------------------------------
    // Private child widgets
    //-------------------------------------------------------------------------
    AUI::Image backgroundImage;

    AUI::Image headerImage;

    AUI::Text windowLabel;

    AUI::VerticalListContainer libraryContainer;

    AUI::Button addButton;
};

} // End namespace SpriteEditor
} // End namespace AM
