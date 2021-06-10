/*
 ============================================================================
 Name        : Discordiador.c
 Author      : Rey de fuego
 Version     : 1
 Copyright   : Mala yuyu
 Description : El discordiador
 ============================================================================
 */
//TODO: Implementar HANDLER de ESCUCHAR
//TODO: en crear_tcb, estamos asignando a las posiciones el ASCII, no el numero (lo cual esta bien?)
#define	IP_MI_RAM_HQ config_get_string_value(config, "IP_MI_RAM_HQ")
#define PUERTO_MI_RAM_HQ config_get_string_value(config, "PUERTO_MI_RAM_HQ")
#define	IP_I_MONGO_STORE config_get_string_value(config, "IP_I_MONGO_STORE")
#define PUERTO_I_MONGO_STORE config_get_string_value(config, "PUERTO_I_MONGO_STORE")
#define	ALGORITMO config_get_string_value(config, "ALGORITMO")
#define	GRADO_MULTITAREA config_get_string_value(config, "GRADO_MULTITAREA")

#include "discordiador.h"

// Vars globales
t_config* config;
t_log* logger;
int socket_a_mi_ram_hq;
int socket_a_mongo_store;
char estado_tripulante[4] = {'N', 'R', 'E', 'B'};
int planificacion_activa = 0;

t_list* lista_pids;
t_list* lista_patotas;
t_list* lista_tripulantes_new;
t_list* lista_tripulantes_exec;

t_queue *cola_tripulantes_new;
t_queue *cola_tripulantes_ready;

int sistema_activo = 1;

int main() {
	logger = log_create("discordiador.log", "discordiador", true, LOG_LEVEL_INFO);
	config = config_create("discordiador.config");

	iniciar_listas();
	iniciar_colas();

	socket_a_mi_ram_hq = crear_socket_cliente(IP_MI_RAM_HQ, PUERTO_MI_RAM_HQ);
	//socket_a_mi_ram_hq = crear_socket_cliente("127.0.0.1", "25430");
	socket_a_mongo_store = crear_socket_cliente(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
	//socket_a_mongo_store = crear_socket_cliente("127.0.0.1", "4000");

	if (socket_a_mi_ram_hq != -1 && socket_a_mongo_store != -1) {
		pthread_t hiloConsola;
		pthread_create(&hiloConsola, NULL, (void*)leer_consola, NULL);
		pthread_detach(hiloConsola);

		//pthread_join(hiloConsola, NULL);
		/*
		args_escuchar args_disc;
		args_disc.socket_oyente = socket_a_mi_ram_hq;
		pthread_t hilo_escucha_ram;
		pthread_create(&hilo_escucha_ram, NULL, (void*) escuchar_discordiador, &args_disc);
		pthread_join(hilo_escucha, NULL);
		*/

	}

	while(sistema_activo){
		usleep(1);
	}

	close(socket_a_mi_ram_hq);
	close(socket_a_mongo_store);
	
	config_destroy(config);
	log_destroy(logger);

	return EXIT_SUCCESS;
}

void leer_consola() {
	char* leido;
	int comando;

	do {

		leido = readline(">>>");

		if (strlen(leido) > 0) {
			comando = reconocer_comando(leido);

			switch (comando) {
				case INICIAR_PATOTA:
					iniciar_patota(leido);
					break;

				case INICIAR_PLANIFICACION:
					iniciar_planificacion();
					break;

				case LISTAR_TRIPULANTES:
					listar_tripulantes();
					break;

				case PAUSAR_PLANIFICACION:
					pausar_planificacion();
					break;

				case OBTENER_BITACORA:
					obtener_bitacora(leido);
					break;
				
				case EXPULSAR_TRIPULANTE:
					expulsar_tripulante(leido);
					break;
				
				case HELP:
					help_comandos();
					break;
				
				case APAGAR_SISTEMA:
					sistema_activo = 0;
					exit(1); //sin esto se BUGGEA DURO
					break;

				case NO_CONOCIDO:
					break;
			}
		}

		free(leido); 

	} while (comando != EXIT);
}

void iniciar_patota(char* leido) {
	char** palabras = string_split(leido, " ");
	int cantidadTripulantes = atoi(palabras[1]);
	char* path = palabras[2];
	
	printf("PATOTA: cantidad de tripulantes %d, url: %s \n", cantidadTripulantes, path);

	int i = 0;
	t_PCB* pcb = crear_pcb(path);

	t_TCB* aux;
	while (palabras[i+3] != NULL){
		printf("POSICION %d: %s \n", i+1, palabras[i+3]);
		//void* funcion = pedir_funcion()
		aux = iniciar_tcb(NULL, pcb, i+1, palabras[i+3]); //Le manda a RAM el tripulante

		queue_push(cola_tripulantes_new, (void*) aux);
		i++;

	}
	for(int j = i+1; j <= cantidadTripulantes; j++){
		printf("POSICION %d: 0|0 \n", j);
		//void* funcion = pedir_funcion()
		aux = iniciar_tcb(NULL, pcb, i+1, "0|0"); //Le manda a RAM el tripulante

		queue_push(cola_tripulantes_new, (void*) aux);
	}
	free(aux);
}

/*
t_patota* crear_patota(t_PCB* un_pcb){
	t_patota* patota = malloc(sizeof(t_patota));
	patota -> pcb = un_pcb;
	//patota -> archivo_de_tareas
	return patota;
}*/

t_PCB* crear_pcb(char* path){
	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb -> PID = nuevo_pid();
	pcb -> direccion_tareas = (uint32_t) path;
	return pcb;
}

int nuevo_pid(){
	int id_patota = 1;
	while(1){
		if(!esta_en_lista(lista_pids, id_patota)){
			list_add(lista_pids, (void*) id_patota);
			return id_patota;
		}
		id_patota++;
	}
}

void iniciar_hilo_tripulante(void* funcion){
	pthread_t hilo1;
	pthread_create(&hilo1, NULL, funcion, NULL);
}

t_TCB* crear_tcb(t_PCB* pcb, int tid, char* posicion){
	t_TCB* tcb = malloc(sizeof(t_TCB));
	tcb -> TID = tid;
	tcb -> estado_tripulante = estado_tripulante[NEW];
	tcb -> coord_x = posicion[0];
	tcb -> coord_y = posicion[2];
	//tcb -> siguiente_instruccion; //TODO
	tcb -> puntero_a_pcb = (uint32_t) pcb;

	return tcb;
}
/*
t_tripulante* crear_tripulante(t_TCB* un_tcb){
	t_tripulante* tripulante = malloc(sizeof(t_tripulante));
	tripulante -> tcb = un_tcb;
	return tripulante;
}
*/
t_TCB* iniciar_tcb(void* funcion, t_PCB* pcb, int tid, char* posicion){
	t_TCB* un_tcb = crear_tcb(pcb, tid, posicion);
	//t_tripulante* nuestro_tripulante = crear_tripulante(un_tcb);
	iniciar_hilo_tripulante(funcion);
	return un_tcb;
}

void iniciar_planificacion() {
	printf("iniciarPlanificacion");
	planificacion_activa = 1;

	// TODO NO TESTEADO
	while(planificacion_activa){
		if(comparar_strings(ALGORITMO, "FIFO")){
			while(list_size(lista_tripulantes_exec) < atoi(GRADO_MULTITAREA)){
				list_add(lista_tripulantes_exec, queue_pop(cola_tripulantes_ready));
			}
		}
	}
}

void listar_tripulantes() {	//TODO falta testear

	printf("listar Tripulantes\n");

    char* fechaHora = fecha_y_hora();

    printf("Estado de la nave: %s\n", fechaHora);

	//tambien existe list_iterate(lista_patotas, ...);, pero no se como se aplicaria
	t_PCB* aux_p;
	t_TCB* aux_t;

	int i,j;

	for(i = 0; i < list_size(lista_patotas); i++){
		aux_p = list_get(lista_patotas, i);

		for(j = 0; j < list_size(lista_tripulantes_patota(aux_p)); j++){
			aux_t = list_get(lista_tripulantes_patota(aux_p), j);
			printf("Tripulante: %d\tPatota: %d\tStatus: %i", aux_t->TID, ((t_PCB*) (aux_t->puntero_a_pcb))->PID, aux_t->estado_tripulante);

		}

	}
	//liberar pcb aux
	//liberar tcb aux
}

t_list* lista_tripulantes_patota(t_PCB* pcb){


	// Necesito listar todos los tripulantes que estan en RAM, y los que estan en NEW
	// Primero; le mando a ram el PID o el PCB (posiblemente este).
	// Segundo, RAM ejecuta su funcion y me devuelve los tripulantes con ese PCB asociado
	// Tercero, los guardo en una lista
	// Cuarto, si lo tenemos diseñado que los tripulantes de NEW no esten en RAM,
	// que es lo teoricamente correcto, entonces verifico lo mismo con la lista o cola de NEW
	// de discordiador y uno las dos listas
	// Quinto: devuelvo la lista

	// t_list* aux_lista_tripulantes = list_create();
	// pedido_ram_tripulantes (t_PCB* pcb);
	// aux_lista_tripulantes = recibir_tripulantes_de_ram();
	// sacar lista_tripulantes_new_de_esa_patota, de Discordiador con las funciones de list.h;
	// juntar(aux_lista_tripulantes, lista_tripulantes_new_de_esa_patota);
	// return aux_lista_tripulantes;
	// list_destroy(aux_lista_tripulantes); debo destrozar esto LUEGO de usarla en la funcion anterior

	return NULL;
}

/*
void lista_tripulantes_patota_version_RAM(t_PCB* pcb){

	// verifico que el pcb/patota esta en la lista de pcbs/patotas
	// en una lista de tripulantes, me quedo con aquellos que cumplan que su puntero_a_pcb es igual al pcb que tenemos.
	// envio esa lista a Discordiador, o bien enviarlos uno por uno que puede ser mas facil

}*/

void pausar_planificacion() {
	printf("Pausar Planificacion\n");
	planificacion_activa = 0;
}

void obtener_bitacora(char* leido) {
	printf("obtenerBitacora");
}

void expulsar_tripulante(char* leido) {
	//TODO plantearselo a los agentes doble j
	printf("expulsarTripulante");
}

/*int pedir_tarea(int id_tripulante){
	t_paquete* paquete = crear_paquete(PEDIR_TAREA); // BORRAR ESTA COSA BONITA
	agregar_a_paquete(paquete, (void*) id_tripulante, sizeof(int)); // BORRAR ESTA COSA BONITA
	enviar_paquete(paquete,socket_a_mi_ram_hq); // BORRAR ESTA COSA BONITA
	return 1;
}

void realizar_tarea(t_tarea tarea){ // TODO 

}*/

char* fecha_y_hora() { // Creo que las commons ya tienen una funcion que hace esto
	time_t tiempo = time(NULL);
	struct tm tiempoLocal = *localtime(&tiempo); // Tiempo actual
	static char fecha_Hora[70]; // El lugar en donde se pondrÃ¡ la fecha y hora formateadas
	char *formato = "%d-%m-%Y %H:%M:%S";  // El formato. Mira mÃ¡s en https://en.cppreference.com/w/c/chrono/strftime
	int bytesEscritos = strftime(fecha_Hora, sizeof fecha_Hora, formato, &tiempoLocal);  // Intentar formatear

	if (bytesEscritos != 0) { // Si no hay error, los bytesEscritos no son 0
		return fecha_Hora;
  	} 
	else {
    	return "Error formateando fecha";
  	}
}

int esta_en_lista(t_list* lista, int elemento){
	bool contiene(void* elemento1){
	return sonIguales(elemento, (int) elemento1);
	}
	int a = list_any_satisfy(lista, contiene);
	return a;
}

int sonIguales(int elemento1, int elemento2){
		return elemento1 == elemento2;
	}


/*//hablar con los agentes doble j
// codear escuchar discordiador YA
// guardarlo en el logger
// SACAS UNO DE LA COLA DE NEW Y LO PASARLO A READY
// CAMBIAR EL ESTADO DEL TCB
*/

void enlistar_tripulante(){ //TODO no testeado, SEBA
	// Utilizo NEW como si fuera una cola, pero no necesariamente lo es
	// no es NECESARIO que el que se pase a ready sea el primero de la cola,
	// pero no deberia ser problema implementarlo asi siempre y cuando
	// se le asigne la tarea al tripulante que sacamos de la cola de new
	t_TCB* tripulante_a_ready = queue_pop(cola_tripulantes_new);
	queue_push(cola_tripulantes_ready, tripulante_a_ready);
	tripulante_a_ready->estado_tripulante = estado_tripulante[READY];
}

void escuchar_discordiador(void* args) { // TODO No se libera args, ver donde liberar
	args_escuchar* p = malloc(sizeof(args_escuchar));
	p = args;
	int socket_escucha = p->socket_oyente;

	struct sockaddr_storage direccion_a_escuchar;
	socklen_t tamanio_direccion;
	int socket_especifico;

	if (listen(socket_escucha, 10) == -1) // Se pone el socket a esperar llamados, con una cola maxima dada por el 2do parametro, se eligio 10 arbitrariamente //TODO esto esta hardcodeado
		printf("Error al configurar recepcion de mensajes\n");

	struct sigaction sa;
		sa.sa_handler = sigchld_handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			printf("Error al limpiar procesos\n");
			exit(1);
		}

	while (1) {
			tamanio_direccion = sizeof(direccion_a_escuchar);
			socket_especifico = accept(socket_escucha, (struct sockaddr*) &direccion_a_escuchar, &tamanio_direccion); // Se acepta (por FIFO si no me equivoco) el llamado entrante a socket escucha

			if (!fork()) { // Se crea un proceso hijo si se pudo forkear correctamente
				close(socket_escucha); // Cierro escucha en este hilo, total no sirve mas
				//atender_clientes(socket_especifico); // Funcion enviada por parametro con puntero para que ejecuten los hijos del proceso
				printf("Recibido, gracias cliente");
				close(socket_especifico); // Cumple proposito, se cierra socket hijo
				exit(0);
			}

			close(socket_especifico); // En hilo padre se cierra el socket hijo, total al arrancar el while se vuelve a settear, evita "port leaks" supongo
		}
}


void iniciar_listas() {

	lista_tripulantes_new = list_create();
	lista_tripulantes_exec = list_create();
	lista_pids = list_create();
	lista_patotas = list_create();

}
void iniciar_colas() {

	cola_tripulantes_ready = queue_create();
	cola_tripulantes_new= queue_create();

}
