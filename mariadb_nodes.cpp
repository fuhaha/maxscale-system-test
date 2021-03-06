/**
 * @file mariadb_nodes.cpp - backend nodes routines
 *
 * @verbatim
 * Revision History
 *
 * Date		Who		Description
 * 17/11/14	Timofey Turenko	Initial implementation
 *
 * @endverbatim
 */

#include "mariadb_nodes.h"
#include "sql_const.h"
#include <string>

Mariadb_nodes::Mariadb_nodes(const char *pref, const char *test_cwd, bool verbose)
{
    strcpy(prefix, pref);
    memset(this->nodes, 0, sizeof(this->nodes));
    memset(blocked, 0, sizeof(blocked));
    no_set_pos = false;
    this->verbose = verbose;
    strcpy(test_dir, test_cwd);
    read_env();
    truncate_mariadb_logs();
    flush_hosts();
    close_active_connections();
}

Mariadb_nodes::~Mariadb_nodes()
{
    for (int i = 0; i < N; i++)
    {
        if (blocked[i])
        {
            unblock_node(i);
        }
    }
}

int Mariadb_nodes::connect()
{
    int res = 0;

    for (int i = 0; i < N; i++)
    {
        if (nodes[i] == NULL || mysql_ping(nodes[i]) != 0)
        {
            if (nodes[i])
            {
                mysql_close(nodes[i]);
            }
            nodes[i] = open_conn_db_timeout(port[i], IP[i], "test", user_name, password, 50, ssl);
        }

        if ((nodes[i] != NULL) && (mysql_errno(nodes[i]) != 0))
        {
            res++;
        }
    }

    return res;
}

void Mariadb_nodes::close_connections()
{
    for (int i = 0; i < N; i++)
    {
        if (nodes[i] != NULL)
        {
            mysql_close(nodes[i]);
            nodes[i] = NULL;
        }
    }
}

int Mariadb_nodes::read_env()
{
    char * env;
    char env_name[64];

    sprintf(env_name, "%s_N", prefix);
    env = getenv(env_name); if (env != NULL) {sscanf(env, "%d", &N); } else {N = 0;}

    sprintf(env_name, "%s_user", prefix);
    env = getenv(env_name); if (env != NULL) {sscanf(env, "%s", user_name); } else {sprintf(user_name, "skysql"); }
    sprintf(env_name, "%s_password", prefix);
    env = getenv(env_name); if (env != NULL) {sscanf(env, "%s", password); } else {sprintf(password, "skysql"); }

    ssl = false;
    sprintf(env_name, "%s_ssl", prefix);
    env = getenv(env_name); if ((env != NULL) && ((strcasecmp(env, "yes") == 0) || (strcasecmp(env, "true") == 0) )) {ssl = true;}


    if ((N > 0) && (N < 255)) {
        for (int i = 0; i < N; i++) {
            //reading IPs
            sprintf(env_name, "%s_%03d_network", prefix, i);
            env = getenv(env_name); if (env != NULL) {sprintf(IP[i], "%s", env);}

            //reading private IPs
            sprintf(env_name, "%s_%03d_private_ip", prefix, i);
            env = getenv(env_name); if (env != NULL) {sprintf(IP_private[i], "%s", env);} else {sprintf(IP_private[i], "%s", IP[i]);}

            //reading ports
            sprintf(env_name, "%s_%03d_port", prefix, i);
            env = getenv(env_name); if (env != NULL) {
                sscanf(env, "%d", &port[i]);
            } else {
                port[i] = 3306;
            }
            //reading sshkey
            sprintf(env_name, "%s_%03d_keyfile", prefix, i);
            env = getenv(env_name); if (env != NULL) {sprintf(sshkey[i], "%s", env);} else {sprintf(sshkey[i], "vagrant.pem");}

            //reading start_db_command
            sprintf(env_name, "%s_%03d_start_db_command", prefix, i);
            env = getenv(env_name); if (env != NULL) {sprintf(start_db_command[i], "%s", env);} else {sprintf(start_db_command[i], "%s", "service mysql start");}

            //reading stop_db_command
            sprintf(env_name, "%s_%03d_stop_db_command", prefix, i);
            env = getenv(env_name); if (env != NULL) {sprintf(stop_db_command[i], "%s", env);} else {sprintf(start_db_command[i], "%s", "service mysql stop");}

            sprintf(env_name, "%s_%03d_kill_vm_command", prefix, i);
            env = getenv(env_name); if (env != NULL) {sscanf(env, "%s", kill_vm_command[i]); } else {sprintf(kill_vm_command[i], "exit 1"); }

            sprintf(env_name, "%s_%03d_start_vm_command", prefix, i);
            env = getenv(env_name); if (env != NULL) {sscanf(env, "%s", start_vm_command[i]); } else {sprintf(start_vm_command[i], "exit 1"); }

            sprintf(env_name, "%s_%03d_whoami", prefix, i);
            env = getenv(env_name); if (env != NULL) {sscanf(env, "%s", access_user[i]); } else {sprintf(access_user[i], "root"); }

            sprintf(env_name, "%s_%03d_access_sudo", prefix, i);
            env = getenv(env_name); if (env != NULL) {sscanf(env, "%s", access_sudo[i]); } else {sprintf(access_sudo[i], " "); }

            if (strcmp(access_user[i], "root") == 0) {
                sprintf(access_homedir[i], "/%s/", access_user[i]);
            } else {
                sprintf(access_homedir[i], "/home/%s/", access_user[i]);
            }

        }
    }
}

