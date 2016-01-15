/**
 * @file local_infile.cpp Load data with LOAD DATA LOCAL INFILE
 *
 * 1. Create a 50Mb test file
 * 2. Load and read it through MaxScale
 */


#include <my_config.h>
#include "testconnections.h"

void create_data_file(char* filename, size_t size)
{
    int fd, i = 0;
    snprintf(filename, size, "local_infile_%u", i++);
    while ((fd = open(filename, O_CREAT | O_RDWR | O_EXCL, 0755)) == -1)
    {
        snprintf(filename, size, "local_infile_%u", i++);
    }

    const size_t maxsize = 1024 * 1024 * 50;
    size_t filesize = 0;
    i = 0;

    while (filesize < maxsize)
    {
        char buffer[1024];
        sprintf(buffer, "%d,'%x','%x'\n", i, i << 10 + i, i << 5 + i);
        int written = write(fd, buffer, strlen(buffer));
        if (written <= 0)
        {
            break;
        }
        i++;
        filesize += written;
    }

    close(fd);
}

int main(int argc, char *argv[])
{

    TestConnections * test = new TestConnections(argc, argv);
    char filename[1024];
    create_data_file(filename, sizeof (filename));

    /** Set max packet size and create test table */
    test->connect_maxscale();
    test->add_result(execute_query(test->conn_rwsplit,
                                   "set global max_allowed_packet=(1048576 * 60)"),
                     "Setting max_allowed_packet failed.");
    test->add_result(execute_query(test->conn_rwsplit,
                                   "DROP TABLE IF EXISTS test.dump"),
                     "Dropping table failed.");
    test->add_result(execute_query(test->conn_rwsplit,
                                   "CREATE TABLE test.dump(a int, b varchar(80), c varchar(80))"),
                     "Creating table failed.");
    test->close_maxscale_connections();

    /** Reconnect, load the data and then read it */
    test->connect_maxscale();
    char query[1024];
    snprintf(query, sizeof (filename), "LOAD DATA LOCAL INFILE '%s' INTO TABLE test.dump FIELDS TERMINATED BY ','",
             filename);
    test->add_result(execute_query(test->conn_rwsplit, query), "Loading data failed.");
    test->add_result(execute_query(test->conn_rwsplit, "SELECT * FROM test.dump"),
                     "Reading data failed.");
    test->close_maxscale_connections();
    test->copy_all_logs();
    unlink(filename);
    return test->global_result;
}