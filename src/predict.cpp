#include <EloquentTinyML.h>
#include "./model.h"
#include "predict.h"

#define NUMBER_OF_INPUTS 160 * 120 * 3
#define NUMBER_OF_OUTPUTS 40
#define TENSOR_ARENA_SIZE 10 * 1024

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

void setupModel()
{
  ml.begin(model_data);
  Serial.println("model loaded!");
}

drawer_predictions runPrediction(camera_fb_t *framebuffer)
{
  float input[NUMBER_OF_INPUTS];
  float prediction[NUMBER_OF_OUTPUTS] = {0};

  // loop over framebuffer and convert from rgb565 to floaty input thing

  // ml.predict(input, prediction);

  drawer_predictions formated_prediction;

  return formated_prediction;
}
