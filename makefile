all: server participant observer
server:
	gcc -g prog3_server.c -o server
participant:
	gcc -g prog3_participant.c -o participant
observer:
	gcc -g prog3_observer.c -o observer
clean:
	rm server participant observer