int Mariadb_nodes::print_env()
{
    for (int i = 0; i < N; i++)
    {
        printf("%s node %d \t%s\tPort=%d\n", prefix, i, IP[i], port[i]);
        printf("%s Access user %s\n", prefix, access_user[i]);
    }
    printf("%s User name %s\n", prefix, user_name);
    printf("%s Password %s\n", prefix, password);

}

int Mariadb_nodes::find_master()
{
    char str[255];
    char master_IP[256];
    int i = 0;
    int found = 0;
    int master_node = 255;
    while ((found == 0) && (i < N)) {
        if (find_field(
                    nodes[i], (char *) "show slave status;",
                    (char *) "Master_Host", &str[0]
                    ) == 0 ) {
            found = 1;
            strcpy(master_IP, str);
        }
        i++;
    }
    if (found == 1) {
        found = 0; i = 0;
        while ((found == 0) && (i < N)) {
            if (strcmp(IP[i], master_IP) == 0) {
                found = 1;
                master_node = i;
            }
            i++;
        }
    }
    return(master_node);
}

int Mariadb_nodes::change_master(int NewMaster, int OldMaster)
{
    int i;
    //int OldMaster = FindMaster();
    char log_file[256];
    char log_pos[256];
    char str[1024];

    for (i = 0; i < N; i++) {
        if (i != OldMaster) {execute_query(nodes[i], (char *) "stop slave;");}
    }
    execute_query(nodes[NewMaster], create_repl_user);
    execute_query(nodes[OldMaster], (char *) "reset master;");
    find_field(nodes[NewMaster], (char *) "show master status", (char *) "File", &log_file[0]);
    find_field(nodes[NewMaster], (char *) "show master status", (char *) "Position", &log_pos[0]);
    for (i = 0; i < N; i++) {
        if (i != NewMaster) {
            sprintf(str, setup_slave, IP[NewMaster], log_file, log_pos, port[NewMaster]);
            execute_query(nodes[i], str);
        }
    }
    //for (i = 0; i < N; i++) {if (i != NewMaster) {execute_query(nodes[i], (char *) "start slave;"); }}
}

int Mariadb_nodes::stop_node(int node)
{
    return(ssh_node(node, stop_db_command[node], true));
}

