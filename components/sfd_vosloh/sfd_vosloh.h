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

      int line_length = this->MAX_ADDRESS;
      int cursor_position = 127;
      int last_char_position = 127;
      
      bool cursor_increment()
      {
        if (this->cursor_position < this->MAX_ADDRESS)
        {
          this->cursor_position++;
          return true;
        }
        ESP_LOGD(TAG, "[cursor_increment]: reached MAX_ADDRESS");
        return false;
      }

      void cursor_reset()
      {
        this->cursor_position = 0;
        this->last_char_position = 0;
      }

      bool next_line()
      {
        int new_position = this->line_length * (1 + ceil(this->cursor_position / this->line_length));
        if (new_position < this->MAX_ADDRESS)
        {
          this->cursor_position = new_position;
          ESP_LOGD(TAG, "[next_line] cursor position: %3d", this->cursor_position);
          return true;
        }
        ESP_LOGD(TAG, "[next_line]: reached MAX_ADDRESS");
        return false;
      }

    public:
      bool write_char(char c, int pos)
      {
        if (pos <= this->MAX_ADDRESS)
        {
          this->write_byte(0x88);
          this->write_byte(pos);
          this->write_byte(c);
          delay(5);
          return true;
        }
        ESP_LOGD(TAG, "[write_char]: reached MAX_ADDRESS");
        return false;
      }

      bool write_char(char c)
      {
        if (!this->write_char(c, this->cursor_position + 1))
          return false;
        if (!this->cursor_increment())
          return false;
        this->last_char_position = (this->last_char_position < this->cursor_position) ? this->cursor_position : this->last_char_position;
        return true;
      }

      void write_string(std::string str)
      {
        ESP_LOGD(TAG, "[write_string] at pos: %3d; string: \"%s\"", this->cursor_position, str.c_str());
        for (int i = 0; i < str.length(); i++)
        {
          if (!this->write_char(str[i]))
            break;
        }
      }

      void set_line_length(uint8_t length) { this->line_length = length; }

      void setup() override
      {
        this->roll();
      }

      void dump_config() override
      {
        ESP_LOGCONFIG(TAG, "line length: %3d", this->line_length);
      }

      void loop() override
      {
      }

      void roll()
      {
        this->write_byte(0x82);
        this->cursor_reset();
      }

      void adapt()
      {
        this->write_byte(0x81);
      }

      void clear()
      {
        for (int i = 0; i < last_char_position; i++)
        {
          this->write_char(' ', i + 1);
        }
        this->cursor_reset();
      }

      void blank()
      {
        this->clear();
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

          int rest_of_line = this->line_length - (this->cursor_position % this->line_length);

          if (rest_of_line < word.length() && !this->next_line())
          {
            break;
          }

          bool nl = false;
          if ((pos = word.find('\n')) != std::string::npos)
          {
            if (pos == 0)
            {
              this->next_line();
              nl = false;
            }
            else
              nl = true;
            word.erase(pos, 1);
          }

          this->write_string(word);

          if (nl)
            this->next_line();
          else
            this->cursor_increment();
        }

        this->adapt();
      }

      void testpattern()
      {
        this->clear();
        for (int i = 0; i < this->MAX_ADDRESS; i++)
        {
          this->write_char(48 + ((i + 1) % 10));
        }
        this->adapt();
      }
    };

  }
}
