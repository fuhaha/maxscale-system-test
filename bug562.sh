#!/bin/bash

rp=`realpath $0`
export test_dir=`dirname $rp`
export test_name=`basename $rp`

$test_dir/non_native_setup $test_name
#$test_dir/configure_maxscale.sh
if [ $? -ne 0 ] ; then 
        echo "configure_maxscale.sh failed"
        exit 1
fi
export ssl_options="--ssl-cert=$test_dir/ssl-cert/client-cert.pem --ssl-key=$test_dir/ssl-cert/client-key.pem"

mariadb_err=`mysql -u no_such_user -psome_pwd -h $repl_001 $ssl_option test 2>&1`
maxscale_err=`mysql -u no_such_user -psome_pwd -h $maxscale_IP -P 4006  $ssl_options test 2>&1`

echo "MariaDB message"
echo "$mariadb_err"
echo " "
echo "Maxscale message"
echo "$maxscale_err"

res=0
#echo "$maxscale_err" | grep "$mariadb_err"
echo "$maxscale_err" |grep "ERROR 1045 (28000): Access denied for user 'no_such_user'@'"
if [ "$?" != 0 ]; then
	echo "Maxscale message is not ok!"
    echo "Message: $maxscale_err"
	res=1
else
	echo "Messages are same"
	res=0
fi

$test_dir/copy_logs.sh bug562
exit $res
