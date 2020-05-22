#include "transfer_unit.h"
#include "common.h"
#include <mram.h>

void read_transfer_unit(__mram_ptr void *src, struct transfer_unit *unit) {
  unit->src = src;
  mram_read(unit->src, unit->data, TRANSFER_SIZE);
}

void write_transfer_unit(struct transfer_unit *unit) {
  mram_write(unit->data, unit->src, TRANSFER_SIZE);
}