int Mariadb_nodes::start_node(int node, char * param)
{
    char cmd[1024];
    if (v51)
    {
        sprintf(cmd, "%s %s --report-host", start_db_command[node], param);
    } else {
        sprintf(cmd, "%s %s", start_db_command[node], param);
    }
    return(ssh_node(node, cmd, true));
}

int Mariadb_nodes::stop_nodes()
{
    int i;
    int local_result = 0;
    connect();
    for (i = 0; i < N; i++)
    {
        printf("Stopping slave %d\n", i); fflush(stdout);
        local_result += execute_query(nodes[i], (char *) "stop slave;");
        printf("Stopping %d\n", i); fflush(stdout);
        local_result += stop_node(i); fflush(stdout);
    }
    return(local_result);
}

int Mariadb_nodes::stop_slaves()
{
    int i;
    int global_result = 0;
    connect();
    for (i = 0; i < N; i++) {
        printf("Stopping slave %d\n", i); fflush(stdout);
        global_result += execute_query(nodes[i], (char *) "stop slave;");
    }
    close_connections();
    return(global_result);
}

int Mariadb_nodes::start_replication()
{
    char str[1024];
    char log_file[256];
    char log_pos[256];
    int local_result = 0;

    // Start all nodes
    for (int i = 0; i < N; i++)
    {
        local_result += start_node(i, (char *) "");
        ssh_node(i, "mysql -u root -e \"STOP SLAVE; RESET SLAVE; RESET SLAVE ALL; RESET MASTER;\"", true);
    }

    sprintf(str, "%s/create_user.sh", test_dir);
    copy_to_node(str, "~/", 0);

    sprintf(str, "export node_user=\"%s\"; export node_password=\"%s\"; ./create_user.sh", user_name, password);
    printf("cmd: %s\n", str);
    ssh_node(0, str, false);


    // Create a database dump from the master and distribute it to the slaves
    ssh_node(0, "mysqldump --all-databases --add-drop-database --flush-privileges --master-data=1 > /tmp/master_backup.sql", true);
    sprintf(str, "%s/master_backup.sql", test_dir);
    copy_from_node("/tmp/master_backup.sql", str, 0);

    for (int i = 1; i < N; i++) {
        // Reset all nodes by first loading the dump and then starting the replication
        printf("Starting node %d\n", i); fflush(stdout);
        copy_to_node(str, "/tmp/master_backup.sql", i);
        ssh_node(i, "mysql -u root < /tmp/master_backup.sql", true);
        char query[512];
        sprintf(query, "mysql -u root -e \"CHANGE MASTER TO MASTER_HOST=\\\"%s\\\", MASTER_PORT=%d, "
                "MASTER_USER=\\\"repl\\\", MASTER_PASSWORD=\\\"repl\\\";"
                "START SLAVE;\"", IP_private[0], port[0]);
        ssh_node(i, query, true);
    }

    return local_result;
}

int Galera_nodes::start_galera()
{
    char sys1[4096];
    char str[1024];
    int i;
    int local_result = 0;
    local_result += stop_nodes();

    printf("Starting new Galera cluster\n");  fflush(stdout);
    local_result += start_node(0, (char *) " --wsrep-cluster-address=gcomm://");

    sprintf(str, "%s/create_user_galera.sh", test_dir);
    copy_to_node(str, "~/", 0);

    sprintf(str, "export galera_user=\"%s\"; export galera_password=\"%s\"; ./create_user_galera.sh", user_name, password);
    ssh_node(0, str, false);

    for (i = 1; i < N; i++) {
        printf("Starting node %d\n", i); fflush(stdout);
        sprintf(&sys1[0], " --wsrep-cluster-address=gcomm://%s", IP_private[0]);
        if (this->verbose)
        {
            printf("%s\n", sys1);
            fflush(stdout);
        }
        local_result += start_node(i, sys1); fflush(stdout);
    }
    sleep(5);

    local_result += connect();
    local_result += execute_query(nodes[0], create_repl_user);

    close_connections();
    return(local_result);
}

