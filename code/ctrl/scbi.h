#ifndef _CTRL_SCBI__H
#define _CTRL_SCBI__H

struct scbi_handle;

struct scbi_handle * scbi_open(struct scbi_handle * hnd, const char * port);
void                 scbi_update(struct scbi_handle * hnd);
int                  scbi_close(struct scbi_handle * hnd);


#endif   // _CTRL_SCBI__H
