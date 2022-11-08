#ifndef _CTRL_SCBI__H
#define _CTRL_SCBI__H

#include <stdint.h>

struct scbi_handle;

enum scbi_protocol
{
	CAN_FORMAT_0      = 0x00,   /* CAN Msgs size <= 8 */
	CAN_FORMAT_BULK   = 0x01,   /* CAN Msgs size >  8 */
	CAN_FORMAT_UPDATE = 0x02    /* CAN Msg transmitting firmware update */
};

union scbi_address_id
{
	unsigned long address_id;
	struct scbi_id_format
	{
		uint8_t prog_type;
		uint8_t subscriber;
		uint8_t func_type;
		uint8_t prot_type:3;
		uint8_t msg_type:2;
	} scbi_id;
};



/* CAN_FORMAT_0 protocol definitions */

enum scbi_prog_type
{
	PRG_CONTROLLER              = 0x0B,
	PRG_REMOTESENSOR            = 0x83,
	PRG_DATALOGGER_NAMEDSENSORS = 0x84, /* REMOTESENSOR  (for compatibility) */
	PRG_HCC                     = 0x85,
	PRG_AVAILABLERESOURCES      = 0x8C,
	PRG_PARAMETERSYNCCONFIG     = 0x90,
	PRG_ROOMSYNC                = 0x91,
	PRG_MSGLOG                  = 0x94,
	PRG_CBCS                    = 0x95,
};

enum scbi_dlg_function_type
{
	DLF_UNDEFINED = 0,
	DLF_SENSOR,
	DLF_RELAY,
	DLG_HYDRAULIC_PROGRAM,
	DLG_ERROR_MESSAGE,
	DLG_PARAM_MONITORING,
	DLG_STATISTIC,
	DLG_OVERVIEW,
	DLG_HYDRAULIC_CONFIG
};




struct scbi_handle * scbi_open(struct scbi_handle * hnd, const char * port);
void                 scbi_update(struct scbi_handle * hnd);
int                  scbi_close(struct scbi_handle * hnd);



#endif   // _CTRL_SCBI__H
