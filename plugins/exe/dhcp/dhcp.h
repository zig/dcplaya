
typedef struct {
  int valid;
  unsigned int yourip;
  unsigned int server_identifier;
  unsigned int lease_time;
  unsigned int subnet_mask;
  unsigned int router;
  unsigned int dns[16];
  unsigned char domain[256 + 1];
} dhcp_conf_t;

void dhcp_discover(unsigned int xid);
void dhcp_request();
int dhcp_handle(unsigned char *pkt);
int dhcp_get_conf(dhcp_conf_t *conf);
