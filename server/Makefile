NAME			:= coap-server
CC				:= gcc
CPPFLAGS	:= -Iinclude
CFLAGS		:= -Wall -Wextra -Werror
LDFLAGS  	:= -lsqlite3
SRCS      := main.c db.c
OBJS 			:= $(SRCS:.c=.o)
DEPS 			:= $(SRCS:.c=.d)
OBJDIR 		:= obj
DEPDIR 		:= dep
SUFFIXES	+= .d

vpath %.c src
vpath %.o $(OBJDIR)
vpath %.d $(DEPDIR)

all: $(NAME)

$(NAME): $(DEPS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(addprefix $(OBJDIR)/, $(OBJS)) $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $(OBJDIR)/$@

%.d: %.c
	$(CC) $(CPPFLAGS) -MM -MF $(DEPDIR)/$@ $<

ifneq "$(MAKECMDGOALS)" "clean"
 -include $(addprefix $(DEPDIR)/, $(DEPS))
endif

debug: CFLAGS += -g -DDEBUG=1
debug: $(NAME)

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(DEPS): | $(DEPDIR)

$(DEPDIR):
	mkdir -p $(DEPDIR)

clean:
	rm -rf $(OBJDIR) $(DEPDIR)

fclean: clean
	$(RM) -rf $(NAME)

re: fclean all

.PHONY: all debug clean fclean re
