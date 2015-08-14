#include <memory>
#include <boost/property_tree/ptree.hpp>
#include "history.h"
#include "cassandra.h"


class DbConnection
{
public:
    DbConnection();
    ~DbConnection();

    void SetCluster( CassCluster *cluster ) { cluster_ = cluster; }
    void SetSession( CassSession *session ) { session_ = session; }
    void SetPreparedInsert( const CassPrepared *prepared ) { prepared_insert_ = prepared; }
    void SetPreparedDelete( const CassPrepared *prepared ) { prepared_delete_ = prepared; }

    CassCluster *GetCluster() const { return cluster_; }
    CassSession *GetSession() const { return session_; }
    const CassPrepared *GetPreparedInsert() const { return prepared_insert_; }
    const CassPrepared *GetPreparedDelete() const { return prepared_delete_; }

private:
    CassCluster *cluster_;
    CassSession *session_;
    const CassPrepared *prepared_insert_, *prepared_delete_;
};

class DbCassandra : public common::IHistory
{
    typedef std::shared_ptr< DbConnection > DbPtr;
    
public:
    virtual void Initialize( const std::string &configPath );
    virtual void Shutdown();

    virtual void Put( const std::string &key, const std::string &value );
    virtual void Delete( const std::string &key );

    typedef void (*GetCallback)( const std::string &key, const std::string &value );
    virtual void GetAll( GetCallback callback );

private:
    void ParseConfig( const std::string &configPath );
    CassError PrepareQuery( CassSession *session, const char *query, const CassPrepared **prepared ) const;

private:
    DbPtr db_;
    boost::property_tree::ptree config_;
};
