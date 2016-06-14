/**
 * @file mxs657_restart_service.cpp mxs657 regression case ("Debug assertion when service is shut down and restarted repeatedly")
 *
 * - shutdown and restart RW Split Router in the loop from a number of threads
 * Note: does not work crash reliable way with 'smoke' option
 */


#include <my_config.h>
#include <iostream>
#include <unistd.h>
#include "testconnections.h"

using namespace std;
void *query_thread1( void *ptr );
TestConnections * Test;
bool exit_flag = false;

int main(int argc, char *argv[])
{
    Test = new TestConnections(argc, argv);

    Test->set_timeout(3000);

    int threads_num = 1000;
    pthread_t thread1[threads_num];

    int  iret1[threads_num];
    int i;

    for (i = 0; i < threads_num; i++) {
        iret1[i] = pthread_create( &thread1[i], NULL, query_thread1, NULL);
    }

    Test->tprintf("Trying to shutdown and restart RW Split router in the loop\n");

    if (Test->smoke)
    {
        sleep(200);
    } else {
        sleep(1200);
    }

    Test->tprintf("Done, exiting threads\n\n");

    exit_flag = true;
    for (int i = 0; i < threads_num; i++) {
        pthread_join(thread1[i], NULL);
    }

    Test->tprintf("Done!\n");

    sleep(5);

    Test->check_maxscale_alive();
    Test->check_log_err((char *) "received fatal signal", FALSE);
    Test->copy_all_logs(); return(Test->global_result);
}

void *query_thread1( void *ptr )
{
    while (!exit_flag)
    {
        Test->execute_maxadmin_command((char *) "shutdown service \"RW Split Router\"");
        Test->execute_maxadmin_command((char *) "restart service \"RW Split Router\"");
    }
}
