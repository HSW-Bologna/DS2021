#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "machine.h"
#include "serial.h"
#include "utils/system_time.h"
#include "gel/timer/timecheck.h"
#include "gel/serializer/serializer.h"
#include "modbus.h"
#include "log.h"
#include "model/model.h"


#define TIMEOUT              50
#define DELAY                (TIMEOUT / 2)
#define REQUEST_SOCKET_PATH  "/tmp/application_machine_request_socket"
#define RESPONSE_SOCKET_PATH "/tmp/application_machine_response_socket"

#define MODBUS_RESPONSE_02_LEN(data_len) (5 + ((data_len % 8) == 0 ? (data_len / 8) : (data_len / 8) + 1))
#define MODBUS_RESPONSE_03_LEN(data_len) (5 + data_len * 2)
#define MODBUS_RESPONSE_05_LEN           8
#define MODBUS_RESPONSE_15_LEN           8
#define MODBUS_RESPONSE_16_LEN           8
#define MODBUS_COMMUNICATION_ATTEMPTS    5
#define MODBUS_MACHINE_ADDRESS           2

#define MACHINE_HOLDING_REGISTER_COMMAND 0

#define CHECK_MSG_WRITE(x, msg_type)                                                                                   \
    if (x != sizeof(msg_type)) {                                                                                       \
        log_error("Error while sending message to machine: %s", strerror(errno));                                      \
    }


typedef enum {
    MACHINE_MESSAGE_CODE_GET_VERSION,
    MACHINE_MESSAGE_CODE_TEST_RELE,
    MACHINE_MESSAGE_CODE_GET_INPUT_STATUS,
    MACHINE_MESSAGE_CODE_GET_STATE,
    MACHINE_MESSAGE_CODE_COMMAND,
    MACHINE_MESSAGE_CODE_RESTART,
} machine_message_code_t;


typedef struct {
    machine_message_code_t code;
    union {
        struct {
            size_t rele;
            int    value;
        };
        uint16_t command;
    };
} machine_message_t;


static void *serial_port_task(void *args);
static int   init_unix_server_socket(char *path);
static int   read_with_timeout(uint8_t *buffer, size_t len, int fd, unsigned long timeout);
static int   write_coil(int fd, ModbusMaster *master, uint8_t address, uint16_t index, int value);
static int   write_coils(int fd, ModbusMaster *master, uint8_t address, uint16_t index, uint8_t *values, size_t len);
static int   read_input_status(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len);
static int   read_holding_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len);
static int   write_holding_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, uint16_t *values,
                                     size_t len);
static void  send_message(machine_message_t *message);
static int   send_request(int fd, ModbusMaster *master, size_t expected_len);
static int   task_manage_message(machine_message_t message, ModbusMaster *master, int fd);
static void  report_error(void);
static void  send_response(machine_response_message_t *message);


static int request_server_fd;
static int response_server_fd;
static int request_client_fd;
static int response_client_fd;




void machine_init(void) {
    request_server_fd  = init_unix_server_socket(REQUEST_SOCKET_PATH);
    response_server_fd = init_unix_server_socket(RESPONSE_SOCKET_PATH);

    request_client_fd  = socket(AF_UNIX, SOCK_DGRAM, 0);
    response_client_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    assert(request_client_fd);
    assert(response_client_fd);

    pthread_t id;
    pthread_create(&id, NULL, serial_port_task, NULL);
    pthread_detach(id);
}


int machine_get_response(machine_response_message_t *msg) {
    struct sockaddr_un peer_socket;
    socklen_t          peer_socket_len = sizeof(peer_socket);

    struct pollfd fds[1] = {{.fd = response_server_fd, .events = POLLIN}};
    if (poll(fds, 1, 0)) {
        return recvfrom(response_server_fd, msg, sizeof(*msg), 0, (struct sockaddr *)&peer_socket, &peer_socket_len) ==
               sizeof(*msg);
    } else {
        return 0;
    }
}


void machine_test_rele(size_t rele, int value) {
    if (rele >= NUM_RELES) {
        return;
    }

    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_TEST_RELE, .rele = rele, .value = value > 0};
    send_message(&message);
}


void machine_send_command(uint16_t command) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_COMMAND, .command = command};
    send_message(&message);
}


void machine_refresh_inputs(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_GET_INPUT_STATUS};
    send_message(&message);
}


void machine_refresh_state(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_GET_STATE};
    send_message(&message);
}


void machine_restart_communication(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_RESTART};
    send_message(&message);
}


