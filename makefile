CFLAGS = -std=c99 -Wextra -Wall -Werror
EXEC = tty-bat
LINKS = -lncurses

all: $(EXEC)

$(EXEC): ttybat.c
	$(CC) $(CFLAGS) ttybat.c -o $(EXEC) $(LINKS)

clean:
	@$(RM) tty-bat

