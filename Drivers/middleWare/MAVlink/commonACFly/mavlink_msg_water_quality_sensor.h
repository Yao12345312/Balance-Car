#pragma once
// MESSAGE WATER_QUALITY_SENSOR PACKING

#define MAVLINK_MSG_ID_WATER_QUALITY_SENSOR 352


typedef struct __mavlink_water_quality_sensor_t {
 float temperature; /*<  temperature of conductivity sensor type*/
 float pH; /*<  pH*/
 float dissolved_oxygen; /*<  Dissolved oxygen*/
 float turbidity; /*<  Turbidity*/
 float conductivity; /*<  Conductivity*/
 uint8_t type; /*<  Type of water quality sensor.*/
} mavlink_water_quality_sensor_t;

#define MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN 21
#define MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN 21
#define MAVLINK_MSG_ID_352_LEN 21
#define MAVLINK_MSG_ID_352_MIN_LEN 21

#define MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC 149
#define MAVLINK_MSG_ID_352_CRC 149



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_WATER_QUALITY_SENSOR { \
    352, \
    "WATER_QUALITY_SENSOR", \
    6, \
    {  { "temperature", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_water_quality_sensor_t, temperature) }, \
         { "pH", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_water_quality_sensor_t, pH) }, \
         { "dissolved_oxygen", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_water_quality_sensor_t, dissolved_oxygen) }, \
         { "turbidity", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_water_quality_sensor_t, turbidity) }, \
         { "conductivity", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_water_quality_sensor_t, conductivity) }, \
         { "type", NULL, MAVLINK_TYPE_UINT8_T, 0, 20, offsetof(mavlink_water_quality_sensor_t, type) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_WATER_QUALITY_SENSOR { \
    "WATER_QUALITY_SENSOR", \
    6, \
    {  { "temperature", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_water_quality_sensor_t, temperature) }, \
         { "pH", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_water_quality_sensor_t, pH) }, \
         { "dissolved_oxygen", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_water_quality_sensor_t, dissolved_oxygen) }, \
         { "turbidity", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_water_quality_sensor_t, turbidity) }, \
         { "conductivity", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_water_quality_sensor_t, conductivity) }, \
         { "type", NULL, MAVLINK_TYPE_UINT8_T, 0, 20, offsetof(mavlink_water_quality_sensor_t, type) }, \
         } \
}
#endif

