/*
 * Copyright (c) 2018 IOTA Stiftung
 * https://gitlab.com/iota-foundation/software/entangled
 *
 * Refer to the LICENSE file for licensing information
 */

#include <stdlib.h>
#include <string.h>
#include "common/trinary/trit_array.h"
#include "common/trinary/trit_byte.h"

#if !defined(TRIT_ARRAY_ENCODING_4_TRITS_PER_BYTE) && \
    !defined(TRIT_ARRAY_ENCODING_5_TRITS_PER_BYTE)
#define TRIT_ARRAY_ENCODING_1_TRIT_PER_BYTE
#endif

size_t flex_trit_array_slice(flex_trit_t *trit_array, flex_trit_t *to_trit_array, size_t start, size_t num_trits) {
  size_t num_bytes = trit_array_bytes_for_trits(num_trits);
#if defined(TRIT_ARRAY_ENCODING_1_TRIT_PER_BYTE)
  memcpy(to_trit_array, trit_array + start, num_trits);
#elif defined(TRIT_ARRAY_ENCODING_4_TRITS_PER_BYTE)
  byte_t buffer;
  uint8_t tshift = (start % 4U) << 1U;
  uint8_t rshift = (8U - tshift) % 8U;
  size_t index = start >> 2U;
  size_t max_index = (start + num_trits - 1) >> 2U;
  // Calculate the number of bytes to copy over
  for (size_t i = index, j = 0; i < index + num_bytes; i++, j++) {
    buffer = trit_array[i];
    buffer = buffer >> tshift;
    if (rshift && i < max_index) {
      buffer |= (trit_array[i + 1] << rshift);
    }
    to_trit_array[j] = buffer;
  }
#elif defined(TRIT_ARRAY_ENCODING_5_TRITS_PER_BYTE)
  byte_t buffer = 0;
  trit_t trits[10];
  size_t index = start / 5U;
  size_t offset = start % 5U;
  size_t max_index = (start + num_trits - 1) / 5U;
  for (size_t i = index, j = 0; i < index + num_bytes; i++, j++) {
    bytes_to_trits(((byte_t *)trit_array + i), 1, trits, 5);
    if (offset && i < max_index) {
      bytes_to_trits(((byte_t *)trit_array + i + 1), 1, ((trit_t *)trits + 5), 5);
    }
    to_trit_array[j] = trits_to_byte(trits + offset, buffer, 5);
  }
#endif
  return num_bytes;
}

size_t flex_trit_array_to_int8(flex_trit_t *trit_array, trit_t *trits, size_t num_trits) {
  size_t num_bytes = trit_array_bytes_for_trits(num_trits);
#if defined(TRIT_ARRAY_ENCODING_1_TRIT_PER_BYTE)
  memcpy(trits, trit_array, num_trits);
#elif defined(TRIT_ARRAY_ENCODING_4_TRITS_PER_BYTE)
  for (size_t i = 0; i < num_trits; i++) {
    trits[i] = flex_trit_array_at(trit_array, i);
  }
#elif defined(TRIT_ARRAY_ENCODING_5_TRITS_PER_BYTE)
  bytes_to_trits(trit_array, num_bytes, trits, num_trits);
#endif
  return num_bytes;
}

/***********************************************************************************************************
 * Trits
 ***********************************************************************************************************/
// To obfuscate the model, the structure definition comes here
// struct _trit_array {...}

/***********************************************************************************************************
 * Public interface
 ***********************************************************************************************************/
size_t trit_array_bytes_for_trits(size_t num_trits) {
  return num_bytes_for_flex_trits(num_trits);
}

/***********************************************************************************************************
 * Accessors
 ***********************************************************************************************************/
void trit_array_set_trits(trit_array_p trit_array, flex_trit_t *trits, size_t num_trits) {
#if !defined(NO_DYNAMIC_ALLOCATION)
  if (trit_array->dynamic) {
    free(trit_array->trits);
    trit_array->dynamic = 0;
  }
#endif //NO_DYNAMIC_ALLOCATION
  trit_array->trits = trits;
  trit_array->num_trits = num_trits;
  trit_array->num_bytes = trit_array_bytes_for_trits(num_trits);
}

trit_array_p trit_array_slice(trit_array_p trit_array, trit_array_p to_trit_array, size_t start, size_t num_trits) {
#if !defined(NO_DYNAMIC_ALLOCATION)
  to_trit_array = to_trit_array ? to_trit_array : trit_array_new(num_trits);
#endif //NO_DYNAMIC_ALLOCATION
  to_trit_array->num_trits = num_trits;
  to_trit_array->num_bytes = flex_trit_array_slice(trit_array->trits, to_trit_array->trits, start, num_trits);
  return to_trit_array;
}

trit_t *trit_array_to_int8(trit_array_p trit_array, trit_t *trits) {
  flex_trit_array_to_int8(trit_array->trits, trits, trit_array->num_trits);
  return trits;
}

#if !defined(NO_DYNAMIC_ALLOCATION)
/***********************************************************************************************************
 * Constructor
 ***********************************************************************************************************/
trit_array_p trit_array_new(size_t num_trits) {
  trit_array_p trit_array;
  trit_array = (trit_array_p)malloc(sizeof(struct _trit_array));
  if (!trit_array) {
    // errno = IOTA_OUT_OF_MEMORY
    return NULL;
  }
  memset(trit_array, 0, sizeof(struct _trit_array));
  trit_array->num_trits = num_trits;
  trit_array->num_bytes = trit_array_bytes_for_trits(num_trits);
  trit_array->trits = malloc(trit_array->num_bytes);
  if (!trit_array->trits) {
    trit_array_free(trit_array);
    // errno = IOTA_OUT_OF_MEMORY
    return NULL;
  }
  memset(trit_array->trits, 0, trit_array->num_bytes);
  trit_array->dynamic = 1;
  return trit_array;
}

/***********************************************************************************************************
 * Destructor
 ***********************************************************************************************************/
void trit_array_free(trit_array_p trit_array) {
  if (trit_array) {
    if (trit_array->dynamic) {
      free(trit_array->trits);
    }
  }
  free(trit_array);
}

#endif //NO_DYNAMIC_ALLOCATION