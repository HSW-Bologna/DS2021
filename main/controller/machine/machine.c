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
#include "utils/socketq.h"
#include "gel/timer/timecheck.h"
#include "gel/serializer/serializer.h"
#include "modbus.h"
#include "log.h"
#include "model/model.h"


#define TIMEOUT              100
#define DELAY                30
#define REQUEST_SOCKET_PATH  "/tmp/.application_machine_request_socket"
#define RESPONSE_SOCKET_PATH "/tmp/.application_machine_response_socket"

#define MODBUS_RESPONSE_02_LEN(data_len) (5 + ((data_len % 8) == 0 ? (data_len / 8) : (data_len / 8) + 1))
#define MODBUS_RESPONSE_03_LEN(data_len) (5 + data_len * 2)
#define MODBUS_RESPONSE_04_LEN(data_len) (5 + data_len * 2)
#define MODBUS_RESPONSE_05_LEN           8
#define MODBUS_RESPONSE_15_LEN           8
#define MODBUS_RESPONSE_16_LEN           8
#define MODBUS_COMMUNICATION_ATTEMPTS    5
#define MODBUS_MACHINE_ADDRESS           2

#define MACHINE_HOLDING_REGISTER_PWM(x)       (MACHINE_HOLDING_REGISTER_PWM1 + x)
#define MACHINE_HOLDING_REGISTER_PARMAC_START MACHINE_HOLDING_REGISTER_TIPO_SONDA_TEMPERATURA
#define MACHINE_HOLDING_REGISTER_STATS_START  MACHINE_HOLDING_REGISTER_CICLI_TOTALI


#define CHECK_MSG_WRITE(x, msg_type)                                                                                   \
    if (x != sizeof(msg_type)) {                                                                                       \
        log_error("Error while sending message to machine: %s", strerror(errno));                                      \
    }


#define FLAG_TIPO_BIT               0
#define FLAG_INVERSIONE_BIT         1
#define FLAG_ATTESA_TEMPERATURA_BIT 2


enum {
    MACHINE_HOLDING_REGISTER_VERSION_HIGH = 0,
    MACHINE_HOLDING_REGISTER_VERSION_LOW,
    MACHINE_HOLDING_REGISTER_BUILD_DATE,
    MACHINE_HOLDING_REGISTER_COMMAND,
    MACHINE_HOLDING_REGISTER_PWM1,
    MACHINE_HOLDING_REGISTER_PWM2,

    MACHINE_HOLDING_REGISTER_TIPO_SONDA_TEMPERATURA = 10,
    MACHINE_HOLDING_REGISTER_TEMPERATURA_SICUREZZA,
    MACHINE_HOLDING_REGISTER_TEMPO_ALLARME_TEMPERATURA,
    MACHINE_HOLDING_REGISTER_TIPO_MACCHINA_OCCUPATA,
    MACHINE_HOLDING_REGISTER_TIPO_RISCALDAMENTO,
    MACHINE_HOLDING_REGISTER_TEMPO_ATTESA_PARTENZA_CICLO,
    MACHINE_HOLDING_REGISTER_FLAG_CONFIGURAZIONE,

    MACHINE_HOLDING_REGISTER_NUMERO_PROGRAMMA = 50,
    MACHINE_HOLDING_REGISTER_NUMERO_STEP,
    MACHINE_HOLDING_REGISTER_TIPO_STEP,
    MACHINE_HOLDING_REGISTER_TEMPO_MARCIA,
    MACHINE_HOLDING_REGISTER_TEMPO_PAUSA,
    MACHINE_HOLDING_REGISTER_TEMPO_DURATA,
    MACHINE_HOLDING_REGISTER_VELOCITA,
    MACHINE_HOLDING_REGISTER_TEMPERATURA,
    MACHINE_HOLDING_REGISTER_UMIDITA,
    MACHINE_HOLDING_REGISTER_FLAG_CONFIGURAZIONE_STEP,
    MACHINE_HOLDING_REGISTER_NUMERO_CICLI,
    MACHINE_HOLDING_REGISTER_TEMPO_RITARDO,

    MACHINE_HOLDING_REGISTER_STATE = 100,
    MACHINE_HOLDING_REGISTER_ALARMS,
    MACHINE_HOLDING_REGISTER_FLAG_FUNZIONAMENTO,
    MACHINE_HOLDING_REGISTER_TEMPO_RIMANENTE,

    /* Statistics */
    MACHINE_HOLDING_REGISTER_CICLI_TOTALI = 150,
    MACHINE_HOLDING_REGISTER_CICLI_PARZIALI,
    MACHINE_HOLDING_REGISTER_TEMPO_ATTIVITA_HI,
    MACHINE_HOLDING_REGISTER_TEMPO_ATTIVITA_LO,
    MACHINE_HOLDING_REGISTER_TEMPO_LAVORO_HI,
    MACHINE_HOLDING_REGISTER_TEMPO_LAVORO_LO,
    MACHINE_HOLDING_REGISTER_TEMPO_MOTO_HI,
    MACHINE_HOLDING_REGISTER_TEMPO_MOTO_LO,
    MACHINE_HOLDING_REGISTER_TEMPO_VENTILAZIONE_HI,
    MACHINE_HOLDING_REGISTER_TEMPO_VENTILAZIONE_LO,
    MACHINE_HOLDING_REGISTER_TEMPO_RISCALDAMENTO_HI,
    MACHINE_HOLDING_REGISTER_TEMPO_RISCALDAMENTO_LO,
};


enum {
    MACHINE_INPUT_REGISTER_GETT1 = 0,
    MACHINE_INPUT_REGISTER_GETT2,
    MACHINE_INPUT_REGISTER_GETT3,
    MACHINE_INPUT_REGISTER_GETT4,
    MACHINE_INPUT_REGISTER_GETT5,
    MACHINE_INPUT_REGISTER_CASSA,
    MACHINE_INPUT_REGISTER_TEMPERATURE_RS485,
    MACHINE_INPUT_REGISTER_HUMIDITY_RS485,
    MACHINE_INPUT_REGISTER_ADC_PTC1,
    MACHINE_INPUT_REGISTER_ADC_PTC2,
    MACHINE_INPUT_REGISTER_TEMPERATURE_PTC1,
    MACHINE_INPUT_REGISTER_TEMPERATURE_PTC2,
    MACHINE_INPUT_REGISTER_actual_temperature,
};


typedef enum {
    MACHINE_MESSAGE_CODE_GET_VERSION,
    MACHINE_MESSAGE_CODE_TEST_RELE,
    MACHINE_MESSAGE_CODE_TEST_PWM,
    MACHINE_MESSAGE_CODE_GET_INPUT_STATUS,
    MACHINE_MESSAGE_CODE_GET_STATE,
    MACHINE_MESSAGE_CODE_GET_EXTENDED_STATE,
    MACHINE_MESSAGE_CODE_GET_SENSORS_VALUES,
    MACHINE_MESSAGE_CODE_SEND_PARMAC,
    MACHINE_MESSAGE_CODE_COMMAND,
    MACHINE_MESSAGE_CODE_RESTART,
    MACHINE_MESSAGE_CODE_STOP,
    MACHINE_MESSAGE_CODE_SEND_STEP,
    MACHINE_MESSAGE_CODE_WRITE_HOLDING_REGISTER,
    MACHINE_MESSAGE_CODE_READ_STATISTICS,
} machine_message_code_t;


typedef struct {
    machine_message_code_t code;
    union {
        struct {
            uint16_t register_index;
            uint16_t register_value;
        };
        struct {
            parameters_step_t step;
            size_t            prog_num;
            size_t            step_num;
            int               start;
        };
        struct {
            size_t   pwm;
            uint16_t speed;
        };
        struct {
            size_t rele;
            int    value;
        };
        uint16_t command;

        struct {
            uint16_t stop_time_in_pause;
            uint16_t temperature_probe_type;
            uint16_t safety_temperature;
            uint16_t temperature_alarm_delay;
            uint16_t disable_alarms;
            uint16_t enable_inverter_alarm;
            uint16_t enable_filter_alarm;
            uint16_t busy_signal_type;
            uint16_t invert_busy_signal;
            uint16_t start_delay;
            uint16_t heating_type;
        };
    };
} machine_message_t;


typedef struct {
    machine_response_message_t *response;
    void (*callback)(machine_response_message_t *, uint16_t, uint16_t);
} modbus_context_t;


