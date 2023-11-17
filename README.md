# JIG.m1s.client
odroid-m1s client

### Documentation for JIG development
* Document : https://docs.google.com/spreadsheets/d/14jsR5Y7Cq3gO_OViS7bzTr-GcHdNRS_1258eUWI2aqs/edit#gid=1673179576
* Protocol : https://docs.google.com/spreadsheets/d/1Of7im-2I5m_M-YKswsubrzQAXEGy-japYeH8h_754WA/edit#gid=0

### Base linux image
 * https://dn.odroid.com/RK3566/ODROID-M1S/Ubuntu/ubuntu-20.04-server-odroidm1s-20231030.img.xz

### Client mode settings

* Install ubuntu package & python3 module
```
// ubuntu system update
root@server:~# apt update && apt upgrade -y

// ubuntu package
root@server:~# apt install samba ssh build-essential python3 python3-pip ethtool net-tools usbutils git i2c-tools vim cups cups-bsd overlayroot nmap

// python3 package
root@server:~# pip install aiohttp asyncio
```

* Edit the /boot/config.ini
```
root@server:~# vi /boot/config.ini
```
```
[generic]
overlay_resize=16384
overlay_profile=""
# default
# overlays="spi0 i2c0 i2c1"

# JIG Server overlays(Vu8s LCD)
# overlays="uart0 i2c0 i2c1 display_vu8s"

# JIG Server overlays(Development for HDMI)
# overlays="uart0 i2c0 i2c1"

# JIG Client overlays
overlays="fiq0_to_uart2"

[overlay_custom]
overlays="i2c0 i2c1"

[overlay_hktft32]
overlays="hktft32"

[overlay_hktft35]
overlays="hktft35 sx865x-i2c1"
```
* disable console options
```
// Edit the /usr/share/flash-kernel/ubootenv.d/upstream/10-console
root@odroid:~# vi /usr/share/flash-kernel/ubootenv.d/upstream/10-console

...
# Default TTY console
# setenv bootargs "${bootargs} console=tty1 console=ttyS2,1500000"
...

root@odroid:~# update-bootscript

```

### Clone the reopsitory with submodule
```
root@odroid:~# git clone --recursive https://github.com/charles-park/JIG.m1s.client

or

root@odroid:~# git clone https://github.com/charles-park/JIG.m1s.client
root@odroid:~# cd JIG.m1s.client
root@odroid:~/JIG.m1s.client# git submodule update --init --recursive
```

### iperf3_odroid client mode install
```
root@server:~# git clone httos://github.com/charles-park/iperf3_odroid

root@server:~# cd iperf3_odroid

// iperf3 deb install
root@server:~/iperf3_odroid# apt install libiperf0 libsctp1
root@server:~/iperf3_odroid# dpkg -i iperf3_deb/iperf3_3.7-3_arm64.deb

or
// iperf3 install
root@server:~/iperf3_odroid# apt install iperf3

root@server:~/iperf3_odroid# make

root@server:~/iperf3_odroid# make install
```

### add root user, ssh root enable (https://www.leafcats.com/176)
```
// root user add
root@server:~# passwd root

root@server:~# vi /etc/ssh/sshd_config

...
# PermitRootLogin prohibit-password
PermitRootLogin yes
StrictModes yes
PubkeyAuthentication yes
...

root@server:~# service ssh restart
```

### client auto login
```
root@server:~# systemctl edit getty@tty1.service
```
```
[Service]
ExecStart=
ExecStart=-/sbin/agetty --noissue --autologin root %I $TERM
Type=idle
```

### client samba config
```
root@server:~# smbpasswd -a root
root@server:~# vi /etc/samba/smb.conf
```
```
[odroidm1s-client]
   comment = odroid-m1s jig root
   path = /root
   guest ok = no
   browseable = no
   writable = yes
   create mask = 0755
   directory mask = 0755
```

### Sound setup
```
root@server:~# aplay -l
**** List of PLAYBACK Hardware Devices ****
card 0: rockchiphdmi0 [rockchip-hdmi0], device 0: fe400000.i2s-i2s-hifi i2s-hifi-0 [fe400000.i2s-i2s-hifi i2s-hifi-0]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 1: rockchiprk809 [rockchip-rk809], device 0: fe410000.i2s-rk817-hifi rk817-hifi-0 [fe410000.i2s-rk817-hifi rk817-hifi-0]
  Subdevices: 1/1
  Subdevice #0: subdevice #0

root@server:~# amixer -c 1
Simple mixer control 'Playback Path',0
  Capabilities: enum
  Items: 'OFF' 'RCV' 'SPK' 'HP' 'HP_NO_MIC' 'BT' 'SPK_HP' 'RING_SPK' 'RING_HP' 'RING_HP_NO_MIC' 'RING_SPK_HP'
  Item0: 'OFF'
Simple mixer control 'Capture MIC Path',0
  Capabilities: enum
  Items: 'MIC OFF' 'Main Mic' 'Hands Free Mic' 'BT Sco Mic'
  Item0: 'MIC OFF'

root@server:~# amixer -c 1 sset 'Playback Path' 'HP'
root@server:~# amixer -c 1
Simple mixer control 'Playback Path',0
  Capabilities: enum
  Items: 'OFF' 'RCV' 'SPK' 'HP' 'HP_NO_MIC' 'BT' 'SPK_HP' 'RING_SPK' 'RING_HP' 'RING_HP_NO_MIC' 'RING_SPK_HP'
  Item0: 'HP'
Simple mixer control 'Capture MIC Path',0
  Capabilities: enum
  Items: 'MIC OFF' 'Main Mic' 'Hands Free Mic' 'BT Sco Mic'
  Item0: 'MIC OFF'

// play audio file
root@server:~# aplay -Dhw:1,0 {audio file} -d {play time}
```

### Overlay root
* overlayroot enable
```
root@server:~# update-initramfs -c -k $(uname -r)
Using DTB: rockchip/rk3566-odroid-m1s.dtb
Installing rockchip into /boot/dtbs/5.10.0-odroid-arm64/rockchip/
Installing rockchip into /boot/dtbs/5.10.0-odroid-arm64/rockchip/
flash-kernel: installing version 5.10.0-odroid-arm64
Generating boot script u-boot image... done.
Taking backup of boot.scr.
Installing new boot.scr.

root@server:~# mkimage -A arm64 -O linux -T ramdisk -C none -a 0 -e 0 -n uInitrd -d /boot/initrd.img-$(uname -r) /boot/uInitrd 
Image Name:   uInitrd
Created:      Fri Oct 27 04:27:58 2023
Image Type:   AArch64 Linux RAMDisk Image (uncompressed)
Data Size:    7805996 Bytes = 7623.04 KiB = 7.44 MiB
Load Address: 00000000
Entry Point:  00000000

// Change overlayroot value "" to "tmpfs" for overlayroot enable
root@server:~# vi /etc/overlayroot.conf
...
overlayroot_cfgdisk="disabled"
overlayroot="tmpfs"
```
* overlayroot disable
```
// get write permission
root@server:~# overlayroot-chroot 
INFO: Chrooting into [/media/root-ro]
root@server:~# 

// Change overlayroot value "tmpfs" to "" for overlayroot disable
root@server:~# vi /etc/overlayroot.conf
