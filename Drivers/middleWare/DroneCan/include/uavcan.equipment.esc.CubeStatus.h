
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <canard.h>




#define UAVCAN_EQUIPMENT_ESC_CUBESTATUS_MAX_SIZE 16
#define UAVCAN_EQUIPMENT_ESC_CUBESTATUS_SIGNATURE (0xAF01D0F9D9D06C08ULL)

#define UAVCAN_EQUIPMENT_ESC_CUBESTATUS_ID 1035





#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
class uavcan_equipment_esc_CubeStatus_cxx_iface;
#endif


struct uavcan_equipment_esc_CubeStatus {

#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
    using cxx_iface = uavcan_equipment_esc_CubeStatus_cxx_iface;
#endif




    uint8_t calib_done;



    float voltage;



    float current;



    float temperature;



    float id;



    float iq;



    int32_t rpm;



    int32_t target_rpm;



    uint8_t esc_index;



};

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t _uavcan_equipment_esc_CubeStatus_encode(struct uavcan_equipment_esc_CubeStatus* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
);
bool _uavcan_equipment_esc_CubeStatus_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeStatus* msg);

static inline uint32_t uavcan_equipment_esc_CubeStatus_encode(struct uavcan_equipment_esc_CubeStatus* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
) {

    return _uavcan_equipment_esc_CubeStatus_encode(msg, buffer
#if CANARD_ENABLE_TAO_OPTION
    , tao
#endif
    );

}

static inline bool uavcan_equipment_esc_CubeStatus_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeStatus* msg) {

    return _uavcan_equipment_esc_CubeStatus_decode(transfer, msg);

}

#if defined(CANARD_DSDLC_INTERNAL)

static inline void __uavcan_equipment_esc_CubeStatus_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeStatus* msg, bool tao);
static inline bool __uavcan_equipment_esc_CubeStatus_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeStatus* msg, bool tao);
void __uavcan_equipment_esc_CubeStatus_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeStatus* msg, bool tao) {

    (void)buffer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;






    canardEncodeScalar(buffer, *bit_ofs, 2, &msg->calib_done);

    *bit_ofs += 2;






    {
        uint16_t float16_val = canardConvertNativeFloatToFloat16(msg->voltage);
        canardEncodeScalar(buffer, *bit_ofs, 16, &float16_val);
    }

    *bit_ofs += 16;






    {
        uint16_t float16_val = canardConvertNativeFloatToFloat16(msg->current);
        canardEncodeScalar(buffer, *bit_ofs, 16, &float16_val);
    }

    *bit_ofs += 16;






    {
        uint16_t float16_val = canardConvertNativeFloatToFloat16(msg->temperature);
        canardEncodeScalar(buffer, *bit_ofs, 16, &float16_val);
    }

    *bit_ofs += 16;






    {
        uint16_t float16_val = canardConvertNativeFloatToFloat16(msg->id);
        canardEncodeScalar(buffer, *bit_ofs, 16, &float16_val);
    }

    *bit_ofs += 16;






    {
        uint16_t float16_val = canardConvertNativeFloatToFloat16(msg->iq);
        canardEncodeScalar(buffer, *bit_ofs, 16, &float16_val);
    }

    *bit_ofs += 16;






    canardEncodeScalar(buffer, *bit_ofs, 18, &msg->rpm);

    *bit_ofs += 18;






    canardEncodeScalar(buffer, *bit_ofs, 18, &msg->target_rpm);

    *bit_ofs += 18;






    canardEncodeScalar(buffer, *bit_ofs, 5, &msg->esc_index);

    *bit_ofs += 5;





}

/*
 decode uavcan_equipment_esc_CubeStatus, return true on failure, false on success
*/
bool __uavcan_equipment_esc_CubeStatus_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeStatus* msg, bool tao) {

    (void)transfer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;





    canardDecodeScalar(transfer, *bit_ofs, 2, false, &msg->calib_done);

    *bit_ofs += 2;







    {
        uint16_t float16_val;
        canardDecodeScalar(transfer, *bit_ofs, 16, true, &float16_val);
        msg->voltage = canardConvertFloat16ToNativeFloat(float16_val);
    }

    *bit_ofs += 16;







    {
        uint16_t float16_val;
        canardDecodeScalar(transfer, *bit_ofs, 16, true, &float16_val);
        msg->current = canardConvertFloat16ToNativeFloat(float16_val);
    }

    *bit_ofs += 16;







    {
        uint16_t float16_val;
        canardDecodeScalar(transfer, *bit_ofs, 16, true, &float16_val);
        msg->temperature = canardConvertFloat16ToNativeFloat(float16_val);
    }

    *bit_ofs += 16;







    {
        uint16_t float16_val;
        canardDecodeScalar(transfer, *bit_ofs, 16, true, &float16_val);
        msg->id = canardConvertFloat16ToNativeFloat(float16_val);
    }

    *bit_ofs += 16;







    {
        uint16_t float16_val;
        canardDecodeScalar(transfer, *bit_ofs, 16, true, &float16_val);
        msg->iq = canardConvertFloat16ToNativeFloat(float16_val);
    }

    *bit_ofs += 16;







    canardDecodeScalar(transfer, *bit_ofs, 18, true, &msg->rpm);

    *bit_ofs += 18;







    canardDecodeScalar(transfer, *bit_ofs, 18, true, &msg->target_rpm);

    *bit_ofs += 18;







    canardDecodeScalar(transfer, *bit_ofs, 5, false, &msg->esc_index);

    *bit_ofs += 5;





    return false; /* success */

}
#endif
#ifdef CANARD_DSDLC_TEST_BUILD
struct uavcan_equipment_esc_CubeStatus sample_uavcan_equipment_esc_CubeStatus_msg(void);
#endif
#ifdef __cplusplus
} // extern "C"

#ifdef DRONECAN_CXX_WRAPPERS
#include <canard/cxx_wrappers.h>


BROADCAST_MESSAGE_CXX_IFACE(uavcan_equipment_esc_CubeStatus, UAVCAN_EQUIPMENT_ESC_CUBESTATUS_ID, UAVCAN_EQUIPMENT_ESC_CUBESTATUS_SIGNATURE, UAVCAN_EQUIPMENT_ESC_CUBESTATUS_MAX_SIZE);


#endif
#endif
