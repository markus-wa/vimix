
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

#ifdef linux
#include <linux/netdevice.h>
#endif

// OSC IP gethostbyname
#include "ip/NetworkingUtils.h"

#include "NetworkToolkit.h"


/***
 *
 *      TCP Server JPEG : broadcast
 * SND:
 * gst-launch-1.0 videotestsrc is-live=true ! jpegenc ! rtpjpegpay ! rtpstreampay ! tcpserversink port=5400
 * RCV:
 * gst-launch-1.0 tcpclientsrc port=5400 ! application/x-rtp-stream,encoding-name=JPEG ! rtpstreamdepay! rtpjpegdepay ! jpegdec ! autovideosink
 *
 *      TCP Server H264 : broadcast
 * SND:
 * gst-launch-1.0 videotestsrc is-live=true ! x264enc ! rtph264pay ! rtpstreampay ! tcpserversink port=5400
 * RCV:
 * gst-launch-1.0 tcpclientsrc port=5400 ! application/x-rtp-stream,media=video,encoding-name=H264,payload=96,clock-rate=90000 ! rtpstreamdepay ! rtpjitterbuffer ! rtph264depay ! avdec_h264 ! autovideosink
 *
 *      UDP unicast
 * SND
 * gst-launch-1.0 videotestsrc is-live=true !  videoconvert ! video/x-raw, format=RGB, framerate=30/1 ! queue max-size-buffers=3 ! jpegenc ! rtpjpegpay ! udpsink port=5000 host=127.0.0.1
 * RCV
 * gst-launch-1.0 udpsrc port=5000 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink
 *
 * *      UDP multicast : hass to know the PORT and IP of all clients
 * SND
 * gst-launch-1.0 videotestsrc is-live=true ! jpegenc ! rtpjpegpay ! multiudpsink clients="127.0.0.1:5000,127.0.0.1:5001"
 * RCV
 * gst-launch-1.0 -v udpsrc port=5000 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink
 * gst-launch-1.0 -v udpsrc port=5001 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink
 *
 *        RAW UDP (caps has to match exactly, and depends on resolution)
 * SND
 * gst-launch-1.0 -v videotestsrc is-live=true ! video/x-raw,format=RGBA,width=1920,height=1080 ! rtpvrawpay ! udpsink port=5000 host=127.0.0.1
 * RCV
 * gst-launch-1.0 udpsrc port=5000 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)RAW, sampling=(string)RGBA, depth=(string)8, width=(string)1920, height=(string)1080, colorimetry=(string)SMPTE240M, payload=(int)96, ssrc=(uint)2272750581, timestamp-offset=(uint)1699493959, seqnum-offset=(uint)14107, a-framerate=(string)30" ! rtpvrawdepay ! videoconvert ! autovideosink
 *
 *
 *       SHM RAW RGB
 * SND
 * gst-launch-1.0 videotestsrc is-live=true ! video/x-raw, format=RGB, framerate=30/1 ! shmsink socket-path=/tmp/blah
 * RCV
 * gst-launch-1.0 shmsrc is-live=true socket-path=/tmp/blah ! video/x-raw, format=RGB, framerate=30/1, width=320, height=240 ! videoconvert ! autovideosink
 *
 * */

const char* NetworkToolkit::protocol_name[NetworkToolkit::DEFAULT] = {
    "Shared Memory",
    "UDP RTP Stream JPEG",
    "UDP RTP Stream H264",
    "TCP Broadcast JPEG",
    "TCP Broadcast H264"
};

const std::vector<std::string> NetworkToolkit::protocol_send_pipeline {

    "video/x-raw, format=RGB, framerate=30/1 ! queue max-size-buffers=3 ! shmsink buffer-time=100000 wait-for-connection=true name=sink",
    "video/x-raw, format=I420, framerate=30/1 ! queue max-size-buffers=3 ! jpegenc ! rtpjpegpay ! udpsink name=sink",
    "video/x-raw, format=I420, framerate=30/1 ! queue max-size-buffers=3 ! x264enc tune=\"zerolatency\" threads=2 ! rtph264pay ! udpsink name=sink",
    "video/x-raw, format=I420 ! queue max-size-buffers=3 ! jpegenc ! rtpjpegpay ! rtpstreampay ! tcpserversink name=sink",
    "video/x-raw, format=I420 ! queue max-size-buffers=3 ! x264enc tune=\"zerolatency\" threads=2 ! rtph264pay ! rtpstreampay ! tcpserversink name=sink"
};

