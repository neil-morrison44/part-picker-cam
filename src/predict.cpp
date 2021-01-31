#include <TensorFlowLite_ESP32.h>
#include <HardwareSerial.h>
#include "./model.h"

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/version.h"

#define NUMBER_OF_INPUTS 160 * 120 * 3
#define NUMBER_OF_OUTPUTS 40
#define TENSOR_ARENA_SIZE 288 * 1024

tflite::ErrorReporter *error_reporter = nullptr;
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *model_input = nullptr;
TfLiteTensor *model_output = nullptr;

uint16_t rgb888torgb565(int red, int green, int blue)
{
  uint16_t b = (blue >> 3) & 0x1f;
  uint16_t g = ((green >> 2) & 0x3f) << 5;
  uint16_t r = ((red >> 3) & 0x1f) << 11;

  return (uint16_t)(r | g | b);
}

void setupModel()
{
  TfLiteStatus tflite_status;

  // Allocate tensor_arena on SPRAM
  uint8_t *tensor_arena;
  tensor_arena = (uint8_t *)heap_caps_malloc(TENSOR_ARENA_SIZE * 4, MALLOC_CAP_SPIRAM);

  Serial.println("About to try loading model...");

  tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

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

  static tflite::MicroInterpreter static_interpreter(model, micro_mutable_op_resolver, tensor_arena,
                                                     TENSOR_ARENA_SIZE, error_reporter);
  interpreter = &static_interpreter;

  Serial.println("Allocating Tensors");
  tflite_status = interpreter->AllocateTensors();
  if (tflite_status != kTfLiteOk)
  {
    error_reporter->Report("AllocateTensors() failed");
  }

  // Assign model input and output buffers (tensors) to pointers
  model_input = interpreter->input(0);
  model_output = interpreter->output(0);

  // Obtain a pointer to the model's input tensor
  TfLiteTensor *input = interpreter->input(0);

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

float *runPrediction(uint8_t *framebuffer, size_t length)
{
  TfLiteStatus tflite_status;
  static float prediction[NUMBER_OF_OUTPUTS] = {0};

  int redSum = 0;
  int blueSum = 0;
  int greenSum = 0;

  for (size_t i = 0; i < length; i++)
  {
    model_input->data.f[i] = (1.0f / 255) * framebuffer[i];
    //model_input->data.f[i] = framebuffer[i];
    if (i % 3 == 0)
    {
      redSum += framebuffer[i];
      blueSum += framebuffer[i + 1];
      greenSum += framebuffer[i + 2];
    }
  }
  // Serial.println("AverageColor: ");
  // printf("rgb(%d, %d, %d)\n", redSum / (160 * 60), blueSum / (160 * 60), greenSum / (160 * 60));
  // Serial.println("");
  tflite_status = interpreter->Invoke();
  if (tflite_status != kTfLiteOk)
  {
    error_reporter->Report("Invoke failed");
    return prediction;
  }

  Serial.println("I think I've got an output");
  Serial.println("");
  for (size_t i = 0; i < 40; i++)
  {
    Serial.print(model_output->data.f[i]);
    prediction[i] = model_output->data.f[i];
    Serial.print(", ");
  }
  Serial.println("");

  return prediction;
}

void writePredictionFrameTo16BitBuffer(uint8_t *framebuffer, size_t length, uint16_t *output_buffer)
{
  int iOver = 0;
  for (size_t i = (length / 2); i < length; i++)
  {
    // only the bottom half of the image

    int outputPixelIndex = (i - (length / 2)) / 3;
    if (i % 3 == 0)
    {
      int r = framebuffer[i];
      int g = framebuffer[i + 1];
      int b = framebuffer[i + 2];
      if (outputPixelIndex < 9600)
      {
        output_buffer[outputPixelIndex] = rgb888torgb565(r, g, b);
      }
      else
      {
        iOver++;
      }
    }
  }
  Serial.println("Over By:");
  Serial.println(iOver);
}
