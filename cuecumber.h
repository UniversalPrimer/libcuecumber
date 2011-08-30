#ifndef __CUECUMBER_H__
#define __CUECUMBER_H__

typedef unsigned char uchar;

struct flv_header {
  uchar signature[3];
  uchar version;
  uchar type_flags;
  uint32_t data_offset;
} __attribute__((__packed__));

static const int FLV_HEADER_OFFSET = 11;

int insert_cuepoint();
void cuecumber_init();

#endif
