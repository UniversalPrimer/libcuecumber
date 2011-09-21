#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint8_t, uint16_t, etc.
#include <string.h> // strlen
#include <netinet/in.h> // hton, ntoh
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "cuecumber.h"

// Variables used for communication between threads
pthread_t pth;
FILE* stream;
FILE* output;

pthread_mutex_t cuepoint_mutex;
void* cuepoint;
uint32_t cuepoint_size;

// Allows dynamic growing of the frame buffer if larger frames are received
int max_frame_size = 100000;
void* frame;

uint32_t process_frame(FILE* in, int out) {
    size_t count;
    uint32_t data_size;
    uchar tag_type;
    uint8_t data_size_bi[3];
    static long frame_num = 0;

    fread(&tag_type, 1, 1, in);
  
    if(feof(in)) { return 0; }

    fread(&data_size_bi, 3, 1, in);
    data_size = (data_size_bi[0] << 16) | (data_size_bi[1] << 8) | data_size_bi[2];

    data_size += FLV_HEADER_OFFSET;

    if(data_size > max_frame_size) {
        free(frame);
        frame = malloc(data_size);
        max_frame_size = data_size;
    }
    
    fread(frame, data_size, 1, in);

    // fd, buf, count
    //    write(&tag_type, 1, out);
    write(out, &tag_type, 1);
    //    write(&data_size_bi, 3, out);
    write(out, &data_size_bi, 3);
    //    write(frame, data_size, out);
    write(out, frame, data_size);

    return data_size;
}

void error(const char *msg)
{
  perror(msg);
  exit(0);
}

int setup_stream(char* servername, int portno) {
  int sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer[256];


  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  server = gethostbyname(servername);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
	server->h_length);

  //serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    error("ERROR connecting");

  return sockfd;
}

void* process_stream() {
  //stream = popen("ffmpeg -i dvgrab_dv1.avi -y -acodec libmp3lame -ar 44100 -vcodec libx264 -vpre medium -f flv - 2>/dev/null", "r");
  //stream = popen("ffmpeg -f video4linux2 -s 320x240 -i /dev/video0 -y -acodec libmp3lame -ar 44100 -vcodec libx264 -vpre medium -f flv - 2>/dev/null", "r");
  //stream = popen("ffmpeg -f video4linux2 -s 320x240 -i /dev/video0 -y -acodec libmp3lame -ar 44100 -vcodec libx264 -vpre ipod320 -f flv - 2>/dev/null", "r");
  //    stream = popen("dvgrab -buffers 5 - | ffmpeg -i - -y  -acodec libmp3lame -ar 44100 -vcodec libx264 -s hd480 -b 200k -f flv - 2> /dev/null", "r");
    stream = popen("dvgrab -buffers 5 - | /opt/ffmpeg/bin/ffmpeg -i - -s 320x240 -y -acodec libmp3lame -ar 44100 -vcodec libx264 -vpre ipod320 -f flv - 2> /dev/null", "r");

    int output = setup_stream("localhost", 6666);

    frame = malloc(max_frame_size);

    void* head = malloc(sizeof(struct flv_header));
    fread(head, sizeof(struct flv_header), 1, stream);
    //    write(head, sizeof(struct flv_header), output);
    write(output, head, sizeof(struct flv_header));
    free(head);
    
     // First PrevTagSize (always 0)
    uint32_t prev;
    fread(&prev, 4, 1, stream);
    write(output, &prev, 4);
    //    write(&prev, 4, output);

    process_frame(stream, output); // Process first frame

    pthread_mutex_unlock(&cuepoint_mutex); // Now we can start injecting cuepoints

    int bytes_read, i = 0;
    for(;;) {
        i++;
        bytes_read = process_frame(stream, output);
        if(!bytes_read) {
            return;
        }
        
        if(feof(stream)) { return; }

        pthread_mutex_lock(&cuepoint_mutex);
        if(cuepoint != 0) {

          //            write(cuepoint, cuepoint_size, output);
            write(output, cuepoint, cuepoint_size);
            prev = htonl(cuepoint_size);
            //            write(&prev, 4, output);
            write(output, &prev, 4);
            free(cuepoint);
            cuepoint=0;
            printf("Injected cuepoint in stream\n");
        }
        pthread_mutex_unlock(&cuepoint_mutex);
    }
}

/* Creates worker thread. */
void cuecumber_init() {
    cuepoint = 0;
    pthread_mutex_init(&cuepoint_mutex, NULL);
    pthread_mutex_lock(&cuepoint_mutex); // Prevents injection of cuepoints in the beginning of file
    pthread_create(&pth,NULL,process_stream,"Init...");
    printf("Starting worker thread\n");
}

void cuecumber_cleanup() {
  //    fclose(output); // Might want to clean up socket here
    pclose(stream);
    free(frame);
    pthread_mutex_destroy(&cuepoint_mutex);
}

/* Kills worker thread and cleans up */
void cuecumber_stop() {
    pthread_cancel(pth);
    printf("Stopped worker thread\n");
}

/* Waits for worker thread to complete. */
void cuecumber_exit() {
    pthread_join(pth, 0);
    cuecumber_cleanup();
    printf("Waited for worker thread \n");
}

/* Called by external process with a cuepoint to insert. */
int insert_cuepoint(uint32_t cuep_size, void* cuep)
{
    pthread_mutex_lock(&cuepoint_mutex);
    cuepoint = malloc(cuep_size);
    memcpy(cuepoint, cuep, cuep_size);
    cuepoint_size = cuep_size;
    printf("insert_cuepoint() done\n");
    pthread_mutex_unlock(&cuepoint_mutex);
}
