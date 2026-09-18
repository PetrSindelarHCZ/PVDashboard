#pragma once
#include <Arduino.h>
#include "../display/IDisplay.h"
#include "../data/DataModel.h"
#include "../navigation/NavigationTypes.h"

class IScreen {
public:
    virtual ~IScreen() = default;

    virtual String getId() const = 0;
    virtual String getTitle() const = 0;
    virtual void render(IDisplay& display, const DataModel& dataModel) = 0;

    // Main screens participate in the vertical sidebar by default. Secondary
    // views (for example an hourly weather detail) can opt out and point back
    // to their owning sidebar item.
    virtual bool isSidebarEntry() const { return true; }
    virtual String getSidebarParentId() const { return getId(); }

    // Navigation is derived from the current runtime page layout, not from a
    // hard-coded neighbour graph. A future configurable page renderer can
    // populate this structure directly from its widget configuration.
    virtual void buildNavigationLayout(const DataModel& dataModel, NavigationLayout& layout) const {
        (void)dataModel;
        layout.clear();
    }

    // Some screens consist of multiple peer subpages (for example one
    // weather page per configured location). With more than one subpage the
    // navigation controller inserts a generic pager level between sidebar and
    // element navigation. Future screens can opt into the same behavior
    // without adding screen-specific rules to NavigationController.
    virtual uint8_t getNavigationSubpageCount(const DataModel& dataModel) const {
        (void)dataModel;
        return 1;
    }

    virtual uint8_t getInitialNavigationSubpage(const DataModel& dataModel) const {
        (void)dataModel;
        return 0;
    }
};
