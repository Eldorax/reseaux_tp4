echoserveur.exe: echoserveur.c
	gcc -Wall echoserveur.c -o echoserveur.exe

clean:
	rm -f *.exe *~
