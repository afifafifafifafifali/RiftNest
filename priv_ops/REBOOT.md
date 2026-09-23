# priv_ops — reboot survival notes

## What happened (2026-09-22)

`ssh root@10.0.3.2` failed with `No route to host` even though the VPN
client showed `Initialization Sequence Completed`, `tun0` was up, and
all routes (`10.0.2.0/24`, `10.0.3-5.0/24`) were present locally.

Cause: **the server had rebooted** (`uptime` showed `up 6 min`).
Reboot wipes all runtime state, so both instances went dark.

## What survives a reboot vs what does not

| Survives (disk)                          | Dies (runtime, must be rebuilt)      |
|------------------------------------------|--------------------------------------|
| Instance rootfs (`~/.riftnest/instances/`)| netns holders (`*-holder`)          |
| dropbear binary + host keys (in rootfs)  | veth pairs (`veth-*`)                |
| root passwords (in rootfs `/etc/shadow`) | dropbear processes                   |
| `/root/mybox-netns.sh`, `/root/priv_ops/`| OpenVPN daemon (stock unit restarts it) |
| `br0` + gateway IPs (netinit unit)       | VPN client sessions (reconnect)      |

## Boot chain (runs automatically, in order)

1. `riftnet-netinit.service` → IP forward, `br0` + `.3.1/.4.1/.5.1`,
   iptables NAT/FORWARD, starts OpenVPN (`127.0.0.1:1194`).
2. Stock `openvpn@server`/`ovpn-server` unit → serves `10.0.2.0/24`,
   pushes routes for `10.0.3-5.0/24`.
3. `riftnest-instances.service` → runs `/root/priv_ops/attach-all.sh`,
   which attaches every `name ip` line in
   `/root/priv_ops/instances.conf` via `/root/mybox-netns.sh`.

## Adding a new instance (full recipe)

```bash
# 1. persistent instance (riftnest CLI, on server)
riftnest create <name> <image>

# 2. SSH server inside it (riftnest CLI, on server)
riftnest exec <name> /sbin/apk add dropbear
riftnest exec <name> /bin/sh -c "mkdir -p /etc/dropbear && /usr/bin/dropbearkey -t rsa -f /etc/dropbear/dropbear_rsa_host_key"
riftnest exec <name> /bin/sh -c "echo root:<password> | chpasswd"

# 3. attach + persist
bash /root/mybox-netns.sh <name> <ip>     # e.g. webvm 10.0.4.2
echo "<name> <ip>" >> /root/priv_ops/instances.conf

# 4. connect (VPN up on client)
ssh root@<ip>
```

## Verify after any reboot

From the VPN client:
```bash
for ip in 10.0.2.1 10.0.3.1 10.0.4.1 10.0.5.1 10.0.3.2 10.0.3.3; do
  printf "%s: " "$ip"; ping -c 1 -W 2 "$ip" | grep -oE "1 received|100% packet loss"
done
ssh root@<instance-ip>   # password from step 2
```

If an instance is dark but gateways answer: its holder died —
just rerun `bash /root/mybox-netns.sh <name> <ip>`.
