typedef struct chunk_s{
	char buffer[1024];
	int sequence_index;
	int bytes_read; // this allows to control the last bytes read in order to prevent the server to receive garbage data
} chunk_t;

//typedef struct {
//	const char *ip;
//	int port;
//	const char *filename;
 // thread_args_t;
