#include <array>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <SFML/Graphics.hpp>

#include "Gamestate.h"
#include "Keypresses.h"

#if 0<1
void decrement_timers(Gamestate &game_state)
{
	if (game_state.delay_timer > 0)
	{
		--game_state.delay_timer;
	}

	if (game_state.sound_timer > 0)
	{
		--game_state.sound_timer;
	}
}

bool check_for_borrow(Gamestate &game_state,auto subtracted_from,auto to_subtract)
{
	if (game_state.registers[subtracted_from] < game_state.registers[to_subtract]) return true;
	return false;
}

void skip_instruction(int& program_counter)
{
	program_counter += 2;
}

void init_fonts(std::array<std::uint8_t, 4096>  &memory, std::uint16_t font_start_address)
{
	std::array<int,5*16> font_bytes 
	{{
		0xF0, 0x90, 0x90, 0x90, 0xF0,
		0x20, 0x60, 0x20, 0x20, 0x70,
		0xF0, 0x10, 0xF0, 0x80, 0xF0,
		0xF0, 0x10, 0xF0, 0x10, 0xF0,
		0x90, 0x90, 0xF0, 0x10, 0x10,
		0xF0, 0x80, 0xF0, 0x10, 0xF0,
		0xF0, 0x80, 0xF0, 0x90, 0xF0,
		0xF0, 0x10, 0x20, 0x40, 0x40,
		0xF0, 0x90, 0xF0, 0x90, 0xF0,
		0xF0, 0x90, 0xF0, 0x10, 0xF0,
		0xF0, 0x90, 0xF0, 0x90, 0x90,
		0xE0, 0x90, 0xE0, 0x90, 0xE0,
		0xF0, 0x80, 0x80, 0x80, 0xF0, 
		0xE0, 0x90, 0x90, 0x90, 0xE0,
		0xF0, 0x80, 0xF0, 0x80, 0xF0,
		0xF0, 0x80, 0xF0, 0x80, 0x80,
	}};
	for (std::size_t i{}; i < font_bytes.size(); ++i)
	{
		memory[i + font_start_address] = font_bytes[i];
	}
}

void dump_memory(const std::array<std::uint8_t, 4096>& memory)
{
	for (auto i : memory)
	{
		std::cout << std::hex << static_cast<int>(i) << ' ';
	}
}

void draw_to_screen(Gamestate & game_state, auto bytes_in_sprite, auto start_x, auto start_y)
{
	game_state.registers[0xF] = 0;
	for (int y{};y < bytes_in_sprite;++y)
	{
		if ((start_y + y) >= 32) break;
		for (int x{}, shift = 7;x < 8;++x, --shift)
		{
			int index = (start_x + x) + (start_y + y) * 64;

			if ((start_x + x) >= 64) break;
			bool copy = game_state.screen[index];
			game_state.screen[index] = copy ^ static_cast<bool>((game_state.memory[game_state.I + y] >> shift) & 1);
			if (game_state.screen[index] == 0 && copy == 1) game_state.registers[0xF] = 1;
		}
	}
} 

void clear_screen(auto &screen)
{
	screen.reset();
}

void return_from_subroutine(Gamestate& game_state)
{
	game_state.program_counter = game_state.stack[game_state.stack.size() - 1];
	game_state.stack.pop_back();
}

void jump_to(Gamestate &game_state, auto address)
{
	game_state.program_counter = address - 2;
}

void call_subroutine(Gamestate &game_state, auto address)
{
	game_state.stack.emplace_back(game_state.program_counter);
	jump_to(game_state, address);
}

void skip_if_equal(Gamestate &game_state, auto condition_one, auto condition_two)
{
	if (condition_one == condition_two)
	{
		skip_instruction(game_state.program_counter);
	}
} 

void skip_if_not_equal(Gamestate& game_state, const auto condition_one, const auto condition_two)
{
	if (condition_one != condition_two)
	{
		skip_instruction(game_state.program_counter);
	}
}

void add_with_carry(Gamestate &game_state, auto num,auto Vx)
{
	if (game_state.registers[Vx] + num > 255)
	{
		game_state.registers[0xF] = 1;
	}
	else
	{
		game_state.registers[0xF] = 0;
	}
	game_state.registers[Vx] += num;
}

