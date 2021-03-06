//$$CDS-header$$

#include "set2/set_insdelfind.h"
#include "set2/set_type_std.h"

namespace set2 {
    CDSUNIT_DEFINE_StdSet(std_implementation_tag, Set_InsDelFind)

    CPPUNIT_TEST_SUITE_PART( Set_InsDelFind, run_StdSet )
        CDSUNIT_TEST_StdSet
    CPPUNIT_TEST_SUITE_END_PART()
} // namespace set2
