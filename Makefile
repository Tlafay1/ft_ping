CC=gcc

CFLAGS= -Wall -Wextra -std=gnu99 -g

RM := rm -f

SRCS := ft_ping.c main.c utils.c init.c print.c stats.c icmp.c

TESTS := tests_utils.cpp tests_icmp.cpp

TESTS := $(addprefix tests/, $(TESTS))

OBJS := $(addprefix obj/, ${SRCS:.c=.o})

INCLUDE := include/ft_ping.h

NAME := ft_ping

LIBARGPARSE_VERSION = 4.0.0

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
		(curl $(LIBARGPARSE_URL) -L -o $(LIBARGPARSE_NAME).tar.gz && \
		tar -xf $(LIBARGPARSE_NAME).tar.gz && \
		$(RM) $(LIBARGPARSE_NAME).tar.gz)

$(NAME): libs $(OBJS)
	$(CC) $(CFLAGS) \
		$(OBJS) \
		-o $(NAME) \
		-Llibft \
		-L $(LIBARGPARSE_NAME)/lib \
		-lm \
		-lft \
		-largparse \
		-Wl,-R./libft
	sudo chown root:root $(NAME)
	sudo chmod u+s $(NAME)

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
		-lm \
		-lgtest \
		-lgtest_main \
		-largparse \
		-lft \
		-no-pie
	valgrind --leak-check=full --show-leak-kinds=all ./tests/test
	$(RM) tests/test

re: fclean all

.PHONY : all \
	re \
	libs \
	tests \
	libft \
	clean \
	fclean \
	libargparse \
	$(LIBARGPARSE_NAME)/configure
.SILENT: