/**
 * @file bug676.cpp  reproducing attempt for bug676 ("Memory corruption when users with long hostnames that can no the resolved are loaded into MaxScale")
 * - Configuration
 * @verbatim
[MySQL Monitor]
type=monitor
module=galeramon
servers=server1,server2,server3
user=skysql
passwd=skysql

[RW Split Router]
type=service
router=readwritesplit
servers=server1,server2,server3
#user=maxpriv
#passwd=maxpwd
user=skysql
passwd=skysql
filters=MyLogFilter
version_string=MariaDBEC-10.0.14
localhost_match_wildcard_host=1
max_slave_connections=1

[Read Connection Router]
type=service
router=readconnroute
router_options=synced
servers=server1,server2,server3
user=skysql
passwd=skysql

[Debug Interface]
type=service
router=debugcli

[RW Split Listener]
type=listener
service=RW Split Router
protocol=MySQLClient
port=4006

[Read Connection Listener]
type=listener
service=Read Connection Router
protocol=MySQLClient
port=4008

[Debug Listener]
type=listener
service=Debug Interface
protocol=telnetd
port=4442

[CLI]
type=service
router=cli

[CLI Listener]
type=listener
service=CLI
protocol=maxscaled
#address=localhost
port=6603

[MyLogFilter]
type=filter
module=qlafilter
options=/tmp/QueryLog
 @endverbatim
 * - connect to RWSplit
 * - stop node0
 * - sleep 30 seconds
 * - reconnect
 * - check if 'USE test ' is ok
 * - check MaxScale is alive
 */

