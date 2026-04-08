# ====== Konfiguracja ======
CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c17
# Flagi linkera (GNU ld) — poprawiają bezpieczeństwo bez zmiany kodu.
# Możesz dopisać własne: make LDFLAGS+=" ..."
LDFLAGS = -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--as-needed
LDLIBS  = -lm
TARGET  = program
SRC     = code.c
OBJ     = $(SRC:.c=.o)

# ====== Domyślny cel ======
all: $(TARGET)

# ====== Budowanie programu ======
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

# ====== Kompilacja plików .c do .o ======
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ====== Czyszczenie ======
clean:
	rm -f $(OBJ) $(TARGET)

# ====== Cele specjalne ======
.PHONY: all clean debug release