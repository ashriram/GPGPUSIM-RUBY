#include "IntPairSet.h"

// Remove from IntPairSet all IntPair where second element matches T2
IntPairSet removeMatchingSecondElement(IntPairSet set, int T2) {
    for(IntPairSet::iterator it=set.begin(); it!=set.end();) {
        if(it->second == T2)
            set.erase(it++);
        else
            it++;
    }
    return set;
}
