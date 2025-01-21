# Ul - Bee Colony Simulation Project

Rój pszczół liczy początkowo N osobników. W danym momencie czasu w ulu może znajdować się co
najwyżej P (P<N/2) pszczół. Pszczoła, która chce dostać się do wnętrza ula, musi przejść przez jedno
z dwóch istniejących wejść. Wejścia te są bardzo wąskie, więc możliwy jest w nich jedynie ruch w
jedną stronę w danej chwili czasu. Zbyt długotrwałe przebywanie w ulu grozi przegrzaniem, dlatego
każda z pszczół instynktownie opuszcza ul po pewnym skończonym czasie Ti. Znajdująca się w ulu
królowa, co pewien czas Tk składa jaja, z których wylęgają się nowe pszczoły. Aby jaja zostały złożone
przez królową, w ulu musi istnieć wystarczająca ilość miejsca. (1 jajo liczone jest tak jak 1 dorosły
osobnik).
Pszczelarz może dołożyć (sygnał1) dodatkowe ramki do ula, które pozwalają zwiększyć liczbę
pszczół maksymalnie do 2*N osobników. Może również wyjąć (sygnał2) ramki ograniczając bieżącą
maksymalną liczbę osobników o 50%.
Napisz program pszczelarza, pszczoły “robotnicy” i królowej, tak by zasymulować cykl życia roju
pszczół. Każda z pszczół “robotnic” umiera po pewnym określonym czasie Xi, liczonym ilością
odwiedzin w ulu.


This project is a simulation of a bee colony, utilizing multithreading and process synchronization mechanisms in operating systems. Key features include:
- Management of hive entrances and exits.
- Handling processes: beekeeper, hive, queen.
- Worker bee threads operating inside and outside the hive.
- Synchronization using mutexes, semaphores, and conditional variables.
- Signal handling (SIGUSR1, SIGUSR2, SIGINT) for managing the bee population.
- FIFO mechanism for communication between the hive and beekeeper processes.

### Project Structure

1. Main Processes:
   - Queen: Responsible for increasing the population by laying eggs.
   - Hive: Manages hive access, synchronizes the work of bees, and handles depopulation.
   - Beekeeper: Monitors the hive's status, sends signals to increase or decrease the bee population, and receives the hive's PID via FIFO.

2. Shared Memory:
   - Stores data on the current number of bees, maximum number of bees, and the depopulation flag.

3. FIFO Mechanism:
   - Used to transfer the hive's PID to the beekeeper process.

4. Message queue:
   - Used to communicate the number of eggs laid by the queen to the hive process.

6. Logging:
   - Colored logs make it easier to monitor the process:
     - Process start: Various background colors.
     - Process termination: Red background.
     - Queen lays eggs: Yellow text.
     - Bee enters the hive: Green text.
     - Bee exits the hive: Blue text.
     - Signals sent by the beekeeper: Purple text.

### How to Run

1. Clone the code to a local repository.
2. Compile and run using the `Makefile`:
   
   ![image](https://github.com/user-attachments/assets/b5d110df-edbd-4141-a031-0bd93a3b7619)

4. To clean up compiled files, use:
   
   ![image](https://github.com/user-attachments/assets/d98702e9-790a-485f-a07d-4ecabee70d82)


### Features

- Queen Laying Eggs: Randomly increases the number of bees in the hive, considering available space.
- Depopulation: Gradually decreases the number of bees when the hive exceeds its capacity or upon receiving a signal.
- Signal Handling:
  - SIGUSR1: Increases the maximum number of bees (triggered by entering 1).
  - SIGUSR2: Triggers hive depopulation (triggered by entering 2).
  - SIGINT: Terminates all processes.
- Logs tracking: Keeping logs from simulation in logs file.

This project demonstrates the practical use of synchronization mechanisms in operating systems and the handling of multiple processes and threads.

