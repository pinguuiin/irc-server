BRED=\033[1;31m
BGREEN=\033[1;32m
BYELLOW=\033[1;33m
BBLUE=\033[1;34m
BPURPLE=\033[1;35m
RESET_COLOR=\033[0m

NAME = ircserv
CC = c++
FLAGS = -Wall -Wextra -Werror -std=c++20
HEADERS = -I./include

SRC_DIR = src
OBJ_DIR = obj

SRCS = main.cpp \
	   Server.cpp \
	   Client.cpp \
	   Commands.cpp

OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

.SECONDARY: $(OBJS)

all: $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) $(HEADERS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(NAME)
	@echo "$(BGREEN)Compiled and linked.$(RESET_COLOR)"

clean:
	rm -rf $(OBJ_DIR)
	@echo "$(BBLUE)All object files cleaned.$(RESET_COLOR)"

fclean: clean
	rm -f $(NAME)
	@echo "$(BPURPLE)All cleaned up.$(RESET_COLOR)"

re: fclean all

.PHONY: all clean fclean re
