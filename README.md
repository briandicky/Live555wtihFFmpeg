RTP Streaming and NAT (Network Address Translation) Traversal
===

## Part 1: Wireshark Labs
#### Getting started
    1. **Q2**: As shown in the screen shot below, the GET was sent at 22:24:24.779707 and the reply was received at 22:24:24.846889. The delay was 0.067182 secs.
![httpreplytime](/uploads/upload_bc0086b3448e1e3b2244611d08086119.png)

    2. **Q4**: The print out of the two HTTP messages are below:
    - HTTP GET message:
    ```=
    No. Time Source Destination Protocol Length Info
    7910 22:24:24.779707 140.114.77.125 140.114.89.208 HTTP 756 GET /history HTTP/1.1
    Frame 7910: 756 bytes on wire (6048 bits), 756 bytes captured (6048 bits) on interface 0
    Interface id: 0 (\Device\NPF_{566C6BE0-C0F9-4A64-8944-53AD4C61C76B})
    Encapsulation type: Ethernet (1)
    Arrival Time: Apr 26, 2017 22:24:24.779707000 Taipei Standard Time
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1493216664.779707000 seconds
    [Time delta from previous captured frame: 0.039432000 seconds]
    [Time delta from previous displayed frame: 0.119897000 seconds]
    [Time since reference or first frame: 32.455729000 seconds]
    Frame Number: 7910
    Frame Length: 756 bytes (6048 bits)
    Capture Length: 756 bytes (6048 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:tcp:http]
    [Coloring Rule Name: HTTP]
    [Coloring Rule String: http || tcp.port == 80 || http2]
    Ethernet II, Src: AsustekC_92:40:8c (74:d0:2b:92:40:8c), Dst: IETF-VRRP-VRID_4d (00:00:5e:00:01:4d)
    Internet Protocol Version 4, Src: 140.114.77.125, Dst: 140.114.89.208
    Transmission Control Protocol, Src Port: 50364, Dst Port: 3000, Seq: 1422, Ack: 647, Len: 702
    Hypertext Transfer Protocol
    ```

    - HTTP REPLY message:
    ```=
    No. Time Source Destination Protocol Length Info
    7928 22:24:24.846889 140.114.89.208 140.114.77.125 HTTP 1189 HTTP/1.1 200 OK (application/json)
    Frame 7928: 1189 bytes on wire (9512 bits), 1189 bytes captured (9512 bits) on interface 0
    Interface id: 0 (\Device\NPF_{566C6BE0-C0F9-4A64-8944-53AD4C61C76B})
    Encapsulation type: Ethernet (1)
    Arrival Time: Apr 26, 2017 22:24:24.846889000 Taipei Standard Time
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1493216664.846889000 seconds
    [Time delta from previous captured frame: 0.000485000 seconds]
    [Time delta from previous displayed frame: 0.067182000 seconds]
    [Time since reference or first frame: 32.522911000 seconds]
    Frame Number: 7928
    Frame Length: 1189 bytes (9512 bits)
    Capture Length: 1189 bytes (9512 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:tcp:http:data:data:json]
    [Coloring Rule Name: HTTP]
    [Coloring Rule String: http || tcp.port == 80 || http2]
    Ethernet II, Src: Cisco_91:bd:40 (7c:ad:74:91:bd:40), Dst: AsustekC_92:40:8c (74:d0:2b:92:40:8c)
    Internet Protocol Version 4, Src: 140.114.89.208, Dst: 140.114.77.125
    Transmission Control Protocol, Src Port: 3000, Dst Port: 50364, Seq: 1097, Ack: 2124, Len: 1135
    [2 Reassembled TCP Segments (1585 bytes): #7916(450), #7928(1135)]
                                               Hypertext Transfer Protocol
                                               JavaScript Object Notation: application/json
                                               ```

### TCP
    1. **Q9**: The minimum amount of buffer space (receiver window) advertised at the received (gaia.cs.umass.edu) is 5840 bytes, which shows in the first acknowledgement from the server. This receiver window grows steadily until a maximum receiver buffer size of 62780 bytes. The sender is never throttled due to lacking of receiver buffer space by inspecting this trace.
