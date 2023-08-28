#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome
{
  namespace sfd_vosloh
  {

    static const char *TAG = "sfd_vosloh.component";

    class sfdVosloh : public Component, public uart::UARTDevice
    {
    protected:
      const int MAX_ADDRESS = 127;
      int _line_length = MAX_ADDRESS;
      int _cursor_position = 127;
      int _last_char_position = 127;

      bool _cursor_increment()
      {
        if (this->_cursor_position < this->MAX_ADDRESS)
        {
          this->_cursor_position++;
          return true;
        }
        ESP_LOGD(TAG, "[_cursor_increment]: reached MAX_ADDRESS");
        return false;
      }

      bool _write_char_at(char c, int pos)
      {
        if (pos <= this->MAX_ADDRESS)
        {
          this->write_byte(_write);
          this->write_byte(pos);
          this->write_byte(c);
          delay(5);
          return true;
        }
        ESP_LOGD(TAG, "[_write_char_at]: reached MAX_ADDRESS");
        return false;
      }

      bool _write_char(char c)
      {
        if (!this->_write_char_at(c, this->_cursor_position + 1))
          return false;
        if (!this->_cursor_increment())
          return false;
        this->_last_char_position = (this->_last_char_position < this->_cursor_position) ? this->_cursor_position : this->_last_char_position;
        return true;
      }

      void _write_string(std::string str)
      {
        ESP_LOGD(TAG, "[_write_string] at pos: %3d; string: \"%s\"", this->_cursor_position, str.c_str());
        for (int i = 0; i < str.length(); i++)
        {
          if (!this->_write_char(str[i]))
            break;
        }
      }

      bool _next_line()
      {
        int new_position = this->_line_length * (1 + ceil(this->_cursor_position / this->_line_length));
        if (new_position < this->MAX_ADDRESS)
        {
          this->_cursor_position = new_position;
          ESP_LOGD(TAG, "[_next_line] cursor position: %3d", this->_cursor_position);
          return true;
        }
        ESP_LOGD(TAG, "[_next_line]: reached MAX_ADDRESS");
        return false;
      }

    public:
      const int _adapt = 0x81;
      const int _roll = 0x82;
      const int _write = 0x88;

      const int _space = 0x20;

      void set_line_length(uint8_t length) { this->_line_length = length; }

      void setup() override
      {
        this->roll();
        delay(2000);
        this->write_string("ready");
      }

      void dump_config() override
      {
        ESP_LOGCONFIG(TAG, "line length: %3d", this->_line_length);
      }

      void loop() override
      {
      }

      void roll()
      {
        this->write_byte(_roll);
        _cursor_position = 0;
        _last_char_position = 0;
      }

      void adapt()
      {
        this->write_byte(_adapt);
      }

      void clear()
      {
        for (int i = 0; i < _last_char_position; i++)
        {
          this->_write_char_at(_space, i + 1);
        }
        _cursor_position = 0;
        _last_char_position = 0;
      }

      void blank()
      {
        this->clear();
        this->adapt();
      }

      void write_string(std::string str)
      {
        this->clear();
        this->_write_string(str);
        this->adapt();
      }

      void write_text(std::string str)
      {
        ESP_LOGD(TAG, "write_text: %s", str.c_str());

        str = str + " ";

        this->clear();
        size_t pos = 0;
        std::string word;
        std::string delimiter = " ";
        while ((pos = str.find(delimiter)) != std::string::npos)
        {
          word = str.substr(0, pos);
          str.erase(0, pos + delimiter.length());

          int rest_of_line = this->_line_length - (this->_cursor_position % this->_line_length);

          if (rest_of_line < word.length())
          {
            if (!this->_next_line())
            {
              break;
            }
          }

          bool nl = false;
          if ((pos = word.find('\n')) != std::string::npos)
          {
            if (pos == 0)
            {
              this->_next_line();
              nl = false;
            }
            else
              nl = true;
            word.erase(pos, 1);
          }

          this->_write_string(word);

          if (nl)
            this->_next_line();
          else
            this->_cursor_increment();
        }

        this->adapt();
      }

      void testpattern()
      {
        this->clear();
        for (int i = 0; i < this->MAX_ADDRESS; i++)
        {
          this->_write_char(48 + ((i + 1) % 10));
        }
        this->adapt();
      }
    };

  }
}