int Mariadb_nodes::clean_iptables(int node)
{
    char sys1[1024];
    int local_result = 0;
    /*
    char * clean_command = (char *)
            "echo '#!/bin/bash' > clean_iptables.sh \n \
            echo 'while [ \"$(iptables -n -L INPUT 1|grep 'mysql\\|%d')\" != \"\" ]; do iptables -D INPUT 1; done' >>  clean_iptables.sh \n \
            chmod a+x  clean_iptables.sh \n sudo  ./clean_iptables.sh \n";
    sprintf(&sys1[0], clean_command, port[node]);
    printf("%s\n", sys1);
    ssh_node(node, sys1, false);*/
    local_result += ssh_node(node, (char *) "echo \"#!/bin/bash\" > clean_iptables.sh", false);
    sprintf(sys1, "echo \"while [ \\\"\\$(iptables -n -L INPUT 1|grep '%d')\\\" != \\\"\\\" ]; do iptables -D INPUT 1; done\" >>  clean_iptables.sh", port[node]);
    local_result += ssh_node(node, (char *) sys1, false);
    local_result += ssh_node(node, (char *) "chmod a+x clean_iptables.sh", false);
    local_result += ssh_node(node, (char *) "./clean_iptables.sh", true);
    return(local_result);
}

int Mariadb_nodes::block_node(int node)
{
    char sys1[1024];
    int local_result = 0;
    local_result += clean_iptables(node);
    sprintf(&sys1[0], "iptables -I INPUT -p tcp --dport %d -j REJECT", port[node]);
    if (this->verbose)
    {
        printf("%s\n", sys1);
        fflush(stdout);
    }
    local_result += ssh_node(node, sys1, true);
    blocked[node] = true;
    return(local_result);
}

int Mariadb_nodes::unblock_node(int node)
{
    char sys1[1024];
    int local_result = 0;
    local_result += clean_iptables(node);
    sprintf(&sys1[0], "iptables -I INPUT -p tcp --dport %d -j ACCEPT", port[node]);
    if (this->verbose)
    {
        printf("%s\n", sys1);
        fflush(stdout);
    }
    local_result += ssh_node(node, sys1, true);
    blocked[node] = false;
    return(local_result);
}


int Mariadb_nodes::unblock_all_nodes()
{
    int rval = 0;
    for (int i = 0; i < this->N; i++)
    {
        rval += this->unblock_node(i);
    }
    return rval;
}

int Mariadb_nodes::check_and_restart_nodes_vm()
{
    int res = 0;
    for (int i = 0; i < N; i++)
    {
        res += check_and_restart_node_vm(i);
    }
    return(res);
}

int Mariadb_nodes::check_node_vm(int node)
{
    int res = 0;
    printf("Checking node %d\n", node); fflush(stdout);

    if (ssh_node(0, (char *) "ls > /dev/null", false) != 0) {
        printf("Node %d is not available\n", node); fflush(stdout);
        res = 1;
    } else {
        printf("Node %d is OK\n", node); fflush(stdout);
    }
    return(res);
}

int Mariadb_nodes::restart_node_vm(int node)
{
    int res = 0;
    printf("stopping node %d: %s\n", node, kill_vm_command[node]);
    system(kill_vm_command[node]);
    printf("starting node %d: %s\n", node, start_vm_command[node]);
    res += system(start_vm_command[node]);
    return(res);
}

int Mariadb_nodes::check_and_restart_node_vm(int node)
{
    if (check_node_vm(node) != 0) {return(restart_node_vm(node));} else {return(0);}
}