/**
 * @brief Pack a water_quality_sensor message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param temperature  temperature of conductivity sensor type
 * @param pH  pH
 * @param dissolved_oxygen  Dissolved oxygen
 * @param turbidity  Turbidity
 * @param conductivity  Conductivity
 * @param type  Type of water quality sensor.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_water_quality_sensor_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               float temperature, float pH, float dissolved_oxygen, float turbidity, float conductivity, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN];
    _mav_put_float(buf, 0, temperature);
    _mav_put_float(buf, 4, pH);
    _mav_put_float(buf, 8, dissolved_oxygen);
    _mav_put_float(buf, 12, turbidity);
    _mav_put_float(buf, 16, conductivity);
    _mav_put_uint8_t(buf, 20, type);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
#else
    mavlink_water_quality_sensor_t packet;
    packet.temperature = temperature;
    packet.pH = pH;
    packet.dissolved_oxygen = dissolved_oxygen;
    packet.turbidity = turbidity;
    packet.conductivity = conductivity;
    packet.type = type;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_WATER_QUALITY_SENSOR;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
}

/**
 * @brief Pack a water_quality_sensor message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param temperature  temperature of conductivity sensor type
 * @param pH  pH
 * @param dissolved_oxygen  Dissolved oxygen
 * @param turbidity  Turbidity
 * @param conductivity  Conductivity
 * @param type  Type of water quality sensor.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_water_quality_sensor_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               float temperature, float pH, float dissolved_oxygen, float turbidity, float conductivity, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN];
    _mav_put_float(buf, 0, temperature);
    _mav_put_float(buf, 4, pH);
    _mav_put_float(buf, 8, dissolved_oxygen);
    _mav_put_float(buf, 12, turbidity);
    _mav_put_float(buf, 16, conductivity);
    _mav_put_uint8_t(buf, 20, type);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
#else
    mavlink_water_quality_sensor_t packet;
    packet.temperature = temperature;
    packet.pH = pH;
    packet.dissolved_oxygen = dissolved_oxygen;
    packet.turbidity = turbidity;
    packet.conductivity = conductivity;
    packet.type = type;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_WATER_QUALITY_SENSOR;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
#endif
}

/**
 * @brief Pack a water_quality_sensor message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param temperature  temperature of conductivity sensor type
 * @param pH  pH
 * @param dissolved_oxygen  Dissolved oxygen
 * @param turbidity  Turbidity
 * @param conductivity  Conductivity
 * @param type  Type of water quality sensor.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_water_quality_sensor_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   float temperature,float pH,float dissolved_oxygen,float turbidity,float conductivity,uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN];
    _mav_put_float(buf, 0, temperature);
    _mav_put_float(buf, 4, pH);
    _mav_put_float(buf, 8, dissolved_oxygen);
    _mav_put_float(buf, 12, turbidity);
    _mav_put_float(buf, 16, conductivity);
    _mav_put_uint8_t(buf, 20, type);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
#else
    mavlink_water_quality_sensor_t packet;
    packet.temperature = temperature;
    packet.pH = pH;
    packet.dissolved_oxygen = dissolved_oxygen;
    packet.turbidity = turbidity;
    packet.conductivity = conductivity;
    packet.type = type;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_WATER_QUALITY_SENSOR;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
}

/**
 * @brief Encode a water_quality_sensor struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param water_quality_sensor C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_water_quality_sensor_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_water_quality_sensor_t* water_quality_sensor)
{
    return mavlink_msg_water_quality_sensor_pack(system_id, component_id, msg, water_quality_sensor->temperature, water_quality_sensor->pH, water_quality_sensor->dissolved_oxygen, water_quality_sensor->turbidity, water_quality_sensor->conductivity, water_quality_sensor->type);
}

/**
 * @brief Encode a water_quality_sensor struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param water_quality_sensor C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_water_quality_sensor_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_water_quality_sensor_t* water_quality_sensor)
{
    return mavlink_msg_water_quality_sensor_pack_chan(system_id, component_id, chan, msg, water_quality_sensor->temperature, water_quality_sensor->pH, water_quality_sensor->dissolved_oxygen, water_quality_sensor->turbidity, water_quality_sensor->conductivity, water_quality_sensor->type);
}

/**
 * @brief Encode a water_quality_sensor struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param water_quality_sensor C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_water_quality_sensor_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_water_quality_sensor_t* water_quality_sensor)
{
    return mavlink_msg_water_quality_sensor_pack_status(system_id, component_id, _status, msg,  water_quality_sensor->temperature, water_quality_sensor->pH, water_quality_sensor->dissolved_oxygen, water_quality_sensor->turbidity, water_quality_sensor->conductivity, water_quality_sensor->type);
}

/**
 * @brief Send a water_quality_sensor message
 * @param chan MAVLink channel to send the message
 *
 * @param temperature  temperature of conductivity sensor type
 * @param pH  pH
 * @param dissolved_oxygen  Dissolved oxygen
 * @param turbidity  Turbidity
 * @param conductivity  Conductivity
 * @param type  Type of water quality sensor.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_water_quality_sensor_send(mavlink_channel_t chan, float temperature, float pH, float dissolved_oxygen, float turbidity, float conductivity, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN];
    _mav_put_float(buf, 0, temperature);
    _mav_put_float(buf, 4, pH);
    _mav_put_float(buf, 8, dissolved_oxygen);
    _mav_put_float(buf, 12, turbidity);
    _mav_put_float(buf, 16, conductivity);
    _mav_put_uint8_t(buf, 20, type);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR, buf, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
#else
    mavlink_water_quality_sensor_t packet;
    packet.temperature = temperature;
    packet.pH = pH;
    packet.dissolved_oxygen = dissolved_oxygen;
    packet.turbidity = turbidity;
    packet.conductivity = conductivity;
    packet.type = type;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR, (const char *)&packet, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
#endif
}

/**
 * @brief Send a water_quality_sensor message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_water_quality_sensor_send_struct(mavlink_channel_t chan, const mavlink_water_quality_sensor_t* water_quality_sensor)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_water_quality_sensor_send(chan, water_quality_sensor->temperature, water_quality_sensor->pH, water_quality_sensor->dissolved_oxygen, water_quality_sensor->turbidity, water_quality_sensor->conductivity, water_quality_sensor->type);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR, (const char *)water_quality_sensor, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
#endif
}

#if MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_water_quality_sensor_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  float temperature, float pH, float dissolved_oxygen, float turbidity, float conductivity, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_float(buf, 0, temperature);
    _mav_put_float(buf, 4, pH);
    _mav_put_float(buf, 8, dissolved_oxygen);
    _mav_put_float(buf, 12, turbidity);
    _mav_put_float(buf, 16, conductivity);
    _mav_put_uint8_t(buf, 20, type);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR, buf, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
#else
    mavlink_water_quality_sensor_t *packet = (mavlink_water_quality_sensor_t *)msgbuf;
    packet->temperature = temperature;
    packet->pH = pH;
    packet->dissolved_oxygen = dissolved_oxygen;
    packet->turbidity = turbidity;
    packet->conductivity = conductivity;
    packet->type = type;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR, (const char *)packet, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_MIN_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_CRC);
#endif
}
#endif

#endif

// MESSAGE WATER_QUALITY_SENSOR UNPACKING


/**
 * @brief Get field temperature from water_quality_sensor message
 *
 * @return  temperature of conductivity sensor type
 */
