
/**
 * @file mxs559_block_master Playing with blocking and unblocking Master
 * It does not reproduce the bug in reliavle way, but it is a good
 * load and robustness test
 * - create load on Master RWSplit
 * - block and unblock Master in the loop
 * - repeat with different time between block/unblock
 * - check logs for lack of errors "authentication failure", "handshake failure"
 * - check for lack of crashes in the log
 */


#include "testconnections.h"
#include "sql_t1.h"

typedef struct  {
    int exit_flag;
    int thread_id;
    long i;
    int rwsplit_only;
    TestConnections * Test;
    MYSQL * conn1;
    MYSQL * conn2;
    MYSQL * conn3;
} openclose_thread_data;

void *disconnect_thread( void *ptr );
int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(20);

    int load_threads_num = 25;
    openclose_thread_data data_master[load_threads_num];

    int i;

    int iterations = 10;
    int t_iterations = 3;
    int tt[t_iterations];

    tt[0] = 3;
    tt[1] = 5;
    tt[2] = 10;

    if (Test->smoke) {
        iterations = 5;
        t_iterations = 1;
    }

    for (i = 0; i < load_threads_num; i++) {
        data_master[i].i = 0;
        data_master[i].exit_flag = 0;
        data_master[i].Test = Test;
        data_master[i].rwsplit_only = 1;
        data_master[i].thread_id = i;
    }

    pthread_t thread_master[load_threads_num];
    int  iret_master[load_threads_num];

    Test->repl->flush_hosts();

    Test->repl->connect();
    Test->connect_maxscale();

    Test->tprintf("Create t1\n");
    create_t1(Test->conn_rwsplit);
    Test->tprintf("set max_connections t1\n");
    execute_query(Test->conn_rwsplit, (char*) "set global max_connections=1000");
    Test->close_maxscale_connections();

    Test->tprintf("Create threads\n");
    for (i = 0; i < load_threads_num; i++) { data_master[i].rwsplit_only = 1;}
    /* Create independent threads each of them will create some load on Mastet */
    for (i = 0; i < load_threads_num; i++) {
        Test->tprintf("Thread %d\n", i);
        iret_master[i] = pthread_create( &thread_master[i], NULL, disconnect_thread, &data_master[i]);
    }

    Test->stop_timeout();
    sleep(30);
    for (int j = 0; j < t_iterations; j++)
    {
        for (i = 0; i < iterations; i++)
        {
            Test->set_timeout(30+tt[j]*10);
            Test->tprintf("Block master\n");
            Test->repl->block_node(0);
            Test->stop_timeout();
            sleep(tt[j]);
            Test->set_timeout(30+tt[j]*10);
            Test->tprintf("Unlock master\n");
            Test->repl->unblock_node(0);
            Test->stop_timeout();
            sleep(tt[j]);
            Test->set_timeout(30+tt[j]*10);
        }
    }

    Test->set_timeout(240);

    Test->tprintf("Waiting for all master load threads exit\n");
    for (i = 0; i < load_threads_num; i++)
    {
        data_master[i].exit_flag = 1;
        pthread_join(iret_master[i], NULL);
    }

    Test->stop_timeout();
    sleep(10);
    Test->tprintf("flush hosts\n");
    Test->repl->flush_hosts();
    Test->repl->check_and_restart_nodes_vm();
    Test->repl->fix_replication();
    sleep(5);
    Test->tprintf("Drop t1\n");
    Test->connect_maxscale();
    Test->try_query(Test->conn_rwsplit, (char *) "DROP TABLE IF EXISTS t1;");
    Test->close_maxscale_connections();
    Test->tprintf("Checking if Maxscale alive\n");
    Test->check_maxscale_alive();
    Test->tprintf("Checking log for unwanted errors\n");
    Test->check_log_err((char *) "due to authentication failure", false);
    Test->check_log_err((char *) "fatal signal 11", false);
    Test->check_log_err((char *) "due to handshake failure", false);
    Test->check_log_err((char *) "Refresh rate limit exceeded for load of users' table", false);

    Test->copy_all_logs(); return(Test->global_result);
}


void *disconnect_thread( void *ptr )
{
    openclose_thread_data * data = (openclose_thread_data *) ptr;
    char sql[1000000];

    sleep(data->thread_id);
    create_insert_string(sql, 50000, 2);

    while (data->exit_flag == 0)
    {
        //data->conn1 = data->Test->open_rwsplit_connection();
        data->conn1 = open_conn_db_timeout(data->Test->rwsplit_port, data->Test->maxscale_IP, (char*) "test", data->Test->maxscale_user, data->Test->maxscale_password, 10, data->Test->ssl);
        execute_query_silent(data->conn1, sql);
        mysql_close(data->conn1);
        data->i++;
    }


    return NULL;
}

