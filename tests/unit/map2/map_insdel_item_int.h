//$$CDS-header$$

#include "map2/map_type.h"
#include "cppunit/thread.h"

#include <vector>

namespace map2 {

#   define TEST_MAP(IMPL, C, X)         void C::X() { test<map_type<IMPL, key_type, value_type>::X >(); }
#   define TEST_MAP_NOLF(IMPL, C, X)    void C::X() { test_nolf<map_type<IMPL, key_type, value_type>::X >(); }
#   define TEST_MAP_EXTRACT(IMPL, C, X)  TEST_MAP(IMPL, C, X)
#   define TEST_MAP_NOLF_EXTRACT(IMPL, C, X) TEST_MAP_NOLF(IMPL, C, X)

    class Map_InsDel_Item_int: public CppUnitMini::TestCase
    {
        static size_t  c_nMapSize;        // map size
        static size_t  c_nThreadCount;    // thread count
        static size_t  c_nGoalItem;
        static size_t  c_nAttemptCount;   // count of SUCCESS insert/delete for each thread
        static size_t  c_nMaxLoadFactor;  // maximum load factor
        static bool    c_bPrintGCState;

        typedef CppUnitMini::TestCase Base;
        typedef size_t  key_type;
        typedef size_t  value_type;

        template <class MAP>
        class Inserter: public CppUnitMini::TestThread
        {
            MAP&     m_Map;

            virtual Inserter *    clone()
            {
                return new Inserter( *this );
            }

            struct ensure_func
            {
                void operator()( bool bNew, std::pair<key_type const, value_type>& item )
                {
                    if ( bNew )
                        item.second = item.first;
                }
                // for boost::container::flat_map
                void operator()( bool bNew, std::pair<key_type, value_type>& item )
                {
                    if ( bNew )
                        item.second = item.first;
                }

                // for BronsonAVLTreeMap
                void operator()( bool bNew, key_type key, value_type& val )
                {
                    if ( bNew )
                        val = key;
                }
            };

        public:
            size_t  m_nInsertSuccess;
            size_t  m_nInsertFailed;

