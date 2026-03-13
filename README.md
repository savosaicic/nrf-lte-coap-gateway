# nrf9151-sensor-gateway

A modular LTE-M to CoAP sensor gateway running on the nRF9151 DK with Zephyr RTOS.
The firmware collects readings from multiple data sources (I2C, SPI, GPIOs, …),
serializes them as JSON snapshots, and forwards them over LTE-M via CoAP to a lightweight
C server that parses each snapshot and stores every reading in a SQLite database.

The focus of this repo is the **data-source abstraction layer**: each physical sensor (or
any data-producing peripheral) implements a two-function interface (`init` / `read`), and
adding a new one requires no changes to the core pipeline.

> **Note:** Builds on the CoAP transport layer explored in
> [nrf9151-coap-backends](https://github.com/savosaicic/nrf9151-coap-backends).
> The `coap_backend_t` interface is carried over unchanged; only the libcoap3 backend
> is wired up here.

## Prerequisites

### Hardware
- A nRF9151 board with a SIM card

### Software — firmware
- nRF Connect SDK + Zephyr SDK toolchain
(follow the [Getting Started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html))
- `libcoap-3`, `cJSON`, `SQLite3` (for the server — see Server Setup below)

## Firmware Setup

### 1. Initialise the workspace

```bash
mkdir nrf-sensor-gateway-ws && cd nrf-sensor-gateway-ws
west init -m https://github.com/savosaicic/nrf-sensor-gateway --mr main
west update
pip install -r zephyr/scripts/requirements.txt
```

### 2. Set the server address

Open `firmware/app/prj.conf` and set your server's hostname or IP:

```
CONFIG_COAP_SERVER_HOSTNAME="your-server.example.com"
```

All options (hostname, port, resource path, sampling interval) can also be set interactively via menuconfig:

```bash
west build -b nrf9151dk/nrf9151/ns firmware/app -t menuconfig
```

### Build & flash

```bash
west build -b nrf9151dk/nrf9151/ns firmware/app
west flash
```

## Server Setup

Make sure UDP port `5683` is reachable on your server.

### Native build

```bash
# Install dependencies
sudo apt install libcoap3-dev libcjson-dev libsqlite3-dev

# Build
cd server/
make

# Run  (pass a database file path as the only argument)
./coap_sensor_server sensors.db
```

### Docker

```bash
cd server/
docker build -t coap-sensor-server .
docker run --rm -p 5683:5683/udp -v $(pwd)/data:/data coap-sensor-server /data/sensors.db
```

Incoming snapshots are printed to stdout and written to the database as they arrive.

---

## Database Schema

The server uses SQLite with two tables.

### `channels`

Stores one row per named data channel, created on first insertion.

| Column | Type    | Description                              |
| ------ | ------- | ---------------------------------------- |
| `id`   | INTEGER | Primary key (autoincrement)              |
| `name` | TEXT    | Unique channel name (e.g. `temperature`) |
| `type` | INTEGER | `sensor_type_t` enum value               |

### `readings`

Stores every individual reading.

| Column        | Type    | Description                                 |
|---------------|---------|---------------------------------------------|
| `id`          | INTEGER | Primary key (autoincrement)                 |
| `channel_id`  | INTEGER | Foreign key → `channels.id`                 |
| `timestamp`   | INTEGER | Unix timestamp in ms                        |
| `value_float` | REAL    | Set for `SENSOR_TYPE_FLOAT`, NULL otherwise |
| `value_int`   | INTEGER | Set for `SENSOR_TYPE_INT`, NULL otherwise   |

---

## Adding a New Sensor Type

This section explains how to extend the system with a new `SENSOR_TYPE_FOO`.
The changes are symmetric: the firmware encodes the new type into JSON;
the server parses and stores it.

### Firmware

**1. `firmware/app/include/sensor.h`**

Add the enum value and a field in the value union, then declare the update helper:

```c
typedef enum {
  SENSOR_TYPE_FLOAT = 0,
  SENSOR_TYPE_INT,
  SENSOR_TYPE_STRING,
  SENSOR_TYPE_FOO,   // ← add
} sensor_type_t;

typedef union {
  float f;
  //...
  foo_t foo;         // ← add the matching C type
} sensor_value_t;

// Add the declaration at the bottom of the file:
int sensor_channel_update_foo(sensor_channel_t *ch, foo_t value);
```

**2. `firmware/app/src/sensor.c`**

Implement the update helper and handle the new type in `sensor_snapshot_take`:

```c
int sensor_channel_update_foo(sensor_channel_t *ch, foo_t value)
{
  if (!ch || ch->type != SENSOR_TYPE_FOO) {
    return -EINVAL;
  }
  k_mutex_lock(&g_mutex, K_FOREVER);
  ch->value.foo = value;
  ch->has_value = true;
  k_mutex_unlock(&g_mutex);
  return 0;
}
```

In `sensor_snapshot_take`, add a case to copy the value into the snapshot:

```c
case SENSOR_TYPE_FOO:
  r->value.foo = ch->value.foo;
  break;
```

**3. `firmware/app/src/main.c`**

In `snapshot_to_json`, add a case to serialise the value into JSON:

```c
case SENSOR_TYPE_FOO:
  cJSON_Add<FooType>ToObject(entry, "v", r->value.foo);
  break;
```

Use the appropriate `cJSON_Add*ToObject` call for your type (e.g. `cJSON_AddBoolToObject`, `cJSON_AddNumberToObject`, `cJSON_AddStringToObject`).

---

### Server

**1. `server/include/sensor.h`**

Mirror the firmware changes — add the enum value, the union field,
and the function declaration:

```c
typedef enum {
  SENSOR_TYPE_FLOAT = SENSOR_TYPE_FIRST,
  SENSOR_TYPE_INT,
  SENSOR_TYPE_STRING,
  SENSOR_TYPE_FOO,   // ← add before SENSOR_TYPE_LAST
  SENSOR_TYPE_LAST
} sensor_type_t;

typedef union {
  float f;
  //...
  foo_t foo;         // ← add
} sensor_value_t;

int sensor_channel_update_foo(sensor_channel_t *ch, foo_t value);
```

**2. `server/src/sensor.c`**

Implement the update helper:

```c
int sensor_channel_update_foo(sensor_channel_t *ch, foo_t value)
{
  if (!ch || ch->type != SENSOR_TYPE_FOO) {
    return -1;
  }
  ch->value.foo = value;
  ch->has_value = true;
  return 0;
}
```

**3. `server/src/snapshot_parser.c`**

Extend the value validation to accept the new JSON type,
then parse it in the switch:

```c
// In parse_reading(), extend the guard:
if (!cJSON_IsNumber(v) && !cJSON_IsString(v) && !cJSON_IsBool(v) /* && !cJSON_Is<Foo>(v) */) {

// In the type switch, add:
case SENSOR_TYPE_FOO:
  out->value.foo = /* extract from cJSON item */;
  break;
```

**4. `server/src/coap_server.c`**

In the switch that dispatches to update helpers, add:

```c
case SENSOR_TYPE_FOO:
  sensor_channel_update_foo(ch, r->value.foo);
  break;
```

**5. `server/src/db.c`**

Add a column to the `readings` table schema:

```c
"  value_text  TEXT,"
"  value_foo   <SQL_TYPE>"   // ← add (e.g. BOOLEAN, INTEGER, REAL, TEXT)
```

Update the `INSERT` statement to include the new column (increment the `VALUES` placeholder count accordingly):

```c
"INSERT INTO readings "
"(channel_id, timestamp, value_float, value_int, value_text, value_foo) "
"VALUES (?, ?, ?, ?, ?, ?)",
```

Finally, bind the value in the type switch, using the right `sqlite3_bind_*` call:

```c
case SENSOR_TYPE_FOO:
  rc = sqlite3_bind_<type>(stmt, 6 /* column index */, ch->value.foo);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_bind_<type> failed: %s\n", sqlite3_errmsg(g_db));
    goto error;
  }
  break;
```

> **Note:** If you add a new column to an existing database, you must either drop and recreate it or run `ALTER TABLE readings ADD COLUMN value_foo <SQL_TYPE>;` manually. The server does not perform schema migrations.

---

## Adding a New Data Source

A data source is anything that produces named readings (sensor, GNSS module,
battery monitor, etc.) It implements the `data_source_t` interface:

```c
// firmware/app/include/data_source.h

typedef struct {
  const char *name;    // human-readable label, e.g. "lsm303agr"
  int (*init)(void);   // called once at boot: register channels, init hardware
  int (*read)(void);   // called every sampling cycle: read hardware, update channels
} data_source_t;
```

### Steps

**1. Create `firmware/app/src/your_source.c`**

```c
#include <errno.h>
#include <zephyr/logging/log.h>
#include "data_source.h"
#include "sensor.h"

LOG_MODULE_REGISTER(your_source, LOG_LEVEL_DBG);

static sensor_channel_t *ch_value;

static int your_source_init(void)
{
    // Configure hardware here (device_is_ready, i2c_configure, …)

    ch_value = sensor_channel_register("your_channel", SENSOR_TYPE_FLOAT);
    if (!ch_value) {
        LOG_ERR("Failed to register channel");
        return -ENOMEM;
    }
    return 0;
}

static int your_source_read(void)
{
    if (!ch_value) return -EINVAL;

    float val = /* read from hardware */;

    return sensor_channel_update_float(ch_value, val);
}

const data_source_t my_sensor_source = {
    .name = "your_source",
    .init = my_sensor_init,
    .read = my_sensor_read,
};
```

A source can register **multiple channels** in `init()`, one struct per
measured quantity (e.g. a BME280 would register `temperature`, `humidity`,
and `pressure`).

**2. Register it in `firmware/app/src/sensor_config.c`**

```c
extern data_source_t my_sensor_source; // ← add

data_source_t *g_data_sources[] = {
    &temperature_sensor_source,
    &my_sensor_source,                 // ← add
};
const size_t g_data_source_count = ARRAY_SIZE(g_data_sources);
```

`sources_init_all()` and `sources_read_all()` will pick it up automatically on the next build.
