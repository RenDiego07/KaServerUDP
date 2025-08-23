#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>

#define MAX_SESSIONS 8
#define CHUNK_MAX (1024)
#define PACKET_OFFSET (8)
#define PACKET_MAX (CHUNK_MAX + PACKET_OFFSET)


typedef struct {
	struct sockaddr_in peer;
	FILE *fp;
	int *chunks_received;
	size_t len_chunks_array;
	size_t count;
	bool finished;
	bool in_use;
	pthread_mutex_t  mx;
} session_t;



session_t sessions[MAX_SESSIONS];
pthread_mutex_t sessions_mx = PTHREAD_MUTEX_INITIALIZER;

void start_server( int port );
int *extend_memory ( int **array, size_t *length );
bool isChunkAlreadySaved ( int *array, size_t length, int sequence);
int same_peer(const struct sockaddr_in *a, const struct sockaddr_in *b); // socket a y socket b 
void ipport_to_path(const struct sockaddr_in *cli, char *out, size_t n);
session_t* get_or_create_session(const struct sockaddr_in *cli);





int main(){

	printf("Everything's ok\n");
	int port = 8500;
	start_server(port);
	return 0;

}


void start_server( int port ){

	int sock;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if ( sock < 0 ) {
		perror("COULD NOT OPEN A FILE DESCRIPTOR FOR THE SOCKET\n");
		exit(1);
	}

	memset(&server_addr , 0 , sizeof(server_addr) );
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons( port );
	server_addr.sin_addr.s_addr = INADDR_ANY;
	printf("DID IT GO THROUGH\n");

	if( bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		perror("SERVER CANNOT START LISTENING TO OTHER CONNECTIONS\n");
		exit(1);
	}
	printf("WAITING FOR ANY CONNECTION.....\n");
//	FILE *fp = fopen("./files_received/server.txt", "ab");
//	if ( !fp ) {
//		perror("COULD NOT OPEN FILE DESCRIPTOR TO WRITE ON THE SERVER TXT\n");
//		exit(1);
//	}										//solo para un emisor "cliente"
//	size_t index_chunk = 0;
//	size_t len_chunks_array = 10;
//	int *chunks_received = malloc( len_chunks_array* sizeof(int));
//	for ( size_t tmp_ind = 0; tmp_ind < len_chunks_array; tmp_ind++){
//		chunks_received[tmp_ind] = -1;
//	}
	while ( 1 ) {
		char packet[PACKET_MAX];
		
		int bytes_received = recvfrom ( sock ,packet , sizeof(packet), 0 , (struct sockaddr*)&client_addr, &client_len);
		if( bytes_received < 0 ){
			perror("CONNECTION FAILED\n");
			close(sock);
			exit(1);
		}
		// verificamos si los bytes read son mayores que 8 bytes
		
		int seq, size;
		memcpy(&seq, packet, 4);
		memcpy(&size, packet+4, 4);
		seq = ntohl(seq);
		size = ntohl(size);
		printf("NUMERO DE BYTES LEIDOS %d\n",size );
		session_t *s = get_or_create_session(&client_addr);
		
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &s->peer.sin_addr, ip, sizeof(ip));
		printf("[CLIENT %s:%hu] BYTES=%d SEQ=%d\n",ip, ntohs(s->peer.sin_port), size, seq);

		
		if( size == 0 ){
			printf("LLEGAMOS AL FINAL DEL ARCHIVO SECUENCIA: %d\n",seq);
			pthread_mutex_lock(&s->mx);
			s->finished = true;
			if ( s->fp ) {
				fclose(s->fp);
				s->fp = NULL;
			}
			pthread_mutex_unlock(&s->mx);
			int response_seq = htonl(seq);
			if (sendto(sock, &response_seq, sizeof(response_seq), 0,(struct sockaddr*)&client_addr, client_len) < 0){
				perror("ACK FIN sendto");
			}
			
			continue; // NO BREAK. TAL VEZ PONER UN TIMEOUT PARA IDENTIFICAR PARA CERRAR EL SERVIDOR
		}
		// {comprobamos primero si es que el array de chunks esta lleno}
		if (size < 0 || size > CHUNK_MAX || bytes_received < (PACKET_OFFSET + size)){
			fprintf(stderr, "Invalid size in packet: size=%d, recv=%d\n", size, bytes_received);
			continue;
		}
		bool duplicated = false;
		pthread_mutex_lock(&s->mx);
		duplicated = isChunkAlreadySaved(s->chunks_received, s->count, seq);
		pthread_mutex_unlock(&s->mx);
		
		// {comprobamos si es que el bloque no se haya repetido}

		if( duplicated ){
			 printf("[DUPLICATED] seq=%d — SKIPPED\n", seq);
			continue;

		}
		pthread_mutex_lock(&s->mx);
		if(size > 0 && s->fp){
			size_t bytes_written = fwrite(packet + PACKET_OFFSET, 1,size, s->fp );
			size_t tmp_bytes_written = (size_t)size;
			if ( bytes_written != tmp_bytes_written ){
				perror("DID NOT WRITE THE AMOUNT OF EXPECTED BYTES\n");
			}else{
				printf("BYTES WRITTEN IN THE CORRECT ORDER\n");
				printf("SIZE : %d, SEQUENCE: %d\n", size, seq);
			}
		}
		if (s->count >= s->len_chunks_array ){
			if(!extend_memory(&s->chunks_received, &s->len_chunks_array)){
				pthread_mutex_unlock(&s->mx);
				perror("COULD NOT ALLOCATE MORE MEMORY\n");
			}
	
		}
		int seq_1 = htonl(seq);
		if( sendto(sock,&seq_1, sizeof(seq_1), 0, (struct sockaddr*)&client_addr, client_len)<0){
			perror("COULD NOT SEND TO CLIEN THE INDEX SEQUENCE\n");
		}else{
			printf("THE SEQUENCE SENT BACK TO THE CLIENT : %d\n", seq);
		}
		//if ( seq >= len_chunks_array ){
		//	int *tmp = extend_memory(&chunks_received, &len_chunks_array);
		//	if( !tmp ){							funciona solo con un cliente.
		//		perror("COULD NOT ALLOCATE MORE MEMORY\n");
		//		close(sock);
		//		exit(1);
		//	}
	//	}
		// {comprobamos si es que el bloque no se haya repetido}
		//if( !isChunkAlreadySaved ( chunks_received,len_chunks_array, seq )){

		//	fwrite(packet + 8,1 ,size, fp);
		//	printf("DATA RECEIVED: secuence #: %d with %d bytes\n", seq, size);
//			printf("%s\n", packet+8);

		//	int seq_1 = htonl(seq);

		//	if( (sendto(sock,&seq_1, sizeof(seq_1),0, (struct sockaddr*)&client_addr, client_len))<0){
		//		perror ("COULD NOT SEND TO CLIENT THE INDEX SEQUENCE \n");

		//	}else{
		//		printf("INDEX SEQUENCE %d  WAS SENT \n", seq);
		//		chunks_received[index_chunk] = seq;
		//	}

		//	if ( size == 0 ) {
		//		break;
		//	}
		//}else{
		//	printf("DUPLICATED CHUNK: %d. SKIPPED\n", seq);
		//	continue;
		//}
		//index_chunk++;

	}
