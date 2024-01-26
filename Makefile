CC=clang

CFLAGS=-Wall -Wextra -Werror

RM := rm -f

SRCS := ft_ping.c

TESTS := main.cpp help_tests.cpp

TESTS := $(addprefix tests/, $(TESTS))

OBJS := $(addprefix obj/, ${SRCS:.c=.o})

INCLUDE := include/ft_ping.h

NAME := ft_ping

LIB := lib/libargparse.a lib/libft.a

all: $(NAME)

$(LIB):
	$(MAKE) -C ./libft
	$(MAKE) -C ./libargparse

$(NAME): $(OBJS) $(LIB)
	$(CC) $(CFLAGS) \
		$(OBJS) \
		-o $(NAME) \
		-Llibft \
		-Llibargparse \
		-lft \
		-largparse \
		-Wl,-R./libft

obj/%.o : src/%.c $(INCLUDE)
	$(CC) $(CFLAGS) $< -o $@ -c -I./include -I./libft -I./libargparse

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

# Run tests with gtest
test: $(LIB)
	@clang++ $(CFLAGS) \
		-Llibft \
		-Llibargparse \
		-Wl,-R./libft \
		-I./include \
		-I./libft \
		-I./libargparse \
		-pthread \
		-o tests/test \
		$(TESTS) \
		-lgtest \
		-largparse \
		-lft
	@./tests/test
	@$(RM) tests/test

re: fclean all

.PHONY: all clean fclean re
.SILENT: