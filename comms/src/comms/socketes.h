#ifndef SOCKETES_H_
#define SOCKETES_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

void sigchld_handler(int s);
int crear_socket_cliente(char *ip_del_servidor_a_conectar, char* puerto_del_servidor);
int crear_socket_oyente(char *ip_del_servidor_a_conectar, char* puerto_del_servidor);
void escuchar(int socket_escucha);
int enviar_mensaje(int socket, void* mensaje, int largo);
int recibir_mensaje(int socket, void* buffer, int largo);

#endif
