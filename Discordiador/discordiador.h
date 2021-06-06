/*
 * Discordiador.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include "utils.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <commons/log.h>
//#include <commons/config.h>
//#include <commons/string.h>
//#include <pthread.h>
//#include <readline/readline.h>
//#include <time.h>
//#include <stddef.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>

void leer_consola();
void iniciar_patota(char* leido);
void iniciar_planificacion();
void listar_tripulantes();
void pausar_planificacion();
void obtener_bitacora(char* leido);
int pedir_tarea(int id_tripulante);
void realizar_tarea(t_tarea tarea);
char* fecha_y_hora();



//HILOS
void expulsar_tripulante(char* leido);
void iniciar_hilo_tripulante(void* funcion);
t_TCB* crear_tcb(t_PCB* pcb, int tid, char* posicion);
t_tripulante* crear_tripulante(t_TCB* un_tcb);
t_tripulante* iniciar_tripulante(void* funcion, t_PCB* pcb, int tid, char* posicion);

//PROCESOS
t_patota* crear_patota(t_PCB* un_pcb);
t_PCB* crear_pcb(char* path);
int nuevo_pid();
//int pids_contiene(int valor); // se puede eliminar, ya es lista

int esta_en_lista(t_list* lista, int elemento);
int sonIguales(int elemento1, int elemento2);



#endif /* DISCORDIADOR_H_ */
