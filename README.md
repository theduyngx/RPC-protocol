# RPC-protocol description

Preface
-------------
The Remote Procedure Protocol provides an ease of use interface for the client to call a function
as if locally, but is actually making a request to the server. It also provides an interface for the
server to handle the requests and provides responses to the clients. The provided program is a written
RPC protocol API library.


Transport Layer Protocol: TCP on IPv6
-------------
The RCP protocol uses TCP, mainly for reliability. This ensures that the data packets are as reliably
transmitted as possible. Specifically, TCP ensures that IP layer packet loss and duplication are
appropriately handled. Moreover, it also handles the restriction of maximum allowed size of IP packets
due to its stream-oriented nature. This is because TCP will already have fragmented large packets into
smaller streams of bytes so as to not exceed Maximum Transmission Unit.

It is also important to note that the TCP protocol of our RPC protocol runs on IPv6.


Multi-threaded
-------------
The RPC protocol is designed with multi-threaded server in mind. For this reason, multiple clients
may connect to the same server at once.


Server usage:
-------------
1. **Creation:**<br>
    On the server's end, the server must first initialize the server RPC by calling:
    ```c
    rpc_server* rpc_init_server(int port);
    ```
    As seen in the function's signature, it is expected of the server to provide the server RPC
    initialization with an appropriate port number.

2. **Registration:**<br>
    The server then must register the functions that clients can request from. Once the server is "on"
    (we will discuss this afterwards), functions cannot be registered anymore unless the server is
    down. The function call is:
    ```c
    int rpc_register(rpc_server* server, char* name, rpc_handler handler);
    ```
    The handler is a function local to the server, and would be part of client's requests once the
    server is on. It is assigned with a name. The return value indicates whether handler registration
    was successful.

3. **Serve:**<br>
    This is when the server is officially on. At this stage, the server will start accepting client's
    connections and will be permanently "on" unless it was manually killed. The function call is:
    ```c
    void rpc_serve_all(rpc_server* server);
    ```
    The type of requests that it will serve will be detailed in the Client usage section.


Client usage:
-------------
1. **Connect:**<br>
    On the client's end, the client is expected to connect to the server themselves with the client
    RPC initialization function:
    ```c
    rpc_client* rpc_init_client(char* addr, int port);
    ```
    To connect, the client must provide:
        - addr: the IP address, in IPv6
        - port: the port number

2. **Find request:**<br>
    The first type of request the client could make is a find. The find request is simply to find the
    function on the server:
    ```c
    rpc_handle* rpc_find(rpc_client* client, char* name);
    ```
    The function requires the name for the function. Once called, the server will send a handle back
    to the client. The handle is a placeholder for the function that the client requests to find.
    If the name is not registered on the server, that handle will be `NULL`. Otherwise, it will be the
    proper function placeholder that allows client to call "locally" on their system.

3. **Call request:**<br>
    With the handle provided, the client can now make the function call with the call request:
    ```c
    rpc_data* rpc_call(rpc_client* client, rpc_handle* handle, rpc_data* payload);
    ```
    The requirements for the function handle call is the payload. In other words, the handle
    technically takes the payload as an argument. As such, the client must provide with the handle
    they receive, and the payload argument for the handle.
    The returned object is the response from the server. Both of which have the same structure.
    
      *NOTE:*
          Due to the un-local nature of the function call, the payload and response are from different
          ends. Hence, this may pose issues with the size capable for receiving of a specific system.
          As such, packets exceeding the limit size will alert both end systems with "**Overlength error**"
          message onto the standard error.
          Read more at Shared usage and Routine failures segments below.

4. **Stop the connection:**<br>
    Once all has been requested, the client must stop their connection to the server:
    ```c
    void rpc_close_client(rpc_client* client);
    ```
    This will free the client RPC and close their connection to the server.


Shared usage:
-------------
As discussed in Client usage segment, the `rpc_data` structure type acts as both the payload for the
client to pass to their function call, and the response given to the client as a result of said call.
It should be noted that this is not actual local client function call, meaning the response actually
comes from the server. In short, it is the RPC protocol-specific data packet to be sent over the
network. So it is both the client's and server's responsibilities to free the RPC data structure:
  ```c
  void rpc_data_free(rpc_data* data);
  ```

It is important to note that the RPC data packet must be able to 'fit' to the other end system for
successful transmission. Here is the structure of RPC data:
  ```c
    typedef struct {
        int data1;
        size_t data2_len;
        void* data2;
    } rpc_data;
  ```

Notably, data2 size is defined by a `size_t data2_len` variable, which is system dependent. The RPC
protocol ensures that `data2_len` shall not exceed either end system's maximum `size_t` value (`SIZE_MAX`).
If it does, the recipient will not be able to receive the packet, and both end system will receive
an error message "**Overlength error**".


Routine failures:
-------------
#### Overlength error:
For details, to ensure that the `size_t` does not exceed `SIZE_MAX`, the protocol will first send and
receive the `UINT_MAX` value from one another. This is a number that can assuredly be sent over the
network (the protocol provides with sending up to 64-bit integers). Then it will pick the smaller
`UINT_MAX` from the 2 systems as the 'pivot'. Then, using the pivot, within the sender's system, we
check for:
  * `n_s`: the number of times `data2_len` has to be sent if pivot was the ceiling value
  * `r_s`: the remainder of that last sending
  
The sender will send these 2 values to the receiver's end. The receiver's end will then do the same
for their own `SIZE_MAX`:
  * `n_r`: the number of chunks if `SIZE_MAX` were to be divided by pivot
  * `r_r`: the remainder

With this, the receiver of data2 must make sure that, either `n_r > n_s`, or they are equal and that
`r_r > r_s`. If this does not satisfy, then that means `data2` has exceeded the packet limit and thus
cannot be transmitted to the receiver's end.

#### Other failure checks:
In requesting server's services, the client and server must both exchange flags for verification.
If the service doesn't exist, then the client simply fails to request and they both move on.
