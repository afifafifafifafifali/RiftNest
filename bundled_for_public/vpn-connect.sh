#!/bin/bash
# RiftNest VPN connect (vpnuser + client2, no embedded passwords)
# Usage: ./vpn-connect.sh   (you will be asked for YOUR sudo password)
set -e
cd "$(dirname "$0")"

echo "[1/3] Killing stale openvpn/forward..."
sudo killall openvpn 2>/dev/null || true
for pid in $(pgrep -f "vpnuser@ecranberry" 2>/dev/null); do kill "$pid" 2>/dev/null || true; done
sleep 1

echo "[2/3] Opening SSH forward as vpnuser (key auth)..."
ssh -i ./vpnuser-key -f -N -o StrictHostKeyChecking=no -o ExitOnForwardFailure=yes -o BatchMode=yes -L 1194:127.0.0.1:1194 -p 45912 vpnuser@ecranberryctfserver.n3.sparkden.cloud
python3 -c "import socket; socket.create_connection(('127.0.0.1',1194),timeout=8); print('forward OK')"

echo "[3/3] Starting OpenVPN client (client2)..."
sudo openvpn --config ./riftnest-client2.ovpn --daemon --log /tmp/openvpn-riftnest.log
sleep 12
sudo grep -E "Initialization Sequence Completed|ERROR" /tmp/openvpn-riftnest.log | tail -2
echo ""
echo "Test: ping -c 2 10.0.2.1  (server)  |  ping -c 2 10.0.3.1  (container bridge)"