![](/uploads/upload_7ee815a9b17a693e19a8efd4e17970cd.png)

    2. **Q13**: TCP Slow Start phase begins at the start of the connection. The identification of the TCP slow start phase and congestion avoidance phase depends on the value of the congestion window size of this TCP sender. However, the value of the congestion window size cannot be obtained directly from the Time-Sequence-Graph (Stevens) graph. Nevertheless, we can estimate the lower bound of the TCP window size by the amount of outstanding data because the outstanding data is the amount of data without acknowledgement. We also know that TCP window is constrained by the receiver window size and the receiver buffer can act as the upper bound of the TCP window size. In this trace, the receiver buffer is not the bottleneck; therefore, this upper bound is not quite useful to infer the TCP window size. 
![](/uploads/upload_48e659fb24f66d29028ed7612e0032f5.png)



### NAT
    1. **Q5**:
    1. 7.075657
    2. Source is 192.168.1.100, port is 4335. Destination is 64.223.169.104, port is 80.
    3. Source is 64.233.169.104, port is 80. Destination is 64.233.169.104, port is 4335.
    4. 7.108986

    2. **Q9**:
    1. 6.035475, and 6.067775, respectively
    2. : Source is 71.192.34.104, port is 4335. Destination is 64.233.169.104, port is 80. For the ACK: Source is 64.233.169.104, port is 80. Destination is 71.192.34.104, port is 4335.


### 802.11
    1. **Q6**: : The support rates are 1.0, 2.0, 5.5, 11.0 Mbps. The extended rates are 6.0, 9.0, 12.0, 18.0, 24.0, 36.0, 48.0 and 54.0 Mbps.

    2. **Q7**: 
    The TCP SYN is sent at t = 24.811093 seconds into the trace. The MAC address for the host sending the TCP SYN is 00:13:02:d1:b6:4f. The MAC address for the destination, which the first hop router to which the host is connected, is 00:16:b6:f4:eb:a8. The MAC address for the BSS is 00:16:b6:f7:1d:51. 
    The IP address of the host sending the TCP SYN is 192.168.1.109. Note that this is a NATed address. The destination address is 128.199.245.12. This corresponds to the server gaia.cs.umass.edu. It is important to understand that the destination MAC address of the frame containing the SYN, is different from the destination IP address of the IP packet contained within this frame. Make sure you understand this distinction! (If you’re a bit hazy on this, re-read pages 468 and 469 in the 4th edition of the text). 
    The idealized behavior of TCP in the text assumes that TCP senders are aggressive in sending data. Too much traffic may congest the network; therefore, TCP senders should follow the AIMD algorithm so that when they detect network congestion (i.e., packet loss), their sending window size should drop down. 
    In the practice, TCP behavior also largely depends on the application. In this example, when the TCP sender can send out data, there are no data available for transmission. In the web application, some of web objects have very small sizes. Before the end of slow start phase, the transmission is over; hence, the transmission of these small web objects suffers from the unnecessary long delay because of the slow start phase of TCP. 

