#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_ETH_SERVER_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_ETH_SERVER_H

/**
 * @file
 * Приём соединений по TCP для эмуляции датчиков (лекция 3).
 *
 * Практика `03-control-program` умеет получать состояние входов не только
 * из файла, но и по сети: так на занятии одну программу можно кормить
 * данными с другой машины. Здесь собрано всё, что для этого нужно от
 * платформы, — включая расхождения Windows и POSIX.
 */

#include "base_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>

/** В Windows сокет закрывается своей функцией, в POSIX — обычным close(). */
#define closesocket(s) close((s))

#endif

/**
 * Слушающий сокет TCP на указанном порту.
 *
 * @param port номер порта строкой, как его принимает getaddrinfo
 * @return дескриптор сокета, готового к accept(), или -1 при ошибке.
 *         Закрывать его — closesocket().
 */
int socket_listen_server(const char *port);

#endif /* C_AUTOMATA_PROGRAMMING_PRACTICE_ETH_SERVER_H */
