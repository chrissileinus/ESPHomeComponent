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
        this->parent_->flush();

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

      for (int pos = 1; pos <= this->last_module->get_state(); pos++)
      {
        char character = this->get_character(pos);

        switch (character)
        {
        case 0x00:
          ESP_LOGD(TAG, "[get_character] pos: %3d; timeout", pos);
          str.push_back('_');
          break;
        case 0x10:
          ESP_LOGD(TAG, "[get_character] pos: %3d; char not valid", pos);
          str.push_back('_');
          break;
        case 0x20 ... 0xFF:
          str.push_back(character);
          break;

        default:
          ESP_LOGD(TAG, "[get_character] pos: %3d; undefined respond: %#x", pos, character);
          str.push_back(' ');
          break;
        }
      }

      this->current_content->publish_state(str);
    }

    void sfdVosloh::update_current_state()
    {
      for (int pos = 1; pos <= this->last_module->get_state(); pos++)
      {
        uint8_t state = this->get_state(pos);
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
    }

    // methods
    void sfdVosloh::roll()
    {
      ESP_LOGD(TAG, "[roll]:");
      this->parent_->write_byte(_ROLL);
      this->current_position = 1;

      this->update_current_content();
      this->update_current_state();
    }
    void sfdVosloh::clear(bool adapt)
    {
      for (int pos = 1; pos <= this->last_module->get_state(); pos++)
      {
        this->set_character(' ', pos);
      }
      this->current_position = 1;
      if (adapt)
        this->parent_->write_byte(_ADAPT);
    }

    void sfdVosloh::set_content(std::string input, uint8_t mode, uint8_t row)
    {
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
        this->parent_->write_byte(_ADAPT);
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
      this->parent_->write_byte(_ADAPT);

      this->update_current_content();
      this->update_current_state();
    }

    void sfdVosloh::set_row(std::string input, uint8_t mode)
    {
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
    }
  }
}