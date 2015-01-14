#./alg
ifconfig tun0 up
ip addr add 10.20.30.1/24 dev tun0
ip addr add 2003::1/24 dev tun0
