CC      := gcc
CFLAGS  := -O2 -Wall -Wno-deprecated-declarations
LDFLAGS :=
LIBS    := -larchive

SRC:=$(wildcard *.c)

TARGET := container

OBJ := $(patsubst %.c, %.o, \
		$(filter %.c, $(SRC)))
all: $(TARGET)

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJ)
	@$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	@rm -f $(OBJ) $(TARGET) 

cleantar: 
	@rm -rf bin/ dev/ etc/ home/ lib lib64 linuxrc mnt/ media/ opt/ proc/ root run/ sys/ sbin/ usr tmp var

.PHONY: all clean
