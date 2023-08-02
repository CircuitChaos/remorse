NAME	= remorse
OBJS	= remorse.o
SRCS	= $(OBJS:.o=.c)

CFLAGS	= -Wall -Wextra -O2
LDFLAGS	= -lm

$(NAME): $(OBJS)
	$(CC) -o $(NAME) $(OBJS) $(LDFLAGS)

$(OBJS): $(SRCS)

.PHONY: clean
clean:
	rm -f $(NAME) $(OBJS)
