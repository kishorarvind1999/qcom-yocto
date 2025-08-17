#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opus/opus.h>
#include <lc3.h>

#define SAMPLE_RATE 48000
#define CHANNELS 1
#define FRAME_SIZE 480  // 10ms at 48kHz
#define MAX_PACKET_SIZE 1000
#define APPLICATION OPUS_APPLICATION_AUDIO

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
    int encoded_size = opus_encode(encoder, input, FRAME_SIZE, encoded, MAX_PACKET_SIZE);
    if (encoded_size < 0) {
        printf("Opus encoding failed: %s\n", opus_strerror(encoded_size));
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        return -1;
    }
    
    // Decode
    int decoded_size = opus_decode(decoder, encoded, encoded_size, output, FRAME_SIZE, 0);
    if (decoded_size < 0) {
        printf("Opus decoding failed: %s\n", opus_strerror(decoded_size));
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        return -1;
    }
    
    printf("Opus test successful! Encoded %d bytes, decoded %d samples\n", encoded_size, decoded_size);
    
    opus_encoder_destroy(encoder);
    opus_decoder_destroy(decoder);
    return 0;
}

int test_lc3_codec() {
    printf("Testing LC3 codec...\n");
    
    // LC3 setup for 10ms frame at 48kHz
    int frame_us = 10000;  // 10ms
    int srate_hz = 48000;
    int nchannels = 1;
    
    // Get memory requirements
    unsigned encoder_size = lc3_encoder_size(frame_us, srate_hz);
    unsigned decoder_size = lc3_decoder_size(frame_us, srate_hz);
    
    // Allocate memory
    void *encoder_mem = malloc(encoder_size);
    void *decoder_mem = malloc(decoder_size);
    
    if (!encoder_mem || !decoder_mem) {
        printf("Failed to allocate LC3 codec memory\n");
        free(encoder_mem);
        free(decoder_mem);
        return -1;
    }
    
    // Setup encoder/decoder
    lc3_encoder_t encoder = lc3_setup_encoder(frame_us, srate_hz, 0, encoder_mem);
    lc3_decoder_t decoder = lc3_setup_decoder(frame_us, srate_hz, 0, decoder_mem);
    
    if (!encoder || !decoder) {
        printf("Failed to setup LC3 codec\n");
        free(encoder_mem);
        free(decoder_mem);
        return -1;
    }
    
    // Test data
    int frame_samples = lc3_frame_samples(frame_us, srate_hz);
    int16_t *input = malloc(frame_samples * sizeof(int16_t));
    int16_t *output = malloc(frame_samples * sizeof(int16_t));
    uint8_t *encoded = malloc(1000);  // Generous buffer
    
    if (!input || !output || !encoded) {
        printf("Failed to allocate LC3 test buffers\n");
        free(encoder_mem);
        free(decoder_mem);
        free(input);
        free(output);
        free(encoded);
        return -1;
    }
    
    // Generate test sine wave
    for (int i = 0; i < frame_samples; i++) {
        input[i] = (int16_t)(32767.0 * sin(2.0 * 3.14159 * 440.0 * i / srate_hz));
    }
    
    // Encode
    int bitrate = 64000;  // 64 kbps
    int encoded_size = lc3_encode(encoder, LC3_PCM_FORMAT_S16, input, nchannels, bitrate, encoded);
    
    if (encoded_size <= 0) {
        printf("LC3 encoding failed\n");
        free(encoder_mem);
        free(decoder_mem);
        free(input);
        free(output);
        free(encoded);
        return -1;
    }
    
    // Decode
    int ret = lc3_decode(decoder, encoded, encoded_size, LC3_PCM_FORMAT_S16, output, nchannels);
    
    if (ret != 0) {
        printf("LC3 decoding failed\n");
        free(encoder_mem);
        free(decoder_mem);
        free(input);
        free(output);
        free(encoded);
        return -1;
    }
    
    printf("LC3 test successful! Encoded %d bytes for %d samples\n", encoded_size, frame_samples);
    
    // Cleanup
    free(encoder_mem);
    free(decoder_mem);
    free(input);
    free(output);
    free(encoded);
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