static void *serial_port_task(void *args);
static int   init_unix_server_socket(char *path, int *server, int *client);
static int   read_with_timeout(uint8_t *buffer, size_t len, int fd, unsigned long timeout);
static int   write_coil(int fd, ModbusMaster *master, uint8_t address, uint16_t index, int value);
static int   write_coils(int fd, ModbusMaster *master, uint8_t address, uint16_t index, uint8_t *values, size_t len);
static int   read_input_status(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len);
static int   read_input_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len);
static int   read_holding_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len);
static int   write_holding_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, uint16_t *values,
                                     size_t len);
static void  send_write_holding_register(uint16_t index, uint16_t value);
static void  send_message(machine_message_t *message);
static int   send_request(int fd, ModbusMaster *master, size_t expected_len);
static int   task_manage_message(machine_message_t message, ModbusMaster *master, int fd, int *stop);
static void  report_error(void);
static void  send_response(machine_response_message_t *message);


static socketq_t requestq  = {0};
static socketq_t responseq = {0};

void machine_init(void) {
    int res1 = socketq_init(&requestq, REQUEST_SOCKET_PATH, sizeof(machine_message_t));
    int res2 = socketq_init(&responseq, RESPONSE_SOCKET_PATH, sizeof(machine_response_message_t));
    assert(res1 == 0 && res2 == 0);

    pthread_t id;
    pthread_create(&id, NULL, serial_port_task, NULL);
    pthread_detach(id);
}


int machine_get_response(machine_response_message_t *msg) {
    return socketq_receive_nonblock(&responseq, (uint8_t *)msg, 0);
}


void machine_test_pwm(size_t pwm, int speed) {
    if (pwm >= 2) {
        return;
    }

    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_TEST_PWM, .pwm = pwm, .speed = speed};
    send_message(&message);
}



void machine_test_rele(size_t rele, int value) {
    if (rele >= NUM_RELES) {
        return;
    }

    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_TEST_RELE, .rele = rele, .value = value > 0};
    send_message(&message);
}


void machine_send_step(parameters_step_t *step, size_t prog_num, size_t step_num, int start) {
    if (step == NULL) {
        return;
    }

    machine_message_t message = {
        .code     = MACHINE_MESSAGE_CODE_SEND_STEP,
        .step     = *step,
        .prog_num = prog_num,
        .step_num = step_num,
        .start    = start,
    };
    send_message(&message);
}


void machine_send_command(uint16_t command) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_COMMAND, .command = command};
    send_message(&message);
}


void machine_read_version(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_GET_VERSION};
    send_message(&message);
}


void machine_read_statistics(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_READ_STATISTICS};
    send_message(&message);
}


void machine_send_parmac(parmac_t *parmac) {
    machine_message_t message = {
        .code                    = MACHINE_MESSAGE_CODE_SEND_PARMAC,
        .temperature_probe_type  = parmac->tipo_sonda_temperatura,
        .safety_temperature      = parmac->posizione_sonda_temperatura == POSIZIONE_SONDA_INGRESSO
                                       ? parmac->temperatura_massima_ingresso
                                       : parmac->temperatura_massima_uscita,
        .temperature_alarm_delay = parmac->tempo_allarme_temperatura,
        .stop_time_in_pause      = parmac->stop_tempo_ciclo,
        .disable_alarms          = !parmac->abilita_allarmi,
        .enable_inverter_alarm   = parmac->allarme_inverter_off_on,
        .enable_filter_alarm     = parmac->allarme_filtro_off_on,
        .busy_signal_type        = parmac->tipo_macchina_occupata,
        .invert_busy_signal      = parmac->inverti_macchina_occupata,
        .start_delay             = parmac->tempo_attesa_partenza_ciclo,
        .heating_type            = parmac->tipo_riscaldamento,
    };
    send_message(&message);
}


void machine_refresh_test_values(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_GET_INPUT_STATUS};
    send_message(&message);
}


void machine_refresh_state(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_GET_STATE};
    send_message(&message);
}


void machine_get_extended_state(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_GET_EXTENDED_STATE};
    send_message(&message);
}


void machine_refresh_sensors(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_GET_SENSORS_VALUES};
    send_message(&message);
}


void machine_restart_communication(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_RESTART};
    send_message(&message);
}


void machine_stop_communication(void) {
    machine_message_t message = {.code = MACHINE_MESSAGE_CODE_STOP};
    send_message(&message);
}


