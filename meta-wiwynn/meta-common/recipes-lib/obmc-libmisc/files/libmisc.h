#ifndef _LIBMISC_H
#define _LIBMISC_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <jansson.h>
#include <stdbool.h>
#include <semaphore.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <systemd/sd-journal.h>
#include <systemd/sd-bus.h>

#define MAX_STRING_SIZE 256
#define SEM_MAX_RETRY 3
#define SEM_MAX_NUM 1
#define SEM_TIMEOUT_S  2
#define SEM_TIMEOUT_NS 0
#define SEM_CPLDUPDATELOCK "/cpld_update_lock"
#define SEM_BIOSUPDATELOCK "/bios_update_lock"

//lib dbus
// uint8_t Set_Dbus_Property(double value, char *object_str, char *path_str, char *interface_str,char *value_str);

//lib math
uint8_t Get_Twos_Complement_Int_Data_Format(uint32_t raw_value, double *real_value);
uint8_t Get_Linear_Data_Format(double* real_value, uint32_t raw_value);
int Get_VID_Data_Format(double* real_value, uint32_t raw_value, uint8_t code_type);
uint8_t zeroChecksum(void *data, int size);

//lib register
int read_register(uint32_t address, uint32_t *result);
int write_register(uint32_t address, uint32_t value);

//lib semaphore
int32_t Sem_Acquired(const char *sem_name);
int32_t Sem_Released(const char *sem_name);

#ifdef  __cplusplus
}
#endif

#endif /* _LIBMISC_H */
