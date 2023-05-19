### ------------------------- FLAGS, OBJECTS AND PATHS ------------------------- ###

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
SRV_C   = server.c
CLI_C   = client.c
SRV_DB  = server
CLI_DB  = client
SRV_A   = server.a
CLI_A   = client.a

# paths
CLI_SRV = client-server/
SRC_DIR = src/
INC_DIR = include/
OUT_DIR = out
CASES   = cases/

# object flags
RPC_SYS_A = rpc.a
SRC_OBJ = $(patsubst $(SRC_DIR)%.c, $(OUT_DIR)/%.o, $(wildcard $(SRC_DIR)*.c))
CLI_IN  = client.in
SRV_IN  = server.in
CLI_OUT = client.out
SRV_OUT = server.out



### ------------------------- STANDARD COMPILATION ------------------------- ###

# all / debug executables
all: $(RPC_SYS_A) $(SRV) $(CLI)

# client-server executables
$(SRV): $(SRV_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)

$(CLI): $(CLI_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)


# object files
$(OUT_DIR)/%.o: $(SRC_DIR)%.c $(INC_DIR)%.h | object
	$(CC) $(CFLAGS) -c -o $@ $<

object:
	mkdir $(OUT_DIR)


# permitting executable
permit:
	chmod +x $(SRV); chmod +x $(CLI)

# formatting
.PHONY: format all
format:
	clang-format -style=file -i *.c *.h

# RPC library
$(RPC_SYS_A): object | $(SRC_OBJ)
	ar rcs $@ $(SRC_OBJ)

# clean
clean:
	rm -f $(SRV) $(CLI) $(SRV_DB) $(CLI_DB) $(SRC_DIR)*.o *.o *.out *.a
	rm -f -r $(OUT_DIR)



### ------------------------- DEBUGGING MODE ------------------------- ###

# all executables
debug: $(SRV_DB) $(CLI_DB)

# client-server executables
$(SRV_DB): $(CLI_SRV)$(SRV_A)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)

$(CLI_DB): $(CLI_SRV)$(CLI_A)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)



### ------------------------- DEBUGGING TEST CASES ------------------------- ###

1+1-srv:
	./$(SRV_DB) < $(CASES)1+1/$(SRV_IN)
1+1-cli:
	./$(CLI_DB) < $(CASES)1+1/$(CLI_IN)

127+127-srv:
	./$(SRV_DB) < $(CASES)127+127/$(SRV_IN)
127+127-cli:
	./$(CLI_DB) < $(CASES)127+127/$(CLI_IN)

abc-srv:
	./$(SRV_DB) < $(CASES)abc/$(SRV_IN)
abc-cli:
	./$(CLI_DB) < $(CASES)abc/$(CLI_IN)

bad.client-srv:
	./$(SRV_DB) < $(CASES)bad.client/$(SRV_IN)
bad.client-cli:
	./$(CLI_DB) < $(CASES)bad.client/$(CLI_IN)

bad.server-srv:
	./$(SRV_DB) < $(CASES)bad.server/$(SRV_IN)
bad.server-cli:
	./$(CLI_DB) < $(CASES)bad.server/$(CLI_IN)

block1-srv:
	./$(SRV_DB) < $(CASES)block1/$(SRV_IN)
block1-cli1:
	./$(CLI_DB) < $(CASES)block1/client1.in
block1-cli2:
	./$(CLI_DB) < $(CASES)block1/client2.in

block2-srv:
	./$(SRV_DB) < $(CASES)block2/$(SRV_IN)
block2-cli1:
	./$(CLI_DB) < $(CASES)block2/client1.in
block2-cli2:
	./$(CLI_DB) < $(CASES)block2/client2.in

call2-srv:
	./$(SRV_DB) < $(CASES)call2/$(SRV_IN)
call2-cli:
	./$(CLI_DB) < $(CASES)call2/$(CLI_IN)

call-twice-srv:
	./$(SRV_DB) < $(CASES)call-twice/$(SRV_IN)
call-twice-cli:
	./$(CLI_DB) < $(CASES)call-twice/$(CLI_IN)

missing-srv:
	./$(SRV_DB) < $(CASES)missing/$(SRV_IN)
missing-cli:
	./$(CLI_DB) < $(CASES)missing/$(CLI_IN)

switch1-srv:
	./$(SRV_DB) < $(CASES)switch1/$(SRV_IN)
switch1-cli:
	./$(CLI_DB) < $(CASES)switch1/$(CLI_IN)

endian1-srv:
	./$(SRV_DB) < $(CASES)endian1/$(SRV_IN)
endian1-cli:
	./$(CLI_DB) < $(CASES)endian1/$(CLI_IN)

endian2-srv:
	./$(SRV_DB) < $(CASES)endian2/$(SRV_IN)
endian2-cli:
	./$(CLI_DB) < $(CASES)endian2/$(CLI_IN)

endian3-srv:
	./$(SRV_DB) < $(CASES)endian3/$(SRV_IN)
endian3-cli:
	./$(CLI_DB) < $(CASES)endian3/$(CLI_IN)


shortcut:
	make clean
	make
	make debug
	make block2-srv
