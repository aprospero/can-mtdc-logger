#ifndef _CTRL_SCBI_COMPAT_H_
#define _CTRL_SCBI_COMPAT_H_


typedef unsigned int  uint32_t;
typedef          int   int32_t;
typedef unsigned char  uint8_t;
typedef          char   int8_t;
typedef unsigned short uint16_t;
typedef          short  int16_t;
typedef unsigned int    size_t;
typedef          int   ssize_t;

/* Minimum of signed integral types.  */
# define INT8_MIN   (-128)
# define INT16_MIN  (-32767-1)
# define INT32_MIN  (-2147483647-1)
/* Maximum of signed integral types.  */
# define INT8_MAX   (127)
# define INT16_MAX  (32767)
# define INT32_MAX  (2147483647)

/* Maximum of unsigned integral types.  */
# define UINT8_MAX  (255)
# define UINT16_MAX (65535)
# define UINT32_MAX (4294967295U)

#define NULL ((void *) 0)

#define CAN_MAX_DLEN 8

struct can_frame {
  uint32_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
  union {
    /* CAN frame payload length in byte (0 .. CAN_MAX_DLEN)
     * was previously named can_dlc so we need to carry that
     * name for legacy support
     */
    uint8_t len;
    uint8_t can_dlc; /* deprecated */
  } __attribute__((packed)); /* disable padding added in some ABIs */
  uint8_t __pad; /* padding */
  uint8_t __res0; /* reserved / padding */
  uint8_t len8_dlc; /* optional DLC for 8 byte payload length (9 .. 15) */
  uint8_t data[CAN_MAX_DLEN] __attribute__((aligned(8)));
};

#endif  // _CTRL_SCBI_COMPAT_H_

