/* !checksrc! disable COPYRIGHT all */

#include "first.h"

#include "tool1394.c"
#include "tool1604.c"
#include "tool1621.c"
#include "tool1622.c"
#include "tool1623.c"
#include "tool1720.c"

const struct entry_s s_entries[] = {
  {"tool1394", test_tool1394},
  {"tool1604", test_tool1604},
  {"tool1621", test_tool1621},
  {"tool1622", test_tool1622},
  {"tool1623", test_tool1623},
  {"tool1720", test_tool1720},
  {NULL, NULL}
};

#include "first.c"
