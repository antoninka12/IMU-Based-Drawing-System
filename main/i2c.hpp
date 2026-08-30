#pragma once
#include "driver/i2c_master.h" /*must have for i2c communication*/
#include "esp_err.h" /*used for logs - errors*/

namespace AZ_i2c
{
    class Bus{
        /*public functions:*/
        public:
            Bus()=default; /*default constructor*/

            /*delating bus megistral*/
            ~Bus(); /*desctructor*/

            /*???*/
            Bus(const Bus&)=delete;
            Bus& operator=(const Bus&)=delete; //żeby nie było kopiowania, ale to sprawdzić
            /*???*/

            /*initialization of i2c*/
            esp_err_t init(i2c_port_num_t port_nr, gpio_num_t sda, gpio_num_t scl, 
                        bool pull_up = true); 

            /*checking if device is responding on i2c magistral, waiting max time_out ms*/
            esp_err_t probe(uint8_t addr, int time_out =50) const;

             /*scaning for devices on magistral*/
            void scan() const;

            i2c_master_bus_handle_t get_handle() const { 
                 return m_bus; 
            }

             bool valid() const { 
                return m_bus != nullptr; 
            }

           

       private:
            i2c_master_bus_handle_t m_bus = nullptr; /*uchwyt do magistrali i2c*/
    };

    class Device{
        /*public functions:*/
        public:
            Device() = default; /*default constructotr*/

             /*delating device handle*/
            ~Device(); /*destructor*/

             Device(const Device&)            = delete;
            Device& operator=(const Device&) = delete;

            /*initialization of device for i2c communication*/
            esp_err_t init(const Bus& bus, uint8_t addr, uint32_t clk=400000);

            /*sendning leb bytes to this I2C device*/
            esp_err_t write(const uint8_t* data, size_t len) const;

            /*first send tx_leb bytes, then read bytes*/
            esp_err_t writeRead(const uint8_t* tx, size_t tx_len,
                                uint8_t* rx, size_t rx_len) const;

            esp_err_t readRegs(uint8_t reg, uint8_t* buf, size_t len) const;

            esp_err_t readReg(uint8_t reg, uint8_t& value) const;

            esp_err_t writeReg(uint8_t reg, uint8_t value) const;

            esp_err_t updateReg(uint8_t reg, uint8_t mask, uint8_t value) const;
            
            void setTimeout(int ms);
        private:
             i2c_master_dev_handle_t m_device = nullptr;
              int m_timeout = 100;

    };
} // namespace AZ_i2c

