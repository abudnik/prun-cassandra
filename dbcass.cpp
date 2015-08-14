#define BOOST_SPIRIT_THREADSAFE

#include "dbcass.h"
#include <boost/property_tree/json_parser.hpp>


DbConnection::DbConnection()
: cluster_( nullptr ), session_( nullptr ),
 prepared_insert_( nullptr ), prepared_delete_( nullptr )
{}

DbConnection::~DbConnection()
{
    if ( session_ )
        cass_session_free( session_ );
    if ( cluster_ )
        cass_cluster_free( cluster_ );
    if ( prepared_insert_ )
        cass_prepared_free( prepared_insert_ );
    if ( prepared_delete_ )
        cass_prepared_free( prepared_delete_ );
}


void DbCassandra::Initialize( const std::string &configPath )
{
    ParseConfig( configPath );
    const std::string remotes = config_.get<std::string>( "remotes" );

    db_ = std::make_shared<DbConnection>();

    auto cluster = cass_cluster_new();
    db_->SetCluster( cluster );
    auto session = cass_session_new();
    db_->SetSession( session );

    cass_cluster_set_contact_points( cluster, remotes.c_str() );

    CassFuture *connect_future = cass_session_connect( session, cluster );
    const CassError err = cass_future_error_code( connect_future );
    cass_future_free( connect_future );
    if ( err != CASS_OK )
        throw std::logic_error( cass_error_desc( err ) );

    const CassPrepared *prepared = nullptr;
    if ( PrepareQuery( session, "INSERT INTO prun.jobs (job_id, job_descr) VALUES (?, ?);", reinterpret_cast<const CassPrepared**>( &prepared ) ) != CASS_OK )
        throw std::logic_error( cass_error_desc( err ) );
    db_->SetPreparedInsert( prepared );

    if ( PrepareQuery( session, "DELETE FROM prun.jobs WHERE job_id = ?;", reinterpret_cast<const CassPrepared**>( &prepared ) ) != CASS_OK )
        throw std::logic_error( cass_error_desc( err ) );
    db_->SetPreparedDelete( prepared );
}

void DbCassandra::Shutdown()
{
    db_.reset();
    config_.clear();
}

void DbCassandra::Put( const std::string &key, const std::string &value )
{
    DbPtr db = db_;
    if ( db )
    {
        auto statement = cass_prepared_bind( db_->GetPreparedInsert() );
        cass_statement_bind_string( statement, 0, key.c_str() );
        cass_statement_bind_string( statement, 1, value.c_str() );
        auto query_future = cass_session_execute( db_->GetSession(), statement );
        cass_statement_free( statement );
        CassError err = cass_future_error_code( query_future );
        cass_future_free( query_future );
        if ( err != CASS_OK )
            throw std::logic_error( cass_error_desc( err ) );
    }
}

void DbCassandra::Delete( const std::string &key )
{
    DbPtr db = db_;
    if ( db )
    {
        auto statement = cass_prepared_bind( db_->GetPreparedDelete() );
        cass_statement_bind_string( statement, 0, key.c_str() );
        auto query_future = cass_session_execute( db_->GetSession(), statement );
        cass_statement_free( statement );
        CassError err = cass_future_error_code( query_future );
        cass_future_free( query_future );
        if ( err != CASS_OK )
            throw std::logic_error( cass_error_desc( err ) );
    }
}

void DbCassandra::GetAll( GetCallback callback )
{
    DbPtr db = db_;
    if ( db )
    {
        auto statement = cass_statement_new( "SELECT * FROM prun.jobs;", 0 );
        auto query_future = cass_session_execute( db_->GetSession(), statement );
        cass_statement_free( statement );

        CassError err = cass_future_error_code( query_future );
        if ( err != CASS_OK )
        {
            cass_future_free( query_future );
            throw std::logic_error( cass_error_desc( err ) );
        }

        auto result = cass_future_get_result( query_future );
        cass_future_free( query_future );

        auto iterator = cass_iterator_from_result( result );

        while( cass_iterator_next( iterator ) )
        {
            const CassRow *row = cass_iterator_get_row( iterator );

            const char *job_id, *job_descr;
            size_t job_id_length, job_descr_length;
            cass_value_get_string( cass_row_get_column( row, 0 ), &job_id, &job_id_length );
            cass_value_get_string( cass_row_get_column( row, 1 ), &job_descr, &job_descr_length );
            callback( std::string( job_id, job_id_length ), std::string( job_descr, job_descr_length ) );
        }

        cass_iterator_free( iterator );
        cass_result_free( result );
    }
}

void DbCassandra::ParseConfig( const std::string &configPath )
{
    std::ifstream file( configPath.c_str() );
    if ( !file.is_open() )
        throw std::logic_error( "DbCassandra::ParseConfig: couldn't open " + configPath );

    boost::property_tree::read_json( file, config_ );
}

CassError DbCassandra::PrepareQuery( CassSession *session, const char *query, const CassPrepared **prepared ) const
{
    CassFuture *future = cass_session_prepare( session, query );
    cass_future_wait( future );

    CassError err = cass_future_error_code( future );
    if ( err == CASS_OK )
        *prepared = cass_future_get_prepared( future );

    cass_future_free( future );
    return err;
}

common::IHistory *CreateHistory( int interfaceVersion )
{
    return interfaceVersion == common::HISTORY_VERSION ?
        new DbCassandra : nullptr;
}

void DestroyHistory( const common::IHistory *history )
{
    delete history;
}
