#ifndef CLUSTERING_ADMINISTRATION_STATS_REQUEST_HPP_
#define CLUSTERING_ADMINISTRATION_STATS_REQUEST_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "clustering/administration/metadata.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/datum.hpp"

class server_name_client_t;

class parsed_stats_t {
public:
    struct table_stats_t {
        double read_docs_per_sec;
        double read_docs_total;
        double written_docs_per_sec;
        double written_docs_total;
        double in_use_bytes;
        double cache_size;
        double metadata_bytes;
        double data_bytes;
        double garbage_bytes;
        double preallocated_bytes;
        double read_bytes_per_sec;
        double read_bytes_total;
        double written_bytes_per_sec;
        double written_bytes_total;
    };

    struct server_stats_t {
        bool responsive;
        double queries_per_sec;
        double queries_total;
        double client_connections;

        std::map<namespace_id_t, table_stats_t> tables;
    };

    parsed_stats_t(const std::map<server_id_t, ql::datum_t> &stats);

    // Accumulate a field in all servers
    double accumulate(double server_stats_t::*field) const;

    // Accumulate a field in all tables (across all servers)
    double accumulate(double table_stats_t::*field) const;

    // Accumulate a field in a specific table (across all servers)
    double accumulate_table(const namespace_id_t &table_id,
                            double table_stats_t::*field) const;

    // Accumulate a field in all tables (on a specific server)
    double accumulate_server(const server_id_t &server_id,
                             double table_stats_t::*field) const;

    std::map<server_id_t, server_stats_t> servers;
    std::set<namespace_id_t> all_table_ids;

private:
    void add_perfmon_value(const ql::datum_t &perf,
                           const std::string &key,
                           double *value_out);

    void add_shard_values(const ql::datum_t &shard_perf,
                          table_stats_t *stats_out);

    void add_serializer_values(const ql::datum_t &ser_perf,
                               table_stats_t *);

    void add_query_engine_stats(const ql::datum_t &qe_perf,
                                server_stats_t *stats_out);

    void add_table_stats(const namespace_id_t &table_id,
                         const ql::datum_t &table_perf,
                         server_stats_t *stats_out);
};

class stats_request_t {
public:
    typedef cluster_semilattice_metadata_t metadata_t;

    static std::set<std::string> global_stats_filter();
    static std::vector<std::pair<server_id_t, peer_id_t> > all_peers(
            server_name_client_t *name_client);

    virtual ~stats_request_t() { }

    virtual std::set<std::string> get_filter() const = 0;

    virtual bool get_peers(server_name_client_t *name_client,
                           std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                           std::string *error_out) const = 0;

    virtual ql::datum_t to_datum(const parsed_stats_t &stats,
                                 const metadata_t &metadata) const = 0;
};

class cluster_stats_request_t : public stats_request_t {
    static const char *cluster_request_type;
public:
    static const char *get_name() { return cluster_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out,
                      std::string *error_out);

    cluster_stats_request_t();

    std::set<std::string> get_filter() const;

    bool get_peers(server_name_client_t *name_client,
                   std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                   std::string *error_out) const;

    ql::datum_t to_datum(const parsed_stats_t &stats,
                         UNUSED const metadata_t &metadata) const;
};

class table_stats_request_t : public stats_request_t {
    static const char *table_request_type;
    const namespace_id_t table_id;
public:

    static const char *get_name() { return table_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out,
                      std::string *error_out);

    table_stats_request_t(const namespace_id_t &_table_id);

    std::set<std::string> get_filter() const;

    bool get_peers(server_name_client_t *name_client,
                   std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                   std::string *error_out) const;

    ql::datum_t to_datum(const parsed_stats_t &stats,
                         const metadata_t &metadata) const;
};

class server_stats_request_t : public stats_request_t {
    static const char *server_request_type;
    const server_id_t server_id;
public:
    static const char *get_name() { return server_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out,
                      std::string *error_out);

    server_stats_request_t(const server_id_t &_server_id);

    std::set<std::string> get_filter() const;

    bool get_peers(server_name_client_t *name_client,
                   std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                   std::string *error_out) const;

    ql::datum_t to_datum(const parsed_stats_t &stats,
                         const metadata_t &metadata) const;
};

class table_server_stats_request_t : public stats_request_t {
    static const char *table_server_request_type;
    const namespace_id_t table_id;
    const server_id_t server_id;
public:
    static const char *get_name() { return table_server_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out,
                      std::string *error_out);

    table_server_stats_request_t(const namespace_id_t &_table_id,
                                 const server_id_t &_server_id);

    std::set<std::string> get_filter() const;

    bool get_peers(server_name_client_t *name_client,
                   std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                   std::string *error_out) const;
    ql::datum_t to_datum(const parsed_stats_t &stats,
                         const metadata_t &metadata) const;
};

#endif // CLUSTERING_ADMINISTRATION_STATS_REQUEST_HPP_
