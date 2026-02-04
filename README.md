# Ściąga z mechanizmów synchronizacji w C (POSIX)

## 1. Semafory (`<semaphore.h>`)
Wymaga linkowania: `-pthread` (lub `-lrt`).

### Semafory nienazwane (wątki / pamięć dzielona dla procesów)
Używane, gdy semafor jest w zmiennej globalnej lub pamięci dzielonej.

```c
sem_t sem;

/* Inicjalizacja: 
   pshared: 0 = między wątkami jednego procesu, 1 = między procesami
   value: wartość początkowa */
sem_init(&sem, 0, 1); 

sem_wait(&sem);     // Opuszczenie (P/Down) - czeka, jeśli value <= 0
sem_trywait(&sem);  // Nieblokujące opuszczenie (zwraca błąd, jeśli zablokowany)
sem_post(&sem);     // Podniesienie (V/Up) - inkrementuje value

sem_destroy(&sem);  // Zwolnienie zasobów
```

### Semafory nazwane (między procesami)
Identyfikowane przez nazwę (string zaczynający się od `/`).

```c
#include <fcntl.h>
#include <sys/stat.h>

/* Otwarcie/Utworzenie: nazwa, flagi, uprawnienia, wartość startowa */
sem_t *sem = sem_open("/moj_sem", O_CREAT, 0644, 1);
if (sem == SEM_FAILED) { /* obsługa błędu */ }

sem_close(sem);           // Zamknięcie deskryptora w procesie
sem_unlink("/moj_sem");   // Usunięcie semafora z systemu (ważne!)
```

---

## 2. Zmienne Warunkowe (`<pthread.h>`)
Służą do oczekiwania na spełnienie warunku. Zawsze używane w parze z **mutexem**.
Wymaga linkowania: `-pthread`.

```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* --- Wątek czekający (np. Konsument) --- */
pthread_mutex_lock(&mutex);
while (!warunek_spelniony) {
    /* Atomowo zwalnia mutex i czeka. 
       Po obudzeniu automatycznie zajmuje mutex ponownie. */
    pthread_cond_wait(&cond, &mutex);
}
// Wykonaj akcję na zasobie...
pthread_mutex_unlock(&mutex);


/* --- Wątek sygnalizujący (np. Producent) --- */
pthread_mutex_lock(&mutex);
// Zmień stan, który sprawia, że warunek staje się prawdziwy...
pthread_cond_signal(&cond);    // Obudź jeden czekający wątek
// LUB
pthread_cond_broadcast(&cond); // Obudź wszystkie czekające wątki
pthread_mutex_unlock(&mutex);


/* Czyszczenie (gdy nieużywane) */
pthread_cond_destroy(&cond);
pthread_mutex_destroy(&mutex);
```

---

## 3. Bariery (`<pthread.h>`)
Synchronizacja grupy wątków w jednym punkcie ("wszyscy czekają na wszystkich").
Wymaga linkowania: `-pthread`.
Może wymagać zdefiniowania: `#define _XOPEN_SOURCE 600` na początku pliku.

```c
pthread_barrier_t barrier;
int liczba_watkow = 5;

/* Inicjalizacja: &barrier, atrybuty, liczba wymaganych wątków */
pthread_barrier_init(&barrier, NULL, liczba_watkow);


/* --- W kodzie wszystkich wątków --- */
// Wykonaj pracę etap 1...

/* Wątek czeka w tym miejscu, aż 'liczba_watkow' dotrze do bariery */
int ret = pthread_barrier_wait(&barrier);

if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
    // Ten kod wykona się tylko w JEDNYM (losowym) wątku po zwolnieniu blokady
}

// Wykonaj pracę etap 2...


/* Czyszczenie */
pthread_barrier_destroy(&barrier);
```

---

## 4. Obsługa Plików (File I/O)

### Niskopoziomowe (System Calls - POSIX)
Wymaga nagłówków: `<unistd.h>`, `<fcntl.h>`, `<sys/stat.h>`, `<sys/types.h>`.
Operuje na deskryptorach (int).

