# UDP client for QOTD service
This project is actually for the Architecture of Networks and Services course at the University of Valladolid.
It is a client for getting QOTD quotes currently supporting UDP, but modular and scalable for other possible services, both TCP, UDP or even ICMP.

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
This client requires the IP of the QOTD server in dot notation: `XXX.XXX.XXX.XXX`. Therefore, you should execute it with that parameter, and also takes the optional flags `-h/--help` and `-s service/--service service`, where the current supported services are "qotd", which is a optional parameter, as it is the default.
```bash
# qotd-udp-client-g201 XXX.XXX.XXX.XXX
```
```bash
# qotd-udp-client-g201 XXX.XXX.XXX.XXX -s service
```
```bash
# qotd-udp-client-g201 XXX.XXX.XXX.XXX --service service
```

> **Note:** The client should be run as superuser, to be able to open a socket against the machine.