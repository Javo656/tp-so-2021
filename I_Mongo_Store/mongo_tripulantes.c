#include "mongo_tripulantes.h"

void manejo_tripulante(void* socket) {
	int socket_tripulante = ((hilo_tripulante*) socket)->socket;
	char* posicion_tripulante;
	t_TCB* tripulante;
	while(1) {
		// Se espera a ver que manda el tripulante
		log_info(logger_mongo, "Esperando mensaje ");

		t_estructura* mensaje = recepcion_y_deserializacion(socket_tripulante);
		log_trace(logger_mongo, "El código recibido es: %i", mensaje->codigo_operacion);

		// Si es primera conexion, se crea la bitacora y se asigna a la lista
		if (mensaje->codigo_operacion == RECIBIR_TCB) {
		    log_info(logger_mongo, "Pedido de crear bitacora");
		    log_trace(logger_mongo, "Creando bitacora para el tripulante %i.", mensaje->tcb->TID);
		    posicion_tripulante = formatear_posicion(mensaje->tcb->coord_x, mensaje->tcb->coord_y);
		    log_info(logger_mongo, "La posicion inicial del tripulante es: %c|%c", posicion_tripulante[0], posicion_tripulante[2]);
			crear_estructuras_tripulante(mensaje->tcb, socket_tripulante);
			log_trace(logger_mongo, "Se creo la bitacora del tripulante %i.", mensaje->tcb->TID);
			tripulante = mensaje->tcb;
		}
		// Si no lo es, puede ser o agregar/quitar recursos o cambiar informacion en la bitacora
		else {
			// Codigos mayores a Basura y menores a Sabotaje corresponden a asignaciones de bitacora
			if (mensaje->codigo_operacion > BASURA && mensaje->codigo_operacion < SABOTAJE) {
				log_info(logger_mongo, "Pedido de modificar bitacora");
				// log_trace(logger_mongo, "Modificando bitacora del tripulante %i", mensaje->tcb->TID); // eventualmente rompe, no perder el tiempo con esto
				modificar_bitacora(mensaje, &posicion_tripulante, socket_tripulante);
				log_info(logger_mongo, "fin de modificar bitacora");
			}

			// Si es otro codigo
			else if(mensaje->codigo_operacion > BITACORA && mensaje->codigo_operacion < MOVIMIENTO){
				log_info(logger_mongo, "Pedido de alterar cositas");
				// log_trace(logger_mongo, "Recibo un pedido de alterar, tripulante %i", mensaje->tcb->TID); // eventualmente rompe, no perder el tiempo con esto
				log_trace(logger_mongo, "Numero de codigo: %i", mensaje->codigo_operacion);
				alterar(mensaje->codigo_operacion, mensaje->cantidad); 
			}
		}

		// Ultimo mensaje del tripulante, al morir o algo, sera la desconexion, lo cual borra la bitacora y libera los recursos
		if (mensaje->codigo_operacion == DESCONEXION) { // Tripulante avisa desconexion para finalizar proceso
			log_info(logger_mongo, "Se desconecto un tripulante.\n");
			borrar_bitacora(tripulante);
			free(tripulante);
			free(posicion_tripulante);

			// Aca finalizaria el hilo creado por el tripulante al conectarse a Mongo
			pthread_exit(0);
		}

		free(mensaje);
	}
}

char* rescatar_bitacora(char* path){
	log_trace(logger_mongo, "Rescatando bitacora");
	t_list* lista_bloques_bitacora = get_lista_bloques(path);

	int lectura = 0;
	int size = tamanio_archivo(path);

	if(size == 0){
		return NULL;
	}

	char* string = malloc(size);
	log_trace(logger_mongo, "Size es: %i", size);
	int* aux;
	for(int i = 0; i < list_size(lista_bloques_bitacora); i++){
		aux = list_get(lista_bloques_bitacora, i);
		for(int j = 0; j < TAMANIO_BLOQUE; j++){

			string[lectura] = *(directorio.mapa_blocks + (TAMANIO_BLOQUE * *aux) + j);
			lectura++;

			if(lectura == size){
				return string;
			}

		}
	}

	return string;
}

