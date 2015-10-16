CC = gcc
LL = -lpthread
C_FILES = merge_sort_pthread_barrier.c

p3cyuan: $(C_FILES)
	$(CC) $(C_FILES) $(LL) -o merge_sort_pthread_barrier

clean:
	rm $(EX_NAME)

$(C_FILES):
	$(CC) -c $(C_FILES)
