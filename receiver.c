#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"

void start_server( int port );

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
	int total = 0;
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
		fwrite(packet + 8,1 ,size, fp);
		total += size;
		printf("TOTAL BYTES WRITTEN %d \n", total);
		printf("%s\n", packet+8);
		printf("DATA RECEIVED: secuence #: %d with %d bytes\n", seq, size);
		if ( size == 0 ) {
			break;
		}
//		if(total >=10240){
//			break;
//		}
	}
	fclose(fp);
}

