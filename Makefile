all: testMyClient
#LIBS = -lGL -lGLU -lX11 -lXxf86vm -lXrandr -pthread -lXi -ldl -lrt -lpthread -lvdpau -lva -lva-drm -lva-x11 -lx264 -lz -lm -lavformat -lavcodec -lswscale -lavutil -lavfilter -lswresample -lavdevice -lpostproc -lfdk-aac
#LIBS =  -lGL -lGLU -lX11 -lXxf86vm -lXrandr -pthread -lXi  -lrt  -lvdpau -lva -lva-drm -lva-x11  -lfdk-aac -lmp3lame -lx264 -lavformat -lavcodec -lxcb-shm -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lXau -lXdmcp  -lSDL -lasound -ldl -lpulse-simple -lpulse -lXext -lcaca  -llzma -lavutil -lswresample -lx264 -lfaac -lm -lz -lpthread -ldl -lvorbis
LIBS = -lGL -lGLU -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lvdpau  -lrt  -lva -lva-drm -lva-x11  -lfdk-aac -lmp3lame -lx264 -lavformat -lavcodec -lxcb-shm -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lXau -lXdmcp  -lSDL -lasound -ldl -lpulse-simple -lpulse -lXext -lcaca  -llzma -lavutil -lx264 -lfaac -lm -lz -lpthread -ldl -lvorbis -lswscale

# use pkg-config for getting CFLAGS and LDLIBS
FFMPEG_LIBS=    libavdevice                        \
                libavformat                        \
                libavfilter                        \
                libavcodec                         \
                libswresample                      \
                libswscale                         \
                libavutil                          \

CFLAGS += -Wall -g
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
LDLIBS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS)

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

FFMPEG_STATIC_LIBS = /home/mosquito/ffmpeg_sources/ffmpeg/libavdevice/libavdevice.a			\
					 /home/mosquito/ffmpeg_sources/ffmpeg/libavformat/libavformat.a			\
					 /home/mosquito/ffmpeg_sources/ffmpeg/libavfilter/libavfilter.a			\
					 /home/mosquito/ffmpeg_sources/ffmpeg/libavcodec/libavcodec.a			\
					 /home/mosquito/ffmpeg_sources/ffmpeg/libswresample/libswresample.a		\
					 /home/mosquito/ffmpeg_sources/ffmpeg/libswscale/libswscale.a			\
					 /home/mosquito/ffmpeg_sources/ffmpeg/libavutil/libavutil.a				\


testMyClient: testMyClient.cpp
	g++ -c -I$(FFMPEG) -I$(USGENV) -I$(GSENV) -I$(LMENV) -I$(BSUSGENV) -I$(PWD) -O3 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -Wall -DBSD=1 -D__STDC_CONSTANT_MACROS testMyClient.cpp $(CFLAGS)
	ar crsv libtestMyCLient.a testMyClient.o
#	g++ testMyClient.o -o testMyClient -I$(FFMPEG) libtestMyCLient.a $(FFMPEG_STATIC_LIBS) $(LMLIB) $(GSLIB) $(BSUSGLIB) $(USGLIB) -D__STDC_CONSTANT_MACROS $(LDLIBS) $(LIBS)
	g++ testMyClient.o -o testMyClient libtestMyCLient.a $(FFMPEG_STATIC_LIBS) $(LMLIB) $(GSLIB) $(BSUSGLIB) $(USGLIB) -D__STDC_CONSTANT_MACROS  $(LIBS)

clean: 
	rm -f testMyClient.o testMyClient libtestMyCLient.a
