/**
 * @file bug143.cpp bug143 regression case (MaxScale ignores host in user authentication)
 *
 * - create  user@'non_existing_host1', user@'%', user@'non_existing_host2' identified by different passwords.
 * - try to connect using RWSplit. First and third are expected to fail, second - succseed.
 */


/*
Kolbe Kegel 2013-08-02 07:07:34 UTC
MaxScale does not consider the hostname of the user account in the mysql.user table when building its user credentials cache or performing authentication.

The MySQL privilege system defines users as a combination of user@host. You can have an effectively unlimited number of users with the same user *name* but different *hosts* and each of those users has essentially no relationship to the others, and each can have totally different privileges.

server/core/dbusers.c:152:

        if (mysql_query(con, "SELECT user, password FROM mysql.user"))

When MaxScale fetches user authentication details, it simply gets user and password from mysql.user and completely ignores the host.

When a user then authenticates to MaxScale, MaxScale simply chooses the *first* matching "user" entry from its hashtable, which may not be the same user MaxScale will use when it attempts to connect to the backend MySQL server.

MaxScale should either exclude all users except those that might be used for authentication or provide a list of possibly conflicting users to the user or take some other action to minimize the possibility of this kind of confusing situation.

server1> select user, host, password from mysql.user;
+---------+---------------+-------------------------------------------+
| user    | host          | password                                  |
+---------+---------------+-------------------------------------------+
| maxuser | 127.0.0.1     | *5EDBD32E469DAE0CE10E6999C3899DEFCB9F12E0 |
| root    | 127.0.0.1     | *196BDEDE2AE4F84CA44C47D54D78478C7E2BD7B7 |
| root    | localhost     | *196BDEDE2AE4F84CA44C47D54D78478C7E2BD7B7 |
| user    | 127.0.0.1     | *B69027D44F6E5EDC07F1AEAD1477967B16F28227 |
| user    | centos6-build | *F24059C44AE7FCD38A595267C522FB133E9F06F1 |
+---------+---------------+-------------------------------------------+
5 rows in set (0.01 sec)

server1> select password('x'), password('z')\G
*************************** 1. row ***************************
password('x'): *B69027D44F6E5EDC07F1AEAD1477967B16F28227
password('z'): *F24059C44AE7FCD38A595267C522FB133E9F06F1
1 row in set (0.00 sec)

[kolbe@centos6-build MaxScale]$ ~/dev/mariadb-5.5.30-linux-x86_64/bin/mysql -h 127.0.0.1 -P 4008 -u user -px -e 'select user, host, password from mysql.user where concat(user,"@",host)=current_user()'
+------+-----------+-------------------------------------------+
| user | host      | password                                  |
+------+-----------+-------------------------------------------+
| user | 127.0.0.1 | *B69027D44F6E5EDC07F1AEAD1477967B16F28227 |
+------+-----------+-------------------------------------------+

server1> alter table mysql.user order by user, host desc;
Query OK, 5 rows affected (0.02 sec)
Records: 5  Duplicates: 0  Warnings: 0

server1> select user, host, password from mysql.user;
+---------+---------------+-------------------------------------------+
| user    | host          | password                                  |
+---------+---------------+-------------------------------------------+
| maxuser | 127.0.0.1     | *5EDBD32E469DAE0CE10E6999C3899DEFCB9F12E0 |
| root    | localhost     | *196BDEDE2AE4F84CA44C47D54D78478C7E2BD7B7 |
| root    | 127.0.0.1     | *196BDEDE2AE4F84CA44C47D54D78478C7E2BD7B7 |
| user    | centos6-build | *F24059C44AE7FCD38A595267C522FB133E9F06F1 |
| user    | 127.0.0.1     | *B69027D44F6E5EDC07F1AEAD1477967B16F28227 |
+---------+---------------+-------------------------------------------+
5 rows in set (0.00 sec)

MaxScale> reload users 0x1884390
Loaded 5 database users for server Test Service.

[kolbe@centos6-build MaxScale]$ ~/dev/mariadb-5.5.30-linux-x86_64/bin/mysql -h 127.0.0.1 -P 4008 -u user -px -e 'select user, host, password from mysql.user where concat(user,"@",host)=current_user()'
ERROR 1045 (2800): Authorization failed
*/



#include <iostream>
#include "testconnections.h"

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);

    Test->set_timeout(5);
    Test->repl->connect();
    Test->connect_maxscale();

    Test->tprintf("Creating user 'user' with 3 different passwords for different hosts\n");
    execute_query(Test->conn_rwsplit, (char *) "GRANT ALL PRIVILEGES ON *.* TO user@'non_existing_host1' identified by 'pass1';  FLUSH PRIVILEGES;");
    execute_query(Test->conn_rwsplit, (char *) "GRANT ALL PRIVILEGES ON *.* TO user@'%%'  identified by 'pass2';  FLUSH PRIVILEGES;");
    execute_query(Test->conn_rwsplit, (char *) "GRANT ALL PRIVILEGES ON *.* TO user@'non_existing_host2' identified by 'pass3';  FLUSH PRIVILEGES;");

    printf("sleeping 20 seconds to let replication happen\n");  fflush(stdout);
    Test->set_timeout(50);
    Test->repl->sync_slaves();

    Test->set_timeout(5);
    MYSQL * conn = open_conn(Test->rwsplit_port, Test->maxscale_IP, (char *) "user", (char *) "pass1", Test->ssl);
    if (mysql_errno(conn) == 0) {
        Test->add_result(1, "MaxScale ignores host in authentification\n");
    }
    if (conn != NULL) {mysql_close(conn);}

    conn = open_conn(Test->rwsplit_port, Test->maxscale_IP, (char *) "user", (char *) "pass2", Test->ssl);
    Test->add_result(mysql_errno(conn), "MaxScale can't connect: %s\n", mysql_error(conn));
    if (conn != NULL) {mysql_close(conn);}

    conn = open_conn(Test->rwsplit_port, Test->maxscale_IP, (char *) "user", (char *) "pass3", Test->ssl);
    if (mysql_errno(conn) == 0) {
        Test->add_result(1, "MaxScale ignores host in authentification\n");
    }
    if (conn != NULL) {mysql_close(conn);}

    execute_query(Test->conn_rwsplit, (char *) "DROP USER user@'%%';");
    execute_query(Test->conn_rwsplit, (char *) "DROP USER user@'non_existing_host1';");
    execute_query(Test->conn_rwsplit, (char *) "DROP USER user@'non_existing_host2';");
    Test->close_maxscale_connections();

    Test->copy_all_logs(); return(Test->global_result);

}
