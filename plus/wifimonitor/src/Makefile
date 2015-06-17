TARGET := wifimonitor
CC := gcc
LIBS := -lpcap -lsqlite3

SRCS := $(wildcard *.c ./sqlite/*.c)

OBJS := $(patsubst %.c, %.o, $(SRCS))
RM := rm

$(TARGET) : clean $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -I. -c $< -o $@


clean: 
	$(RM) -f $(OBJS) $(TARGET)
