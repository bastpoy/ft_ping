CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -Iinclude
LDFLAGS = -lm

NAME = ft_ping

SRCDIR = src
OBJDIR = obj
BINDIR = .
BIN = $(BINDIR)/$(NAME)

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

all: $(NAME)

$(BIN): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -f $(OBJDIR)/*.o

fclean: clean
	rm -f $(BIN)
	rm -rf $(OBJDIR)

re: fclean all

.PHONY: all clean fclean re