/* user/yaos2/kernel/errno.h - errno values (user-space API definitions) */
#ifndef _USER_YAOS2_KERNEL_ERRNO_H
#define _USER_YAOS2_KERNEL_ERRNO_H

#define EUNSPEC			2 /* Unspecified error. TODO: Try to get rid of these. Recurring.. */

#define ENOSTATUS		10 /* Process had an undefined exit status. */

#define ENOMEM			100 /* Ran out of memory. */
#define EOVERFLOW		101 /* A buffer or value would overflow. */
#define EPARAM			102 /* An invalid parameter value was provided. */

#endif
