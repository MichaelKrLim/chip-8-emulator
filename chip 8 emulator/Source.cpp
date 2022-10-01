#include <array>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <SFML/Graphics.hpp>

#include "Keypresses.h"

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

void draw_to_screen(auto& screen, auto start_y,auto start_x,auto height, std::uint16_t I, const auto& memory,auto&registers)
{
	registers[0xF] = 0;
	for (int y{};y < height;++y)
	{
		if ((start_y + y) >= 32) break;
		for (int x{}, shift = 7;x < 8;++x, --shift)
		{
			if ((start_x + x) >= 64) break;
			bool copy = screen[(start_x + x) + (y + start_y) * 64];
			screen[(start_x + x) + (y + start_y) * 64] = copy ^ static_cast<bool>((memory[I + y] >> shift) & 1);
			if (screen[(start_x + x) + (y + start_y) * 64] == 0 && copy == 1) registers[0xF] = 1;
		}
	}
} 

int main()
{
	sf::Uint8* pixels = new sf::Uint8[64 * 32 * 4];
	sf::Texture texture;
	texture.create(64, 32);
	constexpr uint16_t font_start_address = 0x132;
	Keypresses keypresses;
	uint8_t delay_timer{};
	uint8_t sound_timer{};
	std::array<std::uint8_t, 4096> memory{};
	std::array<std::uint8_t, 16> registers{};
	std::bitset<64 * 32> screen{};
	std::uint16_t I{};
	int program_counter{ 0x200 };
	std::vector<int> stack;
	std::ifstream program{ R"(C:\Users\Michael\Downloads\game.ch8)",std::ios_base::binary };

	init_fonts(memory, font_start_address);
	
	{
		int i{};
		while (program.read(reinterpret_cast<char*>(&memory[512 + i]), 1))
		{
			++i;
		} std::cerr << i << "\n";
	}
	dump_memory(memory);
	sf::RenderWindow window(sf::VideoMode(1280, 640), "My window");

	while (window.isOpen())
	{
		sf::Event event;

		if (delay_timer > 0)
		{
			--delay_timer;
		}

		if (sound_timer > 0)
		{
			--sound_timer;
		}
		std::array<int, 2> instruction = { memory[program_counter],memory[program_counter + 1] };
		int NNN = 0x100 * (instruction[0] % 16) + instruction[1];
		switch (instruction[0] / 16)
		{
		case 0:
		{
			if (instruction[1] % 16 == 0xE)
			{
				program_counter = stack[stack.size()-1];
				stack.pop_back();
			}
			if (instruction[1] % 16 == 0x0) screen.reset();
			break;
		}
		case 1:
		{
			program_counter = NNN-2;
			break;
		}
		case 2:
		{
			stack.emplace_back(program_counter);
			program_counter = NNN-2;
			break;
		}
		case 3:
		{
			if (instruction[1] == registers[instruction[0] % 16])
			{
				program_counter += 2;
			}
			break;
		}
		case 4:
		{
			if (instruction[1] != registers[instruction[0] % 16])
			{
				program_counter += 2;
			}
			break;
		}
		case 5:
		{
			if (registers[instruction[0] % 16] == registers[instruction[1] / 16])
			{
				program_counter += 2;
			}
			break;
		}
		case 6:
		{
			registers[instruction[0] % 16] = instruction[1];
			break;
		}
		case 7:
		{
			if (static_cast<int>(registers[instruction[0] % 16]) + instruction[1] > 255)
			{
				registers[0xF] = 1;
			}
			else
			{
				registers[0xF] = 0;
			}
			registers[instruction[0] % 16] += instruction[1];
			break;
		}
		case 8:
		{
			const auto Vx = instruction[0] % 16;
			const auto Vy = instruction[1] / 16;
			if (instruction[1] % 16 == 0)
			{
				registers[Vx] = registers[Vy];
			}
			if (instruction[1] % 16 == 1)
			{
				registers[Vx] = registers[Vx] | registers[Vy];
			}
			if (instruction[1] % 16 == 2)
			{
				registers[Vx] = registers[Vx] & registers[Vy];
			}
			if (instruction[1] % 16 == 3)
			{
				registers[Vx] = registers[Vx] ^ registers[Vy];
			}
			if (instruction[1] % 16 == 4)
			{
				if (static_cast<int>(registers[Vx]) + registers[Vy] > 255)
				{
					registers[0xF] = 1;
				}
				else
				{
					registers[0xF] = 0;
				}
				registers[Vx] += registers[Vy];
			}
			if (instruction[1] % 16 == 5)
			{
				if (registers[Vx] < registers[Vy]) registers[0xF] = 0;
				else registers[0xF] = 1;
				registers[Vx] -= registers[Vy];
			}
			if (instruction[1] % 16 == 6)
			{
				registers[Vx] = registers[Vy] >> 1;
				registers[0xF] = registers[Vy] &1;
			}
			if (instruction[1] % 16 == 7)
			{
				if (registers[Vy] < registers[Vx]) registers[0xF] = 0;
				else registers[0xF] = 1;
				registers[Vx] = registers[Vy] - registers[Vx];
			}
			if (instruction[1] % 16 == 0xE)
			{
				registers[Vx] = registers[Vy] << 1;
				registers[0xF] = (registers[Vy] >> 7)&1;
			}
			break;
		}
		case 9:
		{
			if (registers[instruction[0] % 16] != registers[instruction[1] / 16])
				program_counter += 2;
			break;
		}
		case 0xA:
		{
			I = NNN;
			break;
		}
		case 0xB:
		{
			program_counter = NNN + registers[0]-2;
			break;
		}
		case 0xC:
		{
			static std::mt19937 generator(9999999999995999999ULL);
			std::uniform_int_distribution<int> dis(0, 255);

			int random_number = dis(generator);
			registers[instruction[0] % 16] = random_number & instruction[1];
			break;
		}
		case 0xD:
		{
			const auto register_x = registers[instruction[0] % 16];
			const auto register_y = registers[instruction[1] / 16];
			const auto bytes_in_sprite = instruction[1] % 16;
			draw_to_screen(screen, register_y, register_x, bytes_in_sprite, I, memory, registers);
			break;
		}
		case 0xE:
		{
			if (instruction[1] % 16 == 0xE)
			{
				if (sf::Keyboard::isKeyPressed(keypresses.key_to_code[registers[instruction[0] % 16]]) == true)
				{
					program_counter += 2;
				}
			}
			else
			{
				if (sf::Keyboard::isKeyPressed(keypresses.key_to_code[registers[instruction[0] % 16]]) != true)
				{
					program_counter += 2;
				}
			}
			break;
		}
		case 0xF:
		{

			const auto Vx = instruction[0] % 16;

			if (instruction[1] == 0x07)
			{
				registers[Vx] = delay_timer;
			}
			if (instruction[1] == 0xA)
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
			if (instruction[1] == 0x15)
			{
				delay_timer = registers[Vx];
			}
			if (instruction[1] == 0x18)
			{
				sound_timer = registers[Vx];
			}
			if (instruction[1] == 0x1E)
			{
				I += registers[Vx];
			}
			if (instruction[1] == 0x29)
			{
				I = font_start_address + registers[Vx]*5;
			}
			if (instruction[1] == 0x33)
			{
				uint8_t number = registers[Vx];

				for (int i{ 2 };i >= 0;--i)
				{
					memory[I + i] = number % 10;
					number /= 10;
				}
			}
			if (instruction[1] == 0x55)
			{
				for (int i{}; i <= Vx;++i)
				{
					memory[I + i] = registers[i];
				}
				I = I + Vx + 1;
			}
			if (instruction[1] == 0x65)
			{
				for (int i{}; i <= Vx;++i)
				{
					registers[i] = memory[I + i];
				}
				I = I + Vx + 1;
			}
		}
			break;
	}	
		program_counter += 2;
		

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

		}

		for (int i{}; i < screen.size(); ++i)
		{
			if (screen[i] != 1)
			{
				pixels[i*4] = 255;
				pixels[i * 4 + 1] = 255;
				pixels[i  * 4 + 2] = 255;
				pixels[i * 4 + 3] = 255;
			}
			else
			{
				pixels[i * 4] = 0;
				pixels[i * 4 + 1] = 0;
				pixels[i * 4 + 2] = 0;
				pixels[i * 4 + 3] = 255;
			}
		}
		texture.update(pixels);
		sf::Sprite sprite(texture);
		sprite.setScale(20, 20);
		window.draw(sprite);
		window.display();
	}
	delete pixels;
	return 0;
}
