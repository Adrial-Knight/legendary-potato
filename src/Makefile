EXECSERV = server
EXECCLI = client
CC = gcc
CFLAGS = -Wall -g
OBJDIR = .obj
BINDIR = bin
OBJSERV = $(EXECSERV).o common.o server_request.o
OBJCLI = $(EXECCLI).o common.o client_command.o image.o
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all : $(OBJDIR) $(BINDIR) exec

exec : $(OBJ) $(OBJDIR) $(BINDIR)
	cd $(OBJDIR);\
	$(CC) $(CFLAGS) -o ../$(BINDIR)/$(EXECSERV) $(OBJSERV) -lm;\
	$(CC) $(CFLAGS) -o ../$(BINDIR)/$(EXECCLI) $(OBJCLI) -lm;\
	cd ..

%.o : %.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/$@ -c $<

$(OBJDIR) $(BINDIR) :
	mkdir $@

clean :
	rm -rf $(OBJDIR)

mrproper : clean
	rm -rf $(BINDIR)