```c
int fd;

/* Otwarcie pliku: ścieżka, flagi, uprawnienia (tylko przy tworzeniu) */
// Flagi: O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, O_APPEND
fd = open("plik.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
if (fd == -1) { perror("open"); }

char buf[100];
ssize_t bytes_read, bytes_written;

/* Odczyt (deskryptor, bufor, ile bajtów) */
bytes_read = read(fd, buf, sizeof(buf)); // Zwraca ilość przeczytanych (0 = EOF)

/* Zapis (deskryptor, bufor, ile bajtów) */
bytes_written = write(fd, "Dane", 4);

/* Przesuwanie wskaźnika (lseek) */
// SEEK_SET (pocz.), SEEK_CUR (obec.), SEEK_END (koniec)
off_t pos = lseek(fd, 0, SEEK_SET); 

close(fd);  // Zamknięcie
```

### Wysokopoziomowe (Standard C Library)
Wymaga nagłówka: `<stdio.h>`.
Operuje na wskaźnikach `FILE *`. Zapewnia buforowanie.

```c
FILE *fp;

/* Otwarcie: ścieżka, tryb ("r", "w", "a", "r+", "w+", "a+") */
fp = fopen("plik.txt", "w");
if (fp == NULL) { perror("fopen"); }

char buffer[100];
size_t n;

/* Odczyt binarny/blokowy */
n = fread(buffer, sizeof(char), 100, fp);

/* Zapis binarny/blokowy */
fwrite(buffer, sizeof(char), n, fp);

/* Odczyt/Zapis sformatowany */
fprintf(fp, "Liczba: %d\n", 10);
fscanf(fp, "%d", &n);

/* Odczyt linia po linii */
fgets(buffer, sizeof(buffer), fp); // Zwraca NULL na EOF/błąd

/* Pozycjonowanie */
fseek(fp, 0, SEEK_SET);
long pos = ftell(fp);
rewind(fp);

fclose(fp); // Zamknięcie
```

---

## 5. Sygnały (Signals) `<signal.h>`
Wymaga nagłówków: `<signal.h>`, `<unistd.h>`, `<sys/types.h>`.

### Podstawowe Sygnały
- `SIGINT` (Ctrl+C)
- `SIGTERM` (Domyślny sygnał kończenia)
- `SIGKILL` (Wymuszone zabicie, nieprzechwytywalny)
- `SIGUSR1`, `SIGUSR2` (Do użytku programisty)
- `SIGCHLD` (Zmiana stanu procesu potomnego)

### Wysyłanie sygnałów

```c
/* Do innego procesu */
kill(pid, SIGUSR1); 

/* Do samego siebie */
raise(SIGINT);
```

### Obsługa sygnałów (sigaction - zalecane)

```c
void handler(int signum) {
    // UWAGA: Używaj tylko funkcji "async-signal-safe" (np. write, nie printf)
    write(STDOUT_FILENO, "Sygnał!\n", 8);
}

struct sigaction sa;
sa.sa_handler = handler;      // Funkcja obsługi
sigemptyset(&sa.sa_mask);     // Maska sygnałów blokowanych PODCZAS obsługi
sa.sa_flags = 0;              // Flagi (np. SA_RESTART)

/* Rejestracja obsługi */
if (sigaction(SIGUSR1, &sa, NULL) == -1) { perror("sigaction"); }

/* Stara metoda (niezalecana w nowym kodzie) */
signal(SIGINT, handler); 
```

### Blokowanie (Maskowanie) sygnałów

```c
sigset_t set, oldset;

sigemptyset(&set);            // Pusty zbiór
sigaddset(&set, SIGUSR1);     // Dodaj SIGUSR1 do zbioru
sigfillset(&set);             // Wszystkie sygnały

/* Blokowanie sygnałów ze zbioru 'set' */
// SIG_BLOCK (suma), SIG_UNBLOCK (różnica), SIG_SETMASK (przypisanie)
sigprocmask(SIG_BLOCK, &set, &oldset);

/* SEKCJA KRYTYCZNA - sygnał SIGUSR1 nie przerwie działania tutaj */

/* Przywrócenie starej maski */
sigprocmask(SIG_SETMASK, &oldset, NULL);
```

### Oczekiwanie na sygnał

```c
pause(); // Czeka na dowolny sygnał (który nie jest ignorowany)

/* sigsuspend - atomowo zmienia maskę i czeka */
sigset_t mask;
sigfillset(&mask);
sigdelset(&mask, SIGUSR1);
sigsuspend(&mask); // Czeka tylko na SIGUSR1
```
