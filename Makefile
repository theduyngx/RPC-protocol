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
CASES   = cases/

# object flags
RPC_SYS_A = rpc.a
SYS_REQ = $(SRC_DIR)rpc.c $(INC_DIR)rpc.h
SRC_OBJ = $(patsubst $(SRC_DIR)%.c, $(OUT_DIR)%.o, $(wildcard $(SRC_DIR)*.c))
CLI_IN  = client.in
SRV_IN  = server.in
CLI_OUT = client.out
SRV_OUT = server.out


# all executables
all: $(RPC_SYS_A) $(SRV) $(CLI)

# client-server executables
$(SRV): $(CLI_SRV)$(SRV_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)

$(CLI): $(CLI_SRV)$(CLI_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)


# object files
$(OUT_DIR)%.o: $(SRC_DIR)%.c $(INC_DIR)%.h
	$(CC) $(CFLAGS) -c -o $@ $<

# permitting executable
permit:
	chmod +x $(SRV); chmod +x $(CLI)

# formatting
.PHONY: format all
format:
	clang-format -style=file -i *.c *.h

# RPC library
$(RPC_SYS_A): $(SRC_OBJ)
	ar rcs $@ $(SRC_OBJ)

# clean
clean:
	rm -f $(SRV) $(CLI) $(SRC_DIR)*.o $(OUT_DIR)*.o $(OUT_DIR)*.out *.o *.out *.a


# test cases
opo-srv:
	./$(SRV) < $(CASES)1+1/$(SRV_IN)
opo-cli:
	./$(CLI) < $(CASES)1+1/$(CLI_IN)

hph-srv:
	./$(SRV) < $(CASES)127+127/$(SRV_IN)
hph-cli:
	./$(CLI) < $(CASES)127+127/$(CLI_IN)

abc-srv:
	./$(SRV) < $(CASES)abc/$(SRV_IN)
abc-cli:
	./$(CLI) < $(CASES)abc/$(CLI_IN)