int Mariadb_nodes::check_replication()
{
    int master = 0;
    int res1 = 0;
    char str[1024];
    MYSQL *conn;
    MYSQL_RES *res;
    //bool v51 = false;
    if (verbose)
    {
        printf("Checking Master/Slave setup\n");
        fflush(stdout);
    }

    if (this->nodes[0] == NULL)
    {
        this->connect();
    }

    res1 = get_versions();

    for (int i = 0; i < N; i++) {
        conn = nodes[i];

        if (mysql_errno(conn) != 0) {
            printf("Error connectiong node %d\n", i); fflush(stdout);
            res1 = 1;
        } else {
            if ( i == master ) {
                // checking master
                if (conn != NULL ) {
                    if(mysql_query(conn, (char *) "SHOW SLAVE HOSTS;") != 0) {
                        printf("%s\n", mysql_error(conn));
                        res1 = 1;
                    } else {
                        res = mysql_store_result(conn);
                        if(res == NULL) {
                            printf("Error: can't get the result description\n"); fflush(stdout);
                            res1 = 1;
                        } else {
                            if (mysql_num_rows(res) != N-1) {
                                printf("Number if slaves is not equal to N-1\n"); fflush(stdout);
                                if (v51)
                                {
                                    printf("But version is 5.1 is present in the setup, ignoring number of slaves\n"); fflush(stdout);
                                } else {
                                    res1 = 1;
                                }
                            }
                        }
                        mysql_free_result(res);
                        do {
                            res = mysql_store_result(conn);
                            mysql_free_result(res);
                        } while ( mysql_next_result(conn) == 0 );
                    }
                }

            } else {
                // checking slave
                if (find_field(conn, (char *) "SHOW SLAVE STATUS;", (char *) "Slave_IO_Running", str) != 0) {
                    printf("Slave_IO_Running is not found in SHOW SLAVE STATUS results\n"); fflush(stdout);
                    res1 = 1;
                } else {
                    if (strcmp(str, "Yes") !=0 ) {
                        printf("Slave_IO_Running is not Yes\n"); fflush(stdout);
                        res1 = 1;
                    }
                }

                if (find_field(conn, (char *) "SHOW SLAVE STATUS;", (char *) "Slave_SQL_Running", str) != 0) {
                    printf("Slave_SQL_Running is not found in SHOW SLAVE STATUS results\n"); fflush(stdout);
                    res1 = 1;
                } else {
                    if (strcmp(str, "Yes") !=0 ) {
                        printf("Slave_SQL_Running is not Yes\n"); fflush(stdout);
                        res1 = 1;
                    }
                }
            }
        }
    }

    return(res1);
}

bool Mariadb_nodes::fix_replication()
{
    if (check_replication())
    {
        unblock_all_nodes();

        if (check_and_restart_nodes_vm())
        {
            printf("****** VMS ARE BROKEN! Exiting *****\n");
            return false;
        }

        int attempts = 2;

        while (check_replication() && attempts > 0)
        {
            if (attempts != 2)
            {
                stop_nodes();
            }

            start_replication();
            close_connections();

            attempts--;
        }

        if (attempts == 0 && check_replication())
        {
            printf("****** BACKEND IS STILL BROKEN! Exiting *****\n");
            return false;
        }

        flush_hosts();
    }

    return true;
}

int Galera_nodes::check_galera()
{
    int res1 = 0;

    if (verbose)
    {
        printf("Checking Galera\n");
        fflush(stdout);
    }

    if (this->nodes[0] == NULL)
    {
        this->connect();
    }

    res1 = get_versions();

    for (int i = 0; i < N; i++)
    {
        MYSQL *conn = open_conn(port[i], IP[i], user_name, password, ssl);
        if (conn == NULL || mysql_errno(conn) != 0)
        {
            printf("Error connectiong node %d: %s\n", i, mysql_error(conn));
            res1 = 1;
        }
        else
        {
            char str[1024] = "";

            if (find_field(conn, (char *) "SHOW STATUS WHERE Variable_name='wsrep_cluster_size';", (char *) "Value", str) != 0)
            {
                printf("wsrep_cluster_size is not found in SHOW STATUS LIKE 'wsrep%%' results\n");
                fflush(stdout);
                res1 = 1;
            }
            else
            {
                int cluster_size;
                sscanf(str, "%d", &cluster_size);
                if (cluster_size != N)
                {
                    printf("wsrep_cluster_size is not %d, it is %d\n", N, cluster_size);
                    fflush(stdout);
                    res1 = 1;
                }
            }
        }
        mysql_close(conn);
    }

    return res1;
}