/*

Description Kolbe Kegel 2014-12-31 19:44:24 UTC
Created attachment 174 [details]
MaxScale.cnf

I am getting "Write to backend failed. Session closed." when executing "use" statements in the MariaDB command-line client.

All backends are running MariaDB-Galera-server-10.0.14-1.el6.x86_64.

MaxScale is maxscale-1.0.2-1.x86_64.

[skysql@max1 ~]$ mysql -h 192.168.30.38 -P 4006 -u maxuser -pmaxpwd
...
MariaDB [(none)]> use wordpress
ERROR 2003 (HY000): Write to backend failed. Session closed.
MariaDB [(none)]> show tables from wordpress;
ERROR 2006 (HY000): MySQL server has gone away
No connection. Trying to reconnect...
Connection id:    20552
Current database: *** NONE ***

Empty set (0.00 sec)

MariaDB [(none)]> use wordpress
ERROR 2003 (HY000): Write to backend failed. Session closed.

Trace log snippet:

2014-12-31 11:38:27   Servers and router connection counts:
2014-12-31 11:38:27   current operations : 0 in         db3:3306 RUNNING MASTER
2014-12-31 11:38:27   current operations : 0 in         db2:3306 RUNNING SLAVE
2014-12-31 11:38:27   current operations : 0 in         db1:3306 RUNNING SLAVE
2014-12-31 11:38:27   Selected RUNNING MASTER in        db3:3306
2014-12-31 11:38:27   Selected RUNNING SLAVE in         db2:3306
2014-12-31 11:38:27   Started RW Split Router client session [12] for 'maxuser' from 192.168.30.38
2014-12-31 11:38:27   [12]  > Autocommit: [enabled], trx is [not open], cmd: COM_INIT_DB, type: QUERY_TYPE_SESSION_WRITE, stmt: wordpress
2014-12-31 11:38:27   [12]  Session write, routing to all servers.
2014-12-31 11:38:27   [12]  Route query to master       db3:3306
2014-12-31 11:38:27   [12]  Route query to slave        db2:3306
2014-12-31 11:38:27   [12]  Stopped RW Split Router client session [12]
Comment 1 Timofey Turenko 2015-01-02 09:48:18 UTC
can't reproduce.

What privileges does 'maxpriv' user have?
Comment 2 Kolbe Kegel 2015-01-02 17:08:09 UTC
MariaDB [test]> show grants for 'maxpriv'@'192.168.30.38';
+-----------------------------------------------------------------------------------------------------------------------------+
| Grants for maxpriv@192.168.30.38                                                                                            |
+-----------------------------------------------------------------------------------------------------------------------------+
| GRANT SHOW DATABASES ON *.* TO 'maxpriv'@'192.168.30.38' IDENTIFIED BY PASSWORD '*5EDBD32E469DAE0CE10E6999C3899DEFCB9F12E0' |
| GRANT SELECT ON `mysql`.`user` TO 'maxpriv'@'192.168.30.38'                                                                 |
| GRANT SELECT ON `mysql`.`db` TO 'maxpriv'@'192.168.30.38'                                                                   |
+-----------------------------------------------------------------------------------------------------------------------------+
3 rows in set (0.00 sec)
Comment 3 Kolbe Kegel 2015-01-02 17:09:53 UTC
I get the same error if I GRANT ALL PRIVILEGES ON *.* TO 'maxpriv'@'192.168.30.38' and restart MaxScale, so I don't think privileges are the problem.
Comment 4 Kolbe Kegel 2015-01-02 17:23:59 UTC
Debug log:

[MySQL Client Auth], checking user [maxuser@192.168.30.38]
[MySQL Client Auth], checking user [maxuser@192.168.30.38] with wildcard host [%]
[gw_do_connect_to_backend] Connected to backend server db3:3306, fd 21.
[gw_create_backend_connection] Connection pending to db3:3306, protocol fd 21 client fd 20.
[dcb_connect] Connected to server db3:3306, from backend dcb 0x27fff50, client dcp 0x27e1ed0 fd 20.
[dcb_set_state_nomutex] dcb 0x27fff50 fd 21 DCB_STATE_ALLOC -> DCB_STATE_POLLING
[poll_add_dcb] Added dcb 0x27fff50 in state DCB_STATE_POLLING to poll set.
[gw_do_connect_to_backend] Connected to backend server db2:3306, fd 22.
[gw_create_backend_connection] Connection pending to db2:3306, protocol fd 22 client fd 20.
[dcb_connect] Connected to server db2:3306, from backend dcb 0x2800280, client dcp 0x27e1ed0 fd 20.
[dcb_set_state_nomutex] dcb 0x2800280 fd 22 DCB_STATE_ALLOC -> DCB_STATE_POLLING
[poll_add_dcb] Added dcb 0x2800280 in state DCB_STATE_POLLING to poll set.
[dcb_write] Wrote 11 Bytes to dcb 0x27e1ed0 in state DCB_STATE_POLLING fd 20
[poll_waitevents] epoll_wait found 1 fds
[poll_waitevents] event 5 dcb 0x27e1ed0 role DCB_ROLE_REQUEST_HANDLER
[poll_waitevents] Read in dcb 0x27e1ed0 fd 20
[dcb_read] Read 11 bytes from dcb 0x27e1ed0 in state DCB_STATE_POLLING fd 20.
[gw_MySQLWrite_backend] delayed write to dcb 0x27fff50 fd 21 protocol state MYSQL_PENDING_CONNECT.
[gw_MySQLWrite_backend] delayed write to dcb 0x2800280 fd 22 protocol state MYSQL_PENDING_CONNECT.
[dcb_write] Wrote 53 Bytes to dcb 0x27e1ed0 in state DCB_STATE_POLLING fd 20
[dcb_close]
[dcb_set_state_nomutex] dcb 0x27e1ed0 fd 20 DCB_STATE_POLLING -> DCB_STATE_NOPOLLING
[dcb_close] Removed dcb 0x27e1ed0 in state DCB_STATE_NOPOLLING from poll set.
[gw_client_close]
[RWSplit:closeSession]
[dcb_close]
[dcb_set_state_nomutex] dcb 0x27fff50 fd 21 DCB_STATE_POLLING -> DCB_STATE_NOPOLLING
[dcb_close] Removed dcb 0x27fff50 in state DCB_STATE_NOPOLLING from poll set.
[gw_backend_close]
[gw_MySQLWrite_backend] delayed write to dcb 0x27fff50 fd 21 protocol state MYSQL_PENDING_CONNECT.
[dcb_set_state_nomutex] dcb 0x27fff50 fd 21 DCB_STATE_NOPOLLING -> DCB_STATE_ZOMBIE
[dcb_close]
[dcb_set_state_nomutex] dcb 0x2800280 fd 22 DCB_STATE_POLLING -> DCB_STATE_NOPOLLING
[dcb_close] Removed dcb 0x2800280 in state DCB_STATE_NOPOLLING from poll set.
[gw_backend_close]
[gw_MySQLWrite_backend] delayed write to dcb 0x2800280 fd 22 protocol state MYSQL_PENDING_CONNECT.
[dcb_set_state_nomutex] dcb 0x2800280 fd 22 DCB_STATE_NOPOLLING -> DCB_STATE_ZOMBIE
[dcb_set_state_nomutex] dcb 0x27e1ed0 fd 20 DCB_STATE_NOPOLLING -> DCB_STATE_ZOMBIE
[dcb_process_zombies] Remove dcb 0x27e1ed0 fd 20 in state DCB_STATE_ZOMBIE from the list of zombies.
[dcb_process_zombies] Remove dcb 0x2800280 fd 22 in state DCB_STATE_ZOMBIE from the list of zombies.
[dcb_process_zombies] Remove dcb 0x27fff50 fd 21 in state DCB_STATE_ZOMBIE from the list of zombies.
[dcb_process_zombies] Closed socket -1 on dcb 0x27e1ed0.
[dcb_set_state_nomutex] dcb 0x27e1ed0 fd -1 DCB_STATE_ZOMBIE -> DCB_STATE_DISCONNECTED
[dcb_process_zombies] Closed socket -1 on dcb 0x2800280.
[dcb_set_state_nomutex] dcb 0x2800280 fd -1 DCB_STATE_ZOMBIE -> DCB_STATE_DISCONNECTED
[dcb_process_zombies] Closed socket -1 on dcb 0x27fff50.
[dcb_set_state_nomutex] dcb 0x27fff50 fd -1 DCB_STATE_ZOMBIE -> DCB_STATE_DISCONNECTED
Comment 5 Kolbe Kegel 2015-01-02 18:05:17 UTC
I can reproduce the bug even after completely removing the maxscale package and re-installing it.
Comment 6 Kolbe Kegel 2015-01-02 18:09:00 UTC
I don't encounter this problem when connecting to a service that uses the readconnroute router. It only occurs when I connect to a service that uses readwritesplit.
Comment 7 Kolbe Kegel 2015-01-02 18:36:43 UTC
I only see this problem when I have 3 backends for readwritesplit. The problem does not occur if I have only 1 or 2 backends.
Comment 8 Kolbe Kegel 2015-01-02 20:56:41 UTC
Created attachment 176 [details]
Config & all log files after reproducing using Vilho's debug build
Comment 9 Timofey Turenko 2015-01-02 21:45:38 UTC
reproduced!

Reproducible if backend is configured to use xtrabackup-v2:

wsrep_sst_method=xtrabackup-v2
Comment 10 Kolbe Kegel 2015-01-02 21:46:37 UTC
It seems entirely impossible to me that wsrep_sst_method could have any effect at all on how MaxScale connects to and interacts with a running cluster. That is bewildering.
Comment 11 Kolbe Kegel 2015-01-02 22:06:04 UTC
I can still reproduce this issue even when using wsrep_sst_method=rsync on all nodes and even emptying the datadir on 2 nodes to force them to perform an SST using rsync. So, I don't think there is any relationship between wsrep_sst_method and the behavior reported in this bug.
Comment 12 Kolbe Kegel 2015-01-02 22:27:57 UTC
Vilho's suggestion to change to max_slave_connections=100% seems to have pinpointed the issue. I can reproduce the bug consistently with the default value of max_slave_connections=50% but it disappears when I set max_slave_connections=100%.

(It appears to be necessary to completely restart the maxscale daemon to effect this change. Reloading config and restarting the service isn't adequate.)
Comment 13 Vilho Raatikka 2015-01-02 22:52:29 UTC
readwritesplit router's function route_session_write is called when query like 'use db' is executed. route_session_write routes query to every backend that the client's rwsplit is connected to. By default, every client (rwsplit) session uses master and one slave. Thus, it connects to 2 backends.
in route_session_write there is a loop where the query is routed to all those backends which are 'in use' in the session. There also is a flag indicating whether routing succeed.
The error is that when backend, which is not in use, is met, the success flag is set to 'false'. Thus, the success of whole routing function depends on which order backends are handled in the loop.
In Galera cluster the selection criteria differs slightly and different node becomes chosen to slave and thus the last backend handled was not in use in tests where this issue was noticed.
Regardless of that, the bug appears as well in any cluster.
Comment 14 Kolbe Kegel 2015-01-02 22:58:59 UTC
Vilho, many thanks for the time you put into discovering the underlying cause here.

Timofey, maybe we can connect to talk about putting in place some testing procedures that might have caught this. I was using basically the default/minimal configuration to use readwritesplit w/ Galera and I ran into this showstopper. Some testing process should've caught this before, I think.
Comment 15 Vilho Raatikka 2015-01-02 23:34:18 UTC
readwritesplit.c:route_session_write failed if the last backend on all backends list was not in use. THe situation where not all backends are used by routing session is normal especially if max_slave_connections is not set to 100%. Thus session commands may have failed if user was bit unlucky.
    Changed the logic so that the function fails (and session is closed) if routing fails to any such backend which is in use in the session.
Comment 16 Vilho Raatikka 2015-01-02 23:47:57 UTC
Added complementary fix to readwritesplit.c:route_session_write. It wasn't checked that client's router session was using at least one backend at the time of session command routing. As a consequence it would have been possible to succeed the function even if locking was failed due to simultaneous shutdown of the session. Fixed that.
Comment 17 Timofey Turenko 2015-01-08 21:53:30 UTC
finally found reliable way to reproduce:

- connect to RWSplit
- stop node0
- sleep 60 seconds
- reconnect
- check if 'USE test ' is ok
- check MaxScale is alive

test added, closing

*/


