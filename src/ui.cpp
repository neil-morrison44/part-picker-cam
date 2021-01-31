#include <TFT_eSPI.h>
#include <ColorConverterLib.h>

float hue = (1.0f / 360) * 300;
float saturation = 0.7f;
float max_lightness = 0.5f;

const int DRAWER_PADDING = 2;

const int SMALL_DRAWER_WIDTH = 20;
const int SMALL_DRAWER_HEIGHT = 10;

const int LARGE_DRAWER_WIDTH = (SMALL_DRAWER_WIDTH * 2) + DRAWER_PADDING;
const int LARGE_DRAWER_HEIGHT = (((SMALL_DRAWER_HEIGHT * 5) + (DRAWER_PADDING * 4)) / 3) - (DRAWER_PADDING * 2);

uint32_t rgbToUint32(int red, int green, int blue)
{
  return ((uint32_t)red << 16 | (uint32_t)green << 8 | (uint32_t)blue);
}

uint16_t rgbtorgb565(int red, int green, int blue)
{
  uint16_t b = (blue >> 3) & 0x1f;
  uint16_t g = ((green >> 2) & 0x3f) << 5;
  uint16_t r = ((red >> 3) & 0x1f) << 11;

  return (uint16_t)(r | g | b);
}

int getResultsTotalWidth()
{
  return (SMALL_DRAWER_WIDTH * 6) + (DRAWER_PADDING * 5);
}

int getResultsTotalHeight()
{
  return (SMALL_DRAWER_HEIGHT * 5) + (DRAWER_PADDING * 7) + (LARGE_DRAWER_HEIGHT * 3) + 1;
}

void drawDrawersForResults(int xOffset, int yOffset, float *results, TFT_eSPI tft)
{

  uint8_t red, green, blue;

  int x = xOffset;
  int y = yOffset;

  float maximumResult = 0;
  for (int i = 0; i < 40; i++)
  {
    if (results[i] > maximumResult)
    {
      maximumResult = results[i];
    }
  }

  for (int i = 1; i < 40; i++)
  {
    bool isSmall = i <= 30;
    float result = constrain(results[i], 0, 50);
    printf("\n %0.2f  ", result);
    float lightness = map(result * 1000, 0, maximumResult * 1000, 10, 100) / 100.0f;
    RGBConverter::HslToRgb(hue, saturation, lightness, red, green, blue);

    printf(" hsl(%0.2f,%0.2f,%0.2f) ", hue, saturation, lightness);
    printf(" rgb(%d, %d, %d) ", red, blue, green);

    int width = isSmall ? SMALL_DRAWER_WIDTH : LARGE_DRAWER_WIDTH;
    int height = isSmall ? SMALL_DRAWER_HEIGHT : LARGE_DRAWER_HEIGHT;

    tft.drawRect(x, y, width, height, rgbtorgb565(red, green, blue));
    if (results[i] > 10)
    {
      tft.fillRect(x, y, width, height, rgbtorgb565(red, green, blue));
    }
    if (isSmall)
    {
      x = x + SMALL_DRAWER_WIDTH + DRAWER_PADDING;
    }
    else
    {
      x = x + LARGE_DRAWER_WIDTH + DRAWER_PADDING;
    }

    if (i % 6 == 0 && isSmall)
    {
      // end of small row
      x = xOffset;
      y = y + SMALL_DRAWER_HEIGHT + DRAWER_PADDING;
    }

    if (i % 3 == 0 && !isSmall)
    {
      // end of large row
      x = xOffset;
      y = y + LARGE_DRAWER_HEIGHT + DRAWER_PADDING;
    }
  }
}
