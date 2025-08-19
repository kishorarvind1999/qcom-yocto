#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   
#include <time.h>
#include <opus/opus.h>
#include <lc3.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SAMPLE_RATE 48000
#define CHANNELS 1
#define FRAME_SIZE 480  // 10ms at 48kHz
#define BITRATE 32000 
#define MAX_PACKET_SIZE 1000
#define APPLICATION OPUS_APPLICATION_AUDIO

struct timespec start, end;
double elapsed_ms_e, elapsed_ms_d, avg_ms_e, avg_ms_d;

int test_opus_codec() {
    printf("Testing Opus codec...\n");
    
    OpusEncoder *encoder;
    OpusDecoder *decoder;
    int err;
    
    // Create encoder
    encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, APPLICATION, &err);
    if (err < 0) {
        printf("Failed to create Opus encoder: %s\n", opus_strerror(err));
        return -1;
    }
    
    // Create decoder
    decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
    if (err < 0) {
        printf("Failed to create Opus decoder: %s\n", opus_strerror(err));
        opus_encoder_destroy(encoder);
        return -1;
    }
    
    // Test data (sine wave)
    opus_int16 input[FRAME_SIZE * CHANNELS];
    opus_int16 output[FRAME_SIZE * CHANNELS];
    unsigned char encoded[MAX_PACKET_SIZE];
    
    // Generate test sine wave
    for (int i = 0; i < FRAME_SIZE * CHANNELS; i++) {
        input[i] = (opus_int16)(32767.0 * sin(2.0 * 3.14159 * 440.0 * i / SAMPLE_RATE));
    }
    
    double total_encode_us = 0.0, total_decode_us = 0.0;
    int encoded_size = 0, decoded_size = 0;

    for (int i = 0; i < 100; i++) {
        // Encode
        clock_gettime(CLOCK_MONOTONIC, &start);
        encoded_size = opus_encode(encoder, input, FRAME_SIZE, encoded, MAX_PACKET_SIZE);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
        total_encode_us += elapsed_us;

        if (encoded_size < 0) {
            printf("Opus encoding failed: %s\n", opus_strerror(encoded_size));
            opus_encoder_destroy(encoder);
            opus_decoder_destroy(decoder);
            return -1;
        }

        // Decode
        clock_gettime(CLOCK_MONOTONIC, &start);
        decoded_size = opus_decode(decoder, encoded, encoded_size, output, FRAME_SIZE, 0);
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
        total_decode_us += elapsed_us;

        if (decoded_size < 0) {
            printf("Opus decoding failed: %s\n", opus_strerror(decoded_size));
            opus_encoder_destroy(encoder);
            opus_decoder_destroy(decoder);
            return -1;
        }
    }

    avg_ms_e = total_encode_us / 100.0;
    avg_ms_d = total_decode_us / 100.0;

    printf("Opus test successful! \nAvg Encoding time %.3f µs \nAvg Decoding time %.3f µs\n",
           avg_ms_e, avg_ms_d);
    
    opus_encoder_destroy(encoder);
    opus_decoder_destroy(decoder);
    return 0;
}


int test_lc3_codec() {
    printf("Testing LC3 codec...\n");

    const int frame_us = 10000;   // 10 ms

    unsigned enc_sz = lc3_encoder_size(frame_us, SAMPLE_RATE);
    unsigned dec_sz = lc3_decoder_size(frame_us, SAMPLE_RATE);
    void *enc_mem = malloc(enc_sz);
    void *dec_mem = malloc(dec_sz);
    if (!enc_mem || !dec_mem) { printf("LC3 mem alloc failed\n"); return -1; }

    lc3_encoder_t enc = lc3_setup_encoder(frame_us, SAMPLE_RATE, 0, enc_mem);
    lc3_decoder_t dec = lc3_setup_decoder(frame_us, SAMPLE_RATE, 0, dec_mem);
    if (!enc || !dec) { printf("LC3 setup failed\n"); return -1; }

    int frame_samples = lc3_frame_samples(frame_us, SAMPLE_RATE);
    int nbytes = lc3_frame_bytes(frame_us, BITRATE); 

    int16_t *in  = malloc(frame_samples * sizeof(int16_t));
    int16_t *out = malloc(frame_samples * sizeof(int16_t));
    uint8_t *bit = malloc(nbytes);
    if (!in || !out || !bit) { printf("LC3 buffer alloc failed\n"); return -1; }

    // Sine
    for (int i = 0; i < frame_samples; i++)
        in[i] = (int16_t)(32767.0 * sin(2.0 * 3.14159 * 440.0 * i / SAMPLE_RATE));

    double total_encode_us = 0.0, total_decode_us = 0.0;
    int rc = 0;

    for (int i = 0; i < 100; i++) {
        // Encode
        clock_gettime(CLOCK_MONOTONIC, &start);
        rc = lc3_encode(enc, LC3_PCM_FORMAT_S16, in, 1, nbytes, bit);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 +
                            (end.tv_nsec - start.tv_nsec) / 1e3;
        total_encode_us += elapsed_us;

        if (rc != 0) { 
            printf("LC3 encode failed (rc=%d)\n", rc); 
            return -1; 
        }

        // Decode
        clock_gettime(CLOCK_MONOTONIC, &start);
        rc = lc3_decode(dec, bit, nbytes, LC3_PCM_FORMAT_S16, out, 1);
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 +
                     (end.tv_nsec - start.tv_nsec) / 1e3;
        total_decode_us += elapsed_us;

        if (rc < 0) { 
            printf("LC3 decode failed (rc=%d)\n", rc); 
            return -1; 
        }
        if (rc == 1) printf("Decoder performed PLC\n");
    }

    avg_ms_e = total_encode_us / 100.0;
    avg_ms_d = total_decode_us / 100.0;

    printf("LC3 test successful! \nAvg Encoding time %.3f µs \nAvg Decoding time %.3f µs\n", avg_ms_e, avg_ms_d);
    
    free(in); free(out); free(bit);
    free(enc_mem); free(dec_mem);
    return 0;
}


void pin_to_core(int core_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core_id, &mask);

    pid_t tid = syscall(__NR_gettid);  // get current thread ID
    if (sched_setaffinity(tid, sizeof(mask), &mask) != 0) {
        perror("sched_setaffinity failed");
    } else {
        printf("Pinned to core %d\n", core_id);
    }
}


int main(int argc, char *argv[]) {    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <core_number>\n", argv[0]);
        return 1;
    }

    int core = atoi(argv[1]);   
    pin_to_core(core);          // pin process to specified core
    
    if (test_opus_codec() != 0) {
        printf("Opus test failed!\n");
        return 1;
    }
    
    printf("\n");
    
    if (test_lc3_codec() != 0) {
        printf("LC3 test failed!\n");
        return 1;
    }
    
    printf("All codec tests passed successfully!\n\n\n");
    return 0;
}