all: game_process game_thread

game_process: game_process_group14.c
	gcc -o game_process game_process_code.c

game_thread: game_thread_group14.c
	gcc -o game_thread game_thread_code.c

clean:
	rm -f game_process game_thread
