# compiler flags# compiler flags
CC      = gcc
CMP 	= -c
OBJ	    = -o
INC     = -I$(INC_DIR)
GDB     = -g
CFLAGS  = -Wall
CFLAGS += $(INC)

# client-server executable tags
SRV     = rpc-server
CLI     = rpc-client
SRV_C   = server.c
CLI_C   = client.c

# paths
CLI_SRV = client-server
SRC_DIR = src
INC_DIR = include/
OUT_DIR = out
RPC_SYS = rpc.o
SYS_REQ = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(INC_DIR)*.h)


# all executables
all: $(RPC_SYS) $(SRV) $(CLI)

# RPC executable
$(RPC_SYS): $(SYS_REQ)
	$(CC) $(CFLAGS) $(CMP) $(OBJ) $@ $<

# client-server executables
$(SRV): $(CLI_SRV)/$(SRV_C)
	$(CC) $(CFLAGS) $< $(SYS_REQ) $(OBJ) $@ $(GDB)

$(CLI): $(CLI_SRV)/$(CLI_C)
	$(CC) $(CFLAGS) $< $(SYS_REQ) $(OBJ) $@ $(GDB)


# object files
%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

# permitting executable
permit:
	chmod +x $(SRV); chmod +x $(CLI)

# formatting
.PHONY: format all
format:
	clang-format -style=file -i *.c *.h

# RPC_SYS_A = rpc.a
# $(RPC_SYS_A): rpc.o
# 	ar rcs $(RPC_SYS_A) $(RPC_SYS)

# clean
clean:
	rm -f $(SRV) $(CLI) $(SRC_DIR)/*.o $(OUT_DIR)/*.out *.o *.out
