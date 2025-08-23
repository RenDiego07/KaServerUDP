#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"
#include <stdbool.h>

void start_server( int port );
int *extend_memory ( int **array, size_t *length );
bool isChunkAlreadySaved ( int *array, size_t length, int sequence);
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
	FILE *fp = fopen("./files_received/server.txt", "ab");
	if ( !fp ) {
		perror("COULD NOT OPEN FILE DESCRIPTOR TO WRITE ON THE SERVER TXT\n");
		exit(1);
	}
	size_t index_chunk = 0;
	size_t len_chunks_array = 10;
	int *chunks_received = malloc( len_chunks_array* sizeof(int));
	for ( size_t tmp_ind = 0; tmp_ind < len_chunks_array; tmp_ind++){
		chunks_received[tmp_ind] = -1;
	}
	while ( 1 ) {
		char packet[1024 + 8];
		int bytes_received = recvfrom ( sock ,packet , sizeof(packet), 0 , (struct sockaddr*)&client_addr, &client_len);
		if( bytes_received < 0 ){
			perror("CONNECTION FAILED\n");
			close(sock);
			exit(1);
		}
		int seq, size;
		memcpy(&seq, packet, 4);
		memcpy(&size, packet+4, 4);
		seq = ntohl(seq);
		size = ntohl(size);
		// {comprobamos primero si es que el array de chunks esta lleno}
		if ( seq >= len_chunks_array ){
			int *tmp = extend_memory(&chunks_received, &len_chunks_array);
			if( !tmp ){
				perror("COULD NOT ALLOCATE MORE MEMORY\n");
				close(sock);
				exit(1);
			}
		}
		// {comprobamos si es que el bloque no se haya repetido}
		if( !isChunkAlreadySaved ( chunks_received,len_chunks_array, seq )){

			fwrite(packet + 8,1 ,size, fp);
			printf("%s\n", packet+8);
			printf("DATA RECEIVED: secuence #: %d with %d bytes\n", seq, size);
			int seq_1 = htonl(seq);

			if( (sendto(sock,&seq_1, sizeof(seq_1),0, (struct sockaddr*)&client_addr, client_len))<0){
				perror ("COULD NOT SEND TO CLIENT THE INDEX SEQUENCE \n");

			}else{
				printf("INDEX SEQUENCE %d  WAS SENT \n", seq);
				chunks_received[index_chunk] = seq;
			}

			if ( size == 0 ) {
				break;
			}
		}else{
			printf("DUPLICATED CHUNK: %d. SKIPPED\n", seq);
		}
		index_chunk++;

	}
	fclose(fp);
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
