#!/bin/bash
#
# um das Durchschleifen des Reverse-Proxys zu aktivieren, 
# sollten folgende Einstellungen in der /etc/ssh/sshd_conf
# vorgenommen werden:
#
#	ClientAliveInterval 30
#	ClientAliveCountMax 99999
#	GatewayPorts yes
#	AllowTcpForwarding yes
#
# Der nachfolgende Code richtet einen Reversetunnel zu diesem Rechner ein

# Reverse Tunnel Server Config
PROXYPORT="22"
PROXYADDR="129.217.211.43"
PROXYLOGIN="ng40"
CONNECTIONPORT="5001"

source config.sh

while true;
do

	echo "Reversetunnel eingerichtet."
	echo "Dieser Computer kann nun wie folgt erreicht werden:"
	echo "ssh <username>@$PROXYADDR -p $CONNECTIONPORT"
	echo ""



	#ssh -R $CONNECTIONPORT:localhost:$PROXYPORT $PROXYLOGIN@$PROXYADDR
	autossh -M 20000 -R $CONNECTIONPORT:localhost:$PROXYPORT $PROXYLOGIN@$PROXYADDR
	
	echo "Wait 60s to reconnect..."
	sleep 6

done;
