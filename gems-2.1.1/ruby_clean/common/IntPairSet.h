#ifndef INTPAIRSET_H
#define INTPAIRSET_H

#include "Global.h"
#include "IntPair.h"
#include <set>
typedef std::set<IntPair> IntPairSet;

IntPairSet removeMatchingSecondElement(IntPairSet set, int T2);

// Output operator declaration
ostream& operator<<(ostream& out, const IntPairSet& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const IntPairSet& obj)
{
  //obj.print(out);
  out << flush;
  return out;
}

#endif //INTPAIRSET_H
