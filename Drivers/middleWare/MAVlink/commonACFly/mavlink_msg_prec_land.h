#pragma once
// MESSAGE PREC_LAND PACKING

#define MAVLINK_MSG_ID_PREC_LAND 145


typedef struct __mavlink_prec_land_t {
 double lat; /*< [degE7] Latitude*/
 double lon; /*< [degE7] Longitude*/
 float alt; /*< [m] Altitude*/
} mavlink_prec_land_t;

#define MAVLINK_MSG_ID_PREC_LAND_LEN 20
#define MAVLINK_MSG_ID_PREC_LAND_MIN_LEN 20
#define MAVLINK_MSG_ID_145_LEN 20
#define MAVLINK_MSG_ID_145_MIN_LEN 20

#define MAVLINK_MSG_ID_PREC_LAND_CRC 81
#define MAVLINK_MSG_ID_145_CRC 81



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_PREC_LAND { \
    145, \
    "PREC_LAND", \
    3, \
    {  { "lat", NULL, MAVLINK_TYPE_DOUBLE, 0, 0, offsetof(mavlink_prec_land_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_DOUBLE, 0, 8, offsetof(mavlink_prec_land_t, lon) }, \
         { "alt", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_prec_land_t, alt) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_PREC_LAND { \
    "PREC_LAND", \
    3, \
    {  { "lat", NULL, MAVLINK_TYPE_DOUBLE, 0, 0, offsetof(mavlink_prec_land_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_DOUBLE, 0, 8, offsetof(mavlink_prec_land_t, lon) }, \
         { "alt", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_prec_land_t, alt) }, \
         } \
}
#endif

/**
 * @brief Pack a prec_land message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param lat [degE7] Latitude
 * @param lon [degE7] Longitude
 * @param alt [m] Altitude
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_prec_land_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               double lat, double lon, float alt)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PREC_LAND_LEN];
    _mav_put_double(buf, 0, lat);
    _mav_put_double(buf, 8, lon);
    _mav_put_float(buf, 16, alt);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_PREC_LAND_LEN);
#else
    mavlink_prec_land_t packet;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_PREC_LAND_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_PREC_LAND;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
}

/**
 * @brief Pack a prec_land message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param lat [degE7] Latitude
 * @param lon [degE7] Longitude
 * @param alt [m] Altitude
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_prec_land_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               double lat, double lon, float alt)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PREC_LAND_LEN];
    _mav_put_double(buf, 0, lat);
    _mav_put_double(buf, 8, lon);
    _mav_put_float(buf, 16, alt);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_PREC_LAND_LEN);
#else
    mavlink_prec_land_t packet;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_PREC_LAND_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_PREC_LAND;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN);
#endif
}

/**
 * @brief Pack a prec_land message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param lat [degE7] Latitude
 * @param lon [degE7] Longitude
 * @param alt [m] Altitude
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_prec_land_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   double lat,double lon,float alt)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PREC_LAND_LEN];
    _mav_put_double(buf, 0, lat);
    _mav_put_double(buf, 8, lon);
    _mav_put_float(buf, 16, alt);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_PREC_LAND_LEN);
#else
    mavlink_prec_land_t packet;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_PREC_LAND_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_PREC_LAND;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
}

/**
 * @brief Encode a prec_land struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param prec_land C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_prec_land_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_prec_land_t* prec_land)
{
    return mavlink_msg_prec_land_pack(system_id, component_id, msg, prec_land->lat, prec_land->lon, prec_land->alt);
}

/**
 * @brief Encode a prec_land struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param prec_land C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_prec_land_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_prec_land_t* prec_land)
{
    return mavlink_msg_prec_land_pack_chan(system_id, component_id, chan, msg, prec_land->lat, prec_land->lon, prec_land->alt);
}

/**
 * @brief Encode a prec_land struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param prec_land C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_prec_land_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_prec_land_t* prec_land)
{
    return mavlink_msg_prec_land_pack_status(system_id, component_id, _status, msg,  prec_land->lat, prec_land->lon, prec_land->alt);
}

/**
 * @brief Send a prec_land message
 * @param chan MAVLink channel to send the message
 *
 * @param lat [degE7] Latitude
 * @param lon [degE7] Longitude
 * @param alt [m] Altitude
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_prec_land_send(mavlink_channel_t chan, double lat, double lon, float alt)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PREC_LAND_LEN];
    _mav_put_double(buf, 0, lat);
    _mav_put_double(buf, 8, lon);
    _mav_put_float(buf, 16, alt);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PREC_LAND, buf, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
#else
    mavlink_prec_land_t packet;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PREC_LAND, (const char *)&packet, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
#endif
}

/**
 * @brief Send a prec_land message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_prec_land_send_struct(mavlink_channel_t chan, const mavlink_prec_land_t* prec_land)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_prec_land_send(chan, prec_land->lat, prec_land->lon, prec_land->alt);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PREC_LAND, (const char *)prec_land, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
#endif
}

#if MAVLINK_MSG_ID_PREC_LAND_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_prec_land_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  double lat, double lon, float alt)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_double(buf, 0, lat);
    _mav_put_double(buf, 8, lon);
    _mav_put_float(buf, 16, alt);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PREC_LAND, buf, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
#else
    mavlink_prec_land_t *packet = (mavlink_prec_land_t *)msgbuf;
    packet->lat = lat;
    packet->lon = lon;
    packet->alt = alt;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PREC_LAND, (const char *)packet, MAVLINK_MSG_ID_PREC_LAND_MIN_LEN, MAVLINK_MSG_ID_PREC_LAND_LEN, MAVLINK_MSG_ID_PREC_LAND_CRC);
#endif
}
#endif

#endif

// MESSAGE PREC_LAND UNPACKING


/**
 * @brief Get field lat from prec_land message
 *
 * @return [degE7] Latitude
 */
static inline double mavlink_msg_prec_land_get_lat(const mavlink_message_t* msg)
{
    return _MAV_RETURN_double(msg,  0);
}

/**
 * @brief Get field lon from prec_land message
 *
 * @return [degE7] Longitude
 */
static inline double mavlink_msg_prec_land_get_lon(const mavlink_message_t* msg)
{
    return _MAV_RETURN_double(msg,  8);
}

/**
 * @brief Get field alt from prec_land message
 *
 * @return [m] Altitude
 */
static inline float mavlink_msg_prec_land_get_alt(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Decode a prec_land message into a struct
 *
 * @param msg The message to decode
 * @param prec_land C-struct to decode the message contents into
 */
static inline void mavlink_msg_prec_land_decode(const mavlink_message_t* msg, mavlink_prec_land_t* prec_land)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    prec_land->lat = mavlink_msg_prec_land_get_lat(msg);
    prec_land->lon = mavlink_msg_prec_land_get_lon(msg);
    prec_land->alt = mavlink_msg_prec_land_get_alt(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_PREC_LAND_LEN? msg->len : MAVLINK_MSG_ID_PREC_LAND_LEN;
        memset(prec_land, 0, MAVLINK_MSG_ID_PREC_LAND_LEN);
    memcpy(prec_land, _MAV_PAYLOAD(msg), len);
#endif
}
