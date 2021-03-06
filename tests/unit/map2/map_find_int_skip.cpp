//$$CDS-header$$

#include "map2/map_find_int.h"
#include "map2/map_type_skip_list.h"

namespace map2 {
    CDSUNIT_DEFINE_SkipListMap( cc::skip_list::implementation_tag, Map_find_int)
    CDSUNIT_DEFINE_SkipListMap_nogc( cc::skip_list::implementation_tag, Map_find_int)

    CPPUNIT_TEST_SUITE_PART( Map_find_int, run_SkipListMap )
        CDSUNIT_TEST_SkipListMap
        CDSUNIT_TEST_SkipListMap_nogc
    CPPUNIT_TEST_SUITE_END_PART()
} // namespace map2