static void send_message(machine_message_t *message) {
    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, REQUEST_SOCKET_PATH);
    CHECK_MSG_WRITE(sendto(request_client_fd, message, sizeof(*message), 0, (struct sockaddr *)&remote, sizeof(remote)),
                    machine_message_t);
}


static void setup_port(int fd) {
    serial_set_interface_attribs(fd, B115200);
    serial_set_mincount(fd, 0);
    tcflush(fd, TCIFLUSH);
}


static int look_for_hardware_port() {
    char name[64];
#ifdef TARGET_DEBUG
    char types[] = "/dev/ttyUSB%i";
#else
    char types[] = "/dev/ttyAMA%i";
#endif
    int fd;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 20; j++) {
            snprintf(name, 64, types, j);
            if ((fd = serial_open_tty(name)) > 0) {
                log_info("Porta trovata: %s", name);
                return fd;
            }
        }
    }
    return -1;
}


static ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args) {
    // printf("Received data from %d, reg: %d, value: %d\n", args->address, args->index, args->value);

    machine_response_message_t *msg = (machine_response_message_t *)modbusMasterGetUserPointer(master);

    uint16_t *state_ptrs[] = {&msg->state};

    switch (msg->code) {
        case MACHINE_RESPONSE_MESSAGE_CODE_READ_STATE:
            if (args->index < sizeof(state_ptrs) / sizeof(state_ptrs[0])) {
                *state_ptrs[args->index] = args->value;
            }
            break;

        case MACHINE_RESPONSE_MESSAGE_CODE_TEST_READ_INPUT:
            msg->value |= ((args->value > 0) << args->index);
            break;

        default:
            break;
    }

    return MODBUS_OK;
}


static ModbusError masterExceptionCallback(const ModbusMaster *master, uint8_t address, uint8_t function,
                                           ModbusExceptionCode code) {
    printf("Received exception (function %d) from slave %d code %d\n", function, address, code);
    return MODBUS_OK;
}


static void *serial_port_task(void *args) {
    int communication_error = 0;

    int fd = look_for_hardware_port();
    if (fd < 0) {
        log_warn("Nessuna porta trovata");
        communication_error = 1;
        report_error();
    }
    setup_port(fd);

    ModbusMaster    master;
    ModbusErrorInfo err = modbusMasterInit(&master,
                                           dataCallback,                // Callback for handling incoming data
                                           masterExceptionCallback,     // Exception callback (optional)
                                           modbusDefaultAllocator,      // Memory allocator used to allocate request
                                           modbusMasterDefaultFunctions,        // Set of supported functions
                                           modbusMasterDefaultFunctionCount     // Number of supported functions
    );

    // Check for errors
    assert(modbusIsOk(err) && "modbusMasterInit() failed");
    machine_message_t not_delivered = {0};

    for (;;) {
        struct sockaddr_un peer_socket;
        socklen_t          peer_socket_len = sizeof(peer_socket);
        machine_message_t  message         = {0};

        if (recvfrom(request_server_fd, &message, sizeof(message), 0, (struct sockaddr *)&peer_socket,
                     &peer_socket_len) == sizeof(message)) {

            /* received a new message */
            if (communication_error && message.code == MACHINE_MESSAGE_CODE_RESTART) {
                if (fd >= 0) {
                    close(fd);
                }

                fd = look_for_hardware_port();
                if (fd < 0) {
                    log_warn("Nessuna porta trovata");
                    communication_error = 1;
                    report_error();
                    continue;
                } else {
                    setup_port(fd);

                    if ((communication_error = task_manage_message(not_delivered, &master, fd))) {
                        report_error();
                        continue;
                    }
                }
            } else if (!communication_error) {
                if ((communication_error = task_manage_message(message, &master, fd))) {
                    not_delivered = message;
                    report_error();
                }
            }
        }

        usleep(TIMEOUT * 1000);
    }

    close(fd);
    pthread_exit(NULL);
    return NULL;
}


static void send_response(machine_response_message_t *message) {
    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, RESPONSE_SOCKET_PATH);
    CHECK_MSG_WRITE(
        sendto(response_client_fd, message, sizeof(*message), 0, (struct sockaddr *)&remote, sizeof(remote)),
        machine_response_message_t);
}


static void report_error(void) {
    log_warn("Communication error!");
    machine_response_message_t message = {.code = MACHINE_RESPONSE_MESSAGE_CODE_ERROR};
    send_response(&message);
}


