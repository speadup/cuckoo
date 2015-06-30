EXEC = ated
ATED_OBJS = ated.o

all: $(EXEC)

ated : $(ATED_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(ATED_OBJS)

clean:
	-rm -f $(EXEC) *.o