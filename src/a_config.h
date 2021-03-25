
// uncomment one of the below. small = 11x16, large = 18x24
#define SMALL
//#define LARGE

//uncomment one of the below 
//#define V1_BASE
#define V3_FLIP
//#define V6_SIMPLE

// Display style options:
//  DISPLAY_320x240_SPI_FULL
//  DISPLAY_320x240_SPI_PARTIAL
//  DISPLAY_120x80_I2C

#ifdef V1_BASE
  #define DISPLAY_320x240_SPI_PARTIAL
#endif
#ifdef V3_FLIP
  #define DISPLAY_120x80_I2C
#endif
#ifdef V6_SIMPLE
  #define DISPLAY_320x240_SPI_FULL
#endif

//Uncommenting one of these will initialize the program to one of these states. 
// TODO: Make functions to change the operations while the program is running
#define SERIAL_DEBUG
// #define BT_DEBUG
// #define TELNET_DEBUG

