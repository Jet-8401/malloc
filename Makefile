ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

LIBNAME = libft_malloc
LINK = $(LIBNAME).so
NAME = $(LIBNAME)_$(HOSTTYPE).so
CFLAGS = -std=gnu11 -Wall -Werror -Wextra -fPIC -g -O0
# -O3

SRCS = defines.c utils.c show_alloc_mem.c malloc.c free.c realloc.c calloc.c
OBJS = $(SRCS:.c=.o)

LIBS = libft
LIBS_NAME = libft/libft.a

all: $(LIBS) $(NAME)

$(LIBS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(NAME): $(OBJS)
	$(CC) -shared -o $(NAME) $(OBJS) $(LIBS_NAME)
	@ln -sf $(NAME) $(LINK)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: $(LIBS)
	rm -f $(OBJS)

fclean: $(LIBS) clean
	rm -f $(NAME)
	rm -f $(LINK)

re: fclean all

test: $(NAME)
	$(CC) $(CFLAGS) -o test_malloc test_malloc.c -L. -lft_malloc -lpthread
	@echo "\n$(shell tput bold)$(shell tput setaf 2)Test binary created! Run with: LD_LIBRARY_PATH=. ./test_malloc$(shell tput sgr0)\n"

run_test: test
	@LD_LIBRARY_PATH=. ./test_malloc --enable-dangerous

.PHONY: all clean fclean re test run_test $(LIBS)
