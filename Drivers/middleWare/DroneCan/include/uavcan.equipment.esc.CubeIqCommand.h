
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <canard.h>




#define UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_MAX_SIZE 46
#define UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_SIGNATURE (0xE0F5997C47F2068EULL)

#define UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_ID 1039





#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
class uavcan_equipment_esc_CubeIqCommand_cxx_iface;
#endif


struct uavcan_equipment_esc_CubeIqCommand {

#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
    using cxx_iface = uavcan_equipment_esc_CubeIqCommand_cxx_iface;
#endif




    uint8_t arm;



    struct { uint8_t len; int32_t data[20]; }Iq;



};

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t _uavcan_equipment_esc_CubeIqCommand_encode(struct uavcan_equipment_esc_CubeIqCommand* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
);
bool _uavcan_equipment_esc_CubeIqCommand_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeIqCommand* msg);

static inline uint32_t uavcan_equipment_esc_CubeIqCommand_encode(struct uavcan_equipment_esc_CubeIqCommand* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
) {

    return _uavcan_equipment_esc_CubeIqCommand_encode(msg, buffer
#if CANARD_ENABLE_TAO_OPTION
    , tao
#endif
    );

}

static inline bool uavcan_equipment_esc_CubeIqCommand_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeIqCommand* msg) {

    return _uavcan_equipment_esc_CubeIqCommand_decode(transfer, msg);

}

#if defined(CANARD_DSDLC_INTERNAL)

static inline void __uavcan_equipment_esc_CubeIqCommand_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeIqCommand* msg, bool tao);
static inline bool __uavcan_equipment_esc_CubeIqCommand_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeIqCommand* msg, bool tao);
void __uavcan_equipment_esc_CubeIqCommand_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeIqCommand* msg, bool tao) {

    (void)buffer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;






    canardEncodeScalar(buffer, *bit_ofs, 2, &msg->arm);

    *bit_ofs += 2;






#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    const uint8_t Iq_len = msg->Iq.len > 20 ? 20 : msg->Iq.len;
#pragma GCC diagnostic pop

    if (!tao) {


        canardEncodeScalar(buffer, *bit_ofs, 5, &Iq_len);
        *bit_ofs += 5;


    }

    for (size_t i=0; i < Iq_len; i++) {




        canardEncodeScalar(buffer, *bit_ofs, 18, &msg->Iq.data[i]);

        *bit_ofs += 18;


    }





}

/*
 decode uavcan_equipment_esc_CubeIqCommand, return true on failure, false on success
*/
bool __uavcan_equipment_esc_CubeIqCommand_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeIqCommand* msg, bool tao) {

    (void)transfer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;





    canardDecodeScalar(transfer, *bit_ofs, 2, false, &msg->arm);

    *bit_ofs += 2;








    if (!tao) {


        canardDecodeScalar(transfer, *bit_ofs, 5, false, &msg->Iq.len);
        *bit_ofs += 5;



    } else {

        msg->Iq.len = ((transfer->payload_len*8)-*bit_ofs)/18;


    }



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if (msg->Iq.len > 20) {
        return true; /* invalid value */
    }
#pragma GCC diagnostic pop
    for (size_t i=0; i < msg->Iq.len; i++) {




        canardDecodeScalar(transfer, *bit_ofs, 18, true, &msg->Iq.data[i]);

        *bit_ofs += 18;


    }






    return false; /* success */

}
#endif
#ifdef CANARD_DSDLC_TEST_BUILD
struct uavcan_equipment_esc_CubeIqCommand sample_uavcan_equipment_esc_CubeIqCommand_msg(void);
#endif
#ifdef __cplusplus
} // extern "C"

#ifdef DRONECAN_CXX_WRAPPERS
#include <canard/cxx_wrappers.h>


BROADCAST_MESSAGE_CXX_IFACE(uavcan_equipment_esc_CubeIqCommand, UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_ID, UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_SIGNATURE, UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_MAX_SIZE);


#endif
#endif