void machine_change_remaining_time(uint16_t seconds) {
    send_write_holding_register(MACHINE_HOLDING_REGISTER_TEMPO_RIMANENTE, seconds);
}


void machine_change_humidity(uint16_t humidity) {
    send_write_holding_register(MACHINE_HOLDING_REGISTER_UMIDITA, humidity);
}


void machine_change_temperature(uint16_t temperature) {
    send_write_holding_register(MACHINE_HOLDING_REGISTER_TEMPERATURA, temperature);
}


void machine_change_speed(uint16_t speed) {
    send_write_holding_register(MACHINE_HOLDING_REGISTER_VELOCITA, speed);
}


static void send_write_holding_register(uint16_t index, uint16_t value) {
    machine_message_t message = {
        .code           = MACHINE_MESSAGE_CODE_WRITE_HOLDING_REGISTER,
        .register_index = index,
        .register_value = value,
    };
    send_message(&message);
}


static void send_message(machine_message_t *message) {
    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, REQUEST_SOCKET_PATH);
    socketq_send(&requestq, (uint8_t *)message);
}


static void setup_port(int fd) {
    serial_set_interface_attribs(fd, B230400);
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


static ModbusError data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args) {
    modbus_context_t *context = (modbus_context_t *)modbusMasterGetUserPointer(master);
    if (context != NULL) {
        context->callback(context->response, args->index, args->value);
    }
    return MODBUS_OK;
}


static void read_sensors_cb(machine_response_message_t *response, uint16_t index, uint16_t value) {
    uint16_t *sensors_ptrs[] = {
        &response->coins[0],
        &response->coins[1],
        &response->coins[2],
        &response->coins[3],
        &response->coins[4],
        &response->payment,
        &response->t_rs485,
        &response->h_rs485,
        &response->t1_adc,
        &response->t2_adc,
        &response->t1,
        &response->t2,
        &response->actual_temperature,
    };

    int i = (int)index - MACHINE_INPUT_REGISTER_GETT1;
    if (i >= 0 && i < (int)(sizeof(sensors_ptrs) / sizeof(sensors_ptrs[0]))) {
        *sensors_ptrs[i] = value;
    }
}


