This is a new structure including udp-server(PC)+tunslip+border-router(Root)+udp-client(mote).

1.mote: send sensed data packet
using "udp-client.c" from "rpl-udp" folder.

*if change hardware, should change "Makefile.target" and "build.sh" in rpl-udp folder.

execute command:
> ./build.sh
then load code to hardware: choose "udp-client.bin" file and use uniflase or use upload.

2.Border-router(root): make a border-router to forward packet to host(pc).
using "node.c" from "node-6dr-BR" folder.

*if change hardware, should change "Makefile.target" and "build.sh" in rpl-udp folder.

execute command:
> ./build.sh
then load code to hardware: choose "node.bin" file and use uniflase or use upload.

3.tunslip: make a ipv6 tunnel between root and pc.
(1.)compile in tools/serial-io
>cd contiki-ng_cc1350/tools/serial-io
>make
(2.)execute tunslip
>sudo ./tunslip6 -s fd00::1/64 /dev/ttyUSB0
or
>sudo ./tunslip6 -s fd00::1/64 /dev/ttyACM0

4.Host(pc): using udp_server&parse0614.py
execute udp_server&parse0614.py to make a udp server using port 5678.

>python udp_server&parse0614.py -s 5678