        public:
            Inserter( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Inserter( Inserter& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDel_Item_int&  getTest()
            {
                return reinterpret_cast<Map_InsDel_Item_int&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map;

                m_nInsertSuccess =
                    m_nInsertFailed = 0;

                size_t nGoalItem = c_nGoalItem;
                for ( size_t nAttempt = 0; nAttempt < c_nAttemptCount; ) {
                    if ( nAttempt % 2  == 0 ) {
                        if ( rMap.insert( nGoalItem, nGoalItem )) {
                            ++m_nInsertSuccess;
                            ++nAttempt;
                        }
                        else
                            ++m_nInsertFailed;
                    }
                    else {
                        std::pair<bool, bool> ensureResult = rMap.ensure( nGoalItem, ensure_func() );
                        if ( ensureResult.second ) {
                            ++m_nInsertSuccess;
                            ++nAttempt;
                        }
                        else
                            ++m_nInsertFailed;
                    }
                }
            }
        };

        template <class MAP>
        class Deleter: public CppUnitMini::TestThread
        {
            MAP&     m_Map;

            virtual Deleter *    clone()
            {
                return new Deleter( *this );
            }
        public:
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;

        public:
            Deleter( CppUnitMini::ThreadPool& pool, MAP& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            Deleter( Deleter& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDel_Item_int&  getTest()
            {
                return reinterpret_cast<Map_InsDel_Item_int&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                MAP& rMap = m_Map;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0;

                size_t nGoalItem = c_nGoalItem;
                for ( size_t nAttempt = 0; nAttempt < c_nAttemptCount; ) {
                    if ( rMap.erase( nGoalItem )) {
                        ++m_nDeleteSuccess;
                        ++nAttempt;
                    }
                    else
                        ++m_nDeleteFailed;
                }
            }
        };

    protected:

        template <class MAP>
        void do_test( MAP& testMap )
        {
            typedef Inserter<MAP>       InserterThread;
            typedef Deleter<MAP>        DeleterThread;
            cds::OS::Timer    timer;

            // Fill the map
            CPPUNIT_MSG( "  Fill map (" << c_nMapSize << " items)...");
            timer.reset();
            {
                std::vector<key_type>   v;
                v.reserve( c_nMapSize );
                for ( size_t i = 0; i < c_nMapSize; ++i )
                    v.push_back( i );
                shuffle( v.begin(), v.end() );
                for ( size_t i = 0; i < v.size(); ++i ) {
                    CPPUNIT_ASSERT( testMap.insert( v[i], v[i] ));
                }
            }
            CPPUNIT_MSG( "   Duration=" << timer.duration() );

            CPPUNIT_MSG( "  Insert/delete the key " << c_nGoalItem << " (" << c_nAttemptCount << " successful times)...");
            CppUnitMini::ThreadPool pool( *this );
            pool.add( new InserterThread( pool, testMap ), (c_nThreadCount + 1) / 2 );
            pool.add( new DeleterThread( pool, testMap ), (c_nThreadCount + 1) / 2 );
            pool.run();
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration() );

            size_t nInsertSuccess = 0;
            size_t nInsertFailed = 0;
            size_t nDeleteSuccess = 0;
            size_t nDeleteFailed = 0;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                InserterThread * pThread = dynamic_cast<InserterThread *>( *it );
                if ( pThread ) {
                    CPPUNIT_CHECK( pThread->m_nInsertSuccess == c_nAttemptCount );
                    nInsertSuccess += pThread->m_nInsertSuccess;
                    nInsertFailed += pThread->m_nInsertFailed;
                }
                else {
                    DeleterThread * p = static_cast<DeleterThread *>( *it );
                    CPPUNIT_CHECK( p->m_nDeleteSuccess == c_nAttemptCount );
                    nDeleteSuccess += p->m_nDeleteSuccess;
                    nDeleteFailed += p->m_nDeleteFailed;
                }
            }
            CPPUNIT_CHECK( nInsertSuccess == nDeleteSuccess );
            size_t nGoalItem = c_nGoalItem;
            CPPUNIT_CHECK( testMap.find( nGoalItem ));


            CPPUNIT_MSG( "    Totals: Ins fail=" << nInsertFailed << " Del fail=" << nDeleteFailed );

            // Check if the map contains all items
            CPPUNIT_MSG( "    Check if the map contains all items" );
            timer.reset();
            for ( size_t i = 0; i < c_nMapSize; ++i ) {
                CPPUNIT_CHECK_EX( testMap.find( i ), "key " << i );
            }
            CPPUNIT_MSG( "    Duration=" << timer.duration() );

            check_before_cleanup( testMap );

            testMap.clear();
            additional_check( testMap );
            print_stat( testMap );
            additional_cleanup( testMap );
        }

        template <class MAP>
        void test()
        {
            for ( size_t nLoadFactor = 1; nLoadFactor <= c_nMaxLoadFactor; nLoadFactor *= 2 ) {
                CPPUNIT_MSG( "Load factor=" << nLoadFactor );
                MAP testMap( c_nMapSize, nLoadFactor );
                do_test( testMap );
                if ( c_bPrintGCState )
                    print_gc_state();
            }
        }

        template <class MAP>
        void test_nolf()
        {
            MAP testMap;
            do_test<MAP>( testMap );
            if ( c_bPrintGCState )
                print_gc_state();
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg );

        void run_MichaelMap(const char *in_name, bool invert = false);
        void run_SplitList(const char *in_name, bool invert = false);
        void run_SkipListMap(const char *in_name, bool invert = false);
        void run_StripedMap(const char *in_name, bool invert = false);
        void run_RefinableMap(const char *in_name, bool invert = false);
        void run_CuckooMap(const char *in_name, bool invert = false);
        void run_EllenBinTreeMap(const char *in_name, bool invert = false);
        void run_BronsonAVLTreeMap(const char *in_name, bool invert = false);

        virtual void myRun(const char *in_name, bool invert = false);

#   include "map2/map_defs.h"
        CDSUNIT_DECLARE_MichaelMap
        CDSUNIT_DECLARE_SplitList
        CDSUNIT_DECLARE_SkipListMap
        CDSUNIT_DECLARE_EllenBinTreeMap
        CDSUNIT_DECLARE_BronsonAVLTreeMap
        CDSUNIT_DECLARE_StripedMap
        CDSUNIT_DECLARE_RefinableMap
        CDSUNIT_DECLARE_CuckooMap
        //CDSUNIT_DECLARE_StdMap    // very slow!
    };
} // namespace map2
