#include "sfd_vosloh.h"

#include "esphome/core/log.h"

namespace esphome
{
  namespace sfd_vosloh
  {
    // protected
    // set functions
    bool sfdVosloh::set_string(std::string string, uint8_t position = 1)
    {
      ESP_LOGD(TAG, "[set_string] pos: %3d; string: \"%s\"", position, string.c_str());
      for (int i = 0; i < string.length(); i++)
      {
        if (!this->set_character(string[i], position + i))
          return false;
      }
      return true;
    }

    bool sfdVosloh::set_character(char character, uint8_t position = 1)
    {
      if (position <= this->POSITION_MAX)
      {
        this->parent_->write_byte(_WRITE);
        this->parent_->write_byte(position);
        this->parent_->write_byte(character);
        // this->parent_->flush();

        // ESP_LOGD(TAG, "[set_character]: pos: %3d; char: %#x '%c'", position, character, character);
        this->current_position = position + 1;
        return true;
      }
      ESP_LOGD(TAG, "[set_character]: reached POSITION_MAX");
      return false;
    }

    // get functions
    char sfdVosloh::get_character(uint8_t position)
    {
      this->parent_->write_byte(_READ);
      this->parent_->write_byte(position);
      this->parent_->flush();
      char character = this->collect_respond();

      // ESP_LOGD(TAG, "[get_character]: pos: %3d; char: '%c'", position, character);
      return character;
    }

    uint8_t sfdVosloh::get_state(uint8_t position)
    {
      this->parent_->write_byte(_STATE);
      this->parent_->write_byte(position);
      this->parent_->flush();
      uint8_t state = this->collect_respond();

      // ESP_LOGD(TAG, "[get_state] pos: %3d; state: %#x", position, state);
      return state;
    }

    uint8_t sfdVosloh::collect_respond()
    {
      uint32_t timeOut = millis() + this->UART_TIMEOUT;

      while (!available())
      {
        if (millis() > timeOut)
        {
          return 0x00;
        }
      }

      return read();
    }

    // update
    void sfdVosloh::update_last_module()
    {
      int last_pos = 0;
      for (int pos = 1; pos <= this->POSITION_MAX; pos++)
      {
        if (this->get_character(pos) != 0x00)
        {
          last_pos = pos;
        }
      }

      this->last_module->publish_state(last_pos);
    }

    void sfdVosloh::update_current_content()
    {
      std::string str = "";
      bool rolling = false;

      for (int pos = 1; pos <= this->last_module->get_state(); pos++)
      {
        char character = this->get_character(pos);

        switch (character)
        {
        case 0x00:
          // ESP_LOGD(TAG, "[character] pos: %3d; timeout", pos);
          str.push_back('|');
          break;
        case 0x10:
          // ESP_LOGD(TAG, "[character] pos: %3d; char not valid", pos);
          str.push_back('_');
          break;
        case 0x20 ... 0xFF:
          str.push_back(character);
          break;

        default:
          // ESP_LOGD(TAG, "[character] pos: %3d; undefined respond: %#x", pos, character);
          str.push_back('-');
          break;
        }
      }

      this->current_content->publish_state(str);
    }

    void sfdVosloh::update_current_state()
    {
      bool rolling = false;
      std::string c_str = "";
      std::string m_str = "";

      for (int pos = 1; pos <= this->last_module->get_state(); pos++)
      {
        uint8_t state = this->get_state(pos);
        uint8_t c_state = state | 0x0F;
        uint8_t m_state = state | 0xF0;

        switch (c_state)
        {
        case 0xAF:
          // ESP_LOGD(TAG, "[c_state] pos: %3d; first run", pos);
          c_str.push_back('f');
          break;
        case 0xCF:
          // ESP_LOGD(TAG, "[c_state] pos: %3d; char unknown", pos);
          c_str.push_back('u');
          break;
        case 0x8F:
          // ESP_LOGD(TAG, "[c_state] pos: %3d; char known", pos);
          c_str.push_back('k');
          break;

        default:
          c_str.push_back('_');
          break;
        }

        switch (m_state)
        {
        case 0xF0:
          // ESP_LOGD(TAG, "[m_state] pos: %3d; at position", pos);
          m_str.push_back('s');
          break;
        case 0xFF:
          // ESP_LOGD(TAG, "[m_state] pos: %3d; moving", pos);
          m_str.push_back('m');
          rolling = true;
          break;
        case 0xF4:
          // ESP_LOGD(TAG, "[m_state] pos: %3d; no supply", pos);
          m_str.push_back('v');
          break;
        case 0xF2:
          // ESP_LOGD(TAG, "[m_state] pos: %3d; no respond", pos);
          m_str.push_back('n');
          break;

        default:
          c_str.push_back('_');
          break;
        }
      }

      this->current_c_state->publish_state(c_str);
      this->current_m_state->publish_state(m_str);

      if (!rolling)
      {
        this->rolling->publish_state(false);
      }
    }

    // public
    // setup
    void sfdVosloh::setup()
    {
      this->update_last_module();
      this->roll();
    }
    void sfdVosloh::dump_config()
    {
      ESP_LOGCONFIG(TAG, "row length: %3d", this->row_length);
      ESP_LOGCONFIG(TAG, "last module: %3d", (int)this->last_module->get_state());
      ESP_LOGCONFIG(TAG, "current content: \"%s\"", this->current_content->get_state().c_str());
    }

    void sfdVosloh::loop()
    {
      this->loop_counter++;

      int loop_counter_end = 1000;

      if (this->rolling->state == true)
      {
        loop_counter_end = 10;
      }

      if (this->loop_counter >= loop_counter_end)
      {
        this->loop_counter = 0;
        if (this->blocked == 0)
        {
          this->update_current_state();
        }
      }
    }

    // methods
    void sfdVosloh::roll()
    {
      this->blocked++;

      ESP_LOGD(TAG, "[roll]:");
      this->rolling->publish_state(true);
      delay_microseconds_safe(1000);
      this->parent_->write_byte(_ROLL);
      this->current_position = 1;

      this->blocked--;

      this->update_current_content();
    }
    void sfdVosloh::adapt()
    {
      this->rolling->publish_state(true);
      delay_microseconds_safe(1000);
      this->parent_->write_byte(_ADAPT);
      this->update_current_content();
    }
    void sfdVosloh::clear(bool adapt)
    {
      this->blocked++;

      for (int pos = 1; pos <= this->last_module->get_state(); pos++)
      {
        this->set_character(' ', pos);
      }
      this->current_position = 1;
      if (adapt)
        this->adapt();

      this->blocked--;
    }

    void sfdVosloh::set_content(std::string input, uint8_t mode, uint8_t row)
    {
      this->blocked++;

      if (row < 1)
        row = 1;

      ESP_LOGD(TAG, "[set_content] row: %2d input: %s", row, input.c_str());

      if (!(mode & OVERWRITE))
      {
        this->clear(false);
      }

      this->current_position = (this->row_length * (row - 1)) + 1;

      if (mode == RAW)
      {
        _erase_all(input, '\n');
        _erase_all(input, '\r');

        this->set_string(input);
        this->adapt();
        return;
      }

      input += " ";
      std::string output = "";

      size_t pos = 0;
      std::string word;
      std::string delimiter = " ";
      while ((pos = input.find(delimiter)) != std::string::npos)
      {
        word = input.substr(0, pos);
        input.erase(0, pos + delimiter.length());

        if (mode & WORD_WRAP)
        {
          if ((this->row_length - output.length()) <= word.length())
          {
            this->set_row(output, mode);
            output = "";
          }
        }

        bool nl = false;
        if ((pos = word.find('\n')) != std::string::npos)
        {
          if (pos == 0)
          {
            this->set_row(output, mode);
            output = "";
            nl = false;
          }
          else
            nl = true;
          word.erase(pos, 1);
        }

        if (output.length() != 0)
          output += " ";
        output += word;

        if (nl)
        {
          this->set_row(output, mode);
          output = "";
        }
      }
      this->set_row(output, mode);
      this->adapt();

      this->blocked--;
    }

    void sfdVosloh::set_row(std::string input, uint8_t mode)
    {
      this->blocked++;

      ESP_LOGD(TAG, "[set_row] input: %s", input.c_str());

      if (this->row_length > input.length())
      {
        int rest = (this->row_length - input.length());
        if (mode & ALIGN_CENTER)
          input.insert(0, rest / 2, ' ');
        if (mode & ALIGN_RIGHT)
          input.insert(0, rest, ' ');

        input.insert(input.length(), this->row_length - input.length(), ' ');
      }

      this->set_string(input, this->current_position);

      this->blocked--;
    }
  }
}
