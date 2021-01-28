#include <TensorFlowLite_ESP32.h>
#include <HardwareSerial.h>
#include "./model.h"
// #include "predict.h"

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/version.h"

#define NUMBER_OF_INPUTS 160 * 120 * 3
#define NUMBER_OF_OUTPUTS 40
#define TENSOR_ARENA_SIZE 160 * 1024

const tflite::Model *model;

void setupModel()
{
  // Allocate tensor_arena on SPRAM
  uint8_t *tensor_arena;
  tensor_arena = (uint8_t *)heap_caps_malloc(TENSOR_ARENA_SIZE * 4, MALLOC_CAP_SPIRAM);

  Serial.println("About to try loading model...");

  tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter *error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = ::tflite::GetModel(model_data);
  Serial.println(model->version());
  if (model->version() != TFLITE_SCHEMA_VERSION)
  {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.\n",
                         model->version(), TFLITE_SCHEMA_VERSION);
  }

  Serial.println("Created model...");

  static tflite::MicroMutableOpResolver<6> micro_mutable_op_resolver;
  micro_mutable_op_resolver.AddConv2D();
  micro_mutable_op_resolver.AddMaxPool2D();
  micro_mutable_op_resolver.AddQuantize();
  micro_mutable_op_resolver.AddReshape();
  micro_mutable_op_resolver.AddFullyConnected();
  micro_mutable_op_resolver.AddDequantize();

  // Build an interpreter to run the model with.

  tflite::MicroInterpreter interpreter(model, micro_mutable_op_resolver, tensor_arena,
                                       TENSOR_ARENA_SIZE, error_reporter);

  Serial.println("Allocating Tensors");
  interpreter.AllocateTensors();

  // Obtain a pointer to the model's input tensor
  TfLiteTensor *input = interpreter.input(0);

  Serial.println("Model");
  Serial.println(input->dims->size);
  Serial.println(input->type);

  // Get information about the memory area to use for the model's input.
  // model_input = interpreter->input(0);
  // if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
  //     (model_input->dims->data[1] != kFeatureSliceCount) ||
  //     (model_input->dims->data[2] != kFeatureSliceSize) ||
  //     (model_input->type != kTfLiteUInt8))
  // {
  //   error_reporter->Report("Bad input tensor parameters in model");
  //   return;
  // }

  Serial.println("model loaded!");
}

// drawer_predictions runPrediction(camera_fb_t *framebuffer)
// {
//   float input[NUMBER_OF_INPUTS];
//   float prediction[NUMBER_OF_OUTPUTS] = {0};

//   // loop over framebuffer and convert from rgb565 to floaty input thing

//   // ml.predict(input, prediction);

//   drawer_predictions formated_prediction;

//   return formated_prediction;
// }
