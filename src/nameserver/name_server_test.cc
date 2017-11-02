//
// name_server_test.cc
// Copyright (C) 2017 4paradigm.com
// Author denglong 
// Date 2017-09-07
//

#include "gtest/gtest.h"
#include "logging.h"
#include "timer.h"
#include <gflags/gflags.h>
#include <sched.h>
#include <unistd.h>
#include "tablet/tablet_impl.h"
#include "proto/tablet.pb.h"
#include "proto/name_server.pb.h"
#include <boost/lexical_cast.hpp>
#include "name_server_impl.h"
#include "rpc/rpc_client.h"
#include "base/file_util.h"

DECLARE_string(endpoint);
DECLARE_string(db_root_path);
DECLARE_string(zk_cluster);
DECLARE_string(zk_root_path);
DECLARE_int32(zk_session_timeout);
DECLARE_int32(zk_keep_alive_check_interval);

using ::rtidb::zk::ZkClient;


namespace rtidb {
namespace nameserver {


uint32_t counter = 10;
static int32_t endpoint_size = 1;


inline std::string GenRand() {
    return boost::lexical_cast<std::string>(rand() % 10000000 + 1);
}

class MockClosure : public ::google::protobuf::Closure {

public:
    MockClosure() {}
    ~MockClosure() {}
    void Run() {}

};
class NameServerImplTest : public ::testing::Test {

public:
    NameServerImplTest() {}
    ~NameServerImplTest() {}
};


TEST_F(NameServerImplTest, MakesnapshotTask) {
    FLAGS_endpoint="127.0.0.1:9530";
    FLAGS_zk_cluster="127.0.0.1:12181";
    FLAGS_zk_root_path="/rtidb3";

    FLAGS_endpoint = "127.0.0.1:9631";
    NameServerImpl* nameserver = new NameServerImpl();
    bool ok = nameserver->Init();
    ASSERT_TRUE(ok);
    endpoint_size++;
    sleep(4);
    sofa::pbrpc::RpcServerOptions options;
    sofa::pbrpc::RpcServer rpc_server(options);
    sofa::pbrpc::Servlet webservice =
            sofa::pbrpc::NewPermanentExtClosure(nameserver, &rtidb::nameserver::NameServerImpl::WebService);
    if (!rpc_server.RegisterService(nameserver)) {
       LOG(WARNING, "fail to register nameserver rpc service");
       exit(1);
    }
    rpc_server.RegisterWebServlet("/nameserver", webservice);
    if (!rpc_server.Start(FLAGS_endpoint)) {
        LOG(WARNING, "fail to listen port %s", FLAGS_endpoint.c_str());
        exit(1);
    }
    ::rtidb::RpcClient name_server_client;
    ::rtidb::nameserver::NameServer_Stub *stub = NULL;
    name_server_client.GetStub(FLAGS_endpoint, &stub);

    FLAGS_endpoint="127.0.0.1:9530";
    ::rtidb::tablet::TabletImpl* tablet = new ::rtidb::tablet::TabletImpl();
    ok = tablet->Init();
    ASSERT_TRUE(ok);
    sleep(2);
    sofa::pbrpc::RpcServerOptions options1;
    sofa::pbrpc::RpcServer rpc_server1(options1);
    sofa::pbrpc::Servlet webservice1 =
            sofa::pbrpc::NewPermanentExtClosure(tablet, &rtidb::tablet::TabletImpl::WebService);
    if (!rpc_server1.RegisterService(tablet)) {
       LOG(WARNING, "fail to register nameserver rpc service");
       exit(1);
    }
    rpc_server1.RegisterWebServlet("/tablet", webservice1);
    if (!rpc_server1.Start(FLAGS_endpoint)) {
        LOG(WARNING, "fail to listen port %s", FLAGS_endpoint.c_str());
        exit(1);
    }
    sleep(2);
    
    CreateTableRequest request;
    GeneralResponse response;
    TableInfo *table_info = request.mutable_table_info();
    std::string name = "test" + GenRand();
    table_info->set_name(name);
    TablePartition* partion = table_info->add_table_partition();
    partion->set_endpoint("127.0.0.1:9530");
    partion->set_is_leader(true);
    partion->set_pid(0);
    ok = name_server_client.SendRequest(stub,
            &::rtidb::nameserver::NameServer_Stub::CreateTable,
            &request, &response, 12, 1);
    ASSERT_TRUE(ok);


    MakeSnapshotNSRequest m_request;
    m_request.set_name(name);
    m_request.set_pid(0);
    ok = name_server_client.SendRequest(stub,
            &::rtidb::nameserver::NameServer_Stub::MakeSnapshotNS,
            &m_request, &response, 12, 1);
    ASSERT_TRUE(ok);

    sleep(5);

    ZkClient zk_client(FLAGS_zk_cluster, 1000, FLAGS_endpoint, FLAGS_zk_root_path);
    ok = zk_client.Init();
    ASSERT_TRUE(ok);
    std::string op_index_node = FLAGS_zk_root_path + "/op/op_index";
    std::string value;
    ok = zk_client.GetNodeValue(op_index_node, value);
    ASSERT_TRUE(ok);
    std::string op_node = FLAGS_zk_root_path + "/op/op_data/" + value;
    ok = zk_client.GetNodeValue(op_node, value);
    ASSERT_FALSE(ok);

    value.clear();
    std::string table_index_node = FLAGS_zk_root_path + "/table/table_index";
    ok = zk_client.GetNodeValue(table_index_node, value);
    ASSERT_TRUE(ok);
    std::string snapshot_path = FLAGS_db_root_path + "/" + value + "_0/snapshot/";
	std::vector<std::string> vec;
    ok = ::rtidb::base::GetFileName(snapshot_path, vec);
    ASSERT_EQ(0, ok);
    ASSERT_EQ(2, vec.size());

    std::string table_data_node = FLAGS_zk_root_path + "/table/table_data/" + name; 
    ok = zk_client.GetNodeValue(table_data_node, value);
    ASSERT_TRUE(ok);
    ::rtidb::nameserver::TableInfo table_info1;
    table_info1.ParseFromString(value);
    ASSERT_STREQ(table_info->name().c_str(), table_info1.name().c_str());
    ASSERT_EQ(table_info->table_partition_size(), table_info1.table_partition_size());

}

}
}

int main(int argc, char** argv) {

    ::testing::InitGoogleTest(&argc, argv);
    srand (time(NULL));
    ::baidu::common::SetLogLevel(::baidu::common::DEBUG);
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    FLAGS_db_root_path = "/tmp/" + ::rtidb::nameserver::GenRand();
    return RUN_ALL_TESTS();
}


