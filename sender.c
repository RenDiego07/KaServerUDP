#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"
#include <stddef.h>
#include <sys/time.h>
#define TRIES 7

void send_file(char *server_address, int server_port, char *file);
char *pack_chunk(chunk_t *chunk, int *out_size);
int main (){
	printf("SO FAR EVERYTHING IS FINE\n");

	send_file("192.168.64.5", 8500, "./files_read/text.txt");

	return 0;

}

void send_file ( char *server_address, int server_port, char *file) {


	int sock;  // guarda el identificador del socket. Es necesario para realizar todas las operaciones de comunicacion 
	struct sockaddr_in server_addr, client_addr; // estructura que utilizan el cliente y el servidor
	// configurar para aplicar el protocolo UDP configurar la direccion del host

	// crear el socket con el protocolo UDP
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if ( sock < 0 ){
		perror("COULD NOT CREATE A SOCKET\n");
		exit(1);
	}
	// configuro las direcciones del servidor
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family= AF_INET;
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = inet_addr(server_address);

	int sequence_index = 0;
	size_t bytes_read;
	char buffer[1024];
	FILE *fp = fopen(file, "rb");
	if( !fp ) {
		perror("COULD NOT OPEN THE FILE DESCRIPTOR TO READ THE FILE\n");
		exit(1);
	}


	while ( (bytes_read = fread(buffer, 1, sizeof(buffer), fp)) >=0){

		chunk_t chunk;
		chunk.sequence_index = sequence_index;
		chunk.bytes_read = bytes_read;
		printf("indice %d se copiaron %d\n", chunk.sequence_index, chunk.bytes_read);
		memcpy(chunk.buffer, buffer, bytes_read); // evita copiar bytes basura
		int packet_size;
		char *packet = pack_chunk(&chunk, &packet_size);
		if( bytes_read == 0 ) {
			printf("DEBERIA IMPRIMIRSE. NUMERO DE BYTES LEIDOS: %d\n", bytes_read);
			 if ( sendto(sock,packet,packet_size,0,(struct sockaddr*)&server_addr, sizeof(server_addr))<0 ){
                                perror("COULD NOT SEND THE CHUNKS OF MESSAGE\n");
                                exit(1);
                        }
			break;

		}


		int ack_received = 0;

		int resend_times = 0;
		while (!ack_received ) {

			if ( sendto(sock,packet,packet_size,0,(struct sockaddr*)&server_addr, sizeof(server_addr))<0 ){

				perror("COULD NOT SEND THE CHUNKS OF MESSAGE\n");
				exit(1);
			}
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 500000;
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(sock, &readfds);
			int retval = select(sock + 1, &readfds, NULL, NULL, &tv);
			if ( retval == 0) {
				printf("CHUNK WAS LOST AND THE SERVER DID NOT RESPOND ON TIME\n");
				printf("Timeout, resending sequence:  %d\n", chunk.sequence_index);
				resend_times++;
				printf("times:  %d\n", resend_times);
				if(resend_times > TRIES){
					printf("EXCEED THE AMOUNT OF TRIES TO RESEND THE CHUNK OF DATA\n");
					break;
				}
				continue;

			}else if(retval > 0) {
				char received_buffer[8];
				int fd_server_response = recvfrom(sock, received_buffer, sizeof(received_buffer), 0, NULL, NULL);
				if (fd_server_response < 0){
					perror("ERROR AT CREATING FILE DESCRIPTOR\n");
					continue;
				}
				int server_response_sequence;
				memcpy(&server_response_sequence, received_buffer,4);
				server_response_sequence = ntohl(server_response_sequence);
				if(server_response_sequence == chunk.sequence_index){
					ack_received = 1;
					printf("CHUNK WAS RECEIVED SUCCESFULLY. SEQUENCE RECEIVED: %d\n",server_response_sequence );
				}else if(chunk.sequence_index > server_response_sequence){
					continue;
				}else{
					printf("INDEX_SEQUENCE RECEIVED: %d, EXPECTED %d\n", server_response_sequence, chunk.sequence_index);
					printf("SENDING CHUNK AGAIN\n");
				}

			}
		}
		free(packet);
		sequence_index++;
		printf("BYTES READ : %d\n",bytes_read );
		if ( bytes_read == 0 ) {
			break;
		}

	}
	printf("MESSAGE SENT\n");
	close(sock);
}



char *pack_chunk(chunk_t *chunk, int *out_size){
	*out_size = 4 + 4 + chunk->bytes_read;
	char *packet = malloc(*out_size);
	if(!packet){
		perror("COULD NOT ASSIGN MEMORY TO THE PACKET");
		exit(1);
	}
	int net_seq = htonl(chunk->sequence_index);
        int net_size = htonl(chunk->bytes_read);
    	memcpy(packet, &net_seq, 4);         // copia secuencia
    	memcpy(packet + 4, &net_size, 4);    // copia tamaño de datos
    	memcpy(packet + 8, chunk->buffer, chunk->bytes_read); // copia datos

	return packet;
}