static inline float mavlink_msg_water_quality_sensor_get_temperature(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  0);
}

/**
 * @brief Get field pH from water_quality_sensor message
 *
 * @return  pH
 */
static inline float mavlink_msg_water_quality_sensor_get_pH(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field dissolved_oxygen from water_quality_sensor message
 *
 * @return  Dissolved oxygen
 */
static inline float mavlink_msg_water_quality_sensor_get_dissolved_oxygen(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field turbidity from water_quality_sensor message
 *
 * @return  Turbidity
 */
static inline float mavlink_msg_water_quality_sensor_get_turbidity(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field conductivity from water_quality_sensor message
 *
 * @return  Conductivity
 */
static inline float mavlink_msg_water_quality_sensor_get_conductivity(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field type from water_quality_sensor message
 *
 * @return  Type of water quality sensor.
 */
static inline uint8_t mavlink_msg_water_quality_sensor_get_type(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  20);
}

/**
 * @brief Decode a water_quality_sensor message into a struct
 *
 * @param msg The message to decode
 * @param water_quality_sensor C-struct to decode the message contents into
 */
static inline void mavlink_msg_water_quality_sensor_decode(const mavlink_message_t* msg, mavlink_water_quality_sensor_t* water_quality_sensor)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    water_quality_sensor->temperature = mavlink_msg_water_quality_sensor_get_temperature(msg);
    water_quality_sensor->pH = mavlink_msg_water_quality_sensor_get_pH(msg);
    water_quality_sensor->dissolved_oxygen = mavlink_msg_water_quality_sensor_get_dissolved_oxygen(msg);
    water_quality_sensor->turbidity = mavlink_msg_water_quality_sensor_get_turbidity(msg);
    water_quality_sensor->conductivity = mavlink_msg_water_quality_sensor_get_conductivity(msg);
    water_quality_sensor->type = mavlink_msg_water_quality_sensor_get_type(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN? msg->len : MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN;
        memset(water_quality_sensor, 0, MAVLINK_MSG_ID_WATER_QUALITY_SENSOR_LEN);
    memcpy(water_quality_sensor, _MAV_PAYLOAD(msg), len);
#endif
}
