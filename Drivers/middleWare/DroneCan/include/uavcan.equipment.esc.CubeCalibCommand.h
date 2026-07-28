
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <canard.h>




#define UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_MAX_SIZE 1
#define UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_SIGNATURE (0x5FCE7C0B5A56C35EULL)

#define UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_ID 1037





#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
class uavcan_equipment_esc_CubeCalibCommand_cxx_iface;
#endif


struct uavcan_equipment_esc_CubeCalibCommand {

#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
    using cxx_iface = uavcan_equipment_esc_CubeCalibCommand_cxx_iface;
#endif




    uint8_t esc_index;



};

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t _uavcan_equipment_esc_CubeCalibCommand_encode(struct uavcan_equipment_esc_CubeCalibCommand* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
);
bool _uavcan_equipment_esc_CubeCalibCommand_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeCalibCommand* msg);

static inline uint32_t uavcan_equipment_esc_CubeCalibCommand_encode(struct uavcan_equipment_esc_CubeCalibCommand* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
) {

    return _uavcan_equipment_esc_CubeCalibCommand_encode(msg, buffer
#if CANARD_ENABLE_TAO_OPTION
    , tao
#endif
    );

}

static inline bool uavcan_equipment_esc_CubeCalibCommand_decode(const CanardRxTransfer* transfer, struct uavcan_equipment_esc_CubeCalibCommand* msg) {

    return _uavcan_equipment_esc_CubeCalibCommand_decode(transfer, msg);

}

#if defined(CANARD_DSDLC_INTERNAL)

static inline void __uavcan_equipment_esc_CubeCalibCommand_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeCalibCommand* msg, bool tao);
static inline bool __uavcan_equipment_esc_CubeCalibCommand_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeCalibCommand* msg, bool tao);
void __uavcan_equipment_esc_CubeCalibCommand_encode(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeCalibCommand* msg, bool tao) {

    (void)buffer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;






    canardEncodeScalar(buffer, *bit_ofs, 5, &msg->esc_index);

    *bit_ofs += 5;





}

/*
 decode uavcan_equipment_esc_CubeCalibCommand, return true on failure, false on success
*/
bool __uavcan_equipment_esc_CubeCalibCommand_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_equipment_esc_CubeCalibCommand* msg, bool tao) {

    (void)transfer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;





    canardDecodeScalar(transfer, *bit_ofs, 5, false, &msg->esc_index);

    *bit_ofs += 5;





    return false; /* success */

}
#endif
#ifdef CANARD_DSDLC_TEST_BUILD
struct uavcan_equipment_esc_CubeCalibCommand sample_uavcan_equipment_esc_CubeCalibCommand_msg(void);
#endif
#ifdef __cplusplus
} // extern "C"

#ifdef DRONECAN_CXX_WRAPPERS
#include <canard/cxx_wrappers.h>


BROADCAST_MESSAGE_CXX_IFACE(uavcan_equipment_esc_CubeCalibCommand, UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_ID, UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_SIGNATURE, UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_MAX_SIZE);


#endif
#endif
