#
# Example MaxScale.cnf configuration file
#
#
#
# Number of server threads
# Valid options are:
# 	threads=<number of threads>

[maxscale]
threads=###threads###
log_warning=1

# Define a monitor that can be used to determine the state and role of
# the servers.
#
# Valid options are:
#
# 	module=<name of module to load>
# 	servers=<server name>,<server name>,...
# 	user =<user name - must have slave replication and 
#                          slave client privileges>
# 	passwd=<password of the above user, plain text currently>
#	monitor_interval=<sampling interval in milliseconds,
#                          default value is 10000>

[MySQL Monitor]
type=monitor
module=mmmon
servers= server1, server2
user=maxskysql
passwd= skysql
detect_stale_master=0
monitor_interval=1000

# A series of service definition
#
# Valid options are:
#
# 	router=<name of router module>
# 	servers=<server name>,<server name>,...
# 	user=<User to fetch password inforamtion with>
# 	passwd=<Password of the user, plain text currently>
#	enable_root_user=<0 or 1, default is 0>
#	version_string=<specific string for server handshake,
#		default is the MariaDB embedded library version>
#
# Valid router modules currently are:
# 	readwritesplit, readconnroute and debugcli

[RW Split Router]
type=service
router= readwritesplit
servers=server1,     server2
user=maxskysql
passwd=skysql
router_options=slave_selection_criteria=LEAST_ROUTER_CONNECTIONS
filters=QLA

[Read Connection Router Slave]
type=service
router=readconnroute
router_options= slave
servers=server1,server2
user=maxskysql
passwd=skysql
filters=QLA

[Read Connection Router Master]
type=service
router=readconnroute
router_options=master
servers=server1,server2
user=maxskysql
passwd=skysql
filters=QLA

[Debug Interface]
type=service
router=debugcli

# Listener definitions for the services
#
# Valid options are:
#
# 	service=<name of service defined elsewhere>
# 	protocol=<name of protocol module with which to listen>
# 	port=<Listening port>
#	address=<Address to bind to>
#	socket=<Listening socket>

[RW Split Listener]
type=listener
service=RW Split Router
protocol=MySQLClient
port=4006
#socket=/tmp/rwsplit.sock

[Read Connection Listener Slave]
type=listener
service=Read Connection Router Slave
protocol=MySQLClient
port=4009

[Read Connection Listener Master]
type=listener
service=Read Connection Router Master
protocol=MySQLClient
port=4008

[Debug Listener]
type=listener
service=Debug Interface
protocol=telnetd
port=4442
#address=127.0.0.1


[CLI]
type=service
router=cli

[CLI Listener]
type=listener
service=CLI
protocol=maxscaled
#address=localhost
socket=default


# Definition of the servers

[server1]
type=server
address=###repl_server_IP_1###
port=###repl_server_port_1###
protocol=MySQLBackend

[server2]
type=server
address=###repl_server_IP_2###
port=###repl_server_port_2###
protocol=MySQLBackend

[QLA]
type=filter
module=qlafilter
filebase=/tmp/QueryLog
