#ifndef _CTRL_SCBI_CONFIG_H
#define _CTRL_SCBI_CONFIG_H

/* build time configuration header for Sorella library
 * please adjust to your own needs
 */

// build environment
#define SCBI_LINUX_SUPPORT

// device featureset (defaults for Sorel MTDCv5)
#define SCBI_MAX_SENSORS 4
#define SCBI_MAX_REALYS 2

#endif  // _CTRL_SCBI_CONFIG_H
