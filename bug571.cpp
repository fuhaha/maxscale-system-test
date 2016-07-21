/**
 * @file bug571.cpp  regression case for bug 571 and bug 585 ( "Using regex filter hangs MaxScale" and "modutil_extract_SQL doesn't work with multiple GWBUF buffers" )
 *
 * - Maxscale.cnf
 * @verbatim
 [regex]
 type=filter
 module=regexfilter
 match=[Ff][Oo0][rR][mM]
 replace=FROM

 [r2]
 type=filter
 module=regexfilter
 match=fetch
 replace=select

 [hints]
 type=filter
 module=hintfilter

 [RW Split Router]
 type=service
 router= readwritesplit
 servers=server1,     server2,              server3,server4
 user=skysql
 passwd=skysql
 max_slave_connections=100%
 use_sql_variables_in=all
 router_options=slave_selection_criteria=LEAST_BEHIND_MASTER
 filters=hints|regex|r2
 @endverbatim
 * for bug585:
 * @verbatim
[regex]
type=filter
module=regexfilter
match=fetch
replace=select

[typo]
type=filter
module=regexfilter
match=[Ff][Oo0][Rr][Mm]
replace=from

[RW Split Router]
type=service
router= readwritesplit
servers=server1,     server2,              server3,server4
user=skysql
passwd=skysql
max_slave_connections=100%
use_sql_variables_in=all
router_options=slave_selection_criteria=LEAST_BEHIND_MASTER
filters=regex|typo
 @endverbatim
 * - fetch * from mysql.user;
 * - fetch count(*) form mysql.user;
 * - check if Maxscale is alive
 */


// the same code is used for bug585

#include <my_config.h>
#include <iostream>
#include <unistd.h>
#include "testconnections.h"

using namespace std;

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);

    Test->tprintf("Connecting to all MaxScale services\n");
    Test->set_timeout(10);
    Test->add_result(Test->connect_maxscale(), "Error connectiong to Maxscale\n");

    Test->tprintf("executing fetch * from mysql.user \n");
    Test->set_timeout(10);
    Test->try_query(Test->conn_rwsplit, (char *) "fetch * from mysql.user;");
    Test->set_timeout(10);
    Test->try_query(Test->conn_rwsplit, (char *) "fetch count(*) form mysql.user;");

    Test->set_timeout(10);
    Test->close_maxscale_connections();
    Test->check_maxscale_alive();
    Test->copy_all_logs(); return(Test->global_result);
}
