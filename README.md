# UDP server for QOTD service
This project is actually for the Architecture of Networks and Services course at the University of Valladolid.
It is a server for serving QOTD, currently supporting UDP, but modular and scalable for other possible services, both TCP, other UDP protocols or even ICMP.

### Information for building the project
It is compiled using Makefiles, so just execute make to compile the project, and you will get the output program in bin/.
```bash
$ make
```

To clean all object files and the program linked, run:
```bash
$ make clean
```

### Information on executing the program
This server takes the optional flags `-h/--help` and `-s service/--service service`, where the current supported services are "qotd", which is a optional parameter, as it is the default.
```bash
# qotd-udp-server-g201
```
```bash
# qotd-udp-server-g201 -s service
```
```bash
# qotd-udp-server-g201 --service service
```

> **Note:** The client should be run as superuser, to be able to open a socket against the machine.