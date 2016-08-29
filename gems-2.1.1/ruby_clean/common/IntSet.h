#ifndef INSET_H
#define INSET_H

#include "Global.h"
#include <set>
typedef std::set<int> IntSet;

// Output operator declaration
ostream& operator<<(ostream& out, const IntSet& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const IntSet& obj)
{
  //obj.print(out);
  out << flush;
  return out;
}

#endif //INSET_H
