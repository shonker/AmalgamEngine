#pragma once

#include "LibraryListItem.h"
#include "BoundingBoxID.h"
#include "SpriteSets.h"
#include "IconID.h"
#include "AUI/Window.h"
#include "AUI/Text.h"
#include "AUI/Image.h"
#include "AUI/VerticalListContainer.h"
#include "AUI/Button.h"
#include "entt/signal/sigh.hpp"
#include <unordered_map>

namespace AM
{
namespace ResourceImporter
{
class MainScreen;
class DataModel;
struct EditorBoundingBox;
struct EditorSpriteSheet;
class ParentListItem;
struct EditorFloorSpriteSet;
struct EditorFloorCoveringSpriteSet;
struct EditorWallSpriteSet;
struct EditorObjectSpriteSet;
struct EditorIconSheet;

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
    LibraryWindow(MainScreen& inScreen, DataModel& inDataModel);

    /**
     * Returns the currently selected list items.
     */
    const std::vector<LibraryListItem*>& getSelectedListItems() const;

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
    enum Category {
        BoundingBoxes,
        SpriteSheets,
        Floors,
        FloorCoverings,
        Walls,
        Objects,
        /*Entities,*/
        IconSheets,
        Count,
        None
    };

    /**
     * Adds the given item to the library.
     */
    void onBoundingBoxAdded(BoundingBoxID boundingBoxID,
                            const EditorBoundingBox& bounds);
    void onSpriteSheetAdded(int sheetID, const EditorSpriteSheet& sheet);
    void onFloorAdded(Uint16 floorID, const EditorFloorSpriteSet& floor);
    void
        onFloorCoveringAdded(Uint16 floorCoveringID,
                             const EditorFloorCoveringSpriteSet& floorCovering);
    void onWallAdded(Uint16 wallID, const EditorWallSpriteSet& wall);
    void onObjectAdded(Uint16 objectID, const EditorObjectSpriteSet& object);
    template<typename T>
    void onSpriteSetAdded(Uint16 spriteSetID, const T& spriteSet);
    void onIconSheetAdded(int sheetID, const EditorIconSheet& sheet);

    /**
     * Removes the given item from the library.
     */
    void onBoundingBoxRemoved(BoundingBoxID boundingBoxID);
    void onSpriteSheetRemoved(int sheetID);
    void onSpriteSetRemoved(SpriteSet::Type type, Uint16 spriteSetID);
    void onIconSheetRemoved(int sheetID);

    /**
     * Updates the display name on the associated list item.
     */
    void onBoundingBoxDisplayNameChanged(BoundingBoxID boundingBoxID,
                                         const std::string& newDisplayName);
    void onSpriteDisplayNameChanged(int spriteID,
                                    const std::string& newDisplayName);
    void onSpriteSetDisplayNameChanged(SpriteSet::Type type, Uint16 spriteSetID,
                                       const std::string& newDisplayName);
    void onIconDisplayNameChanged(IconID iconID,
                                  const std::string& newDisplayName);

    /**
     * If there are other currently selected list items, checks if the given
     * list item is compatible with them. If so, adds it to the vector.
     */
    void processSelectedListItem(LibraryListItem* selectedListItem);

    /**
     * Adds the given sprite to the given sprite sheet list item.
     */
    void addSpriteToSheetListItem(ParentListItem& sheetListItem,
                                  const EditorSpriteSheet& sheet, int spriteID);

    /**
     * Adds the given icon to the given icon sheet list item.
     */
    void addIconToSheetListItem(ParentListItem& sheetListItem,
                                const EditorIconSheet& sheet, IconID iconID);

    /**
     * @return true if the given type is removable.
     */
    bool isRemovable(LibraryListItem::Type listItemType);

    /**
     * Removes the given list item widget from the library and all secondary
     * data structures.
     */
    void removeListItem(LibraryListItem* listItem);

    LibraryListItem::Type toListItemType(SpriteSet::Type spriteSetType);
    Category toCategory(SpriteSet::Type spriteSetType);

    /** Used to open the confirmation dialog when removing a sheet. */
    MainScreen& mainScreen;

    /** Used to update the model when a sheet is removed. */
    DataModel& dataModel;

    /** An array of maps, indexed by list item types. Each map holds the
        list items for the associated type. */
    std::array<std::unordered_map<int, LibraryListItem*>,
               LibraryListItem::Type::Count>
        listItemMaps;

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

    //-------------------------------------------------------------------------
    // Signals
    //-------------------------------------------------------------------------
    entt::sigh<void(const LibraryListItem& selectedItem)> listItemSelectedSig;
    entt::sigh<void(const LibraryListItem& deselectedItem)>
        listItemDeselectedSig;

public:
    //-------------------------------------------------------------------------
    // Signal Sinks
    //-------------------------------------------------------------------------
    entt::sink<entt::sigh<void(const LibraryListItem& selectedItem)>>
        listItemSelected;
    entt::sink<entt::sigh<void(const LibraryListItem& deselectedItem)>>
        listItemDeselected;
};

} // End namespace ResourceImporter
} // End namespace AM