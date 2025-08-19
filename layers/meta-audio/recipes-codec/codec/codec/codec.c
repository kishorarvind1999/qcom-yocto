#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   
#include <time.h>
#include <opus/opus.h>
#include <lc3.h>

#define SAMPLE_RATE 48000
#define CHANNELS 1
#define FRAME_SIZE 480  // 10ms at 48kHz
#define BITRATE 32000 
#define MAX_PACKET_SIZE 1000
#define APPLICATION OPUS_APPLICATION_AUDIO

struct timespec start, end;
double elapsed_ms_e, elapsed_ms_d;

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
    
    // Encode
    clock_gettime(CLOCK_MONOTONIC, &start);
    int encoded_size = opus_encode(encoder, input, FRAME_SIZE, encoded, MAX_PACKET_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ms_e = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e3;
    if (encoded_size < 0) {
        printf("Opus encoding failed: %s\n", opus_strerror(encoded_size));
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        return -1;
    }
    
    // Decode
    clock_gettime(CLOCK_MONOTONIC, &start);
    int decoded_size = opus_decode(decoder, encoded, encoded_size, output, FRAME_SIZE, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ms_d = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e3;
    if (decoded_size < 0) {
        printf("Opus decoding failed: %s\n", opus_strerror(decoded_size));
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        return -1;
    }
    
    printf("Opus test successful! \nEncoding time %.3fus \nDecoding time %.3fus\n", elapsed_ms_e, elapsed_ms_d);
    
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

    // Encode: returns status (0 success), NOT size
    clock_gettime(CLOCK_MONOTONIC, &start);
    int rc = lc3_encode(enc, LC3_PCM_FORMAT_S16, in, 1, nbytes, bit);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ms_e = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e3;

    if (rc != 0) { printf("LC3 encode failed (rc=%d)\n", rc); return -1; }

    // Decode: pass the same nbytes you asked the encoder to produce
    clock_gettime(CLOCK_MONOTONIC, &start);
    rc = lc3_decode(dec, bit, nbytes, LC3_PCM_FORMAT_S16, out, 1);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ms_d = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e3;

    if (rc < 0) { printf("LC3 decode failed (rc=%d)\n", rc); return -1; }
    if (rc == 1) printf("Decoder performed PLC\n");

    printf("LC3 test successful! \nEncoding time %.3fus \nDecoding time %.3fus\n", elapsed_ms_e, elapsed_ms_d);

    free(in); free(out); free(bit);
    free(enc_mem); free(dec_mem);
    return 0;
}

int main() {
    printf("Starting audio codec smoke test...\n");
    printf("========================================\n");
    
    if (test_opus_codec() != 0) {
        printf("Opus test failed!\n");
        return 1;
    }
    
    printf("\n");
    
    if (test_lc3_codec() != 0) {
        printf("LC3 test failed!\n");
        return 1;
    }
    
    printf("\n========================================\n");
    printf("All codec tests passed successfully!\n");
    return 0;
}