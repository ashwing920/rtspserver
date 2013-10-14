CC = arm-none-linux-gnueabi-gcc
CPP = arm-none-linux-gnueabi-g++
STRIP = arm-none-linux-gnueabi-strip
AR = arm-none-linux-gnueabi-ar

SRCDIRS     =./ \
			  ./h264 \
        ./h264/linux_lib

INCLUDES := $(foreach dir,$(SRCDIRS),-I$(dir))
SRCCS    = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJ  	 := $(SRCCS:%.c=%.o)


			  
#CFLAGS = -Wall -O0 -g -static
CFLAGS =  -static
CFLAGS := $(CFLAGS) $(INCLUDES)

CFLAGS += -D__OS_LINUX
LIBS += -pthread -L. ./h264/libv4lconvert.a -lm -lrt ./h264/linux_lib/libcedarv_osal.a ./h264/linux_lib/libcedarxalloc.a ./h264/linux_lib/libh264enc.a ./h264/linux_lib/libcedarv.a 

TARGET = rtspserver

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CPP) $(CFLAGS) -o $@ $^ $(LIBS) 
#	mv enc_dec_test test
clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