#include <iostream>
#include "testconnections.h"
#include "mariadb_func.h"

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(20);

    int i;
    char sys1[4096];

    MYSQL * conn = open_conn_no_db(Test->rwsplit_port, Test->maxscale_IP, Test->maxscale_user, Test->maxscale_password, Test->ssl);

    Test->tprintf("Stopping %d\n", 0);
    Test->galera->stop_node(0);

    Test->stop_timeout();
    sleep(30);
    Test->set_timeout(20);
    mysql_close(conn);

    conn = open_conn_no_db(Test->rwsplit_port, Test->maxscale_IP, Test->maxscale_user, Test->maxscale_password, Test->ssl);

    if (conn == 0) {
        Test->add_result(1, "Error connection to RW Split\n");
        Test->copy_all_logs();
        exit(1);
    }

    Test->tprintf("selecting DB 'test' for rwsplit\n");
    Test->try_query(conn, "USE test");

    Test->tprintf("Closing connection\n");
    mysql_close(conn);

    Test->connect_rwsplit();
    Test->try_query(Test->conn_rwsplit, "show processlist;");
    Test->close_maxscale_connections();

    Test->tprintf("Stopping all Galera nodes\n");
    for (i = 1; i < Test->galera->N; i++) {
        Test->set_timeout(30);
        Test->tprintf("Stopping %d\n", i);
        Test->galera->stop_node(i);
        fflush(stdout);
    }

    Test->stop_timeout();
    Test->tprintf("Restarting Galera cluster\n");
    Test->galera->start_replication();

    Test->copy_all_logs(); return(Test->global_result);
}