## Part 2: UDP Streaming
    - [(FAQ) about the "LIVE555 Streaming Media" libraries](http://www.live555.com/liveMedia/faq.html#testRTSPClient-how-to-decode-data)
    - [[live555][OpenRTSP][SDL][ffmpeg] 利用ffmpeg 與SDL 達成 streaming 筆記(1)](http://www.evanlin.com/live555openrtspsdlffmpeg-e5-88-a9-e7-94-a8ffmpeg-e8-88-87sdl-e9-81-94-e6-88-90-streaming-e7-ad-86-e8-a8-981/)
    - [[live555][OpenRTSP][SDL][ffmpeg] 利用ffmpeg 與SDL 達成 streaming 筆記(2)](http://www.evanlin.com/live555openrtspsdlffmpeg-e5-88-a9-e7-94-a8ffmpeg-e8-88-87sdl-e9-81-94-e6-88-90-streaming-e7-ad-86-e8-a8-982/)
    - [Live555接收h264使用ffmpeg解碼為YUV420](http://blog.csdn.net/gubenpeiyuan/article/details/19072223)
    - [demoLive555withFFMPEG](https://github.com/yuvalk/demoLive555withFFMPEG)
    - [基於live555的rtsp客戶端接收及ffmpeg解碼](http://blog.chinaunix.net/uid-15063109-id-4482932.html)
    - [Live555 + h264 + ffmpeg客戶端解碼筆記](http://blog.csdn.net/ghgui008/article/details/21552603)
    - [Receiving RTSP stream using FFMPEG library](http://stackoverflow.com/questions/10715170/receiving-rtsp-stream-using-ffmpeg-library)

### Compile FFmpeg 
    Here is my guideline about how to compile ffmpeg step by step, you can also check [ffmpeg official guideline](https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu) to do the same thing.
    1. Install needed libraries:
    ```shell=
# First get all the dependencies
    sudo apt-get update
    sudo apt-get -y install autoconf automake build-essential libass-dev libfreetype6-dev libsdl2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo zlib1g-dev

# Install yasm (version ≥ 1.2.0)
    sudo apt-get install yasm 

#Install libx264 (version ≥ 118)
    sudo apt-get install libx264-dev

# Install libx265 (version ≥ 68)
    sudo apt-get install libx265-dev

# Install libfdk-aac
    sudo apt-get install libfdk-aac-dev

# Install libmp3lame (version ≥ 3.98.3)
    sudo apt-get install libmp3lame-dev

# Install libopus (version ≥ 1.1)
    sudo apt-get install libopus-dev

# Install libvpx (version ≥ 0.9.7)
    sudo apt-get install libvpx-dev
    ```

    2. Make a directory for the source files that will be downloaded later in this guide:
    ```shell=
    mkdir ~/ffmpeg_sources
    ```

    3. Make a directory for the build files:
    ```shell=
    mkdir ~/ffmpeg_build
    ```

    4. Now, you are ready to compile ffmpeg:
    ```shell=
    cd ~/ffmpeg_sources
    wget http://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2
    tar xjvf ffmpeg-snapshot.tar.bz2
    cd ffmpeg
    PATH="$HOME/bin:$PATH" PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig" ./configure \
          --prefix="$HOME/ffmpeg_build" \
          --pkg-config-flags="--static" \
          --extra-cflags="-I$HOME/ffmpeg_build/include" \
          --extra-ldflags="-L$HOME/ffmpeg_build/lib" \
          --bindir="$HOME/bin" \
          --enable-gpl \
          --enable-libass \
          --enable-libfdk-aac \
          --enable-libfreetype \
          --enable-libmp3lame \
          --enable-libopus \
          --enable-libtheora \
          --enable-libvorbis \
          --enable-libvpx \
          --enable-libx264 \
          --enable-libx265 \
          --enable-nonfree
          PATH="$HOME/bin:$PATH" make
          make install
          ```
          That's it.

          5. (Optional) If you counter some error like failed to install libx265 library like this `ffmpeg - ERROR: x265 not found using pkg-config`, please delete libx265 and re-install it again. You can also find some detail solution [here](https://bitbucket.org/multicoreware/x265/issues/125/x265-not-found-using-pkg-config).

### Compile Live555
          You can find the detail guideline [here](http://www.live555.com/liveMedia/#config-unix)
          1. Download a `live555.tar.gz` file, then use "tar -x" and "gunzip" (or "tar -xz", if available) to extract the package.

          2. cd to the "live" directory. Then run
          ```shell=
# According yout os-platform, run this command
          ./genMakefiles linux
          ```
          This will generate a Makefile in the "live" directory and each subdirectory.

          3. Then run `make`. That's it.

### Compile Simple DirectMedia Layer (SDL)
          - [What is the general procedure to install development libraries in Ubuntu?](https://askubuntu.com/questions/344512/what-is-the-general-procedure-to-install-development-libraries-in-ubuntu)

          1. Install the needed libraries
          ```shell=
          sudo apt-get install libsdl1.2-dev
          sudo apt-get install liblzma-dev
          sudo apt-get install libfaac-dev
          sudo apt-get install libfdk-aac-dev
          sudo apt-get install libav-tools
          sudo apt-get install zlib1g-dev
          sudo apt-get install libswresample-dev
          sudo apt-get install libswresample-dev
          sudo apt-get install build-essential xorg-dev libudev-dev libts-dev libgl1-mesa-dev libglu1-mesa-dev libasound2-dev libpulse-dev libopenal-dev libogg-dev libvorbis-dev libaudiofile-dev libpng12-dev libfreetype6-dev libusb-dev libdbus-1-dev zlib1g-dev libdirectfb-dev
          ```
          If you unable to locate package, like this `E: Unable to locate package libts-dev`, just skip it.

          2. Now you can go [here](http://www.libsdl.org/download-2.0.php) download "SDL2-2.0.5.tar.gz" extract the archive (you can extract the archive using tar: `tar -xvzf SDL2-2.0.0.tar.gz`)

          3. Now run the follow commands
          ```shell=
          cd SDL2-2.0.5
          ./configure 
          make 
          sudo make install
          ```
          That's all.

### Compile the modified code, testMyRTSPClient.cpp, which is written by myself
          1. Go to live555 directory `cd live555 && cd live`
          2. Create a your own folder
          ```shell=
          mkdir [your_folder_name]
          cd [your_folder_name]
          ```
          3. Then run `make` to compile the code, testMyRTSPClient.cpp. That's it.
          4. How to use it?
          ```shell=
# Go to live555 folder, called mediaServer, run the streaming server at first
          cd mediaServer
          ./live555MediaServer 
          ```

          After that, go to the folder you created then run the `testMyRTSPClient`
          ```shell=
          ./testMyRTSPClient rtsp://140.114.77.170:8554/test.264
          ```

          It should work successfully, and show up a window playing the video you received.

### (Optional) [Creating Static Library and linking using Makefile](http://maxubuntu.blogspot.tw/2010/02/makefile.html)
          Static libraries are .a files. All the code relating to the library is in this file, and it is directly linked into the program at compile time. Static libraries increase the overall size of the binary, but it means that you don't need to carry along a copy of the library that is being used. This increases the portability of our program.
          ```=
          FFMPEG = /home/mosquito/ffmpeg_build/include
          USGENV = ../UsageEnvironment/include
          GSENV = ../groupsock/include
          LMENV = ../liveMedia/include
          BSUSGENV = ../BasicUsageEnvironment/include
          PWD = .

          LMLIB = ../liveMedia/libliveMedia.a
          GSLIB = ../groupsock/libgroupsock.a
          BSUSGLIB = ../BasicUsageEnvironment/libBasicUsageEnvironment.a
          USGLIB = ../UsageEnvironment/libUsageEnvironment.a

          FFMPEG_STATIC_LIBS = /home/mosquito/ffmpeg_sources/ffmpeg/libavdevice/libavdevice.a         \   
          /home/mosquito/ffmpeg_sources/ffmpeg/libavformat/libavformat.a         \   
          /home/mosquito/ffmpeg_sources/ffmpeg/libavfilter/libavfilter.a         \   
          /home/mosquito/ffmpeg_sources/ffmpeg/libavcodec/libavcodec.a           \   
          /home/mosquito/ffmpeg_sources/ffmpeg/libswresample/libswresample.a     \   
          /home/mosquito/ffmpeg_sources/ffmpeg/libswscale/libswscale.a           \   
          /home/mosquito/ffmpeg_sources/ffmpeg/libavutil/libavutil.a             \
              ```


### testMyRTSPClient.cpp
              This part will briefly describe how my program work.
              - My program is modified from `live555/testProgs/testRTSPClient.cpp`
              - I combine live555, ffmpeg, and SDL into one program

              1. Using `testRTSPClient.cpp` code as a model, how can I decode the received video (and/or audio) data?
              - The "testRTSPClient" application receives each (video and/or audio) frame into a memory buffer, but does not do anything with the frame data. 
    - The `DummySink` class that the "testRTSPClient" application uses and the (non-static) `DummySink::afterGettingFrame()` function. When this function is called, a complete `frame` (for H.264, this will be a `NAL unit`) will have already been delivered into `fReceiveBuffer`. Note that the `DummySink` implementation doesn't actually do anything with this data; that's why it's called a 'dummy' sink.
- If we want to decode (or otherwise process) these frames, you would replace `DummySink` with your own `MediaSink` subclass. Its `afterGettingFrame()` function would pass the data (at `fReceiveBuffer`, of length `frameSize`) to a decoder. (A decoder would also use the `presentationTime` timestamp to properly time the rendering of each frame, and to synchronize audio and video.)
    - After receiving H.264 video data, there is one more thing that we have to do before we start feeding frames to your decoder. H.264 streams have out-of-band configuration information (`SPS` and `PPS` NAL units) that we may need to feed to the decoder to initialize it. To get this information, call `MediaSubsession::fmtp_spropparametersets()` (on the video `subsession` object). This will give you an ASCII character string. You can then pass this to `parseSPropParameterSets()` (defined in the file `include/H264VideoRTPSource.hh`), to generate binary NAL units for your decoder.

    2. Now, you have the YUV420p files from h264 bitstream. Then feed the video/audio files to SDL `SDL_DisplayYUVOverlay()`, there are something details, please find them in my `testMyRTSPClient.cpp`.

## Part 3: NAT Traversal

### Part 3-1
    In this part, we will find an NAT box, such as a WiFi access point, a Linux box with iptable, or a VM hypervisor. Then run my streaming server on the public IP domain (WAN port), and my streaming client on the private IP domain (LAN port). Use wireshark to capture the packets while we stream a short video. Analyze the captured packet traces and discuss:

    - What are the protocols used to setup and carry out the streaming session?
    - We can find that streaming server (WAN port) and streaming client (LAN port) communicate with each other using TCP protocol. They setup the streaming session at first, such as date, time, server IP address, client IP address, port, type of bitstream, version of streaming server and so on. After carrying out the setup session, the Read-Time Control Protocol (RTCP) provides out-of-band statistics and control information for the RTP (based on UDP) sessions.
![photo_of_testbed](/uploads/upload_b96e00cc866bab51748c0936180a8bbe.png)

    - How does the UDP streaming server sends the packet through the NAT box? Observe the IP header of the UDP packets before and after passing through the NAT box.
    - In the figure, we can find that 1) streaming sever address is 140.114.77.170 (WAN) and port is 8554, 2) streaming server address is 192.168.0.104 (LAN) and port is 62929 (for TCP), 65487 (for RTCP), and 65486 (for RTP). In streaming server side, I think it did the same thing whether client is WAN or LAN. In the client side, the WiFi access point using the port forwarding technique to transfer the packages (from streaming server) to streaming client. We know the WiFi access point IP address is 140.114.77.160. But in this figure, we are not able to know which port is used to do port forwarding. 


### Part 3-2
    To find two NAT boxes, and put both the streaming server and client behind the NAT boxes (one behind each NAT box). Note that both sender and receiver now have private IP addresses.

    - Use wireshark to analyze what happened, and propose solution to solve the problem. Hint: you may want to study the NAT traversal approaches, such as connection reversal, NAT hole punching, and NAT configuration protocols. 

    - Enhance your streaming server and client to enable UDP video streaming over two NAT boxes.

    Ideas:
    1. UDP hole punching
2. Third-party server with public IP address (WAN)

    Due to streaming server and client are on the private IP domain (LAN port). Both of them cannot directly connect to each other. Network Address Translator (NAT), however, will maintain User Datagram Protocol (UDP) packet streams that traverse in the Internet. NAT will give an public IP (ex: 140.114.77.170) and port (ex: 9487) for streaming server (or streaming client).
    Hence, we have to build a third-party host server with a public IP, it is in charge of transiting network states from streaming server and client. These are used to establish UDP port states that may be used for direct communications between the streaming server and client. 

    - **Third-party server with public IP address**
    I built it using python language, it is in charge of 1) receiving the requests from both streaming server and streaming client, 2) establish direct communications between streaming server and client, 3) redirecting the UDP datagram from streaming server to client or vice versa.
![3rd_server](/uploads/upload_f264d04043c1afc8e76336653d3064d6.png)
    In the above figure, streaming server IP address is `140.114.77.160` and port is `44652`. Streaming client IP address is `140.114.77.125` and port is `63590`. These IP address and ports were assigned by NAT layer.

    - **Streaming server (`140.114.77.169:44652`)**
    Here is the message captured in streaming server side.
![streaming_server](/uploads/upload_93ebee1af738b6aea675b29dd22baa28.png)

    - **Streaming client (`140.114.77.126:63590`)**
    Here is the message captured in streaming client side.
![streaming_client](/uploads/upload_703beca2db74a22a4d7964a8e1327b76.png)

    To get this to work when both end-points are behind NATs or firewalls would require that both end-points send packets to each other at about the same time. This means that both sides need to know each others public IP addresses and port numbers and need to communicate this to each-other by some other means. There is no way for a program to directly determine its own public IP address if it is behind an NAT (it will only see its private address, such as `192.168.x.x`). Hence, I built a third-party server to handle this.

    In the below figure, we can find that both end-points are behind NAT. They communicate with each other. The IP address in the packet was modified by NAT layer, which make both end-points be able to talk with each others.
![wireshark_result](/uploads/upload_810b9a6d25488caa6894c41600149cf7.png)


