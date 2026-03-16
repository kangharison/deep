CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE
LDFLAGS =

TARGETS = nvme_trace_reader

READER_SRCS = nvme_trace_reader.c nvme_trace_decoder.c
READER_HDRS = nvme_trace_decoder.h

.PHONY: all clean

all: $(TARGETS)

nvme_trace_reader: $(READER_SRCS) $(READER_HDRS)
	$(CC) $(CFLAGS) -o $@ $(READER_SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)
