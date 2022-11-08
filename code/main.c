#include <stddef.h>

#include "ctrl/scbi.h"




int main(void)
{
	struct scbi_handle * scbi = scbi_open(NULL, "can0");

	union scbi_address_id id;
	scbi_update(scbi);
	scbi_close(scbi);
	return 0;
}
