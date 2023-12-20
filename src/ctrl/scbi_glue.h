#ifndef _CTRL_SCBI_GLUE__H
#define _CTRL_SCBI_GLUE__H

#include <stdint.h>

#include "scbi_api.h"

void scbi_glue_log(enum scbi_log_level ll, const char * format, ...);

struct scbi_glue_handle * scbi_glue_create(struct scbi_handle * scbi_hnd, const char *port, void * broker);
void scbi_glue_update(struct scbi_glue_handle * hnd);
void scbi_glue_destroy(struct scbi_glue_handle * hnd);

#endif   // _CTRL_SCBI_GLUE__H
