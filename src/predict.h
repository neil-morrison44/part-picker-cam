// #include <TFT_eSPI.h>

void setupModel();

float *runPrediction(uint8_t *framebuffer, size_t length);

void writePredictionFrameTo16BitBuffer(uint8_t *framebuffer, size_t length, uint16_t *output_buffer);
