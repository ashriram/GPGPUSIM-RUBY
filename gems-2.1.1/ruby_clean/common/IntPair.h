#ifndef INTPAIR_H
#define INTPAIR_H

#include "Global.h"
typedef std::pair<int,int> IntPair;

IntPair getIntPair(int T1, int T2);

// Output operator declaration
ostream& operator<<(ostream& out, const IntPair& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const IntPair& obj)
{
  //obj.print(out);
  out << flush;
  return out;
}

#endif //INTPAIR_H
