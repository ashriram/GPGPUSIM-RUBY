// This template class is an alias for the CacheMemory template class.
// This is needed if one wants to define two CacheMemory in the same controller
// and have each use a different ENTRY type.
// Since the older C++ standard does not allow aliasing template classes,
// this class is simply inheriting CacheMemory

#ifndef DIRECTORYCACHEMEMORY_H
#define DIRECTORYCACHEMEMORY_H

#include "CacheMemory.h"

template<typename ENTRY>
class DirectoryCacheMemory : public CacheMemory<ENTRY> {

public:
    // Constructors
    DirectoryCacheMemory(AbstractChip* chip_ptr, int numSetBits, int cacheAssoc, const MachineType machType, const string& description)
    : CacheMemory<ENTRY>(chip_ptr, numSetBits, cacheAssoc, machType, description) {

    }

};

#endif // DIRECTORYCACHEMEMORY_H