int Mariadb_nodes::wait_all_vm()
{
    int i = 0;

    while ((check_and_restart_nodes_vm() != 0) && (i < 20)) {
        sleep(10);
    }
    return(check_and_restart_nodes_vm());
}

int Mariadb_nodes::kill_all_vm()
{
    int res = 0;
    char sys[1024];
    for (int i = 0; i < N; i++) {
        sprintf(sys, "%s", kill_vm_command[i]);
        if (system(sys) != 0) {res = 1;}
    }
    return(res);
}

int Mariadb_nodes::start_all_vm()
{
    int res = 0;
    char sys[1024];
    for (int i = 0; i < N; i++) {
        printf("starting node %d\n", i);
        sprintf(sys, "%s", start_vm_command[i]);
        if (system(sys) != 0) {res = 1;}
    }
    return(res);
}

int Mariadb_nodes::restart_all_vm()
{
    kill_all_vm();
    start_all_vm();
    return(wait_all_vm());
}

int Mariadb_nodes::set_slave(MYSQL * conn, char master_host[], int master_port, char log_file[], char log_pos[])
{
    char str[1024];

    sprintf(str, setup_slave, master_host, log_file, log_pos, master_port);
    if (no_set_pos) {sprintf(str, setup_slave_no_pos, master_host, master_port);}

    if (this->verbose)
    {
        printf("Setup slave SQL: %s\n", str);
    }
    return(execute_query(conn, str));
}

int Mariadb_nodes::set_repl_user()
{
    int global_result = 0;
    global_result += connect();
    for (int i = 0; i < N; i++) {
        global_result += execute_query(nodes[i], create_repl_user);
    }
    close_connections();
    return(global_result);
}

int Mariadb_nodes::get_server_id(int index)
{
    int id = -1;
    char str[1024];

    if (find_field(this->nodes[index], "SELECT @@server_id", "@@server_id", (char*) str) == 0)
    {
        id = atoi(str);
    }
    else
    {
        printf("find_field failed for %s:%d\n", this->IP[index], this->port[index]);
    }

    return id;
}

void Mariadb_nodes::generate_ssh_cmd(char *cmd, int node, const char *ssh, bool sudo)
{
    if (sudo)
    {
        sprintf(cmd, "ssh -i %s -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o LogLevel=quiet %s@%s '%s %s\'",
                sshkey[node], access_user[node], IP[node], access_sudo[node], ssh);
    } else
    {
        sprintf(cmd, "ssh -i %s -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o LogLevel=quiet %s@%s '%s'",
                sshkey[node], access_user[node], IP[node], ssh);
    }
}

char * Mariadb_nodes::ssh_node_output(int node, const char *ssh, bool sudo, int *exit_code)
{
    char sys[strlen(ssh) + 1024];
    generate_ssh_cmd(sys, node, ssh, sudo);
    FILE *output = popen(sys, "r");
    if (output == NULL)
    {
        printf("Error opening ssh %s\n", strerror(errno));
        return NULL;
    }
    char buffer[1024];
    size_t rsize = sizeof(buffer);
    char* result = (char*)calloc(rsize, sizeof(char));

    while(fgets(buffer, sizeof(buffer), output))
    {
        result = (char*)realloc(result, sizeof(buffer) + rsize);
        rsize += sizeof(buffer);
        strcat(result, buffer);
    }
    int code = pclose(output);
    if (WIFEXITED(code))
    {
        * exit_code = WEXITSTATUS(code);
    }
    else
    {
        * exit_code = 256;
    }
    return result;
}

