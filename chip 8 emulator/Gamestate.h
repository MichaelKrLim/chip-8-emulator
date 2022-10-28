#ifndef GAMESTATE_H
#define GAMESTATE_H

class Gamestate
{

public:

	std::array<sf::Uint8,64*32*4> pixels{};
	std::bitset<64 * 32> screen{};

	std::array<std::uint8_t, 4096> memory{};
	std::array<std::uint8_t, 16> registers{};

	std::vector<int> stack;

	int program_counter{ 0x200 };
	int screen_width = 1280;
	int screen_length = 960;

	uint8_t delay_timer{};
	uint8_t sound_timer{};
	uint16_t I{};


	void convert_to_binary_coded_decimal(auto Vx)
	{
		uint8_t number = registers[Vx];

		for (int i{ 2 };i >= 0;--i)
		{
			memory[I + i] = number % 10;
			number /= 10;
		}
	}

	void copy(auto& source, auto offset_source, auto& target, auto offset_target, auto Vx)
	{
		for (int i{}; i <= Vx;++i)
		{
			target[offset_target + i] = source[offset_source + i];
		}
		I = I + Vx + 1;
	}

	void return_from_subroutine()
	{
		program_counter = stack[stack.size() - 1];
		stack.pop_back();
	}

	void jump_to(auto address)
	{
		program_counter = address - 2;
	}

	void call_subroutine(auto address)
	{
		stack.emplace_back(program_counter);
		jump_to(address);
	}

	void skip_if_equal(auto condition_one, auto condition_two)
	{
		if (condition_one == condition_two)
		{
			program_counter += 2;
		}
	}

	void skip_if_not_equal(const auto condition_one, const auto condition_two)
	{
		if (condition_one != condition_two)
		{
			program_counter += 2;
		}
	}

	void add_with_carry(auto num, auto Vx)
	{
		if (registers[Vx] + num > 255)
		{
			registers[0xF] = 1;
		}
		else
		{
			registers[0xF] = 0;
		}
		registers[Vx] += num;
	}

	void wait_for_keypress(const auto keypresses, auto Vx)
	{
		bool any_key_pressed{};
		for (int i{}; i < 16; ++i)
		{
			if (sf::Keyboard::isKeyPressed(keypresses.key_to_code[i]) == true)
			{
				any_key_pressed = true;
				registers[Vx] = i;
			}
		}
		if (any_key_pressed == false)
		{
			program_counter -= 2;
		}
	}

	void draw_to_screen( auto bytes_in_sprite, auto start_x, auto start_y)
	{
		registers[0xF] = 0;
		for (int y{};y < bytes_in_sprite;++y)
		{
			if ((start_y + y) >= 32) break;
			for (int x{}, shift = 7;x < 8;++x, --shift)
			{
				int index = (start_x + x) + (start_y + y) * 64;

				if ((start_x + x) >= 64) break;
				bool copy = screen[index];
				screen[index] = copy ^ static_cast<bool>((memory[I + y] >> shift) & 1);
				if (screen[index] == 0 && copy == 1) registers[0xF] = 1;
			}
		}
	}

	void decrement_timers()
	{
		if (delay_timer > 0)
		{
			--delay_timer;
		}

		if (sound_timer > 0)
		{
			--sound_timer;
		}
	}

	bool check_for_borrow(auto subtracted_from, auto to_subtract)
	{
		if (registers[subtracted_from] < registers[to_subtract]) return true;
		return false;
	}
};

#endif