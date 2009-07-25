
#include "ffmpeg.h"

#undef printf

extern URLProtocol mms_protocol;

static void register_all(void)
{
  static int inited = 0;
    
  if (inited != 0)
    return;
  inited = 1;

  //register_protocol(&pipe_protocol);

/*   rtsp_init(); */
/*   rtp_init(); */
/*   register_protocol(&udp_protocol); */
/*   register_protocol(&rtp_protocol); */
/*   register_protocol(&tcp_protocol); */
/*   register_protocol(&http_protocol); */
  register_protocol(&mms_protocol);
}


int codec_main()
{
  printf("codec_net initializing ...\n");

  register_all();

  return 0;
}

int lef_main()
{
  return 0;
}
