#include "libmisc.h"
#if 0
uint8_t Set_Dbus_Property(double value, char *object_str, char *path_str, char *interface_str,char *value_str){ 
    int32_t ret = 0;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus *bus = NULL;
    char obj_path[MAX_STRING_SIZE] = "";

    ret = sd_bus_open_system(&bus);
    if (ret < 0) {
        printf("Failed to connect to system bus: %d \n", strerror(-ret));
        goto finish;
    } 
    
    ret = sd_bus_set_property(bus,
                              object_str,                 /* service to contact */
                              path_str,                   /* object path */
                              interface_str,              /* interface name */
                              value_str,
                              &error,                     /* object to return error in */
                              "d",
                              value                       /* set value */
                             );
    if (ret < 0) {
        printf("Failed to parse response message: %s\n", strerror(-ret));
        goto finish;
    }
finish:
    sd_bus_error_free(&error);
    bus = sd_bus_flush_close_unref(bus);
    
    return 0;
}
#endif
