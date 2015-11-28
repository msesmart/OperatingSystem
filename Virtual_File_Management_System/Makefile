CC = gcc
LL = -lpthread
C_FILES = myApp.c p6cyuan.c disk.c
p3cyuan: $(C_FILES)
	$(CC) $(C_FILES) $(LL) -o p6cyuan

clean:
	rm $(EX_NAME)

$(C_FILES):
	$(CC) -c $(C_FILES)
