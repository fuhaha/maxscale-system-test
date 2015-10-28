/**
 * @file bug493.cpp regression case for bug 493 ( Can have same section name multiple times without warning)
 *
 * - Maxscale.cnf in which 'server2' is defined twice and tests checks error log for proper error message.
 * - check if Maxscale is alive
 */


#include <my_config.h>
#include <iostream>
#include <unistd.h>
#include "testconnections.h"

using namespace std;

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(10);
    Test->check_log_err((char *) "Error : Configuration object 'server2' has multiple parameters", TRUE);
    Test->check_maxscale_alive();
    Test->copy_all_logs(); return(Test->global_result);
}