int Mariadb_nodes::ssh_node(int node, const char *ssh, bool sudo)
{
    char sys[strlen(ssh) + 1024];
    generate_ssh_cmd(sys, node, ssh, sudo);
    int return_code = system(sys);
    if (WIFEXITED(return_code))
    {
        return WEXITSTATUS(return_code);
    }
    else
    {
      return 256;
    }
}

int Mariadb_nodes::flush_hosts()
{

    if (this->nodes[0] == NULL)
    {
        this->connect();
    }

    int local_result = 0;

    for (int i = 0; i < N; i++)
    {
        if (mysql_query(nodes[i], "FLUSH HOSTS"))
        {
            local_result++;
        }

        if (mysql_query(nodes[i], "SET GLOBAL max_connections=10000"))
        {
            local_result++;
        }
    }

    return local_result;
}

int Mariadb_nodes::execute_query_all_nodes(const char* sql)
{
    int local_result = 0;
    connect();
    for (int i = 0; i < N; i++) {
        local_result += execute_query(nodes[i], sql);
    }
    close_connections();
    return(local_result);
}

int Mariadb_nodes::get_versions()
{
    int local_result = 0;
    char * str;
    v51 = false;

    for (int i = 0; i < N; i++) {
        if ((local_result += find_field(nodes[i], (char *) "SELECT @@version", (char *) "@@version", version[i])))
        {
            printf("Failed to get version: %s\n", mysql_error(nodes[i]));
        }
        strcpy(version_number[i], version[i]);
        str = strchr(version_number[i], '-');
        if (str != NULL) {str[0] = 0;}
        strcpy(version_major[i], version_number[i]);
        if (strstr(version_major[i], "5.") ==  version_major[i]) {version_major[i][3] = 0;}
        if (strstr(version_major[i], "10.") ==  version_major[i]) {version_major[i][4] = 0;}

        if (verbose)
            printf("Node %s%d: %s\t %s \t %s\n", prefix, i, version[i], version_number[i], version_major[i]);
    }

    for (int i = 0; i < N; i++)
    {
        if (strcmp(version_major[i], "5.1") == 0)
        {
            v51 = true;
        }
    }

    return(local_result);
}

int Mariadb_nodes::truncate_mariadb_logs()
{
    int local_result = 0;
    for (int node = 0; node < N; node++)
    {
       char sys[1024];
        sprintf(sys, "ssh -i %s -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -o LogLevel=quiet %s@%s 'sudo truncate  /var/lib/mysql/*.err --size 0\' &",
                sshkey[node], access_user[node], IP[node]);
       local_result += system(sys);
    }
    return local_result;
}

int Mariadb_nodes::configure_ssl(bool require)
{
    int local_result = 0;
    char str[1024];

    this->ssl = 1;

    for (int i = 0; i < N; i++)
    {
        printf("Node %d\n", i);
        stop_node(i);
        sprintf(str, "%s/ssl-cert", test_dir);
        local_result += copy_to_node(str, (char *) "~/", i);
        sprintf(str, "%s/ssl.cnf", test_dir);
        local_result += copy_to_node(str, (char *) "~/", i);
        local_result += ssh_node(i, (char *) "cp ~/ssl.cnf /etc/my.cnf.d/", true);
        local_result += ssh_node(i, (char *) "cp -r ~/ssl-cert /etc/", true);
        local_result += ssh_node(i,  (char *) "chown mysql:mysql -R /etc/ssl-cert", true);
        start_node(i,  (char *) "");
    }

    if (require) {
        // Create DB user on first node
        printf("Set user to require ssl: %s\n", str);
        sprintf(str, "%s/create_user_ssl.sh", test_dir);
        copy_to_node(str, (char *) "~/", 0);

        sprintf(str, "export node_user=\"%s\"; export node_password=\"%s\"; ./create_user_ssl.sh", user_name, password);
        printf("cmd: %s\n", str);
        ssh_node(0, str, false);
    }

    return local_result;
}

