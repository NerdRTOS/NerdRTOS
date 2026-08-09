#include "phy.h"

#include "eth.h"
#include "lan8742.h"

static lan8742_Object_t phy;

static int32_t phy_io_init(void);
static int32_t phy_io_deinit(void);
static int32_t phy_io_read(uint32_t phy_address,
                           uint32_t register_address,
                           uint32_t *value);
static int32_t phy_io_write(uint32_t phy_address,
                            uint32_t register_address,
                            uint32_t value);
static int32_t phy_io_get_tick(void);

static lan8742_IOCtx_t phy_io = {
    .Init     = phy_io_init,
    .DeInit   = phy_io_deinit,
    .WriteReg = phy_io_write,
    .ReadReg  = phy_io_read,
    .GetTick  = phy_io_get_tick,
};

static int32_t phy_io_init(void)
{
    HAL_ETH_SetMDIOClockRange(&g_eth_handle);

    return LAN8742_STATUS_OK;
}

static int32_t phy_io_deinit(void)
{
    return LAN8742_STATUS_OK;
}

static int32_t phy_io_read(uint32_t phy_address,
                           uint32_t register_address,
                           uint32_t *value)
{
    return HAL_ETH_ReadPHYRegister(&g_eth_handle,
                                   phy_address,
                                   register_address,
                                   value) == HAL_OK
               ? LAN8742_STATUS_OK
               : LAN8742_STATUS_READ_ERROR;
}

static int32_t phy_io_write(uint32_t phy_address,
                            uint32_t register_address,
                            uint32_t value)
{
    return HAL_ETH_WritePHYRegister(&g_eth_handle,
                                    phy_address,
                                    register_address,
                                    value) == HAL_OK
               ? LAN8742_STATUS_OK
               : LAN8742_STATUS_WRITE_ERROR;
}

static int32_t phy_io_get_tick(void)
{
    return (int32_t)HAL_GetTick();
}

int32_t phy_init(void)
{
    int32_t status;

    status = LAN8742_RegisterBusIO(&phy, &phy_io);
    if (status != LAN8742_STATUS_OK) {
        return status;
    }

    status = LAN8742_Init(&phy);
    if (status != LAN8742_STATUS_OK) {
        return status;
    }

    return LAN8742_StartAutoNego(&phy);
}

int32_t phy_get_link_state(void)
{
    return LAN8742_GetLinkState(&phy);
}
