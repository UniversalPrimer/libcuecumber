#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint8_t, uint16_t, etc.
#include <string.h> // strlen
#include <netinet/in.h> // hton, ntoh
#include <pthread.h>
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

uint32_t process_frame(FILE* in, FILE* out) {
    size_t count;
    uint32_t data_size;
    uchar tag_type;
    uint8_t data_size_bi[3];

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

    fwrite(&tag_type, 1, 1, out);
    fwrite(&data_size_bi, 3, 1, out);
    fwrite(frame, data_size, 1, out);

    return data_size;
}

void* process_stream() {
    //stream = popen("ffmpeg -i dvgrab_dv1.avi -y -acodec libmp3lame -ar 44100 -vcodec libx264 -vpre medium -f flv - 2>/dev/null", "r");
    //stream = popen("ffmpeg -f video4linux2 -s 320x240 -i /dev/video0 -y -acodec libmp3lame -ar 44100 -vcodec libx264 -vpre medium -f flv - 2>/dev/null", "r");
    stream = popen("cat no_cuepoints.flv", "r");
    output = fopen("output.flv", "w");

    frame = malloc(max_frame_size);

    void* head = malloc(sizeof(struct flv_header));
    fread(head, sizeof(struct flv_header), 1, stream);
    fwrite(head, sizeof(struct flv_header), 1, output);
    free(head);
    
    uint32_t prev;
     // First PrevTagSize (always 0)
    fread(&prev, 4, 1, stream);
    fwrite(&prev, 4, 1, output);

    int bytes_read, i = 0;
    for(;;) {
        i++;
        bytes_read = process_frame(stream, output);
        if(!bytes_read) {
            return;
        }
        
        if(feof(stream)) { return; } // TODO Doesn't work

        pthread_mutex_lock(&cuepoint_mutex);
        if(cuepoint != 0) {
            fwrite(cuepoint, cuepoint_size, 1, output);
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
	pthread_create(&pth,NULL,process_stream,"Init...");
    printf("Starting worker thread\n");
}

void cuecumber_cleanup() {
    fclose(output);
    pclose(stream);
    free(frame);
    pthread_mutex_destroy(&cuepoint_mutex);
}

/* Kills worker thread and cleans up */
void cuecumber_stop() {
    pthread_cancel(pth);
    cuecumber_cleanup();
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
