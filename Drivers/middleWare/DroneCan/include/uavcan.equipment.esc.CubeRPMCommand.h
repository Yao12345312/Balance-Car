
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <canard.h>




#define UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_MAX_SIZE 46
#define UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_SIGNATURE (0x52B08320C4B2AADFULL)

#define UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_ID 1038





#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
class uavcan_equipment_esc_CubeRPMCommand_cxx_iface;
#endif


struct uavcan_equipment_esc_CubeRPMCommand {

#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
    using cxx_iface = uavcan_equipment_esc_CubeRPMCommand_cxx_iface;
#endif




    uint8_t arm;



    struct { uint8_t len; int32_t data[20]; }rpm;



};

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t _uavcan_equipment_esc_CubeRPMCommand_encode(struct uavcan_equipment_esc_CubeRPMCommand* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
);
bool _uavcan_equipment_esc_CubeRPMCommand_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeRPMCommand* msg);

static inline uint32_t uavcan_equipment_esc_CubeRPMCommand_encode(struct uavcan_equipment_esc_CubeRPMCommand* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
) {

    return _uavcan_equipment_esc_CubeRPMCommand_encode(msg, buffer
#if CANARD_ENABLE_TAO_OPTION
    , tao
#endif
    );

}

static inline bool uavcan_equipment_esc_CubeRPMCommand_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeRPMCommand* msg) {

    return _uavcan_equipment_esc_CubeRPMCommand_decode(transfer, msg);

}

#if defined(CANARD_DSDLC_INTERNAL)

static inline void __uavcan_equipment_esc_CubeRPMCommand_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeRPMCommand* msg, bool tao);
static inline bool __uavcan_equipment_esc_CubeRPMCommand_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeRPMCommand* msg, bool tao);
void __uavcan_equipment_esc_CubeRPMCommand_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeRPMCommand* msg, bool tao) {

    (void)buffer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;






    canardEncodeScalar(buffer, *bit_ofs, 2, &msg->arm);

    *bit_ofs += 2;






#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    const uint8_t rpm_len = msg->rpm.len > 20 ? 20 : msg->rpm.len;
#pragma GCC diagnostic pop

    if (!tao) {


        canardEncodeScalar(buffer, *bit_ofs, 5, &rpm_len);
        *bit_ofs += 5;


    }

    for (size_t i=0; i < rpm_len; i++) {




        canardEncodeScalar(buffer, *bit_ofs, 18, &msg->rpm.data[i]);

        *bit_ofs += 18;


    }





}

/*
 decode uavcan_equipment_esc_CubeRPMCommand, return true on failure, false on success
*/
bool __uavcan_equipment_esc_CubeRPMCommand_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeRPMCommand* msg, bool tao) {

    (void)transfer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;





    canardDecodeScalar(transfer, *bit_ofs, 2, false, &msg->arm);

    *bit_ofs += 2;








    if (!tao) {


        canardDecodeScalar(transfer, *bit_ofs, 5, false, &msg->rpm.len);
        *bit_ofs += 5;



    } else {

        msg->rpm.len = ((transfer->payload_len*8)-*bit_ofs)/18;


    }



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if (msg->rpm.len > 20) {
        return true; /* invalid value */
    }
#pragma GCC diagnostic pop
    for (size_t i=0; i < msg->rpm.len; i++) {




        canardDecodeScalar(transfer, *bit_ofs, 18, true, &msg->rpm.data[i]);

        *bit_ofs += 18;


    }






    return false; /* success */

}
#endif
#ifdef CANARD_DSDLC_TEST_BUILD
struct uavcan_equipment_esc_CubeRPMCommand sample_uavcan_equipment_esc_CubeRPMCommand_msg(void);
#endif
#ifdef __cplusplus
} // extern "C"

#ifdef DRONECAN_CXX_WRAPPERS
#include <canard/cxx_wrappers.h>


BROADCAST_MESSAGE_CXX_IFACE(uavcan_equipment_esc_CubeRPMCommand, UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_ID, UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_SIGNATURE, UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_MAX_SIZE);


#endif
#endif
