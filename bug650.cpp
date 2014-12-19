/**
 * @file bug650.cpp  regression case for bug 650 ("Hints, RWSplit: MaxScale goes into infinite loop and crashes") and bug645
 * - setup RWSplit in the following way
 * @verbatim
[RW_Router]
type=service
router=readconnroute
servers=server1
user=skysql
passwd=skysql
version_string=5.1-OLD-Bored-Mysql
filters=DuplicaFilter

[RW_Split]
type=service
router=readwritesplit
servers=server3,server2
user=skysql
passwd=skysql

[DuplicaFilter]
type=filter
module=tee
service=RW_Split

[RW_Listener]
type=listener
service=RW_Router
protocol=MySQLClient
port=4006

[RW_Split_list]
type=listener
service=RW_Split
protocol=MySQLClient
port=4016

 @endverbatim
 * - try to connect
 * - try simple query using ReadConn router (both, master and slave)
 * - check errors in the log "Error :
 @verbatim
    Error : Couldn't find suitable Master from 2 candidates
    Error : Failed to create RW_Split session.
    Error : Creating client session for Tee filter failed. Terminating session.
    Error : Failed to create filter 'DuplicaFilter' for service 'RW_Router'
    Error : Setting up filters failed. Terminating session RW_Router
 @endverbatim
 */

#include <my_config.h>
#include <iostream>
#include "testconnections.h"

int main()
{
    TestConnections * Test = new TestConnections();
    int global_result = 0;

    Test->ReadEnv();
    Test->PrintIP();

    Test->ConnectMaxscale();
    global_result += execute_query(Test->conn_master, (char *) "show processlist");
    global_result += execute_query(Test->conn_slave, (char *) "show processlist");
    execute_query(Test->conn_rwsplit, (char *) "show processlist");
    Test->CloseMaxscaleConn();

    global_result += CheckLogErr((char *) "Error : Couldn't find suitable Master from 2 candidates", TRUE);
    global_result += CheckLogErr((char *) "Error : Failed to create RW_Split session.", TRUE);
    global_result += CheckLogErr((char *) "Error : Creating client session for Tee filter failed. Terminating session.", TRUE);
    global_result += CheckLogErr((char *) "Error : Failed to create filter 'DuplicaFilter' for service 'RW_Router'", TRUE);
    global_result += CheckLogErr((char *) "Error : Setting up filters failed. Terminating session RW_Router", TRUE);

    return(global_result);
}
