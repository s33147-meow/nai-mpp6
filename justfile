speed:
    nasm -f elf64 bruteforce.asm
    gcc backp.c bruteforce.o -std=c11 -no-pie -march=native -Ofast -DMAIN=main_brute -o backp.brute
    gcc backp.c bruteforce.o -std=c11 -no-pie -march=native -Ofast -DMAIN=main_heur -o backp.heur
    rm bruteforce.o

dbgs:
    nasm -f elf64 bruteforce.asm
    gcc backp.c bruteforce.o -g -fsanitize=address -std=c11 -no-pie -march=native -DMAIN=main_brute -o backp.brute
    gcc backp.c bruteforce.o -g -fsanitize=address -std=c11 -no-pie -march=native -DMAIN=main_heur -o backp.heur
    rm bruteforce.o