const std::vector<std::string> NetworkToolkit::protocol_receive_pipeline {

    "shmsrc socket-path=XXXX ! video/x-raw, format=RGB, framerate=30/1 ! queue max-size-buffers=3",
    "udpsrc buffer-size=200000 timeout=200000 port=XXXX ! application/x-rtp,encoding-name=JPEG,payload=26 ! queue max-size-buffers=3 ! rtpjitterbuffer ! rtpjpegdepay ! jpegdec",
    "udpsrc buffer-size=200000 timeout=200000 port=XXXX ! application/x-rtp,encoding-name=H264,payload=96,clock-rate=90000 ! queue max-size-buffers=3 ! rtph264depay ! avdec_h264",
    "tcpclientsrc timeout=1 port=XXXX ! queue max-size-buffers=3 ! application/x-rtp-stream,media=video,encoding-name=JPEG,payload=26 ! rtpstreamdepay ! rtpjpegdepay ! jpegdec",
    "tcpclientsrc timeout=1 port=XXXX ! queue max-size-buffers=3 ! application/x-rtp-stream,media=video,encoding-name=H264,payload=96,clock-rate=90000 ! rtpstreamdepay ! rtph264depay ! avdec_h264"
};

std::vector<std::string> ipstrings;
std::vector<unsigned long> iplongs;


void add_interface(int fd, const char *name) {
    struct ifreq ifreq;
    char host[128];
    memset(&ifreq, 0, sizeof ifreq);
    strncpy(ifreq.ifr_name, name, IFNAMSIZ);
    if(ioctl(fd, SIOCGIFADDR, &ifreq)==0) {
        int family;
        switch(family=ifreq.ifr_addr.sa_family) {
            case AF_INET:
            case AF_INET6:
                getnameinfo(&ifreq.ifr_addr, sizeof ifreq.ifr_addr, host, sizeof host, 0, 0, NI_NUMERICHOST);
                break;
            default:
            case AF_UNSPEC:
                return; /* ignore */
        }
        // add only if not already listed
        if ( std::find(ipstrings.begin(), ipstrings.end(), std::string(host)) == ipstrings.end() )
        {
            ipstrings.push_back( std::string(host) );
            iplongs.push_back( GetHostByName(host) );
//            printf("%-24s%s %lu\n", name, host, GetHostByName(host));
        }
    }
}

void list_interfaces()
{
    struct ifreq *ifreq;
    struct ifconf ifconf;
    char buf[16384];
    int fd=socket(PF_INET, SOCK_DGRAM, 0);
    if(fd > -1) {
        ifconf.ifc_len=sizeof buf;
        ifconf.ifc_buf=buf;
        if(ioctl(fd, SIOCGIFCONF, &ifconf)==0) {
            ifreq=ifconf.ifc_req;
            for(int i=0;i<ifconf.ifc_len;) {
                size_t len;
#ifndef linux
                len=IFNAMSIZ + ifreq->ifr_addr.sa_len;
#else
                len=sizeof *ifreq;
#endif
                add_interface(fd, ifreq->ifr_name);
                ifreq=(struct ifreq*)((char*)ifreq+len);
                i+=len;
            }
        }
    }
    close(fd);
}

std::vector<std::string> NetworkToolkit::host_ips()
{
    if (ipstrings.empty())
        list_interfaces();

    return ipstrings;
}

bool NetworkToolkit::is_host_ip(const std::string &ip)
{
    if ( ip.compare("localhost") == 0)
        return true;

    if (ipstrings.empty())
        list_interfaces();

    return std::find(ipstrings.begin(), ipstrings.end(), ip) != ipstrings.end();
}

std::string NetworkToolkit::closest_host_ip(const std::string &ip)
{
    std::string address = "localhost";

    if (iplongs.empty())
        list_interfaces();

    // discard trivial case
    if ( ip.compare("localhost") != 0)
    {
        int index_mini = -1;
        unsigned long host = GetHostByName( ip.c_str() );
        unsigned long mini = host;

        for (size_t i=0; i < iplongs.size(); i++){
            unsigned long diff = host > iplongs[i] ? host-iplongs[i] : iplongs[i]-host;
            if (diff < mini) {
                mini = diff;
                index_mini = (int) i;
            }
        }

        if (index_mini>0)
            address = ipstrings[index_mini];

    }

    return address;
}

std::string NetworkToolkit::hostname()
{
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    return std::string(hostname);
}