static void read_statistics_cb(machine_response_message_t *response, uint16_t index, uint16_t value) {
#define OVERWRITE_HI(x, hi) x = ((x)&0x0000FFFF) | ((uint32_t)value << 16)
#define OVERWRITE_LO(x, hi) x = ((x)&0xFFFF0000) | ((uint32_t)value)

    switch (index) {
        case MACHINE_HOLDING_REGISTER_CICLI_TOTALI:
            response->stats.complete_cycles = value;
            break;

        case MACHINE_HOLDING_REGISTER_CICLI_PARZIALI:
            response->stats.partial_cycles = value;
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_ATTIVITA_HI:
            OVERWRITE_HI(response->stats.active_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_ATTIVITA_LO:
            OVERWRITE_LO(response->stats.active_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_LAVORO_HI:
            OVERWRITE_HI(response->stats.work_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_LAVORO_LO:
            OVERWRITE_LO(response->stats.work_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_MOTO_HI:
            OVERWRITE_HI(response->stats.rotation_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_MOTO_LO:
            OVERWRITE_LO(response->stats.rotation_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_VENTILAZIONE_HI:
            OVERWRITE_HI(response->stats.ventilation_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_VENTILAZIONE_LO:
            OVERWRITE_LO(response->stats.ventilation_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_RISCALDAMENTO_HI:
            OVERWRITE_HI(response->stats.heating_time, value);
            break;

        case MACHINE_HOLDING_REGISTER_TEMPO_RISCALDAMENTO_LO:
            OVERWRITE_LO(response->stats.heating_time, value);
            break;
    }

#undef OVERWRITE_HI
#undef OVERWRITE_LO
}


static void read_version_cb(machine_response_message_t *response, uint16_t index, uint16_t value) {
    if (index == MACHINE_HOLDING_REGISTER_VERSION_HIGH) {
        response->version_major = (value >> 8) & 0xFF;
        response->version_minor = value & 0xFF;
    } else if (index == MACHINE_HOLDING_REGISTER_VERSION_LOW) {
        response->version_patch = value;
    } else if (index == MACHINE_HOLDING_REGISTER_BUILD_DATE) {
        response->build_day   = value & 0x1F;
        response->build_month = (value >> 5) & 0x1F;
        response->build_year  = (value >> 10) & 0x1F;
    }
}


static void read_state_first_part_cb(machine_response_message_t *response, uint16_t index, uint16_t value) {
    uint16_t *state_ptrs[] = {
        &response->state,
        &response->alarms,
        &response->flags,
        &response->remaining,
    };

    int i = (int)index - MACHINE_HOLDING_REGISTER_STATE;
    if (i >= 0 && i < (int)(sizeof(state_ptrs) / sizeof(state_ptrs[0]))) {
        *state_ptrs[i] = value;
    }
}


static void read_program_state_cb(machine_response_message_t *response, uint16_t index, uint16_t value) {
    uint16_t *state_ptrs[] = {
        &response->program_number,
        &response->step_number,
        &response->step_type,
    };

    int i = (int)index - MACHINE_HOLDING_REGISTER_NUMERO_PROGRAMMA;
    if (i >= 0 && i < (int)(sizeof(state_ptrs) / sizeof(state_ptrs[0]))) {
        *state_ptrs[i] = value;
    }
}


static void read_test_input_cb(machine_response_message_t *response, uint16_t index, uint16_t value) {
    response->value |= ((value > 0) << index);
}


static ModbusError masterExceptionCallback(const ModbusMaster *master, uint8_t address, uint8_t function,
                                           ModbusExceptionCode code) {
    printf("Received exception (function %d) from slave %d code %d\n", function, address, code);
    return MODBUS_OK;
}


static void *serial_port_task(void *args) {
    int communication_error = 0, communication_stop = 0;

    int fd = look_for_hardware_port();
    if (fd < 0) {
        log_warn("Nessuna porta trovata");
        communication_error = 1;
        report_error();
    }
    setup_port(fd);

    ModbusMaster    master;
    ModbusErrorInfo err = modbusMasterInit(&master,
                                           data_callback,               // Callback for handling incoming data
                                           masterExceptionCallback,     // Exception callback (optional)
                                           modbusDefaultAllocator,      // Memory allocator used to allocate request
                                           modbusMasterDefaultFunctions,        // Set of supported functions
                                           modbusMasterDefaultFunctionCount     // Number of supported functions
    );

    // Check for errors
    assert(modbusIsOk(err) && "modbusMasterInit() failed");
    machine_message_t not_delivered = {0};

    for (;;) {
        machine_message_t message = {0};

        if (socketq_receive(&requestq, (uint8_t *)&message)) {
            /* received a new message */
            if ((communication_error || communication_stop) && message.code == MACHINE_MESSAGE_CODE_RESTART) {
                communication_stop = 0;
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

                    if ((communication_error = task_manage_message(not_delivered, &master, fd, &communication_stop))) {
                        report_error();
                        continue;
                    }
                }
            } else if (!(communication_error || communication_stop)) {
                if ((communication_error = task_manage_message(message, &master, fd, &communication_stop))) {
                    not_delivered = message;
                    report_error();
                }
            }
        }

        usleep(DELAY * 1000);
    }

    close(fd);
    pthread_exit(NULL);
    return NULL;
}


static void send_response(machine_response_message_t *message) {
    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, RESPONSE_SOCKET_PATH);
    socketq_send(&responseq, (uint8_t *)message);
}


static void report_error(void) {
    log_warn("Communication error!");
    machine_response_message_t message = {.code = MACHINE_RESPONSE_MESSAGE_CODE_ERROR};
    send_response(&message);
}


static int task_manage_message(machine_message_t message, ModbusMaster *master, int fd, int *stop) {
    int res = 0;
    modbusMasterSetUserPointer(master, NULL);

    switch (message.code) {
        case MACHINE_MESSAGE_CODE_STOP:
            *stop = 1;
            break;

        case MACHINE_MESSAGE_CODE_READ_STATISTICS: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_READ_STATISTICS};
            modbus_context_t           context  = {.response = &response, .callback = read_statistics_cb};
            modbusMasterSetUserPointer(master, (void *)&context);
            res = read_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_STATS_START, 10);
            if (res) {
                break;
            }
            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_WRITE_HOLDING_REGISTER:
            res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, message.register_index,
                                          &message.register_value, 1);
            break;

        case MACHINE_MESSAGE_CODE_GET_SENSORS_VALUES: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_READ_SENSORS};
            modbus_context_t           context  = {.response = &response, .callback = read_sensors_cb};
            modbusMasterSetUserPointer(master, (void *)&context);
            res = read_input_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_INPUT_REGISTER_GETT1, 13);
            if (res) {
                break;
            }
            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_GET_EXTENDED_STATE: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_READ_EXTENDED_STATE};
            modbus_context_t           context  = {.response = &response, .callback = read_state_first_part_cb};
            modbusMasterSetUserPointer(master, (void *)&context);
            res = read_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_STATE, 4);
            if (res) {
                break;
            }

            context.callback = read_program_state_cb;
            res = read_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_NUMERO_PROGRAMMA,
                                         3);
            if (res) {
                break;
            }
            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_GET_STATE: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_READ_STATE};
            modbus_context_t           context  = {.response = &response, .callback = read_state_first_part_cb};
            modbusMasterSetUserPointer(master, (void *)&context);
            res = read_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_STATE, 4);
            if (res) {
                break;
            }

            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_SEND_PARMAC: {
            uint16_t flags = (message.stop_time_in_pause > 0) | ((message.invert_busy_signal > 0) << 1) |
                             ((message.disable_alarms > 0) << 2) | ((message.enable_inverter_alarm > 0) << 3) |
                             ((message.enable_filter_alarm > 0) << 4);
            uint16_t buffer[] = {
                message.temperature_probe_type,
                message.safety_temperature,
                message.temperature_alarm_delay,
                message.busy_signal_type,
                message.heating_type,
                message.start_delay,
                flags,
            };
            res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_PARMAC_START,
                                          buffer, sizeof(buffer) / sizeof(buffer[0]));

            if (res) {
                break;
            }

            uint16_t command = COMMAND_REGISTER_INITIALIZE;
            res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_COMMAND,
                                          &command, 1);
            break;
        }

        case MACHINE_MESSAGE_CODE_COMMAND: {
            res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_COMMAND,
                                          &message.command, 1);
            break;
        }

        case MACHINE_MESSAGE_CODE_TEST_PWM: {
            res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_PWM(message.pwm),
                                          &message.speed, 1);
            break;
        }

        case MACHINE_MESSAGE_CODE_TEST_RELE: {
            uint32_t reles   = ((message.value > 0) << message.rele);
            uint8_t  data[4] = {0};
            serialize_uint32_le(data, reles);

            res = write_coils(fd, master, MODBUS_MACHINE_ADDRESS, 0, data, NUM_RELES);
            break;
        }

        case MACHINE_MESSAGE_CODE_GET_INPUT_STATUS: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_TEST_READ_INPUT};
            modbus_context_t           context  = {.response = &response, .callback = read_test_input_cb};
            modbusMasterSetUserPointer(master, (void *)&context);
            res = read_input_status(fd, master, MODBUS_MACHINE_ADDRESS, 0, 16);
            if (res) {
                break;
            }
            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_SEND_STEP: {
            switch (message.step.type) {
                case DRYER_PROGRAM_STEP_TYPE_DRYING: {
                    uint16_t flags =
                        ((message.step.drying.type > 0) << FLAG_TIPO_BIT) |
                        ((message.step.drying.enable_reverse > 0) << FLAG_INVERSIONE_BIT) |
                        ((message.step.drying.enable_waiting_for_temperature > 0) << FLAG_ATTESA_TEMPERATURA_BIT);
                    uint16_t parameters[] = {
                        message.prog_num,
                        message.step_num,
                        message.step.type,
                        message.step.drying.rotation_time,
                        message.step.drying.pause_time,
                        message.step.drying.duration,
                        message.step.drying.speed,
                        message.step.drying.temperature,
                        message.step.drying.humidity,
                        flags,
                        message.step.drying.cooling_hysteresis,
                        message.step.drying.heating_hysteresis,
                        message.step.drying.vaporization_temperature,
                        message.step.drying.vaporization_duration,
                    };
                    res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS,
                                                  MACHINE_HOLDING_REGISTER_NUMERO_PROGRAMMA, parameters,
                                                  sizeof(parameters) / sizeof(parameters[0]));
                    break;
                }

                case DRYER_PROGRAM_STEP_TYPE_COOLING: {
                    uint16_t flags = ((message.step.cooling.type > 0) << FLAG_TIPO_BIT) |
                                     ((message.step.cooling.enable_reverse > 0) << FLAG_INVERSIONE_BIT);
                    uint16_t parameters[] = {
                        message.prog_num,
                        message.step_num,
                        message.step.type,
                        message.step.cooling.rotation_time,
                        message.step.cooling.pause_time,
                        message.step.cooling.duration,
                        0,
                        message.step.cooling.temperature,
                        0,
                        flags,
                        0,
                        0,
                        0,
                        0,
                        message.step.cooling.deodorant_delay,
                        message.step.cooling.deodorant_duration,
                    };
                    res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS,
                                                  MACHINE_HOLDING_REGISTER_NUMERO_PROGRAMMA, parameters,
                                                  sizeof(parameters) / sizeof(parameters[0]));
                    break;
                }

                case DRYER_PROGRAM_STEP_TYPE_UNFOLDING: {
                    uint16_t parameters[] = {
                        message.prog_num,
                        message.step_num,
                        message.step.type,
                        message.step.unfolding.rotation_time,
                        message.step.unfolding.pause_time,
                        message.step.unfolding.max_duration,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        message.step.unfolding.max_cycles,
                        message.step.unfolding.start_delay,
                    };
                    res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS,
                                                  MACHINE_HOLDING_REGISTER_NUMERO_PROGRAMMA, parameters,
                                                  sizeof(parameters) / sizeof(parameters[0]));
                    break;
                }

                default:
                    break;
            }

            if (res) {
                break;
            }

            uint16_t command = COMMAND_REGISTER_RUN_STEP;
            if (message.start) {
                command = COMMAND_REGISTER_RUN_STEP;
            } else {
                command = COMMAND_REGISTER_PAUSE;
            }
            res = write_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_COMMAND,
                                          &command, 1);
            break;
        }

        case MACHINE_MESSAGE_CODE_GET_VERSION: {
            machine_response_message_t response = {.code = MACHINE_RESPONSE_MESSAGE_CODE_VERSION};
            modbus_context_t           context  = {.response = &response, .callback = read_version_cb};
            modbusMasterSetUserPointer(master, (void *)&context);
            res = read_holding_registers(fd, master, MODBUS_MACHINE_ADDRESS, MACHINE_HOLDING_REGISTER_VERSION_HIGH, 3);
            if (res) {
                break;
            }
            send_response(&response);
            break;
        }

        case MACHINE_MESSAGE_CODE_RESTART:
            break;
    }

    return res;
}


