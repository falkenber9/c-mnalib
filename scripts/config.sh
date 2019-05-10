 
### Traffic Test Config

APPLICATION="traffic_test"
APP_PARAMS="-n 10 -p 30 -s 3000000 -o $LOGDIR"

LOCAL_HOSTNAME=$(hostname)

STORAGE_SERVER_ADDR="129.217.211.43"
STORAGE_SERVER_USER="ng40"
STORAGE_SERVER_BASEDIR="c-mna-log"


### Reverse Tunnel Config

PROXYPORT="22"
PROXYADDR="129.217.211.43"
PROXYLOGIN="ng40"
CONNECTIONPORT="5001"