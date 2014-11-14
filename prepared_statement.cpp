#include <my_config.h>
#include <iostream>
#include "testconnections.h"
#include "sql_t1.h"

using namespace std;

int main()
{
    TestConnections * Test = new TestConnections();
    int global_result = 0;
    int i;
    int N=4;

    Test->ReadEnv();
    Test->PrintIP();
    Test->repl->Connect();
    if (Test->ConnectMaxscale() !=0 ) {
        printf("Error connecting to MaxScale\n");
        exit(1);
    }

    create_t1(Test->conn_rwsplit);
    insert_into_t1(Test->conn_rwsplit, N);

    global_result += execute_query(Test->conn_rwsplit, (char *) "PREPARE stmt FROM 'SELECT * FROM t1 WHERE fl=@x;';");
    global_result += execute_query(Test->conn_rwsplit, (char *) "SET @x = 3;");
    global_result += execute_query(Test->conn_rwsplit, (char *) "EXECUTE stmt");
    global_result += execute_query(Test->conn_rwsplit, (char *) "SET @x = 4;");
    global_result += execute_query(Test->conn_rwsplit, (char *) "EXECUTE stmt");

    global_result += CheckMaxscaleAlive();
    exit(global_result);
}


/*
  Hi Timofey, I can't imagine repeatable way to run in to the situation where session command replies would arrive in different order, at least without additional instrumentation. You can, however, increase the probability for it to occur by setting up master and multiple slaves, the more the better.

Then start to prepare statements which return much data. The length of response packet depends on number of columns so good query would be something that produces lots of columns, like select a.user, b.user, c.user, ... z.user from mysql.user a, mysql.user b, mysql.user c, ... mysql.user z

I'm not sure that it will happen but it is possible. You can also try to make it happen by decreasing the size of network packet because the smaller that is the more likely it is that responses are split into multiple packets - which can then becomen interleaved with packets from different slaves.
*/