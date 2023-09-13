CC = gcc
S = hex/shader_256.hex \
    hex/shader_512.hex \
    hex/shader_1k.hex \
    hex/shader_2k.hex \
    hex/shader_4k.hex \
    hex/shader_8k.hex \
    hex/shader_16k.hex \
    hex/shader_32k.hex \
    hex/shader_64k.hex \
    hex/shader_128k.hex \
    hex/shader_256k.hex \
    hex/shader_512k.hex \
    hex/shader_1024k.hex \
    hex/shader_2048k.hex \
    hex/shader_4096k.hex

C = mailbox.c gpu_fft.c gpu_fft_base.c gpu_fft_twiddles.c gpu_fft_shaders.c local_avdeg_cal.c

C1D = $(C) sensing.c

C2D = $(C) $(CAL) doubleps.c

H1D = gpu_fft.h mailbox.h local_avdeg_cal.h ftd2xx.h WinTypes.h

INCDIR = -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/usr/local/include

LIBS = -lrt -lm -ldl -lpthread -lusb -lwiringPi -lbrcmGLESv2 -lbrcmEGL -lm -lbcm_host -L/opt/vc/lib -L/usr/local/lib
CFLAGS=./libftd2xx.a -lpthread -lrt
all:	sensing.bin doubleps.bin

sensing.bin:	$(S) $(C1D) $(H1D)
	$(CC) -o sensing.bin $(LIBS) $(C1D) $(INCDIR) $(CFLAGS)

doubleps.bin:  $(S) $(C2D) $(H1D)
	$(CC) -o doubleps.bin $(LIBS) $(C2D) $(INCDIR)


clean:
	rm -f *.bin
