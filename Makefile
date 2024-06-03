CC=clang

CFLAGS= -Wall -Wextra -g -std=gnu99 -fsanitize=leak,address,undefined

RM := rm -f

SRCS := ft_ping.c main.c utils.c

TESTS := main.cpp tests_utils.cpp

TESTS := $(addprefix tests/, $(TESTS))

OBJS := $(addprefix obj/, ${SRCS:.c=.o})

INCLUDE := include/ft_ping.h

NAME := ft_ping

LIBARGPARSE_VERSION = 3.1.1

LIBARGPARSE_URL = https://github.com/Tlafay1/libargparse/releases/download/v$(LIBARGPARSE_VERSION)/libargparse-$(LIBARGPARSE_VERSION).tar.gz

LIBARGPARSE_NAME = libargparse-$(LIBARGPARSE_VERSION)

all: $(NAME)

libs: libft libargparse

libft:
	$(MAKE) -C ./libft

libargparse: $(LIBARGPARSE_NAME) $(LIBARGPARSE_NAME)/configure
	$(MAKE) -C ./$(LIBARGPARSE_NAME)

$(LIBARGPARSE_NAME)/configure:
	cd $(LIBARGPARSE_NAME) && ./configure

$(LIBARGPARSE_NAME):
	[ -d "./$(LIBARGPARSE_NAME)" ] || \
		(curl $(LIBARGPARSE_URL) -L -o $(LIBARGPARSE_NAME).tar.gz; \
		tar -xf $(LIBARGPARSE_NAME).tar.gz; \
		$(RM) $(LIBARGPARSE_NAME).tar.gz)

$(NAME): libs $(OBJS)
	$(CC) $(CFLAGS) \
		$(OBJS) \
		-o $(NAME) \
		-Llibft \
		-L $(LIBARGPARSE_NAME)/lib \
		-lft \
		-largparse \
		-Wl,-R./libft

obj/%.o : src/%.c $(INCLUDE)
	mkdir -p obj
	$(CC) $(CFLAGS) $< -o $@ -c -I./include -I./libft -I./$(LIBARGPARSE_NAME)/include

clean :
	$(MAKE) -C ./libft $@
	$(MAKE) -C ./$(LIBARGPARSE_NAME) clean
	$(RM) $(OBJS)

fclean : clean
	$(MAKE) -C ./libft $@
	$(RM) $(NAME)

distclean: fclean
	$(RM) -rf $(LIBARGPARSE_NAME)
	$(RM) config.log config.status

test: tests/test

# Run tests with gtest
tests/test: libs $(OBJS)
	g++ $(CFLAGS) \
		$(filter-out obj/main.o, $(OBJS)) \
		$(TESTS) \
		-o tests/test \
		-Llibft \
		-L $(LIBARGPARSE_NAME)/lib \
		-Wl,-R./libft \
		-I./include \
		-I./libft \
		-I./$(LIBARGPARSE_NAME)/include \
		-pthread \
		-lgtest \
		-largparse \
		-lft \
		-no-pie
	./tests/test
	$(RM) tests/test

re: fclean all

.PHONY : all \
	re \
	libs \
	libft \
	clean \
	fclean \
	libargparse \
	$(LIBARGPARSE_NAME)/configure
.SILENT: