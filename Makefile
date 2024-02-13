CC=clang

CFLAGS=-Wall -Wextra -Werror

RM := rm -f

SRCS := ft_ping.c main.c

TESTS := main.cpp help_tests.cpp

TESTS := $(addprefix tests/, $(TESTS))

OBJS := $(addprefix obj/, ${SRCS:.c=.o})

INCLUDE := include/ft_ping.h

NAME := ft_ping

all: $(NAME)

libs:
	$(MAKE) -C ./libft
	$(MAKE) -C ./libargparse

$(NAME): libs $(OBJS)
	$(CC) $(CFLAGS) \
		$(OBJS) \
		-o $(NAME) \
		-Llibft \
		-L libargparse/lib \
		-lft \
		-largparse \
		-Wl,-R./libft

obj/%.o : src/%.c $(INCLUDE)
	$(CC) $(CFLAGS) $< -o $@ -c -I./include -I./libft -I./libargparse/include

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

# Run tests with gtest
test: libs $(OBJS)
	clang++ $(CFLAGS) \
		$(OBJS) \
		$(TESTS) \
		-o tests/test \
		-Llibft \
		-Llibargparse \
		-Wl,-R./libft \
		-I./include \
		-I./libft \
		-I./libargparse \
		-pthread \
		-lgtest \
		-largparse \
		-lft
	./tests/test
	$(RM) tests/test

re: fclean all

.PHONY: all clean fclean re
.SILENT: