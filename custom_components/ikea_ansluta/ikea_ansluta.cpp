#include "esphome/core/log.h"
#include "ikea_ansluta.h"
#include "cc2500.h"

namespace esphome
{
  namespace ikea_ansluta
  {
    static const char *TAG = "ikea_ansluta";

    IkeaAnsluta::IkeaAnsluta() : PollingComponent(100) {};

    void IkeaAnsluta::setup()
    {
      this->spi_setup();
      this->enable();
      this->send_strobe(CC2500_SRES);
      // Init CC2500
      this->write_reg(REG_IOCFG2, VAL_IOCFG2);
      this->write_reg(REG_IOCFG0, VAL_IOCFG0);
      this->write_reg(REG_PKTLEN, VAL_PKTLEN);
      this->write_reg(REG_PKTCTRL1, VAL_PKTCTRL1);
      this->write_reg(REG_PKTCTRL0, VAL_PKTCTRL0);
      this->write_reg(REG_ADDR, VAL_ADDR);
      this->write_reg(REG_CHANNR, VAL_CHANNR);
      this->write_reg(REG_FSCTRL1, VAL_FSCTRL1);
      this->write_reg(REG_FSCTRL0, VAL_FSCTRL0);
      this->write_reg(REG_FREQ2, VAL_FREQ2);
      this->write_reg(REG_FREQ1, VAL_FREQ1);
      this->write_reg(REG_FREQ0, VAL_FREQ0);
      this->write_reg(REG_MDMCFG4, VAL_MDMCFG4);
      this->write_reg(REG_MDMCFG3, VAL_MDMCFG3);
      this->write_reg(REG_MDMCFG2, VAL_MDMCFG2);
      this->write_reg(REG_MDMCFG1, VAL_MDMCFG1);
      this->write_reg(REG_MDMCFG0, VAL_MDMCFG0);
      this->write_reg(REG_DEVIATN, VAL_DEVIATN);
      this->write_reg(REG_MCSM2, VAL_MCSM2);
      this->write_reg(REG_MCSM1, VAL_MCSM1);
      this->write_reg(REG_MCSM0, VAL_MCSM0);
      this->write_reg(REG_FOCCFG, VAL_FOCCFG);
      this->write_reg(REG_BSCFG, VAL_BSCFG);
      this->write_reg(REG_AGCCTRL2, VAL_AGCCTRL2);
      this->write_reg(REG_AGCCTRL1, VAL_AGCCTRL1);
      this->write_reg(REG_AGCCTRL0, VAL_AGCCTRL0);
      this->write_reg(REG_WOREVT1, VAL_WOREVT1);
      this->write_reg(REG_WOREVT0, VAL_WOREVT0);
      this->write_reg(REG_WORCTRL, VAL_WORCTRL);
      this->write_reg(REG_FREND1, VAL_FREND1);
      this->write_reg(REG_FREND0, VAL_FREND0);
      this->write_reg(REG_FSCAL3, VAL_FSCAL3);
      this->write_reg(REG_FSCAL2, VAL_FSCAL2);
      this->write_reg(REG_FSCAL1, VAL_FSCAL1);
      this->write_reg(REG_FSCAL0, VAL_FSCAL0);
      this->write_reg(REG_RCCTRL1, VAL_RCCTRL1);
      this->write_reg(REG_RCCTRL0, VAL_RCCTRL0);
      this->write_reg(REG_FSTEST, VAL_FSTEST);
      this->write_reg(0x3E, 0xFF);
      this->disable();
    }

    void IkeaAnsluta::dump_config()
    {
      LOG_PIN("  CS Pin: ", this->cs_);
    }

    void IkeaAnsluta::update()
    {
      this->enable();
      auto packet = this->read_packet();
      if (this->valid_packet(packet)) {
        uint16_t address = (packet[2] << 8) + packet[3];
        IkeaAnslutaCommand command = (IkeaAnslutaCommand) packet[4];
        ESP_LOGD("radio", "Sniffed command %u from remote %#04x", command, address);
        for (auto &listener : this->listeners_)
          if (listener.remote_address == address)
            listener.on_command(command);
      }
      this->disable();
    }

    bool IkeaAnsluta::valid_packet(std::vector<uint8_t> packet)
    {
      return packet.size() == 6
        && packet.front() == 0x55
        && packet.at(1) == 0x01
        && this->valid_cmd((IkeaAnslutaCommand) packet.at(4))
        && packet.back() == 0xAA;
    }

    bool IkeaAnsluta::valid_cmd(IkeaAnslutaCommand cmd)
    {
      return cmd == IkeaAnslutaCommand::OFF
        || cmd == IkeaAnslutaCommand::ON_50
        || cmd == IkeaAnslutaCommand::ON_100;
    }

    void IkeaAnsluta::register_listener(uint16_t remote_address, const std::function<void(IkeaAnslutaCommand)> &func)
    {
      auto listener = IkeaAnslutaListener{
        .remote_address = remote_address,
        .on_command = func,
      };
      this->listeners_.push_back(listener);
    }

    std::vector<uint8_t> IkeaAnsluta::read_packet()
    {
      this->send_strobe(CC2500_SRX);
      this->write_reg(REG_IOCFG1, 0x01);
      delay(20);

      uint8_t len = this->read_reg(CC2500_FIFO);

      std::vector<uint8_t> buffer;

      if (len == 6)
      {
        for (int i = 0; i < 6; i++)
        {
          buffer.push_back(this->read_reg(CC2500_FIFO));
        }
      }

      this->send_strobe(CC2500_SIDLE);
      this->send_strobe(CC2500_SFRX);
      return buffer;
    }

    void IkeaAnsluta::send_strobe(uint8_t strobe)
    {
      this->cs_->digital_write(false);
      delayMicroseconds(1);
      this->write_byte(strobe);
      this->cs_->digital_write(true);
      delayMicroseconds(2000);
    }

    void IkeaAnsluta::write_reg(uint8_t addr, uint8_t value)
    {
      this->cs_->digital_write(false);
      delayMicroseconds(1);
      this->write_byte(addr);
      delayMicroseconds(200);
      this->write_byte(value);
      delayMicroseconds(1);
      this->cs_->digital_write(true);
    }

    uint8_t IkeaAnsluta::read_reg(uint8_t addr)
    {
      addr += 0x80;
      this->cs_->digital_write(false);
      delayMicroseconds(1);
      this->write_byte(addr);
      delay(10);
      uint8_t y = this->read_byte();
      delayMicroseconds(1);
      this->cs_->digital_write(true);
      return y;
    }

    void IkeaAnsluta::send_command(uint16_t address, IkeaAnslutaCommand command) {
      for (int i = 0; i < 200; i++)
      {
        this->enable();
        this->send_strobe(CC2500_SFTX);  // 0x3B
        this->send_strobe(CC2500_SIDLE); // 0x36
        this->cs_->digital_write(false);
        delayMicroseconds(1);
        this->write_byte(0x7F);
        this->write_byte(0x06);
        this->write_byte(0x55);
        this->write_byte(0x01);
        this->write_byte(address >> 8); // ansluta data address byte A
        this->write_byte(address & 0xFF); // ansluta data address byte B
        this->write_byte((uint8_t)command); // ansluta data command 0x01=Light OFF 0x02=50% 0x03=100% 0xFF=Pairing
        this->write_byte(0xAA);
        this->write_byte(0xFF);
        digitalWrite(SS, HIGH);
        this->send_strobe(CC2500_STX); //0x35 STX In IDLE state: Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled: Only go to TX if channel is clear
        delayMicroseconds(10);
      }
      this->disable();
    }
  } // namespace ikea_ansluta
} // namespace esphome