int Mariadb_nodes::disable_ssl()
{
    int local_result = 0;
    char str[1024];

    local_result += connect();
    sprintf(str, "DROP USER %s;  grant all privileges on *.*  to '%s'@'%%' identified by '%s';", user_name, user_name, password);
    local_result += execute_query(nodes[0], (char *) "");
    close_connections();

    for (int i = 0; i < N; i++)
    {
        stop_node(i);
        local_result += ssh_node(i, (char *) "rm -f /etc/my.cnf.d/ssl.cnf", true);
        start_node(i,  (char *) "");
    }

    return local_result;
}

int Mariadb_nodes::copy_to_node(const char* src, const char* dest, int i)
{
    if (i >= N)
    {
        return 1;
    }
    char sys[strlen(src) + strlen(dest) + 1024];

    sprintf(sys, "scp -q -r -i %s -o UserKnownHostsFile=/dev/null "
            "-o StrictHostKeyChecking=no -o LogLevel=quiet %s %s@%s:%s",
            sshkey[i], src, access_user[i], IP[i], dest);

    if (verbose)
    {
        printf("%s\n", sys);
    }

    return system(sys);
}


int Mariadb_nodes::copy_from_node(const char* src, const char* dest, int i)
{
    if (i >= N)
    {
        return 1;
    }
    char sys[strlen(src) + strlen(dest) + 1024];

    sprintf(sys, "scp -q -r -i %s -o UserKnownHostsFile=/dev/null "
            "-o StrictHostKeyChecking=no -o LogLevel=quiet %s@%s:%s %s",
            sshkey[i], access_user[i], IP[i], src, dest);

    if (verbose)
    {
        printf("%s\n", sys);
    }

    return system(sys);
}

static void wait_until_pos(MYSQL *mysql, int filenum, int pos)
{
    int slave_filenum = 0;
    int slave_pos = 0;

    do
    {
        if (mysql_query(mysql, "SHOW SLAVE STATUS"))
        {
            printf("Failed to execute SHOW SLAVE STATUS: %s", mysql_error(mysql));
            break;
        }

        MYSQL_RES *res = mysql_store_result(mysql);

        if (res)
        {
            MYSQL_ROW row = mysql_fetch_row(res);

            if (row && row[6] && row[21])
            {
                char *file_suffix = strchr(row[5], '.') + 1;
                slave_filenum = atoi(file_suffix);
                slave_pos = atoi(row[21]);
            }
            mysql_free_result(res);
        }
    }
    while(slave_filenum < filenum || slave_pos < pos);
}

void Mariadb_nodes::sync_slaves()
{
    if (this->nodes[0] == NULL)
    {
        this->connect();
    }

    if (mysql_query(this->nodes[0], "SHOW MASTER STATUS"))
    {
        printf("Failed to execute SHOW MASTER STATUS: %s", mysql_error(this->nodes[0]));
    }
    else
    {
        MYSQL_RES *res = mysql_store_result(this->nodes[0]);

        if (res)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && row[0] && row[1])
            {
                const char* file_suffix = strchr(row[0], '.') + 1;
                int filenum = atoi(file_suffix);
                int pos = atoi(row[1]);

                for (int i = 1; i < this->N; i++)
                {
                    wait_until_pos(this->nodes[i], filenum, pos);
                }
            }
            mysql_free_result(res);
        }
    }
}

void Mariadb_nodes::close_active_connections()
{
    if (this->nodes[0] == NULL)
    {
        this->connect();
    }

    const char *sql = "select id from information_schema.processlist where id != @@pseudo_thread_id and user not in ('system user', 'repl')";

    for (int i = 0; i < N; i++)
    {
        if (!mysql_query(nodes[i], sql))
        {
            MYSQL_RES *res = mysql_store_result(nodes[i]);
            if (res)
            {
                MYSQL_ROW row;

                while ((row = mysql_fetch_row(res)))
                {
                    std::string q("KILL ");
                    q += row[0];
                    execute_query_silent(nodes[i], q.c_str());
                }
                mysql_free_result(res);
            }
        }
    }
}
