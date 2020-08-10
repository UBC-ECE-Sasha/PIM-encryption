#ifndef TRANSFER_UNIT_H
#define TRANSFER_UNIT_H

#include "common.h"
#include <mram.h>

struct transfer_unit {
  void __mram_ptr *src;
  __dma_aligned uint8_t data[TRANSFER_SIZE];
};

// Read data from MRAM into unit->data from unit->src
void read_transfer_unit(struct transfer_unit *unit);

// Write unit->data to MRAM at the address in unit->src
void write_transfer_unit(struct transfer_unit *unit);

#endif /* !TRANSFER_UNIT_H */
