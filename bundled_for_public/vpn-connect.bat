@echo off
REM RiftNest VPN connect for Windows 10 (vpnuser + client2, no passwords in file)
REM Run this file as Administrator (right-click -> Run as administrator).
REM Requires: OpenVPN for Windows installed (https://openvpn.net/community-downloads/)
REM           + built-in Windows OpenSSH client (present on Win10 1809+).
cd /d "%~dp0"

REM --- must be admin (TUN device needs it) ---
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] Right-click this file and choose "Run as administrator".
    pause
    exit /b 1
)

echo [1/3] Killing stale openvpn...
taskkill /F /IM openvpn.exe >nul 2>&1

echo [2/3] Locking down key perms and opening SSH forward as vpnuser...
icacls "%~dp0vpnuser-key" /inheritance:r >nul 2>&1
icacls "%~dp0vpnuser-key" /grant:r "%USERNAME%:F" >nul 2>&1
start "riftnest-ssh-forward" /min ssh.exe -N -o StrictHostKeyChecking=no -o ExitOnForwardFailure=yes -o BatchMode=yes -i "%~dp0vpnuser-key" -L 1194:127.0.0.1:1194 -p 45912 vpnuser@ecranberryctfserver.n3.sparkden.cloud
timeout /t 4 /nobreak >nul

echo [3/3] Starting OpenVPN client (client2)... leave this window open.
"C:\Program Files\OpenVPN\bin\openvpn.exe" --config "%~dp0riftnest-client2.ovpn"