static int task_manage_message(machine_message_t message, ModbusMaster *master, int fd) {
    int res = 0;

    switch (message.code) {
        case MACHINE_MESSAGE_CODE_GET_STATE: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_READ_STATE};
            modbusMasterSetUserPointer(master, (void *)&response);
            res = read_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, 1, 1);
            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_COMMAND: {
            res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_COMMAND,
                                          &message.command, 1);
            break;
        }

        case MACHINE_MESSAGE_CODE_TEST_RELE: {
            uint32_t reles   = ((message.value > 0) << message.rele);
            uint8_t  data[4] = {0};
            serialize_uint32_le(data, reles);

            res = write_coils(fd, master, MODBUS_MACHINE_ADDRESS, 0, data, NUM_RELES);
            // res = write_coil(fd, master, MODBUS_MACHINE_ADDRESS, message.rele, message.value);
            break;
        }

        case MACHINE_MESSAGE_CODE_GET_INPUT_STATUS: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_TEST_READ_INPUT};
            modbusMasterSetUserPointer(master, (void *)&response);
            res = read_input_status(fd, master, MODBUS_MACHINE_ADDRESS, 0, 4);
            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_GET_VERSION:
            break;

        case MACHINE_MESSAGE_CODE_RESTART:
            break;
    }

    return res;
}


static int init_unix_server_socket(char *path) {
    struct sockaddr_un local;
    int                len;
    int                fd;

    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("Error creating server socket");
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(fd, (struct sockaddr *)&local, len) == -1) {
        perror("binding");
        exit(1);
    }

    return fd;
}


static int write_coil(int fd, ModbusMaster *master, uint8_t address, uint16_t index, int value) {
    int    res     = 0;
    size_t counter = 0;

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest05RTU(master, address, index, value);
        assert(modbusIsOk(err));

        if (send_request(fd, master, MODBUS_RESPONSE_05_LEN)) {
            res = 1;
            usleep(DELAY * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}


static int write_coils(int fd, ModbusMaster *master, uint8_t address, uint16_t index, uint8_t *values, size_t len) {
    int    res     = 0;
    size_t counter = 0;

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest15RTU(master, address, index, len, values);
        assert(modbusIsOk(err));

        if (send_request(fd, master, MODBUS_RESPONSE_15_LEN)) {
            res = 1;
            usleep(DELAY * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}


static int read_input_status(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len) {
    int    res     = 0;
    size_t counter = 0;

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest02RTU(master, address, index, len);
        assert(modbusIsOk(err));

        if (send_request(fd, master, MODBUS_RESPONSE_02_LEN(len))) {
            res = 1;
            usleep(DELAY * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}


static int read_holding_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len) {
    int    res     = 0;
    size_t counter = 0;

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest03RTU(master, address, index, len);
        assert(modbusIsOk(err));

        if (send_request(fd, master, MODBUS_RESPONSE_03_LEN(len))) {
            res = 1;
            usleep(DELAY * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}


static int write_holding_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, uint16_t *values,
                                   size_t len) {
    int    res     = 0;
    size_t counter = 0;

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest16RTU(master, address, index, len, values);
        assert(modbusIsOk(err));

        if (send_request(fd, master, MODBUS_RESPONSE_16_LEN)) {
            res = 1;
            usleep(DELAY * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}




static int read_with_timeout(uint8_t *buffer, size_t len, int fd, unsigned long timeout) {
    size_t        buffer_index = 0;
    unsigned long startts      = get_millis();

    do {
        int res = read(fd, &buffer[buffer_index], len - buffer_index);

        if (res > 0) {
            buffer_index += res;
        }
    } while (buffer_index != len && !is_expired(startts, get_millis(), timeout));

    return buffer_index;
}


static int send_request(int fd, ModbusMaster *master, size_t expected_len) {
    uint8_t buffer[expected_len];
    tcflush(fd, TCIFLUSH);
    memset(buffer, 0, expected_len);
    int tosend = modbusMasterGetRequestLength(master);

    if (write(fd, modbusMasterGetRequest(master), tosend) != tosend) {
        log_error("Unable to write to serial: %s", strerror(errno));
        return -1;
    }

    int             len = read_with_timeout(buffer, sizeof(buffer), fd, TIMEOUT);
    ModbusErrorInfo err = modbusParseResponseRTU(master, modbusMasterGetRequest(master),
                                                 modbusMasterGetRequestLength(master), buffer, len);

    if (!modbusIsOk(err)) {
        log_warn("Modbus error: %i %i (%i)", err.source, err.error, len);
        return 1;
    } else {
        return 0;
    }
}