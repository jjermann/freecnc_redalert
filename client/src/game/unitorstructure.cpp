#include "unitorstructure.h"

UnitOrStructure::UnitOrStructure() {
    references = 0;
    deleted = false;
    selected = false;
    showorder_timer = 0;
    target = NULL;
    targetCell = 0;
}

UnitOrStructure::~UnitOrStructure() {}

UnitOrStructureType::UnitOrStructureType() {
    valid = false;
    ptype = 0;
}

UnitOrStructureType::~UnitOrStructureType() {}

inline bool UnitOrStructureType::isValid() const {
    return valid;
}

inline Uint32 UnitOrStructureType::getPType() const {
    return ptype;
}

inline void UnitOrStructureType::setPType(Uint32 p) {
    ptype = p;
}

inline void UnitOrStructure::referTo() {
    ++references;
}

inline void UnitOrStructure::unrefer() {
    --references;
    if (deleted && references == 0) {
        delete this;
    }
}

inline void UnitOrStructure::select() {
    selected = true;
    showorder_timer = 0;
}

inline void UnitOrStructure::unSelect() {
    selected = false;
    showorder_timer = 0;
}

inline bool UnitOrStructure::isSelected() {
    return selected;
}

inline bool UnitOrStructure::isAlive() {
    return !deleted;
}

inline void UnitOrStructure::remove() {
    deleted = true;
    if (references == 0) {
        delete this;
    }
}

inline void UnitOrStructure::setXoffset(Sint8 xo) {}

inline void UnitOrStructure::setYoffset(Sint8 yo) {}

inline Uint16 UnitOrStructure::getTargetCell() const {
    return targetCell;

}

inline UnitOrStructure* UnitOrStructure::getTarget() {
    return target;
}