static int init_unix_server_socket(char *path, int *server, int *client) {
    if (server != NULL) {
        struct sockaddr_un local;
        int                len;
        int                fd;

        if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
            log_error("Error creating server socket: %s", strerror(errno));
            return -1;
        }

        local.sun_family = AF_UNIX;
        strcpy(local.sun_path, path);
        unlink(local.sun_path);
        len = strlen(local.sun_path) + sizeof(local.sun_family);
        if (bind(fd, (struct sockaddr *)&local, len) == -1) {
            log_error("Error binding server socket: %s", strerror(errno));
            return -1;
        }

        *server = fd;
    }

    if (client != NULL) {
        int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd <= 0) {
            log_error("Error creating client socket: %s", strerror(errno));
            return -1;
        } else {
            *client = fd;
        }
    }

    return 0;
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

    if (res) {
        log_warn("Unable to write coil");
    }

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

    if (res) {
        log_warn("Unable to write coils");
    }

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

    if (res) {
        log_warn("Unable to read digital inputs");
    }

    return res;
}


static int read_input_registers(int fd, ModbusMaster *master, uint8_t address, uint16_t index, size_t len) {
    int    res     = 0;
    size_t counter = 0;

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest04RTU(master, address, index, len);
        assert(modbusIsOk(err));

        if (send_request(fd, master, MODBUS_RESPONSE_04_LEN(len))) {
            res = 1;
            usleep(DELAY * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    if (res) {
        log_warn("Unable to read inputs");
    }

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

    if (res) {
        log_warn("Unable to read holding registers");
    }

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

    if (res) {
        log_warn("Unable to write holding registers");
    }

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