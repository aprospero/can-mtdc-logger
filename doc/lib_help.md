# Sorella™ library

#### Public Header:

* ctrl/scbi_api.h
* ctrl/scbi_compat.h (optional)

#### Private implementation and header:

* ctrl/scbi.h
* ctrl/scbi.c

#### Build environment:

* std-c support for variadic functions required
* define SCBI_NO_LINUX_SUPPORT macro in non-linux environments in order to resort to the scbi_compat.h compatibility header.
* disable 'strict aliasing' compiler optimization (eg. -fno-strict-aliasing for GCC)

#### Runtime environment:

One instance of Sorella™ for 4 sensors and 2 relays consumes 13KiByte data memory. Code size depends on build system config.

# Sorella™ API

## Quickstart

### Abstract

Sorella™ API provides a parsing engine for interpreting MTDC/LTDC CAN-bus frames to discrete device parameters. Further there is a mechanism to register on single MTDC/LTDC device parameters for runtime observation.

### Configuration/Initialization

Setting up Sorella™ is divided in two phases.

- initialization of data structures and auxiliary capabilities (malloc, logging)
- parameter registration

The data structures are initialized with a call to [**scbi_init**](#function-scbi_init). The call gets as parameters a function for memory allocation (required) and a function for handling log output (optional). In addition, the timeout is defined for which a parameter is not repeated without a value change.

Registration of parameters takes place by calling one of the registration functions for [sensors](#function-scbi_register_sensor), [relays](#function-scbi_register_relay) und [statistic parameters](#function-scbi_register_overview).

### At Runtime

Sorel MTDC/LTDC CAN bus messages are fed to Sorella™ by calling scbi_parse() for each incoming frame.

If an incoming [**scbi_frame**](#struct-scbi_frame) reports a registered parameter and either its value has changed or a configurable period of time has elapsed since its last emission, it is stored in a queue for output.

After parsing a frame, repeated calls to [**scbi_pop_param**](#function-scbi_pop_param) provide change information on registered parameters until the call returns an empty result.

# API Reference

Typical Sorella™ API usage can be divided in three phases

- [Initialization](#Initialization)
- [Parameter Registration](#Parameter-Registration)
- [Runtime](#Runtime)

## Initialization

It ain't need a lot to get Sorella™ up and running. Provide the ability to allocate memory and optionally log messages, that's it. The API defines two function pointer definitions in order to provide those.

#### typedef alloc_fn

Defines a function that is expected to return a pointer to available and reserved memory which is at least  **__size** bytes long. Platform specific alignment restrictions are supposed to be handled internally. If there is not enough memory available it should return 'zero'.

```c
typedef  void * (* alloc_fn) (size_t __size);
```

#### typedef log_push_fn

Defines a function that is expected to handle arbitrarily emitted log messages. In oder to supply information about the gravity of emitted messages Sorella™ provides a [**enum scbi_log_level**](#enum-scbi_log_level). Filtering based on log levels is supposed to be applied externally.

```c
typedef void (* log_push_fn) (enum scbi_log_level ll, const char * format, ...);
```

---

#### enum scbi_log_level

Categorizes emitted log messages into several verbosity/gravity classes. 

```c
enum scbi_log_level
{
  SCBI_LL_CRITICAL,
  SCBI_LL_ERROR,
  SCBI_LL_WARNING,
  SCBI_LL_INFO,
  SCBI_LL_DEBUG,
  SCBI_LL_CNT
};
```

---

#### Function scbi_init

Initializes Sorella™ internal data structures.

##### Parameters

- [**alloc_fn**](#typedef-alloc_fn) **alloc**                         
  - memory allocating function (required)
- [**log_push_fn**](#typedef-log_push_fn) **log_push**        
  - log message handling function (optional)
- **uint32_t repost_timeout_s**  
  - timeout in seconds until a parameter is repeated

##### Return Value

* **struct scbi_handle \***    
  
  *  transparent structure holding instance data of Sorella™

```c
struct scbi_handle * scbi_init(alloc_fn alloc, log_push_fn log_push, 
                               uint32_t repost_timeout_s);
```

---

## Parameter Registration

There are three types of parameters:

* [Sensors](#Sensors)

* [Relays](#Relays)

* [Statistics (overview)](#Statistical-Data-overview)

Each type has its own registration function. Calling one of these functions registers a single parameter. If a registered parameters value is read from an incoming message the parameter will be reported. A parameter can only be registered once. A subsequent call of a register function for the same parameter will result in overwriting the registration information from the first call. Unregister a parameter by calling its registration function while setting entity to NULL.

#### enum **scbi_param_type**

datalogger monitor parameter types

```c
enum scbi_param_type
{
  SCBI_PARAM_TYPE_SENSOR,
  SCBI_PARAM_TYPE_RELAY,
  SCBI_PARAM_TYPE_OVERVIEW,
  SCBI_PARAM_TYPE_COUNT,
  SCBI_PARAM_TYPE_NONE
};
```

---

### Sensors

Sensors are recognized as a certain type. This information is used in the registration process.

#### function scbi_register_sensor

##### Parameters

- **[struct scbi_handle](Return-Value) * hnd**                         
  - Sorella™ instance handle
- **size_t id**
  - zero based index with id max = [**SCBI_MAX_SENSORS**](#SCBI_MAX_SENSORS--SCBI_MAX_RELAYS)
- [**enum scbi_dlg_sensor_type**](#enum-scbi_dlg_sensor_type) **type**
  - the supposed sensor type
- **const char * entity**
  - unique parameter identifcation c-string. Parameters will reference it on output. Set to NULL to unregister.

##### Return Value

- **int**
  
  - zero on success, nonzero on fail

```c
int scbi_register_sensor(struct scbi_handle * hnd, 
                         size_t id, enum scbi_dlg_sensor_type type, 
                         const char * entity);
```

---

#### enum scbi_dlg_sensor_type

Differentiates between available sensor types.

```c
enum scbi_dlg_sensor_type
{
  DST_UNKNOWN          = 0x00,
  DST_FLOW             = 0x01,           
  DST_RELPRESSURE      = 0x02,         
  DST_DIFFPRESSURE     = 0x03, 
  DST_TEMPERATURE      = 0x04,
  DST_HUMIDIDY         = 0x05,
  DST_ROOM_CTRL_WHEEL  = 0x06,
  DST_ROOM_CTRL_SWITCH = 0x07,
  DST_COUNT,
  DST_UNDEFINED        = 0xFF,
};
```

---

### Relays

Relays are categorized by its mode and the associated external function.

#### function scbi_register_relay

##### Parameters

- **[struct scbi_handle](Return-Value) * hnd**                  
  - Sorella™ instance handle
- **size_t id**
  - a zero based index with id max = [**SCBI_MAX_RELAYS**](#SCBI_MAX_SENSORS--SCBI_MAX_RELAYS)
- [**enum scbi_dlg_relay_mode**](#enum-scbi_dlg_relay_mode) **mode**
  - the supposed relay mode.
- [**enum scbi_dlg_relay_ext_fct**](#enum-scbi_dlg_relay_ext_func) **ext_fct**
  - the relays supposed external function.
- **const char * entity**
  - unique parameter identifcation c-string. Parameters will report it on output. Set to NULL to unregister.

##### Return Value

- **int**    
  
  -  zero on success, nonzero on fail

```c
int scbi_register_relay(struct scbi_handle * hnd, 
                        size_t id, enum scbi_dlg_relay_mode mode, 
                        enum scbi_dlg_relay_ext_func ext_fct, 
                        const char * entity);
```

---

#### enum scbi_dlg_relay_mode

Used in the registration process to differentiate between available relay types.

```c
enum scbi_dlg_relay_mode 
{
  DRM_RELAYMODE_SWITCHED = 0x00,
  DRM_RELAYMODE_PHASE    = 0x01,
  DRM_RELAYMODE_PWM      = 0x02,
  DRM_RELAYMODE_VOLTAGE  = 0x03,
  DRM_COUNT
};
```

---

#### enum scbi_dlg_relay_ext_func

Used in the registration process to differentiate between available relay functions.

```c
enum scbi_dlg_relay_ext_func
{
  DRE_SOLARBYPASS         = 0x00,
  DRE_HEATING             = 0x01,
  DRE_HEATING2            = 0x02,
  DRE_COOLING             = 0x03,
  DRE_RET_FLOW_INCREASE   = 0x04,
  DRE_DISSIPATION         = 0x05,
  DRE_ANTILEGIO           = 0x06,
  DRE_REVERSE_LOADING     = 0x07,
  DRE_DIFFERENCE          = 0x08,
  DRE_WOOD_BOILER         = 0x09,

  DRE_SAFETY_FCT          = 0x0A,
  DRE_PRESSURE_CTRL       = 0x0B,
  DRE_BOOSTER             = 0x0C,
  DRE_R1PARALLEL_OP       = 0x0D,
  DRE_R2PARALLEL_OP       = 0x0E,
  DRE_ALWAYS_ON           = 0x0F,
  DRE_HEATING_CIRCUIT_RC21= 0x10,
  DRE_CIRCULATION         = 0x11,
  DRE_STORAGEHEATING      = 0x12,
  DRE_STORAGESTACKING     = 0x13,

  DRE_R_V1_PARALLEL       = 0x14,
  DRE_R_V2_PARALLEL       = 0x15,
  DRE_R1_PERMANENTLY_ON   = 0x16,
  DRE_R2_PERMANENTLY_ON   = 0x17,
  DRE_R3_PERMANENTLY_ON   = 0x18,

  DRE_V2_PERMANENTLY_ON   = 0x19,
  DRE_EXTERNALALHEATING   = 0x1A,
  DRE_NEWLOGMESSAGE       = 0x1B,

  DRE_EXTRAPUMP           = 0x1C,
  DRE_PRIMARYMIXER_UP     = 0x1D,
  DRE_PRIMARYMIXER_DOWN   = 0x1E,
  DRE_SOLAR               = 0x1F,
  DRE_CASCADE             = 0x20,

  DRE_DISABLED            = 0xFE,
  DRE_UNSELECTED          = 0xFF,
  DRE_COUNT               = DRE_CASCADE + 2 
};
```

---

### Statistical Data (overview)

These parameters are summarized in the CAN bus protocol as 'overview data'. They are addressed by stating a type and a mode.

#### function scbi_register_overview

##### Parameters

- **[struct scbi_handle](Return-Value) * hnd**                     
  - Sorella™ instance handle
- [**enum scbi_scbi_dlg_overview_type**](#enum-scbi_dlg_overview_type) **type**
- - the requested data type.
- [**enum scbi_dlg_overview_mode**](#enum-scbi_dlg_overview_mode) **mode**
  - the relays supposed external function.
- **const char * entity**
  - unique parameter identifcation c-string. Parameters will report it on output. Set to NULL to unregister.

##### Return Value

- **int**    
  
  -  zero on success, nonzero on fail

```c
int scbi_register_overview(struct scbi_handle * hnd, 
                           enum scbi_dlg_overview_type type, 
                           enum scbi_dlg_overview_mode mode, 
                           const char * entity);
```

---

#### enum scbi_dlg_overview_type

Used in the registration process for overview parameters to differentiate between available statistical values regarding yield over time.

```c
enum scbi_dlg_overview_type
{
  DOT_DAYS      = 0x01,
  DOT_WEEKS     = 0x02,
  DOT_MONTHS    = 0x03,
  DOT_YEARS     = 0x04,
  DOT_TOTAL     = 0x05,
  DOT_STATUS    = 0x06,
  DOT_UNKNOWN07 = 0x07,
  DOT_UNKNOWN08 = 0x08,
  DOT_UNKNOWN09 = 0x09,
  DOT_UNKNOWN10 = 0x0A,
  DOT_COUNT
};
```

---

#### enum scbi_dlg_overview_mode

```c
enum scbi_dlg_overview_mode
{
  DOM_00    = 0x00,  /* unknown meaning */
  DOM_01    = 0x01,
  DOM_02    = 0x02,
  DOM_COUNT
};
```

---

## Runtime

### Providing Input

To parse a CAN bus message from an MTDC/LTDC device the CAN frame data must be provided along with a timestamp by using a predefined structure.

#### struct scbi_frame

Data structure for passing SCBI messages to Sorellas™ parsing function.

###### Members

* [struct can_frame](#struct-can_frame) **msg** - payload consisting of a CAN bus frame

* [scbi_time](#typedef-scbi_time) **recvd** - timestamp holding time of dispatch/reception

```c
struct scbi_frame
{
  struct can_frame msg;    
  scbi_time        recvd; 
};
```

 ---

#### struct can_frame

From the linux header linux/can.h - the CAN-frame definition (extended frame).

```c
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
```

 ---

#### typedef scbi_time

A simple revolving timestamp in ms, overflowing at **[SCBI_TIME_MAX](#SCBI_TIME_MAX)** to zero.

```c
typedef uint32_t scbi_time;
```

 ---

#### function scbi_parse

##### Parameters

- **[struct scbi_handle](Return-Value) * hnd**    
  - Sorella™ instance handle
- **size_t id**
  - a zero based index with id max = [**SCBI_MAX_RELAYS**](#SCBI_MAX_SENSORS--SCBI_MAX_RELAYS)
- **[struct scbi_frame](#struct-scbi_frame) * frame**
  - the data frame to parse

##### Return Value

- **int**    
  
  -  zero on success, nonzero on fail - to date Sorella™ only supports single extended CAN frame messages. Bulk messages may cause a nonzero return.

```c
int scbi_parse(struct scbi_handle * hnd, struct scbi_frame * frame);
```

---

### Reap output

Sorella™ provides parameters by popping them from a queue. It delivers a structure containing type (sensor/relay/statistics), name (entity provided at  registration) and its actual value. 

#### struct scbi_param

Contains data for a parameter of the MTDC/LTDC.

###### Member

- **[enum scbi_param_type](#enum-scbi_param_type)** **type** - the device feature type the parameter belongs to

- const char * **name** - the name that was used upon registration.

- int32_t **value** - the actual parameter value - unit and division is defined intrinsically

```c
struct scbi_param
{
  enum scbi_param_type type;
  const char *         name;
  int32_t              value;
};
```

---

#### function scbi_pop_param

Retrieves the next parameter from Sorellas™ output queue.

##### Parameters

- **[struct scbi_handle](Return-Value) * hnd**                      
  - Sorella™ instance handle

##### Return Value

- **[struct scbi_param](#struct-scbi_param)**
  
  - a structure containing a parameters identifyers and its value

```c
struct scbi_param * scbi_pop_param(struct scbi_handle * hnd);
```

---

#### function scbi_peek_param

Does exactly the same as pop, but doesn't delete the parameter from Sorellas™ internal queue. If for any reason further processing of a peeked parameter is not possible it can be later reattempted by calling peek/pop again. After successful processing a final call to [**scbi_pop_param**](#function-scbi_pop_param) is necessary to go on with the next parameter.

##### Parameters

- **[struct scbi_handle](Return-Value) * hnd**                   
  - Sorella™ instance handle

##### Return Value

- **[struct scbi_param](#struct-scbi_param)**
  
  - a structure containing a parameters identifyers and its value

```c
struct scbi_param * scbi_peek_param(struct scbi_handle * hnd);
```

---

## Helper functions

#### scbi_print_frame

logs a data frame if a log message function was provided when calling **[scbi_init](#function-scbi_init)**.

##### Parameters

- **[struct scbi_handle](Return-Value) * hnd**                      
  - Sorella™ instance handle
- **[enum scbi_log_level](#enum-scbi_log_level) ll**
  - which loglevel should the log message get
- **const char * msg_type** 
  - will be printed embedded in braces at beginning of log message 
- **const char * txt** (optional) 
  - additional text following msg_type
- **[struct scbi_frame](#struct-scbi_frame) * frame**
  - frame data to be printed

```c
void scbi_print_frame (struct scbi_handle * hnd, enum scbi_log_level ll,
                       const char * msg_type, const char * txt, 
                       struct scbi_frame * frame); 
```

---

## Macros

#### SCBI_NO_LINUX_SUPPORT

define this macro if there is no linux support available, namely the header 

* stdint.h 

* stddef.h 

* linux/can.h

If SCBI_NO_LINUX_SUPPORT is defined the optional header file 'scbi_compat.h' is included which recreates missing definitions.

```c
#ifndef SCBI_NO_LINUX_SUPPORT
  #include <stdint.h>
  #include <stddef.h>
  #include <linux/can.h>
#else
  #include "scbi_compat.h"
#endif  // SCBI_NO_LINUX_SUPPORT
```

---

#### SCBI_MAX_SENSORS / SCBI_MAX_RELAYS

MTDC device features.

The actual values are representing the featureset of MTDCv5 - these numbers probably differ on other MTDC/LTDCs.

```c
#define SCBI_MAX_SENSORS 4
#define SCBI_MAX_RELAYS  2
```

---

#### SCBI_TIME_MAX

 [**scbi_time**](#typedef-scbi_time) max value

SCBI_TIME_MAX = 2³² = 4294967296 ms
 at this value scbi timestamps overflow to zero.

```c
#define SCBI_TIME_MAX UINT32_MAX 
```

---