void crear_estructuras_tripulante(t_TCB* tcb, int socket_tripulante) {
	// Se obtiene el path particular del tripulante, identificado con su TID
    log_trace(logger_mongo, "INICIO crear_estructuras_tripulante");

	char* path_tripulante = fpath_tripulante(tcb);

	// Se crea el archivo del tripulante y se lo abre
	FILE* file_tripulante = fopen(path_tripulante, "w+");
	
	//Se inicializan los datos del tripulante
	escribir_archivo_tripulante(path_tripulante, 0, NULL);

	// Se lo guarda en la bitacora
	acomodar_bitacora(file_tripulante, path_tripulante, tcb);
    log_trace(logger_mongo, "FIN crear_estructuras_tripulante");

}

void acomodar_bitacora(FILE* file_tripulante, char* path_tripulante, t_TCB* tcb) {
	// Se utiliza un struct que conoce al tripulante y a su archivo, para luego saber donde se realizan los cambios pedidos por el mismo
    log_trace(logger_mongo, "Acomodando bitacora");
	t_bitacora* nueva_bitacora = malloc(sizeof(t_bitacora));
	nueva_bitacora->bitacora_asociada = file_tripulante;
	nueva_bitacora->path = path_tripulante;
	nueva_bitacora->tripulante = tcb;

	list_add(bitacoras, nueva_bitacora);

	asignar_nuevo_bloque(nueva_bitacora->path, 0);
    log_trace(logger_mongo, "Acomodada bitacora");
}

void modificar_bitacora(t_estructura* mensaje, char** posicion, int socket) {
	log_error(logger_mongo, "INICIO modificar_bitacora");

	t_bitacora* bitacora = obtener_bitacora(mensaje->tcb->TID);
	char* pos_inicial = NULL;
	char* pos_final = NULL;
	char* nombre_tarea;
	char* cadenita;
	int largo_cadenita;
	t_estructura* mensaje_tarea;

	switch (mensaje->codigo_operacion) {
		case MOVIMIENTO:
			log_debug(logger_mongo, "Me llega un movimiento de %i", mensaje->tcb->TID);
			pos_inicial = malloc(sizeof(char)*3);
			strcpy(pos_inicial, *posicion);
			pos_final = formatear_posicion(mensaje->tcb->coord_x, mensaje->tcb->coord_y);
			cadenita = malloc(strlen("Se mueve de ") + strlen(" a ") + 2*strlen(pos_final) + 1 + 1);
			strcpy(cadenita, "Se mueve de ");
			strcat(cadenita, pos_inicial);
			strcat(cadenita, " a ");
			strcat(cadenita, pos_final);
			strcat(cadenita, ".");

			escribir_bitacora(bitacora, cadenita);
			largo_cadenita = strlen(cadenita);
			free(pos_inicial);
			free(pos_final);
			break;
		case INICIO_TAREA:
			log_debug(logger_mongo, "Inicio de tarea de %i", mensaje->tcb->TID);
			free(mensaje->tcb);
			free(mensaje);
			mensaje_tarea = recepcion_y_deserializacion(socket);
			nombre_tarea = malloc(strlen(mensaje_tarea->tarea->nombre) + 1);
			strcpy(nombre_tarea, mensaje_tarea->tarea->nombre);

			cadenita = malloc(strlen("Comienza ejecucion de tarea ") + strlen(nombre_tarea) + 1 + 1);
			strcpy(cadenita, "Comienza ejecucion de tarea ");
			strcat(cadenita, nombre_tarea);
			strcat(cadenita, ".");

			escribir_bitacora(bitacora, cadenita);
			largo_cadenita = strlen(cadenita) + 1;
			free(nombre_tarea);
			free(mensaje_tarea->tarea->nombre); // Revisar
			free(mensaje_tarea->tarea);
			free(mensaje_tarea);
			break;
		case FIN_TAREA:

			log_debug(logger_mongo, "Fin de tarea de %i", mensaje->tcb->TID);
			free(mensaje->tcb);
			free(mensaje);
			mensaje_tarea = recepcion_y_deserializacion(socket);
			nombre_tarea = malloc(strlen(mensaje_tarea->tarea->nombre) + 1);
			strcpy(nombre_tarea, mensaje_tarea->tarea->nombre);

			cadenita = malloc(strlen("Se finaliza la tarea ") + strlen(nombre_tarea) + 1 + 1);
			strcpy(cadenita, "Se finaliza la tarea ");
			strcat(cadenita, nombre_tarea);
			strcat(cadenita, ".");

			escribir_bitacora(bitacora, cadenita);
			largo_cadenita = strlen(cadenita) + 1;
			free(nombre_tarea);
			free(mensaje_tarea);
			break;
		case CORRE_SABOTAJE:
			log_debug(logger_mongo, "Corre hacia el sabotaje %i", mensaje->tcb->TID);
			escribir_bitacora(bitacora, "Se corre en panico a la ubicacion del sabotaje.");
			cadenita = malloc(strlen("Se corre en panico a la ubicacion del sabotaje." + 1));
			strcpy(cadenita, "Se corre en panico a la ubicacion del sabotaje.");
			largo_cadenita = strlen(cadenita) + 1;
			break;
		case RESUELVE_SABOTAJE:
			log_debug(logger_mongo, "Resuelve sabotaje %i", mensaje->tcb->TID);
			escribir_bitacora(bitacora, "Se resuelve el sabotaje.");
			cadenita = malloc(strlen("Se resuelve el sabotaje." + 1));
			strcpy(cadenita, "Se resuelve el sabotaje.");
			largo_cadenita = strlen(cadenita) + 1;
			break;
	}

	free(cadenita);
	//Actualizo struct bitacora
	t_list* lista_bloques = get_lista_bloques(bitacora->path);
	uint32_t tamanio = tamanio_archivo(bitacora->path);
	bitacora->bloques = lista_bloques;
	bitacora->tamanio = tamanio; //+ largo_cadenita;
	set_tam(bitacora->path, tamanio + largo_cadenita);

	liberar_lista(lista_bloques); // Revisar
}

