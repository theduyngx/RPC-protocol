### ------------------------- FLAGS, OBJECTS AND PATHS ------------------------- ###

# compiler flags# compiler flags
CC        = cc
C 	      = -c
O         = -o
INC       = -I$(INC_DIR)
GDB       = -g
CFLAGS    = -Wall
CFLAGS   += $(INC)
VALGRIND  = valgrind --leak-check=full --track-origins=yes

# client-server executable tags
SRV_C     = server.c
CLI_C     = client.c
SRV_A     = server.a
CLI_A     = client.a
SRV       = $(OUT_DIR)rpc-server
CLI       = $(OUT_DIR)rpc-client
SRV_DB    = $(OUT_DIR)server
CLI_DB    = $(OUT_DIR)client

# paths
CS_DIR    = client-server/
SRC_DIR   = src/
INC_DIR   = include/
OUT_DIR   = out/
CASES     = cases/

# object flags
SRC_OBJ   = $(patsubst $(SRC_DIR)%.c, $(OUT_DIR)%.o, $(wildcard $(SRC_DIR)*.c))
CLI_IN    = client.in
CLI1_IN   = client1.in
CLI2_IN   = client2.in
SRV_IN    = server.in
CLI_OUT   = client.out
SRV_OUT   = server.out
CLI1_OUT  = client1.out
CLI2_OUT  = client2.out
RPC_SYS_A = $(OUT_DIR)rpc.a



### ------------------------- STANDARD COMPILATION ------------------------- ###

# all / debug executables
all: $(RPC_SYS_A) $(SRV) $(CLI)

# client-server executables
$(SRV): $(CS_DIR)$(SRV_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)

$(CLI): $(CS_DIR)$(CLI_C)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)


# object files
$(OUT_DIR)%.o: $(SRC_DIR)%.c $(INC_DIR)%.h
	$(CC) $(CFLAGS) -c -o $@ $< $(GDB)

object:
	mkdir -p $(OUT_DIR:/=)


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
	rm -f $(SRV) $(CLI) $(SRV_DB) $(CLI_DB) *.o *.a
	rm -f -r $(OUT_DIR)



### ------------------------- DEBUGGING MODE ------------------------- ###

# all debug executables
debug: $(SRV_DB) $(CLI_DB)

# client-server libraries
$(SRV_DB): $(CS_DIR)$(SRV_A)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)

$(CLI_DB): $(CS_DIR)$(CLI_A)
	$(CC) $(CFLAGS) $< $(O) $@ $(RPC_SYS_A) $(GDB)

# debugging test cases
%-srv:
	./$(SRV_DB) < $(CASES)$*/$(SRV_IN)
%-cli:
	./$(CLI_DB) < $(CASES)$*/$(CLI_IN)
%-cli1:
	./$(CLI_DB) < $(CASES)$*/$(CLI1_IN)
%-cli2:
	./$(CLI_DB) < $(CASES)$*/$(CLI2_IN)

# check diff
%-diff-srv:
	make $*-srv > $(OUT_DIR)$(SRV_OUT)
	diff $(OUT_DIR)$(SRV_OUT) $(CASES)$*/$(SRV_OUT)
%-diff-cli:
	make $*-cli > $(OUT_DIR)$(CLI_OUT)
	diff $(OUT_DIR)$(CLI_OUT) $(CASES)$*/$(CLI_OUT)
%-diff-cli1:
	make $*-cli1 > $(OUT_DIR)$(CLI1_OUT)
	diff $(OUT_DIR)$(CLI1_OUT) $(CASES)$*/$(CLI1_OUT)
%-diff-cli2:
	make $*-cli2 > $(OUT_DIR)$(CLI2_OUT)
	diff $(OUT_DIR)$(CLI2_OUT) $(CASES)$*/$(CLI2_OUT)

# shortcut to clean, re-compile and debug
shortcut:
	make clean
	make
	make debug
	clear
