#include <stdio.h>

typedef unsigned char uint8_t ;

static uint8_t crc_update(uint8_t incrc, uint8_t indata) {
  uint8_t i;
  uint8_t data;

  data = incrc ^ indata;
  for( i = 0; i < 8; i++ ) {
    if(( data & 0x80) != 0) {
      data <<= 1;
      data ^= 0x07;
    } else {
      data <<= 1;
    }
  }
  
  return data;
}

static void comp_crc8(uint8_t *tx) {
  int i;

  tx[4] = 0x00;
  for( i = 0; i < 4; i++ ) {
    tx[4] = crc_update(tx[4], tx[i]);
  }
}

int main() {
  uint8_t tx[6];

  tx[0] = 0x16;
  tx[1] = 0x09;
  tx[2] = 0x55;
  tx[3] = 0xaa;
  
  comp_crc8(tx);
  printf( "%02x\n", tx[4] );
}
