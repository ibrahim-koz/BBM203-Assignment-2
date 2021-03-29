OBJS	= sellticket.o
SOURCE	= sellticket.c
HEADER	= 
OUT	= sellticket
CC	 = gcc
FLAGS	 = -g -c -Wall -std=gnu11
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

sellticket.o: sellticket.c
	$(CC) $(FLAGS) sellticket.c 


clean:
	rm -f $(OBJS) $(OUT)