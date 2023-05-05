#define SNAPLEN         1600
#define DEFAULT_IFACE   "eth0"

void initPromiscuity(char *interface);
void idlePromiscuity(void);
void termPromiscuity(void);

