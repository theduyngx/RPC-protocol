# compiler flags# compiler flags
CC      = cc
C 	    = -c
O       = -o
INC     = -I$(INC_DIR)
GDB     = -g
CFLAGS  = -Wall
CFLAGS += $(INC)

# client-server executable tags
SRV     = rpc-server
CLI     = rpc-client
SRV_C   = server.a
CLI_C   = client.a

# paths
CLI_SRV = client-server/
SRC_DIR = src/
INC_DIR = include/
OUT_DIR = out/

# object flags
RPC_SYS_A = rpc.a
RPC_SYS = rpc.o
SYS_REQ = $(SRC_DIR)rpc.c $(INC_DIR)rpc.h
SRC_OBJ = $(filter-out $(RPC_SYS), $(patsubst $(SRC_DIR)%.c, %.o, $(wildcard $(SRC_DIR)*.c)))


# all executables
all:
	$(RPC_SYS_A) $(SRV) $(CLI)

# RPC executable
$(RPC_SYS): $(SYS_REQ)
	$(CC) $(CFLAGS) $(C) $(O) $@ $<

# client-server executables
$(SRV): $(CLI_SRV)$(SRV_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)

$(CLI): $(CLI_SRV)$(CLI_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)


# object files
%.o: $(SRC_DIR)%.c $(INC_DIR)%.h
	$(CC) $(CFLAGS) -c -o $@ $<

# permitting executable
permit:
	chmod +x $(SRV); chmod +x $(CLI)

# formatting
.PHONY: format all
format:
	clang-format -style=file -i *.c *.h

# static library
$(RPC_SYS_A): $(RPC_SYS) $(SRC_OBJ)
	ar rcs $(RPC_SYS_A) $(RPC_SYS) $(SRC_OBJ)

# clean
clean:
	rm -f $(SRV) $(CLI) $(SRC_DIR)*.o $(OUT_DIR)*.out *.o *.out
