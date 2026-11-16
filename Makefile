CC      = gcc
CFLAGS  = -Wall -Wextra -g -I. -Iheader
LDLIBS  = -pthread -lrt
HDR     = $(wildcard header/*.h)

# αντικείμενα κάθε εκτελέσιμου
START_OBJS = build/start.o build/ipc.o build/checksum.o build/enc.o build/chan.o
PROC_OBJS  = build/proc.o build/ipc.o

all: start proc

start: $(START_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

proc: $(PROC_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c $(HDR) | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build start proc

.PHONY: all clean