void escribir_bitacora(t_bitacora* bitacora, char* mensaje) {

	log_trace(logger_mongo, "INICIO escribir_bitacora, path: %s", bitacora->path);

	t_list* lista_bloques = get_lista_bloques(bitacora->path);

	log_trace(logger_mongo, "Es null? %i", lista_bloques == NULL);
	log_trace(logger_mongo, "ta vacia? %i", list_is_empty(lista_bloques));
	log_trace(logger_mongo, "tamanio? %i", list_size(lista_bloques));

	if(list_is_empty(lista_bloques)){
		log_trace(logger_mongo, "La lista de bloques esta vacia, proceso a signar nuevo bloque");
		asignar_nuevo_bloque(bitacora->path, strlen(mensaje));
		lista_bloques = get_lista_bloques(bitacora->path);
	}

	escribir_bloque_bitacora(mensaje, bitacora);


	liberar_lista(lista_bloques);
}

void escribir_bloque_bitacora(char* mensaje, t_bitacora* bitacora) {
	log_trace(logger_mongo, "INICIO escribir_bloque_bitacora");

	int cantidad_alcanzada = 0;
	t_list* lista_bloques = get_lista_bloques(bitacora->path);

	log_debug(logger_mongo, "Pre asignar memoria al aux, escribir_bloque_bitacora");
	int* aux = malloc(sizeof(int));

	log_debug(logger_mongo, "Pre-for escribir_bloque_bitacora");
	for(int i = 0; i < list_size(lista_bloques); i++){

		log_trace(logger_mongo, "Entra for, list size es %i", list_size(lista_bloques));

		aux = list_get(lista_bloques, i);

		log_trace(logger_mongo, "Obtiene lista, aux vale %i", *aux);

		for(int j = 0; j < TAMANIO_BLOQUE; j++){

			if (cantidad_alcanzada == strlen(mensaje)) {
				break;
			}

			if (*(directorio.mapa_blocks + *aux * TAMANIO_BLOQUE + j) == ',') {
				*(directorio.mapa_blocks + *aux * TAMANIO_BLOQUE + j) = mensaje[cantidad_alcanzada];

				// TODO ver si se actualiza
				memcpy(mapa, directorio.mapa_blocks, CANTIDAD_BLOQUES * TAMANIO_BLOQUE);
				msync(mapa, CANTIDAD_BLOQUES * TAMANIO_BLOQUE, MS_SYNC);

				cantidad_alcanzada++;
			}
		}

	}

	if (cantidad_alcanzada != strlen(mensaje)) {
		log_debug(logger_mongo, "Quedo un pedacito de mensaje");
		log_debug(logger_mongo, "Alcance %i bytes de %i bytes, ", cantidad_alcanzada, strlen(mensaje));
		// el size lo podria dejar aca, y no pasar por param
		asignar_nuevo_bloque(bitacora->path, cantidad_alcanzada);
		char* resto_mensaje = malloc(strlen(mensaje + cantidad_alcanzada) + 1);
		log_debug(logger_mongo, "Me falta copiar: %s", mensaje + cantidad_alcanzada);
		log_debug(logger_mongo, "De longitud:", strlen(mensaje + cantidad_alcanzada));
		strcpy(resto_mensaje, (mensaje + cantidad_alcanzada));
		log_debug(logger_mongo, "El resto del mensaje queda: %s", resto_mensaje);

		escribir_bitacora(bitacora, resto_mensaje);
		free(resto_mensaje);
	}

	log_trace(logger_mongo, "Pre free 2");

	free(aux);
}

