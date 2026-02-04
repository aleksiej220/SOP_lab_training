# ====== Konfiguracja ======
CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c17
LDFLAGS =
TARGET  = program
SRC     = code.c
OBJ     = $(SRC:.c=.o)

# ====== Domyślny cel ======
all: $(TARGET)

# ====== Budowanie programu ======
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# ====== Kompilacja plików .c do .o ======
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ====== Czyszczenie ======
clean:
	rm -f $(OBJ) $(TARGET)

# ====== Cele specjalne ======
.PHONY: all clean debug release