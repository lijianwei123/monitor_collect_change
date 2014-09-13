#!/bin/sh


#mem leak check
check_leak() {
	valgrind --tool=memcheck --leak-check=full --show-reachable=yes ./look -c config.ini
}

#look mem info
look_mem() {
	local pid=`ps -ef|grep 'look -c' |grep -v 'grep'|awk '{print $2}'`
	top -d 1 -p $pid
}

#stop 
stop() {
   echo "stop look"
   local pid=`ps -ef|grep 'look -c' |grep -v 'grep'|awk '{print $2}'`
   kill -2 $pid
}

case "$1" in
  leak)
        check_leak
        ;;
  mem)
        look_mem
        ;;
  stop)
	stop
	;;
esac