char* formatear_posicion(int coord_x, int coord_y) { // Puede generar memory leaks
	char* posicion_formateada = malloc(sizeof(char) * 3);

	char* x = string_itoa(coord_x);
	char* y = string_itoa(coord_y);

	strcpy(posicion_formateada, x);
	strcat(posicion_formateada, "|");
	strcat(posicion_formateada, y);

	free(x);
	free(y);

	return posicion_formateada;
}

void borrar_bitacora(t_TCB* tcb) {

	t_bitacora* bitacora = quitar_bitacora_lista(tcb);
	fclose(bitacora->bitacora_asociada);
	free(bitacora->tripulante);
	free(bitacora->path);
	free(bitacora);

}

t_bitacora* quitar_bitacora_lista(t_TCB* tcb) {

	bool contains(void* bitacora) {
		return (tcb->TID == ((t_bitacora*) bitacora)->tripulante->TID);
	}

	t_bitacora* bitacora = list_remove_by_condition(bitacoras, contains);
	return bitacora;
}

t_bitacora* obtener_bitacora(int tid) {
	log_trace(logger_mongo, "INICIO obtener_bitacora");

	bool contains(void* bitacora) {
		return (tid == ((t_bitacora*) bitacora)->tripulante->TID);
	}

	t_bitacora* bitacora = list_find(bitacoras, contains);

	if(bitacora == NULL){
		log_error(logger_mongo, "No se encontro la bitacora");
		return NULL;
	}

	return bitacora;
}

char* fpath_tripulante(t_TCB* tcb) {

	char* tid_tripu = string_itoa((int) (tcb->TID));
	char* path_tripulante = malloc(strlen(path_bitacoras) + strlen("/Tripulante.ims") + strlen(tid_tripu) + 1);
	strcpy(path_tripulante, path_bitacoras);
	path_tripulante = strcat(path_tripulante, "/Tripulante");
	path_tripulante = strcat(path_tripulante, tid_tripu);
	path_tripulante = strcat(path_tripulante, ".ims");
	log_debug(logger_mongo, "Creado path bitacora: %s", path_tripulante);

	free(tid_tripu);
	return path_tripulante;
}
