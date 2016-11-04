/**
 * @file max720_line_with_no_equal.cpp mxs720 regression case - first part: line without "=", second - weird lines  ("MaxScale fails to start and doesn't log any useful message when there are spurious characters in the config file")
 *
 * - use incorrect maxscale.cnf
 * - check log for error
 */

#include <my_config.h>
#include <iostream>
#include <unistd.h>
#include "testconnections.h"

using namespace std;

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(30);
    Test->check_log_err((char *) "Failed to pre-parse configuration file", true);

    Test->check_maxscale_processes(0);
    Test->copy_all_logs(); return(Test->global_result);
}