//	fclose(fp);
}


int *extend_memory(int **array, size_t *length){
	size_t new_len = *length * 2;
	int *tmp = realloc(*array,  new_len * sizeof(int));
	if ( !tmp ) {
		return NULL;
	}
	for ( size_t i = *length ; i< new_len; i++){
		tmp[i] = 0;
	}
	*array = tmp;
	*length = new_len;
	return tmp;
}

bool isChunkAlreadySaved ( int *array, size_t length, int sequence){
	for( size_t i = 0; i<length; i++){
		if( array[i] == sequence ){
			return true;
		}
	}
	return false;

}

int same_peer(const struct sockaddr_in *a, const struct sockaddr_in *b) {
        return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

void ipport_to_path(const struct sockaddr_in *cli, char *out, size_t n){
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli->sin_addr, ip, sizeof(ip));
        unsigned short port = ntohs(cli->sin_port);
        snprintf(out, n, "./files_received/%s_%hu.bin", ip, port);

}


session_t *get_or_create_session(const struct sockaddr_in *client){
	session_t *ret = NULL;
	pthread_mutex_lock(&sessions_mx);
	for (int i = 0; i < MAX_SESSIONS; ++i){
		if (sessions[i].in_use && same_peer(&sessions[i].peer, client)){
			ret = &sessions[i];
			pthread_mutex_unlock(&sessions_mx);
			 return ret;
		}
	}

	for (int i = 0; i < MAX_SESSIONS; ++i){
		if (!sessions[i].in_use){
			sessions[i].in_use   = true;
			sessions[i].peer     = *client;
			sessions[i].finished = false;
			sessions[i].count    = 0;
			sessions[i].len_chunks_array = 1024;
			pthread_mutex_init(&sessions[i].mx, NULL);

			sessions[i].chunks_received = (int*)malloc(sessions[i].len_chunks_array * sizeof(int));
			if (!sessions[i].chunks_received){
				sessions[i].in_use = false;
				pthread_mutex_unlock(&sessions_mx);
				return NULL;
			}
			char path[256];
			ipport_to_path(client, path, sizeof(path));
			sessions[i].fp = fopen(path, "ab");
			if (!sessions[i].fp){
				free(sessions[i].chunks_received);
				sessions[i].in_use = false;
				pthread_mutex_unlock(&sessions_mx);
				return NULL;

			}
			ret = &sessions[i];
		}	break;
	}
	pthread_mutex_unlock(&sessions_mx);
	return ret;

}


