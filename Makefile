ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

LIBNAME = libft_malloc
LINK = $(LIBNAME).so
NAME = $(LIBNAME)_$(HOSTTYPE).so
CFLAGS = -std=gnu11 -Wall -Werror -Wextra -fPIC -g -O0
# -O3

SRCS =	malloc.c
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) -shared -o $(NAME) $(OBJS)
	@ln -sf $(NAME) $(LINK)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)
	rm -f $(LINK)

re: fclean all

.PHONY: all clean fclean re