void wait_for_keypress(Gamestate& game_state, const auto keypresses, auto Vx)
{
	bool any_key_pressed{};
	for (int i{}; i < 16; ++i)
	{
		if (sf::Keyboard::isKeyPressed(keypresses.key_to_code[i]) == true)
		{
			any_key_pressed = true;
			game_state.registers[Vx] = i;
		}
	}
	if (any_key_pressed == false)
	{
		game_state.program_counter -= 2;
	}
}

sf::Sprite create_sprite(sf::Texture &texture)
{
	sf::Sprite sprite(texture);

	sprite.setScale(20, 20);
	return sprite;
}

void convert_to_binary_coded_decimal(Gamestate &game_state, auto Vx)
{
	uint8_t number = game_state.registers[Vx];

	for (int i{ 2 };i >= 0;--i)
	{
		game_state.memory[game_state.I + i] = number % 10;
		number /= 10;
	}
}

void copy(Gamestate & game_state, auto &source, auto offset_source, auto & target, auto offset_target, auto Vx)
{
	for (int i{}; i <= Vx;++i)
	{
		target[offset_target+i] = source[offset_source+i];
	}
	game_state.I = game_state.I + Vx + 1;
}

int main()
{
	constexpr uint16_t font_start_address = 0x132;


	constexpr int base_hex = 16;
	Gamestate game_state;
	sf::Texture texture;
	texture.create(64, 32);
	Keypresses keypresses;
	std::ifstream program{ R"(C:\Users\Michael\Downloads\BULLETHELL.ch8)",std::ios_base::binary };

	init_fonts(game_state.memory, font_start_address);
	{
		int i{}; 
		while (program.read(reinterpret_cast<char*>(&game_state.memory[512 + i]), 1))
		{
			++i;
		} std::cerr << i << "\n";
	}
	dump_memory(game_state.memory);
	sf::RenderWindow window(sf::VideoMode(game_state.screen_width, game_state.screen_length), "My window");
	//window.setFramerateLimit(125);
	while (window.isOpen())
	{
		game_state.I = game_state.I & 0xFFF;
		sf::Event event;
		decrement_timers(game_state);

		std::array<int, 2> instruction = { game_state.memory[game_state.program_counter],game_state.memory[game_state.program_counter + 1] };
		auto NNN = 0x100 * (instruction[0] % base_hex) + instruction[1];		
		auto Vy = instruction[1] / base_hex;
		auto Vx = instruction[0] % base_hex;
		switch (instruction[0] / base_hex)
		{
		case 0:
		{
			if (instruction[1] % base_hex == 0xE)
			{
				return_from_subroutine(game_state);
			}
			if (instruction[1] % base_hex == 0x0) clear_screen(game_state.screen);
			break;
		}
		case 1:
		{
			jump_to(game_state, NNN);
			break;
		}
		case 2:
		{
			call_subroutine(game_state,NNN);
			break;
		}
		case 3:
		{
			skip_if_equal(game_state, game_state.registers[Vx], instruction[1]);
			break;
		}
		case 4:
		{
			skip_if_not_equal(game_state, instruction[1], game_state.registers[Vx]);
			break;
		}
		case 5:
		{
			skip_if_equal(game_state, game_state.registers[Vy], game_state.registers[Vx]);
			break;
		}
		case 6:
		{
			game_state.registers[Vx] = instruction[1];
			break;
		}
		case 7:
		{
			game_state.registers[Vx] += instruction[1];
			break;
		}
		case 8:
		{
			const auto identifier = instruction[1] % base_hex;
			if (identifier == 0)
			{
				game_state.registers[Vx] = game_state.registers[Vy];
			}
			if (identifier == 1)
			{
				game_state.registers[Vx] = game_state.registers[Vx] | game_state.registers[Vy];
			}
			if (identifier == 2)
			{
				game_state.registers[Vx] = game_state.registers[Vx] & game_state.registers[Vy];
			}
			if (identifier == 3)
			{
				game_state.registers[Vx] = game_state.registers[Vx] ^ game_state.registers[Vy];
			}
			if (identifier == 4)
			{
				add_with_carry(game_state, game_state.registers[Vy], Vx);
			}
			if (identifier  == 5)
			{
				if (check_for_borrow(game_state, Vx, Vy) == true) game_state.registers[0xF] = 0;
				else game_state.registers[0xF] = 1;
				game_state.registers[Vx] -= game_state.registers[Vy];
			}
			if (identifier  == 6)
			{
				game_state.registers[Vx] = game_state.registers[Vy] >> 1;
				game_state.registers[0xF] = game_state.registers[Vy] & 1;
			}
			if (identifier  == 7)
			{
				if (check_for_borrow(game_state, Vy,Vx)) game_state.registers[0xF] = 0;
				else game_state.registers[0xF] = 1;
				game_state.registers[Vx] = game_state.registers[Vy] - game_state.registers[Vx];
			}
			if (identifier == 0xE)
			{
				game_state.registers[Vx] = game_state.registers[Vy] << 1;
				game_state.registers[0xF] = (game_state.registers[Vy] >> 7) & 1;
			}
			break;
		}
		case 9:
		{
			skip_if_not_equal(game_state, game_state.registers[Vy], game_state.registers[Vx]);
			break;
		}
		case 0xA:
		{
			game_state.I = NNN;
			break;
		}
		case 0xB:
		{
			jump_to(game_state, game_state.registers[0]+NNN);
			break;
		}
		case 0xC:
		{
			static std::mt19937 generator(9999999999995999999ULL);
			std::uniform_int_distribution<int> dis(0, 255);

			int random_number = dis(generator);
			game_state.registers[Vx] = random_number & instruction[1];
			break;
		}
		case 0xD:
		{
			auto register_x = game_state.registers[Vx];
			auto register_y = game_state.registers[Vy];
			auto bytes_in_sprite = instruction[1] % base_hex;

			draw_to_screen(game_state, bytes_in_sprite, register_x, register_y);
			break;
		}
		case 0xE:
		{
			if (instruction[1] % 16 == 0xE)
			{
				if (sf::Keyboard::isKeyPressed(keypresses.key_to_code[game_state.registers[Vx]]) == true)
				{
					skip_instruction(game_state.program_counter);
				}
			}
			else
			{
				if (sf::Keyboard::isKeyPressed(keypresses.key_to_code[game_state.registers[Vx]]) != true)
				{
					skip_instruction(game_state.program_counter);
				}
			}
			break;
		}
		case 0xF:
		{
			if (instruction[1] == 0x07)
			{
				game_state.registers[Vx] = game_state.delay_timer;
			}
			if (instruction[1] == 0x0A)
			{
				wait_for_keypress(game_state, keypresses,Vx);
			}
			if (instruction[1] == 0x15)
			{
				game_state.delay_timer = game_state.registers[Vx];
			}
			if (instruction[1] == 0x18)
			{
				game_state.sound_timer = game_state.registers[Vx];
			}
			if (instruction[1] == 0x1E)
			{
				game_state.I += game_state.registers[Vx];
			}
			if (instruction[1] == 0x29)
			{
				game_state.I = font_start_address + game_state.registers[Vx] * 5;
			}
			if (instruction[1] == 0x33)
			{
				convert_to_binary_coded_decimal(game_state,Vx);
			}

			if (instruction[1] == 0x55)
			{
				copy(game_state, game_state.registers,0, game_state.memory, game_state.I,Vx);
			}

			if (instruction[1] == 0x65)
			{
				copy(game_state, game_state.memory, game_state.I, game_state.registers,0,Vx);
			}
			break;	
		} 
	}	
		skip_instruction(game_state.program_counter);

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

		}

		for (int i{}; i < game_state.screen.size(); ++i)
		{
			int byte_size = 4;
			int index = byte_size * i;

			if (game_state.screen[i] != 1)
			{
				for (int x{}; x < byte_size; ++x)
				{
					game_state.pixels[index + x] = 255;
				}
			}
			else
			{
				for (int x{}; x < byte_size; ++x)
					game_state.pixels[index+x] = 0;
					
				game_state.pixels[index + 3] = 255;
			}
		}
		texture.update(game_state.pixels);
		sf::Sprite sprite = create_sprite(texture);
		window.draw(sprite);
		window.display();
	}
	delete game_state.pixels;
	return 0;
}
#endif