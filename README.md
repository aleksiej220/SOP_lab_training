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
