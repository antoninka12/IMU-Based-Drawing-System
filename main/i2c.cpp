#include "esp_log.h" /*for logs and debugging*/

#include "i2c.hpp"

namespace AZ_i2c
{
    static const char* TAG="AZ_i2c";

    /*BUS API*/

    esp_err_t Bus::init(i2c_port_num_t port_nr, gpio_num_t sda, gpio_num_t scl, 
                        bool pull_up)
    {
        if (m_bus)
        return ESP_ERR_INVALID_STATE;

         i2c_master_bus_config_t configuration{}; /*creating structure*/

        configuration.i2c_port = port_nr;
        configuration.sda_io_num = sda;
        configuration.scl_io_num = scl;
        configuration.clk_source = I2C_CLK_SRC_DEFAULT;
        configuration.glitch_ignore_cnt = 7;
        configuration.intr_priority = 0; /*auto priority*/
        configuration.trans_queue_depth = 0; /*synchronic*/
        configuration.flags.enable_internal_pullup = pull_up; /*enable pull_up*/

        esp_err_t err = i2c_new_master_bus(&configuration, &m_bus);
        
        if(err!=ESP_OK){
            ESP_LOGE(TAG, "failure with i2c_new_master_bus: %s", esp_err_to_name(err));
            m_bus=nullptr;
            return err;
        }

        ESP_LOGI(TAG, "bus %d OK (SDA=%d SCL=%d)", (int)port_nr, (int)sda, (int)scl);
        return ESP_OK;
    }

    Bus::~Bus(){
        if(m_bus){
            i2c_del_master_bus(m_bus);
            m_bus=nullptr;
        }
    }

    esp_err_t Bus::probe(uint8_t addr, int time_out) const
    {
        if(!m_bus){
            return ESP_ERR_INVALID_STATE;
        }
        return i2c_master_probe(m_bus, addr, time_out);
    }

    void Bus::scan() const{
        if(!m_bus){
            ESP_LOGE(TAG, "scanning: bus not initialized");
            return;
        }

        ESP_LOGI(TAG, "scan 0x08 to 0x77 ");

        int f=0;

        for(uint8_t i=0x08; i<=0x77;i++){
            if(probe(i,30)==ESP_OK){
                ESP_LOGI(TAG, "ACK: 0x%02X",i);
                f++;
            }
        }
        if(f) 
        {
            ESP_LOGI(TAG, "found: %d devices",f);
        }
        else{
            ESP_LOGW(TAG, "no devices found");
        }

    }

    /*Device API*/

    esp_err_t Device::init(const Bus& bus, uint8_t addr,uint32_t clk){
        if(m_device){
            return ESP_ERR_INVALID_STATE;
        }
        if(!bus.valid()){
            return ESP_ERR_INVALID_ARG;
        }

        i2c_device_config_t configuration{};

        configuration.dev_addr_length=I2C_ADDR_BIT_LEN_7;
        configuration.device_address=addr;
        configuration.scl_speed_hz=clk;

        esp_err_t err=i2c_master_bus_add_device(bus.get_handle(), &configuration, &m_device);

        if(err!=ESP_OK){
            ESP_LOGE(TAG, "failure: add_device 0x%02X: %s", addr, esp_err_to_name(err));
            m_device = nullptr;
            return err;
        }
        ESP_LOGI(TAG, "dev 0x%02X @ %lu Hz", addr, (unsigned long)clk);
        return ESP_OK;
    }

    Device::~Device()
    {
        if (m_device) {
            i2c_master_bus_rm_device(m_device);
            m_device = nullptr;
        }
    }

    esp_err_t Device::write(const uint8_t* data, size_t len) const
    {
        if (!m_device) return ESP_ERR_INVALID_STATE;
        return i2c_master_transmit(m_device, data, len, m_timeout);
    }

    esp_err_t Device::writeRead(const uint8_t* tx, size_t tx_len,
                                uint8_t* rx, size_t rx_len) const
    {
        if (!m_device) return ESP_ERR_INVALID_STATE;
        return i2c_master_transmit_receive(m_device, tx, tx_len, rx, rx_len, m_timeout);
    }

    esp_err_t Device::readRegs(uint8_t reg, uint8_t* buf, size_t len) const
    {
        return writeRead(&reg, 1, buf, len);
    }

    esp_err_t Device::readReg(uint8_t reg, uint8_t& value) const
    {
        return readRegs(reg, &value, 1);
    }

    esp_err_t Device::writeReg(uint8_t reg, uint8_t value) const
    {
        const uint8_t buf[2] = { reg, value };
        return write(buf, sizeof(buf));
    }

    
    esp_err_t Device::updateReg(uint8_t reg, uint8_t mask, uint8_t value) const
    {
        uint8_t old = 0;
        esp_err_t err = readReg(reg, old);
        if (err != ESP_OK) return err;

        const uint8_t neu = (old & ~mask) | (value & mask);
        if (neu == old) return ESP_OK;          
        return writeReg(reg, neu);
    }

    void  Device::setTimeout(int ms){
        if(ms>0){
            m_timeout=ms;
        }
    }

} // namespace AZ_i2c
