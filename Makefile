# compiler flags# compiler flags
CC      = gcc
INC     = -Iinclude/
GDB     = -g
CFLAGS  = -Wall
CFLAGS += $(INC)
COMPILE = -c -o

# client-server executable tags
SRV     = server
CLI     = client
SRV_C   = server.c
CLI_C   = client.c

# paths
CLI_SRV = client-server
SRC_DIR = src
OUT_DIR = out
RPC_SYS = rpc.o
SYS_REQ = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.h)


# RPC executable
all: $(RPC_SYS)

$(RPC_SYS): $(SYS_REQ)
	$(CC) $(CFLAGS) $(COMPILE) $@ $<

# client-server executables
$(SRV): $(CLI_SRV)/$(SRV_C) $(SYS_REQ)
	$(CC) $(CFLAGS) $(COMPILE) $@ $< $(GDB)

$(CLI): $(CLI_SRV)/$(CLI_C) $(SYS_REQ)
	$(CC) $(CFLAGS) $(COMPILE) $@ $< $(GDB)


# object files
%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

# formatting
.PHONY: format all
format:
	clang-format -style=file -i *.c *.h

# RPC_SYS_A = rpc.a
# $(RPC_SYS_A): rpc.o
# 	ar rcs $(RPC_SYS_A) $(RPC_SYS)

# clean
clean:
	rm -f $(EXE) $(EXE_PRC) $(SRC_DIR)/*.o $(OUT_DIR)/*.out *.o *.out
