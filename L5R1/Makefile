# ====== Konfiguracja ======
CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c17
LDFLAGS =
TARGET  = program
SRC     = main.c
OBJ     = $(SRC:.c=.o)

TARGET_FIFO = program_fifo
SRC_FIFO    = main_fifo.c
OBJ_FIFO    = $(SRC_FIFO:.c=.o)

# ====== Domyślny cel ======
all: $(TARGET) $(TARGET_FIFO)

# ====== Budowanie programu ======
$(TARGET): $(OBJ)
		$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(TARGET_FIFO): $(OBJ_FIFO)
		$(CC) $(OBJ_FIFO) -o $@ $(LDFLAGS)

# ====== Kompilacja plików .c do .o ======
%.o: %.c
		$(CC) $(CFLAGS) -c $< -o $@

# ====== Czyszczenie ======
clean:
		rm -f $(OBJ) $(OBJ_FIFO) $(TARGET) $(TARGET_FIFO)

# ====== Cele specjalne ======
.PHONY: all clean debug release