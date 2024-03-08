#include "AnimationTimeline.h"
#include "EditorAnimation.h"
#include "TimelineCell.h"
#include "Paths.h"
#include "AMAssert.h"
#include "Log.h"

namespace AM
{
namespace ResourceImporter
{
AnimationTimeline::AnimationTimeline(const SDL_Rect& inLogicalExtent,
                                     const std::string& inDebugName)
: AUI::Widget(inLogicalExtent, inDebugName)
, numberContainer{{0, 0, logicalExtent.w, 18}, "NumberContainer"}
, cellContainer{{0, 22, logicalExtent.w, 26}, "CellContainer"}
, scrubber{{1, 0}}
, selectedFrameNumber{0}
, activeAnimation{nullptr}
{
    // Add our children so they're included in rendering, etc.
    children.push_back(numberContainer);
    children.push_back(cellContainer);
    children.push_back(scrubber);

    /* Numbers. */
    numberContainer.setNumRows(1);
    numberContainer.setCellWidth(24 + 96);
    numberContainer.setCellHeight(18);

    /* Cell container. */
    cellContainer.setNumRows(1);
    cellContainer.setCellWidth(24);
    cellContainer.setCellHeight(26);
}

void AnimationTimeline::setActiveAnimation(
    const EditorAnimation& newActiveAnimation)
{
    activeAnimation = &newActiveAnimation;

    refreshCells();

    selectedFrameNumber = 0;
    moveScrubberToCell(selectedFrameNumber);
}

void AnimationTimeline::setFrameCount(Uint8 newFrameCount)
{
    selectedFrameNumber = 0;
    moveScrubberToCell(selectedFrameNumber);

    refreshCells();
}

void AnimationTimeline::setFrame(Uint8 frameNumber, bool hasSprite)
{
    TimelineCell& cell{static_cast<TimelineCell&>(*cellContainer[frameNumber])};
    cell.hasSprite = hasSprite;
}

void AnimationTimeline::setOnSelectionChanged(
    std::function<void(const EditorSprite*)> inOnSelectionChanged)
{
    onSelectionChanged = std::move(inOnSelectionChanged);
}

// TODO: Make ScrollArea, put containers inside, disable their scrolling
void AnimationTimeline::refreshCells()
{
    // Fill the cell container with empty cells to match the animation's frame 
    // count.
    cellContainer.clear();
    numberContainer.clear();
    for (int i = 0; i < activeAnimation->frameCount; ++i) {
        auto cell{std::make_unique<TimelineCell>()};
        cell->setOnPressed([&, i]() { moveScrubberToCell(i); });

        // Darken every 5th cell and add a number above it.
        if (i % 5 == 0) {
            cell->drawDarkBackground = true;

            auto numberText{std::make_unique<AUI::Text>(SDL_Rect{0, 0, 24, 18},
                                                        "NumberText")};
            styleNumberText(*numberText, std::to_string(i));
            numberContainer.push_back(std::move(numberText));
        }

        cellContainer.push_back(std::move(cell));
    }

    // Fill the cells that contain frames.
    for (const EditorAnimation::Frame& frame : activeAnimation->frames) {
        AM_ASSERT(frame.frameNumber < cellContainer.size(),
                  "Invalid cell index.");
        TimelineCell& cell{
            static_cast<TimelineCell&>(*cellContainer[frame.frameNumber])};
        cell.hasSprite = true;
    }
}

void AnimationTimeline::moveScrubberToCell(Uint8 cellIndex)
{
    // Center the scrubber over the given cell.
    AM_ASSERT(cellIndex < cellContainer.size(), "Invalid cell index.");
    const auto& cell{cellContainer[cellIndex]};
    const SDL_Rect& cellExtent{cell->getLogicalExtent()};

    SDL_Rect newScrubberExtent{scrubber.getLogicalExtent()};
    newScrubberExtent.x
        = cellExtent.x + (cellExtent.w / 2) - (newScrubberExtent.w / 2);
    scrubber.setLogicalExtent(newScrubberExtent);

    // If the user set a callback, call it.
    if (onSelectionChanged) {
        // If this cell contains a frame, pass it to the user.
        const auto& frames{activeAnimation->frames};
        for (const EditorAnimation::Frame& frame : frames) {
            if (frame.frameNumber == cellIndex) {
                onSelectionChanged(&(frame.sprite.get()));
                return;
            }
        }

        // No frame in this cell. Pass nullptr.
        onSelectionChanged(nullptr);
    }
}

void AnimationTimeline::styleNumberText(AUI::Text& textObject,
                                        const std::string& text)
{
    textObject.setFont((Paths::FONT_DIR + "B612-Regular.ttf"), 16);
    textObject.setColor({255, 255, 255, 255});
    textObject.setHorizontalAlignment(AUI::Text::HorizontalAlignment::Center);
    textObject.setText(text);
}

} // End namespace ResourceImporter
} // End namespace AM