    /**
 * @file bug565.cpp  regression case for bug 565 ( "Clients CLIENT_FOUND_ROWS setting is ignored by maxscale" )
 *
 * - open connection with CLIENT_FOUND_ROWS flag
 * - CREATE TABLE t1(id INT PRIMARY KEY, val INT, msg VARCHAR(100))
 * - INSERT INTO t1 VALUES (1, 1, 'foo'), (2, 1, 'bar'), (3, 2, 'baz'), (4, 2, 'abc')"
 * - check 'affected_rows' for folloing UPDATES:
 *   + UPDATE t1 SET msg='xyz' WHERE val=2" (expect 2)
 *   + UPDATE t1 SET msg='xyz' WHERE val=2 (expect 0)
 *   + UPDATE t1 SET msg='xyz' WHERE val=2 (expect 2)
 */

#include <my_config.h>
#include <iostream>
#include "testconnections.h"

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(30);
    MYSQL * conn_found_rows;
    my_ulonglong rows;


    Test->repl->connect();
    Test->connect_maxscale();

    conn_found_rows = open_conn_db_flags(Test->rwsplit_port, Test->maxscale_IP, (char *) "test", Test->maxscale_user, Test->maxscale_password, CLIENT_FOUND_ROWS, Test->ssl);

    Test->set_timeout(30);
    execute_query(Test->conn_rwsplit, "DROP TABLE IF EXISTS t1");
    execute_query(Test->conn_rwsplit, "CREATE TABLE t1(id INT PRIMARY KEY, val INT, msg VARCHAR(100))");
    execute_query(Test->conn_rwsplit, "INSERT INTO t1 VALUES (1, 1, 'foo'), (2, 1, 'bar'), (3, 2, 'baz'), (4, 2, 'abc')");

    Test->set_timeout(30);
    execute_query_affected_rows(Test->conn_rwsplit, "UPDATE t1 SET msg='xyz' WHERE val=2", &rows);
    Test->tprintf("update #1: %ld (expeced value is 2)\n", (long) rows);
    if (rows != 2) {Test->add_result(1, "Affected rows is not 2\n");}

    Test->set_timeout(30);
    execute_query_affected_rows(Test->conn_rwsplit, "UPDATE t1 SET msg='xyz' WHERE val=2", &rows);
    Test->tprintf("update #2: %ld  (expeced value is 0)\n", (long) rows);
    if (rows != 0) {Test->add_result(1, "Affected rows is not 0\n");}

    Test->set_timeout(30);
    execute_query_affected_rows(conn_found_rows, "UPDATE t1 SET msg='xyz' WHERE val=2", &rows);
    Test->tprintf("update #3: %ld  (expeced value is 2)\n", (long) rows);
    if (rows != 2) {Test->add_result(1, "Affected rows is not 2\n");}

    Test->close_maxscale_connections();

    Test->copy_all_logs(); return(Test->global_result);

}
