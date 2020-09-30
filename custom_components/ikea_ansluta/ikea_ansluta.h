#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"

namespace esphome
{
  namespace ikea_ansluta
  {
    enum class IkeaAnslutaCommand : uint8_t
    {
      OFF    = 0x01,
      ON_50  = 0x02,
      ON_100 = 0x03,
      PAIR   = 0xFF,
    };

    struct IkeaAnslutaListener
    {
      uint16_t remote_address;
      std::function<void(IkeaAnslutaCommand)> on_command;
    };

    class IkeaAnsluta : public PollingComponent, public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_4MHZ>
    {
    public:
      IkeaAnsluta();
      void setup() override;
      void update() override;
      void dump_config() override;
      void send_command(uint16_t addr, IkeaAnslutaCommand command);
      void register_listener(uint16_t remote_address, const std::function<void(IkeaAnslutaCommand)> &func);
    protected:
      void send_strobe(uint8_t strobe);
      void write_reg(uint8_t addr, uint8_t value);
      uint8_t read_reg(uint8_t addr);
      std::vector<uint8_t> read_packet();
      bool valid_packet(std::vector<uint8_t>);
      bool valid_cmd(IkeaAnslutaCommand cmd);
      std::vector<IkeaAnslutaListener> listeners_;
    };
  } // namespace ikea_ansluta
} // namespace esphome
