#!/bin/bash

function check_server_count()
{
    maxadmin_output=$(ssh -i $maxscale_sshkey -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null $maxscale_access_user@$maxscale_IP "$maxdir_bin/maxadmin -p$maxadmin_password -uadmin list servers")
    echo "$maxadmin_output"
    server_count=$(echo "$maxadmin_output"|grep -i 'server[1-5]'|wc -l)
    if [[ $server_count -ne $1 ]]
    then
        echo "Error: Incorrect amount of servers!"
        echo "Found $server_count servers when $1 was expected"
        return 1
    fi
    return 0
}

rp=`realpath $0`
export test_dir=`dirname $rp`
export test_name=`basename $rp`

# Save the old configuration value

# Start with a 4 server config file
$test_dir/non_native_setup $test_name
#$test_dir/configure_maxscale.sh
#echo "Waiting for 15 seconds"
#sleep 15

check_server_count 4
if [[ $? -ne 0 ]]
then
    exit 1
fi

# Reload the same config file but with one extra server
# and check that the new server is detected
export test_name=reload_server
$test_dir/non_native_setup $test_name
#$test_dir/configure_maxscale.sh
#echo "Waiting for 15 seconds"
#sleep 15

check_server_count 5
if [[ $?  -ne 0 ]]
then
    exit 1
fi

# Remove the extra server and check that it is also
# removed from MaxScale
export test_name=config_reload
$test_dir/non_native_setup $test_name
#$test_dir/configure_maxscale.sh
#echo "Waiting for 15 seconds"
#sleep 15

check_server_count 4
if [[ $? -ne 0 ]]
then
    exit 1
fi
